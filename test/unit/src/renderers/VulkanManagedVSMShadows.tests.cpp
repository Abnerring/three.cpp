#include "renderers/VulkanRenderer.h"
#include "renderers/WebGLRenderTarget.h"
#include "textures/DepthTexture.h"
#include "materials/RawShaderMaterial.h"
#include "materials/MeshBasicMaterial.h"
#include "materials/MeshLambertMaterial.h"
#include "scenes/Scene.h"
#include "cameras/OrthographicCamera.h"
#include "geometries/PlaneGeometry.h"
#include "objects/Mesh.h"
#include "lights/DirectionalLight.h"
#include "lights/SpotLight.h"
#include "math/Color.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

// Source VSM factory, canonical floating targets and real Vulkan submissions.
// GL wrappers observe and delegate the public source calls; they provide no
// rendering services, resource substitutes or shadow shader replacements.
namespace {
using Args=std::vector<JSValue>;
int assertions=0,renders=0,validationRequested=0,validationEnabled=0,validationMessages=0;
void require(bool condition,const char* message){++assertions;if(!condition)throw std::runtime_error(message);}
JSValue call(JSValue receiver,const char* name,const Args& args={}){return receiver.get(name).call(receiver,args);}
JSValue fields(std::initializer_list<std::pair<const char*,JSValue>> values){auto result=JSValue::object();for(const auto& [name,value]:values)result.set(name,value);return result;}
JSValue uniform(JSValue value){return fields({{"value",value}});}
JSValue camera(){auto result=nativeOrthographicCameraConstructor().construct({-1,1,1,-1,.1,10});result.get("position").set("z",3);return result;}
JSValue scene(const Args& values={}){auto result=nativeSceneConstructor().construct();if(!values.empty())call(result,"add",values);return result;}
JSValue mesh(JSValue material,double width=2,double height=2,double x=0,double z=0){auto result=nativeMeshConstructor().construct({nativePlaneGeometryConstructor().construct({width,height}),material});result.set("frustumCulled",false);call(result.get("position"),"set",{x,0,z});return result;}
JSValue paint(unsigned color=0xff0000){return nativeMeshBasicMaterialConstructor().construct({fields({{"color",color}})});}
void linear(VulkanRenderer& renderer){renderer.srgbOutput=false;renderer.object().set("outputColorSpace",LinearSRGBColorSpace);}
void bind(JSValue renderer,JSValue target=nullptr){call(renderer,"setRenderTarget",{target});}
void render(VulkanRenderer& renderer,JSValue world,JSValue view){renderer.render(world,view);++renders;}
void noError(JSValue gl,const char* message){require(call(gl,"getError").strictEquals(0),message);}
void expect(const VulkanRenderer& renderer,std::array<int,3> expected,const char* message,unsigned x=32,unsigned y=32){
    const auto bytes=renderer.readPixels();require(bytes.size()==64*64*4,"screen readback is a complete 64x64 actual GPU image");const auto index=(y*64+x)*4;
    for(unsigned channel=0;channel<3;++channel)if(std::abs(int(bytes.at(index+channel))-expected[channel])>2){std::cerr<<message<<" actual="<<int(bytes.at(index))<<','<<int(bytes.at(index+1))<<','<<int(bytes.at(index+2))<<'\n';require(false,message);}require(true,message);
}
double half(std::uint16_t bits){const int sign=bits&0x8000?-1:1,exponent=(bits>>10)&31,fraction=bits&1023;if(exponent==31)return fraction?std::nan(""):sign*INFINITY;return sign*std::ldexp(exponent?1.+fraction/1024.:fraction/1024.,exponent?exponent-15:-14);}
void expectHalf(const std::vector<std::uint16_t>& bytes,unsigned channels,std::array<double,4> expected,const char* message){
    require(bytes.size()>=channels,"floating readback contains every actual attachment channel");for(unsigned i=0;i<channels;++i)require(std::abs(half(bytes.at(i))-expected[i])<.002,message);
}
struct Validation {
    const VulkanRenderer& renderer;
    Validation(const VulkanRenderer& value,bool requested):renderer(value){if(requested)++validationRequested;if(value.getInfo().validationEnabled)++validationEnabled;}
    void verify(){const auto messages=renderer.getValidationMessages();validationMessages+=int(messages.size());for(const auto& message:messages)std::cerr<<message<<'\n';require(messages.empty(),"float attachments and VSM passes emit no collected Vulkan validation messages");}
};
const char* vertex="precision highp float;in vec3 position;in vec2 uv;uniform mat4 projectionMatrix;uniform mat4 modelViewMatrix;out vec2 vUv;void main(){vUv=uv;gl_Position=projectionMatrix*modelViewMatrix*vec4(position,1.);}";
JSValue raw(const char* fragment,JSValue uniforms=JSValue::object()){return nativeRawShaderMaterialConstructor().construct({fields({{"glslVersion",GLSL3},{"vertexShader",vertex},{"fragmentShader",fragment},{"uniforms",uniforms}})});}
JSValue sampleMaterial(JSValue texture,bool rg,bool divided){
    return raw("precision highp float;uniform sampler2D image;uniform bool rg;uniform bool divided;in vec2 vUv;out vec4 result;void main(){vec4 value=texture(image,vUv);result=vec4(divided?vec3(value.r*.25,value.g*.125,rg?value.a*.75:value.b*.5):vec3(value.r,value.g,rg?value.a:value.b),1.);}",fields({{"image",uniform(texture)},{"rg",uniform(rg)},{"divided",uniform(divided)}}));
}
JSValue enableFloat(JSValue gl){
    auto supported=call(gl,"getSupportedExtensions"),again=call(gl,"getSupportedExtensions");bool found=false;
    require(supported.type()==JSValue::Type::Array&&!supported.sameValue(again),"getSupportedExtensions returns a new public array each time");
    for(unsigned i=0;i<supported.length();++i)if(supported.get(JSValue(i)).strictEquals("EXT_color_buffer_float"))found=true;
    require(found,"the physical device advertises all required EXT_color_buffer_float formats");
    auto extension=call(gl,"getExtension",{"EXT_color_buffer_float"});require(extension.isObject()&&extension.sameValue(call(gl,"getExtension",{"EXT_color_buffer_float"})),"enabled float extension has a stable native object identity");
    require(call(gl,"getExtension",{"THREECPP_nonexistent_extension"}).isNull(),"an unsupported extension remains null");return extension;
}

void floatExtensionAndNativeAttachments(bool validation){
    std::cout<<"floatExtensionAndNativeAttachments"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);
    auto gl=call(renderer.object(),"getContext");
    struct Attachment{unsigned format;JSValue texture,framebuffer;};std::vector<Attachment> attachments;
    for(unsigned format:{0x822du,0x822fu,0x881au,0x822eu,0x8230u,0x8814u,0x8c3au}){
        auto texture=call(gl,"createTexture"),framebuffer=call(gl,"createFramebuffer");call(gl,"bindTexture",{0x0de1,texture});call(gl,"texStorage2D",{0x0de1,1,format,4,4});
        call(gl,"bindFramebuffer",{0x8d40,framebuffer});call(gl,"framebufferTexture2D",{0x8d40,0x8ce0,0x0de1,texture,0});
        require(call(gl,"checkFramebufferStatus",{0x8d40}).strictEquals(0x8cd6),"allocated float attachment is incomplete before its public extension is enabled");attachments.push_back({format,texture,framebuffer});
    }
    enableFloat(gl);
    for(const auto& attachment:attachments){
        call(gl,"bindFramebuffer",{0x8d40,attachment.framebuffer});require(call(gl,"checkFramebufferStatus",{0x8d40}).strictEquals(0x8cd5),"each of the seven required floating color formats becomes complete after extension enablement");
        if(attachment.format==0x822f||attachment.format==0x881a){
            call(gl,"viewport",{0,0,4,4});call(gl,"clearColor",{.25,.5,.75,1});call(gl,"clear",{0x4000});
            const unsigned channels=attachment.format==0x822f?2:4;const auto pixels=renderer.readPixelsHalfFloat();require(pixels.size()==4*4*channels,"raw native floating framebuffer readback has its actual channel count");expectHalf(pixels,channels,{.25,.5,.75,1},"raw RG16F/RGBA16F clear reaches actual half-float device storage");
        }
    }
    call(gl,"bindFramebuffer",{0x8d40,nullptr});call(gl,"bindTexture",{0x0de1,nullptr});for(const auto& attachment:attachments){call(gl,"deleteFramebuffer",{attachment.framebuffer});call(gl,"deleteTexture",{attachment.texture});}
    noError(gl,"float extension enablement, completeness, clear and deletion retain a clean GL error state");layer.verify();
}

void sourceFloatingTargetSampling(bool validation){
    std::cout<<"sourceFloatingTargetSampling"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    auto facade=renderer.object(),gl=call(facade,"getContext"),view=camera();enableFloat(gl);
    for(bool rg:{false,true}){
        auto destination=nativeWebGLRenderTargetConstructor().construct({16,16,fields({{"format",rg?RGFormat:RGBAFormat},{"type",HalfFloatType},{"depthBuffer",false},{"minFilter",NearestFilter},{"magFilter",NearestFilter}})});
        auto texture=destination.get("texture"),source=texture.get("source"),image=texture.get("image");const unsigned channels=rg?2:4;
        bind(facade,destination);call(facade,"setClearColor",{nativeColorConstructor().construct({.25,.5,.75}),1});call(facade,"clear",{true,false,false});
        auto cleared=renderer.readPixelsHalfFloat();require(cleared.size()==16*16*channels,"canonical half-float target readback retains its physical channel count");expectHalf(cleared,channels,{.25,.5,.75,1},"source clear writes canonical float target storage");
        bind(facade);render(renderer,scene({mesh(sampleMaterial(texture,rg,false))}),view);expect(renderer,rg?std::array<int,3>{64,128,255}:std::array<int,3>{64,128,191},"canonical floating target clear is sampled by the actual screen shader");
        bind(facade,destination);render(renderer,scene({mesh(raw("precision highp float;out vec4 result;void main(){result=vec4(2.,4.,.5,.25);}"))}),view);
        auto pixels=renderer.readPixelsHalfFloat();require(pixels.size()==16*16*channels,"rendered floating target readback retains all actual components");expectHalf(pixels,channels,{2,4,.5,.25},"raw shader values above one are preserved in canonical half-float attachments");
        bind(facade);render(renderer,scene({mesh(sampleMaterial(texture,rg,true))}),view);expect(renderer,rg?std::array<int,3>{128,128,191}:std::array<int,3>{128,128,64},"screen sampler divides unclamped floating values back into the analytic byte range");
        require(destination.get("texture").sameValue(texture)&&texture.get("source").sameValue(source)&&texture.get("image").sameValue(image)&&texture.get("renderTarget").sameValue(destination),"float rendering and sampling preserve canonical target/texture/source/image ownership");
        noError(gl,"canonical RG16F/RGBA16F clear, HDR shader draw, readback and screen sampling leave no GL errors");
    }
    layer.verify();
}

int directLight(bool spot,unsigned x){constexpr double y=1-2*(32.5/64);const double position=2*(double(x)+.5)/64-1;return int(std::lround(255*(spot?2/std::sqrt(4+(position+1)*(position+1)+y*y):2/std::sqrt(5.))));}
void verifyBlurPair(JSValue log,unsigned start,JSValue shadow,JSValue camera,unsigned samples,double radius){
    require(log.length()==start+2,"one VSM update issues exactly two actual blur draws");auto vertical=log.get(JSValue(start)),horizontal=log.get(JSValue(start+1));
    auto map=shadow.get("map"),pass=shadow.get("mapPass");
    require(vertical.get("target").sameValue(pass)&&vertical.get("input").sameValue(map.get("depthTexture")),"vertical VSM draw targets the original mapPass and reads the original native depth texture");
    require(horizontal.get("target").sameValue(map)&&horizontal.get("input").sameValue(pass.get("texture")),"horizontal VSM draw targets the original shadow map and reads the first pass's actual color texture");
    require(vertical.get("camera").sameValue(camera)&&horizontal.get("camera").sameValue(camera),"both source blur draws retain the main render camera argument");
    require(vertical.get("mesh").sameValue(horizontal.get("mesh"))&&vertical.get("geometry").sameValue(horizontal.get("geometry")),"both blur draws reuse the original fullscreen mesh and geometry");
    require(vertical.get("horizontal").isUndefined()&&horizontal.get("horizontal").strictEquals(1),"the original horizontal material alone defines HORIZONTAL_PASS");
    for(auto value:{vertical,horizontal}){
        require(value.get("samples").strictEquals(samples)&&value.get("radius").strictEquals(radius),"source blur shader defines and radius retain the current LightShadow values");
        require(value.get("width").sameValue(map.get("width"))&&value.get("height").sameValue(map.get("height")),"source blur uniforms use the actual shadow target dimensions");
        require(!value.get("framebuffer").isNull(),"each source blur draw observes a real bound native framebuffer");
    }
}

void sourceVSM(bool validation,bool spot){
    std::cout<<(spot?"sourceSpotVSM":"sourceDirectionalVSM")<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    constexpr double pi=3.14159265358979323846;auto facade=renderer.object(),gl=call(facade,"getContext"),view=camera(),manager=facade.get("shadowMap");enableFloat(gl);manager.set("enabled",true);manager.set("type",VSMShadowMap);
    auto light=(spot?nativeSpotLightConstructor():nativeDirectionalLightConstructor()).construct({0xffffff,pi});light.set("name",spot?"spot-vsm":"directional-vsm");light.set("castShadow",true);call(light.get("position"),"set",{-1,0,2});if(spot)light.set("decay",0);
    auto shadow=light.get("shadow"),shadowCamera=shadow.get("camera"),lightTarget=light.get("target");call(shadow.get("mapSize"),"set",{128,128});shadow.set("radius",2);shadow.set("bias",-.001);shadowCamera.set("near",.1);shadowCamera.set("far",10);
    if(!spot){shadowCamera.set("left",-2);shadowCamera.set("right",2);shadowCamera.set("top",2);shadowCamera.set("bottom",-2);}call(shadowCamera,"updateProjectionMatrix");
    auto receiverMaterial=nativeMeshLambertMaterialConstructor().construct(),casterMaterial=paint();casterMaterial.set("shadowSide",DoubleSide);receiverMaterial.set("shadowSide",DoubleSide);
    auto receiver=mesh(receiverMaterial),caster=mesh(casterMaterial,.3,1.2,-.6,1);receiver.set("receiveShadow",true);caster.set("castShadow",true);
    auto callbacks=fields({{"caster",0},{"receiver",0}}),log=JSValue::array(),originalDirect=facade.get("renderBufferDirect");
    for(const auto& pair:std::initializer_list<std::pair<JSValue,const char*>>{{caster,"caster"},{receiver,"receiver"}}){auto object=pair.first;object.set("onBeforeShadow",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
        require(self.sameValue(c[0])&&args.size()==7&&args[0].sameValue(c[1])&&args[1].sameValue(self)&&args[2].sameValue(c[2])&&args[3].sameValue(c[3]),"VSM caster/receiver callbacks retain the actual object, renderer, main and shadow camera identities");
        auto counts=c[4];counts.set(c[5],counts.get(c[5]).number()+1);return {};
    },{object,facade,view,shadowCamera,callbacks,pair.second}));}
    facade.set("renderBufferDirect",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
        require(self.sameValue(c[0]),"source renderBufferDirect calls remain observable through the public renderer receiver");
        if(args.size()==6&&args[3].get("isShaderMaterial").strictEquals(true)&&args[3].get("defines").hasOwn("VSM_SAMPLES")){
            auto material=args[3],uniforms=material.get("uniforms"),resolution=uniforms.get("resolution").get("value");
            require(args[1].isNull()&&args[5].isNull()&&args[2].sameValue(args[4].get("geometry")),"VSM source calls retain null scene/group and the original fullscreen geometry");
            auto row=fields({{"target",call(self,"getRenderTarget")},{"input",uniforms.get("shadow_pass").get("value")},{"camera",args[0]},{"mesh",args[4]},{"geometry",args[2]},{"material",material},
                {"horizontal",material.get("defines").get("HORIZONTAL_PASS")},{"samples",material.get("defines").get("VSM_SAMPLES")},{"radius",uniforms.get("radius").get("value")},{"width",resolution.get("x")},{"height",resolution.get("y")},{"framebuffer",call(c[3],"getParameter",{0x8ca6})}});
            auto log=c[1];log.set(JSValue(log.length()),row);
        }
        return c[2].call(self,args);
    },{facade,log,originalDirect,gl}));
    auto world=scene({receiver,caster,light,lightTarget});render(renderer,world,view);verifyBlurPair(log,0,shadow,view,8,2);
    expect(renderer,{0,0,0},"two real VSM blur passes retain an opaque shadow at the analytic receiver point",28);const int lit=directLight(spot,48);expect(renderer,{lit,lit,lit},"unoccluded VSM Lambert light matches its analytic directional/spot golden",48);
    auto map=shadow.get("map"),pass=shadow.get("mapPass"),texture=map.get("texture"),passTexture=pass.get("texture"),depth=map.get("depthTexture"),depthSource=depth.get("source"),mapSource=texture.get("source"),passSource=passTexture.get("source");
    require(!map.sameValue(pass)&&map.get("isWebGLRenderTarget").strictEquals(true)&&pass.get("isWebGLRenderTarget").strictEquals(true),"VSM creates two distinct canonical render-target objects");
    for(auto value:{texture,passTexture})require(value.get("format").strictEquals(RGFormat)&&value.get("type").strictEquals(HalfFloatType)&&value.get("minFilter").strictEquals(LinearFilter)&&value.get("magFilter").strictEquals(LinearFilter)&&value.get("generateMipmaps").strictEquals(false),"both source VSM targets retain pinned RG half-float, linear-filter, no-mipmap defaults");
    require(texture.get("renderTarget").sameValue(map)&&passTexture.get("renderTarget").sameValue(pass),"source VSM color textures retain their original target ownership");
    require(depth.get("isDepthTexture").strictEquals(true)&&depth.get("type").strictEquals(FloatType)&&depth.get("format").strictEquals(DepthFormat)&&depth.get("compareFunction").isNull()&&depth.get("minFilter").strictEquals(NearestFilter)&&depth.get("magFilter").strictEquals(NearestFilter),"VSM reads the pinned ordinary D32F nearest depth texture rather than a comparison sampler");
    require(depth.get("renderTarget").sameValue(map)&&callbacks.get("caster").strictEquals(1)&&callbacks.get("receiver").strictEquals(1),"source VSM writes depth for both shadow casters and receiveShadow-only receivers once");
    require(call(facade,"getRenderTarget").isNull()&&call(gl,"getParameter",{0x8ca6}).isNull(),"source VSM restores both the public render target and the actual default framebuffer");
    auto vertical=log.get("0").get("material"),horizontal=log.get("1").get("material");const auto verticalVersion=vertical.get("version"),horizontalVersion=horizontal.get("version");
    // A standard deviation above zero proves actual neighborhood filtering,
    // beyond just recording the two required fullscreen commands.
    auto distribution=raw("precision highp float;uniform sampler2D moments;in vec2 vUv;out vec4 result;void main(){vec2 d=texture(moments,vUv).rg;result=vec4(d.r,min(1.,d.g*8.),0.,1.);}",fields({{"moments",uniform(texture)}}));
    render(renderer,scene({mesh(distribution)}),view);const auto moments=renderer.readPixels();bool nonzeroVariance=false;for(std::size_t i=0;i<moments.size();i+=4)if(moments[i+1]>4)nonzeroVariance=true;require(nonzeroVariance,"actual two-pass RG16F moments contain nonzero blurred edge variance");require(log.length()==2,"sampling the distribution without lights does not execute extra shadow passes");
    manager.set("autoUpdate",false);caster.get("position").set("x",.6);render(renderer,world,view);expect(renderer,{0,0,0},"manager autoUpdate=false retains the earlier GPU variance shadow after its caster moves",28);require(log.length()==2&&callbacks.get("caster").strictEquals(1),"globally frozen VSM skips depth and both blur draws");
    manager.set("needsUpdate",true);render(renderer,world,view);verifyBlurPair(log,2,shadow,view,8,2);const int refreshed=directLight(spot,28);expect(renderer,{refreshed,refreshed,refreshed},"manager needsUpdate regenerates actual moments and releases the old receiver shadow",28);require(manager.get("needsUpdate").strictEquals(false)&&callbacks.get("caster").strictEquals(2),"manager consumes one forced VSM update");
    manager.set("autoUpdate",true);shadow.set("autoUpdate",false);caster.get("position").set("x",-.6);render(renderer,world,view);expect(renderer,{refreshed,refreshed,refreshed},"LightShadow autoUpdate=false freezes its own actual variance map",28);require(log.length()==4,"a frozen individual light skips its two blur passes");
    shadow.set("needsUpdate",true);render(renderer,world,view);verifyBlurPair(log,4,shadow,view,8,2);expect(renderer,{0,0,0},"LightShadow needsUpdate refreshes the actual variance shadow once",28);require(shadow.get("needsUpdate").strictEquals(false)&&callbacks.get("caster").strictEquals(3),"individual light consumes one forced update and retains its callback contract");
    auto disposals=fields({{"map",0},{"pass",0}});
    for(const auto& pair:std::initializer_list<std::pair<JSValue,const char*>>{{map,"map"},{pass,"pass"}})call(pair.first,"addEventListener",{"dispose",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{require(self.sameValue(c[0])&&args[0].get("target").sameValue(self),"resizing source VSM target dispatches its original dispose event target");auto counts=c[1];counts.set(c[2],counts.get(c[2]).number()+1);return {};},{pair.first,disposals,pair.second})});
    call(shadow.get("mapSize"),"set",{64,64});shadow.set("blurSamples",5);shadow.set("radius",1);shadow.set("needsUpdate",true);render(renderer,world,view);verifyBlurPair(log,6,shadow,view,5,1);
    require(shadow.get("map").sameValue(map)&&shadow.get("mapPass").sameValue(pass)&&map.get("texture").sameValue(texture)&&pass.get("texture").sameValue(passTexture)&&map.get("depthTexture").sameValue(depth),"VSM resize retains both canonical targets and all original texture identities");
    require(texture.get("source").sameValue(mapSource)&&passTexture.get("source").sameValue(passSource)&&depth.get("source").sameValue(depthSource)&&shadow.get("camera").sameValue(shadowCamera),"VSM resize keeps the original source objects and shadow camera");
    require(map.get("width").strictEquals(64)&&pass.get("width").strictEquals(64)&&map.get("height").strictEquals(64)&&pass.get("height").strictEquals(64)&&depth.get("image").get("width").strictEquals(64),"source resize regenerates both float attachments and the actual depth dimensions");
    require(disposals.get("map").strictEquals(1)&&disposals.get("pass").strictEquals(1),"source resize disposes each existing target allocation exactly once");
    require(log.get("6").get("material").sameValue(vertical)&&log.get("7").get("material").sameValue(horizontal)&&vertical.get("version").number()>verticalVersion.number()&&horizontal.get("version").number()>horizontalVersion.number(),"changing blurSamples updates the original two shader materials and their program versions");
    require(!log.get("6").get("framebuffer").sameValue(log.get("0").get("framebuffer"))&&!log.get("7").get("framebuffer").sameValue(log.get("1").get("framebuffer")),"resize replaces the real native map/mapPass framebuffer generations");
    require(caster.get("material").sameValue(casterMaterial)&&receiver.get("material").sameValue(receiverMaterial),"all VSM depth and blur passes preserve the visible application materials");
    facade.set("renderBufferDirect",originalDirect);noError(gl,"VSM real depth/blur/sample/update/resize sequence leaves no GL errors");layer.verify();
}
}
int main(int argc,char** argv){
    try{bool validation=false;std::string only;for(int i=1;i<argc;++i){const std::string arg=argv[i];if(arg=="--validation")validation=true;else if(arg.starts_with("--only="))only=arg.substr(7);}
        const auto run=[&](const char* name,auto function){if(only.empty()||only==name)function(validation);};run("extension",floatExtensionAndNativeAttachments);run("float",sourceFloatingTargetSampling);run("directional",[](bool enabled){sourceVSM(enabled,false);});run("spot",[](bool enabled){sourceVSM(enabled,true);});
        require(only.empty()||only=="extension"||only=="float"||only=="directional"||only=="spot","--only selects an existing VSM GPU scenario");
        std::cout<<"managed VSM shadows assertions="<<assertions<<" renders="<<renders<<" validation_requested="<<validationRequested<<" validation_enabled="<<validationEnabled<<" validation_messages="<<validationMessages<<'\n';return 0;
    }catch(const std::exception& error){std::cerr<<"Managed VSM shadows failed: "<<error.what()<<'\n';return 1;}
}
