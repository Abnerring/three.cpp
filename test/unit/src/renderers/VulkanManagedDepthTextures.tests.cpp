#include "renderers/VulkanRenderer.h"
#include "renderers/WebGLRenderTarget.h"
#include "renderers/WebGLCubeRenderTarget.h"
#include "textures/DepthTexture.h"
#include "textures/CubeDepthTexture.h"
#include "materials/RawShaderMaterial.h"
#include "materials/MeshBasicMaterial.h"
#include "materials/MeshLambertMaterial.h"
#include "core/NativeArray.h"
#include "scenes/Scene.h"
#include "cameras/OrthographicCamera.h"
#include "geometries/PlaneGeometry.h"
#include "objects/Mesh.h"
#include "lights/DirectionalLight.h"
#include "lights/SpotLight.h"
#include "lights/PointLight.h"
#include "math/Vector3.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

// Real source renderer -> native depth attachment/upload -> original ESSL
// sampler2DShadow/sampler2D -> Vulkan RGBA pixels. The companion pinned CPU
// oracle establishes source defaults/protocol and analytic goldens, not a GPU.
namespace {
using Args=std::vector<JSValue>;
int assertions=0,renders=0,validationRequested=0,validationEnabled=0,validationMessages=0;
void require(bool value,const char* message){++assertions;if(!value)throw std::runtime_error(message);}
JSValue call(JSValue self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
JSValue fields(std::initializer_list<std::pair<const char*,JSValue>> values){auto result=JSValue::object();for(const auto& [name,value]:values)result.set(name,value);return result;}
JSValue uniform(JSValue value){return fields({{"value",value}});}
JSValue camera(){auto result=nativeOrthographicCameraConstructor().construct({-1,1,1,-1,.1,10});result.get("position").set("z",3);return result;}
JSValue scene(const Args& objects={}){auto result=nativeSceneConstructor().construct();if(!objects.empty())call(result,"add",objects);return result;}
JSValue mesh(JSValue material,double width=2,double height=2,double x=0,double z=0){
    auto result=nativeMeshConstructor().construct({nativePlaneGeometryConstructor().construct({width,height}),material});
    result.set("frustumCulled",false);call(result.get("position"),"set",{x,0,z});return result;
}
JSValue paint(unsigned color=0xff0000,JSValue map={}){auto options=fields({{"color",color}});if(!map.isUndefined())options.set("map",map);return nativeMeshBasicMaterialConstructor().construct({options});}
void bind(JSValue renderer,JSValue target=nullptr){call(renderer,"setRenderTarget",{target});}
void linear(VulkanRenderer& renderer){renderer.srgbOutput=false;renderer.object().set("outputColorSpace",LinearSRGBColorSpace);}
void render(VulkanRenderer& renderer,JSValue world,JSValue view){renderer.render(world,view);++renders;}
void expect(const VulkanRenderer& renderer,std::array<int,3> expected,const char* message,unsigned x=32,unsigned y=32){
    const auto data=renderer.readPixels();require(data.size()==64*64*4,"depth sampling returns a complete 64x64 screen image");const auto at=(y*64+x)*4;
    for(unsigned i=0;i<3;++i)if(std::abs(int(data.at(at+i))-expected[i])>2){std::cerr<<message<<" actual="<<int(data.at(at))<<','<<int(data.at(at+1))<<','<<int(data.at(at+2))<<'\n';require(false,message);}
    require(true,message);
}
void noError(JSValue gl,const char* message){require(call(gl,"getError").strictEquals(0),message);}
struct Validation {
    const VulkanRenderer& renderer;
    Validation(const VulkanRenderer& value,bool requested):renderer(value){if(requested)++validationRequested;if(value.getInfo().validationEnabled)++validationEnabled;}
    void verify(){const auto messages=renderer.getValidationMessages();validationMessages+=int(messages.size());for(const auto& message:messages)std::cerr<<message<<'\n';require(messages.empty(),"depth GPU work emits no collected Vulkan validation messages");}
};
const char* vertex="precision highp float;in vec3 position;in vec2 uv;uniform mat4 projectionMatrix;uniform mat4 modelViewMatrix;out vec2 vUv;void main(){vUv=uv;gl_Position=projectionMatrix*modelViewMatrix*vec4(position,1.);}";
JSValue raw(const char* fragment,JSValue uniforms){return nativeRawShaderMaterialConstructor().construct({fields({{"glslVersion",GLSL3},{"vertexShader",vertex},{"fragmentShader",fragment},{"uniforms",uniforms}})});}
JSValue shadow(JSValue texture,double reference=.5,bool split=false){return raw(
    "precision highp float;uniform highp sampler2DShadow depthMap;uniform float reference;uniform bool split;in vec2 vUv;out vec4 result;void main(){float ref=split?(vUv.x<.5?.1:.9):reference;float d=texture(depthMap,vec3(vUv,ref));result=vec4(vec3(d),1.);}",
    fields({{"depthMap",uniform(texture)},{"reference",uniform(reference)},{"split",uniform(split)}}));}
JSValue ordinary(JSValue texture){return raw(
    "precision highp float;uniform highp sampler2D depthMap;in vec2 vUv;out vec4 result;void main(){float d=texture(depthMap,vUv).r;result=vec4(vec3(d),1.);}",fields({{"depthMap",uniform(texture)}}));}
JSValue depthTexture(int width,int height,double type=UnsignedShortType,JSValue compare=LessEqualCompare){
    auto texture=nativeDepthTextureConstructor().construct({width,height,type});texture.set("compareFunction",compare);return texture;
}
JSValue target(JSValue depth,int width=32,int height=32){return nativeWebGLRenderTargetConstructor().construct({width,height,fields({{"depthTexture",depth}})});}
JSValue depthHandle(JSValue gl){return call(gl,"getFramebufferAttachmentParameter",{0x8d40,0x8d00,0x8cd1});}
void clearDepth(JSValue renderer,JSValue gl,JSValue destination,double value){
    bind(renderer,destination);call(gl,"clearDepth",{value});call(renderer,"clear",{false,true,false});
    // Public GL changes bypass the source WebGLState clear-depth cache. Restore
    // its default before a later screen pass, without changing this attachment.
    call(gl,"clearDepth",{1});
}
void sample(VulkanRenderer& renderer,JSValue material,JSValue view){bind(renderer.object());render(renderer,scene({mesh(material)}),view);}
void setNativeCompare(JSValue gl,JSValue handle,unsigned function,unsigned mode=0x884e){
    call(gl,"activeTexture",{0x84c0});call(gl,"bindTexture",{0x0de1,handle});call(gl,"texParameteri",{0x0de1,0x884c,mode});call(gl,"texParameteri",{0x0de1,0x884d,function});
}

void canonicalClearAndCompare(bool validation){
    std::cout<<"canonicalClearAndCompare"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto facade=renderer.object(),gl=call(facade,"getContext"),view=camera();
    for(const double type:{double(UnsignedShortType),double(UnsignedIntType),double(FloatType)}){
        const auto depth=depthTexture(32,32,type),source=depth.get("source"),image=depth.get("image"),destination=target(depth),color=destination.get("texture");
        require(depth.get("isDepthTexture").strictEquals(true)&&depth.get("format").strictEquals(DepthFormat),"canonical DepthTexture keeps its source class and depth format");
        require(destination.get("depthTexture").sameValue(depth)&&depth.get("renderTarget").sameValue(destination),"render-target setter retains the original depth texture and owner");
        clearDepth(facade,gl,destination,.25);const auto handle=depthHandle(gl);
        require(!handle.isNull()&&call(gl,"checkFramebufferStatus",{0x8d40}).strictEquals(0x8cd5),"real depth texture attachment makes the native framebuffer complete");
        const auto attachment=call(gl,"getFramebufferAttachmentParameter",{0x8d40,0x8d00,0x8cd0});require(attachment.strictEquals(0x1702),"DEPTH_ATTACHMENT exposes a real GL texture object, not a renderbuffer");
        sample(renderer,shadow(depth,.5,true),view);
        expect(renderer,{255,255,255},"ref .1 LEQUAL depth clear .25 passes the actual shadow comparison",16);
        expect(renderer,{0,0,0},"ref .9 LEQUAL depth clear .25 fails the actual shadow comparison",48);
        require(destination.get("texture").sameValue(color)&&depth.get("source").sameValue(source)&&depth.get("image").sameValue(image),"depth GPU allocation and sampling preserve color/source/image identities");
        bind(facade,destination);require(depthHandle(gl).sameValue(handle),"switching between screen and depth target retains the actual native attachment handle");bind(facade);
        noError(gl,"D16/D24/D32F clear and comparison complete without GL errors");
    }
    layer.verify();
}

void nativeDepthUploadAndOrdinarySampling(bool validation){
    std::cout<<"nativeDepthUploadAndOrdinarySampling"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto facade=renderer.object(),gl=call(facade,"getContext"),view=camera(),depth=depthTexture(2,1),destination=target(depth,2,1);
    clearDepth(facade,gl,destination,.9);const auto handle=depthHandle(gl),version=depth.get("version");bind(facade);
    call(gl,"activeTexture",{0x84c0});call(gl,"bindTexture",{0x0de1,handle});call(gl,"pixelStorei",{0x0cf5,1});
    call(gl,"texSubImage2D",{0x0de1,0,0,0,2,1,0x1902,0x1403,NativeArray_detail::wrap(Uint16Array{0x4000,0xc000})});
    sample(renderer,shadow(depth,.5),view);
    expect(renderer,{0,0,0},"public native D16 upload .25 rejects a .5 comparison independently of attachment clear",16);
    expect(renderer,{255,255,255},"public native D16 upload .75 accepts a .5 comparison independently of attachment clear",48);
    require(depth.get("version").sameValue(version),"native depth upload does not invent an application texture version");
    setNativeCompare(gl,handle,0x0203,0); // COMPARE_MODE NONE: sampler2D is now legal.
    sample(renderer,ordinary(depth),view);
    expect(renderer,{64,64,64},"comparison-disabled sampler2D reads the actual uploaded normalized .25 depth",16);
    expect(renderer,{191,191,191},"comparison-disabled sampler2D reads the actual uploaded normalized .75 depth",48);
    setNativeCompare(gl,handle,0x0203);sample(renderer,shadow(depth,.5),view);
    expect(renderer,{0,0,0},"restoring native comparison mode recovers shadow sampling",16);expect(renderer,{255,255,255},"restored native comparison mode preserves uploaded depth contents",48);
    noError(gl,"native D16 upload/compare-mode changes and ordinary sampling leave no GL error");
    for(const double type:{double(UnsignedIntType),double(FloatType)}){
        const auto otherDepth=depthTexture(2,1,type),otherTarget=target(otherDepth,2,1);
        clearDepth(facade,gl,otherTarget,.9);const auto otherHandle=depthHandle(gl),otherVersion=otherDepth.get("version");bind(facade);
        call(gl,"activeTexture",{0x84c0});call(gl,"bindTexture",{0x0de1,otherHandle});
        const bool floating=type==FloatType;
        const auto pixels=floating?NativeArray_detail::wrap(Float32Array{.25f,.25f}):NativeArray_detail::wrap(Uint32Array{0x40000000u,0x40000000u});
        // GL unsigned-int depth is normalized across all 32 input bits. Vulkan
        // X8_D24 instead stores its normalized value in the low 24 bits. A raw
        // memcpy of 0x40000000 into X8_D24 would produce zero and fail both tests.
        call(gl,"texSubImage2D",{0x0de1,0,0,0,2,1,0x1902,floating?0x1406:0x1405,pixels});
        sample(renderer,shadow(otherDepth,.5,true),view);
        expect(renderer,{255,255,255},floating?"public D32F .25 upload accepts shadow ref .1":"public D24 32-bit normalized .25 upload accepts shadow ref .1",16);
        expect(renderer,{0,0,0},floating?"public D32F .25 upload rejects shadow ref .9":"public D24 32-bit normalized .25 upload rejects shadow ref .9",48);
        setNativeCompare(gl,otherHandle,0x0203,0);sample(renderer,ordinary(otherDepth),view);
        expect(renderer,{64,64,64},floating?"ordinary sampler2D reads the actual uploaded D32F .25":"ordinary sampler2D reads .25 after GL uint32 to Vulkan low24 conversion");
        require(otherDepth.get("version").sameValue(otherVersion),"native D24/D32F upload preserves the original source texture version");
        noError(gl,"native D24/D32F upload and both depth sampler modes leave no GL errors");
    }
    // Source is uint32 while the actual attachment is D16. Non-default unpack
    // skips must use the four-byte source stride before quantizing to two bytes.
    const auto convertedDepth=depthTexture(2,1),convertedTarget=target(convertedDepth,2,1);
    clearDepth(facade,gl,convertedTarget,.9);const auto convertedHandle=depthHandle(gl);bind(facade);
    call(gl,"activeTexture",{0x84c0});call(gl,"bindTexture",{0x0de1,convertedHandle});
    call(gl,"pixelStorei",{0x0cf2,4});call(gl,"pixelStorei",{0x0cf3,1});call(gl,"pixelStorei",{0x0cf4,1});
    call(gl,"texSubImage2D",{0x0de1,0,0,0,2,1,0x1902,0x1405,NativeArray_detail::wrap(Uint32Array{0,0,0,0,0,0x40000000u,0xc0000000u,0})});
    call(gl,"pixelStorei",{0x0cf2,0});call(gl,"pixelStorei",{0x0cf3,0});call(gl,"pixelStorei",{0x0cf4,0});
    sample(renderer,shadow(convertedDepth,.5),view);expect(renderer,{0,0,0},"D16 accepts uint32 normalized .25 after unpack row/pixel skips",16);expect(renderer,{255,255,255},"D16 accepts uint32 normalized .75 after unpack row/pixel skips",48);
    setNativeCompare(gl,convertedHandle,0x0203,0);sample(renderer,ordinary(convertedDepth),view);
    expect(renderer,{64,64,64},"D16 converts uint32 input to the actual 16-bit normalized .25",16);expect(renderer,{191,191,191},"D16 uint32 conversion preserves independently unpacked .75",48);
    const auto clampedDepth=depthTexture(2,1,FloatType),clampedTarget=target(clampedDepth,2,1);
    clearDepth(facade,gl,clampedTarget,.5);const auto clampedHandle=depthHandle(gl);bind(facade);
    call(gl,"activeTexture",{0x84c0});call(gl,"bindTexture",{0x0de1,clampedHandle});
    call(gl,"texSubImage2D",{0x0de1,0,0,0,2,1,0x1902,0x1406,NativeArray_detail::wrap(Float32Array{-.5f,1.5f})});
    sample(renderer,shadow(clampedDepth,.5),view);expect(renderer,{0,0,0},"D32F negative source depth clamps to zero for comparison",16);expect(renderer,{255,255,255},"D32F source above one clamps to one for comparison",48);
    setNativeCompare(gl,clampedHandle,0x0203,0);sample(renderer,ordinary(clampedDepth),view);
    expect(renderer,{0,0,0},"D32F ordinary sampling reads clamped zero from negative upload",16);expect(renderer,{255,255,255},"D32F ordinary sampling reads clamped one from upload above one",48);
    sample(renderer,raw("precision highp float;uniform highp sampler2D depthMap;in vec2 vUv;out vec4 result;void main(){float d=.25+.5*texture(depthMap,vUv).r;result=vec4(vec3(d),1.);}",fields({{"depthMap",uniform(clampedDepth)}})),view);
    expect(renderer,{64,64,64},"scaled ordinary read proves negative D32F depth was clamped before sampling",16);expect(renderer,{191,191,191},"scaled ordinary read proves D32F depth above one was clamped before sampling",48);
    noError(gl,"uint32-to-D16 conversion and float depth clamping leave no GL errors");
    layer.verify();
}

void comparisonFunctionsAndSamplerLifetime(bool validation){
    std::cout<<"comparisonFunctionsAndSamplerLifetime"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    auto facade=renderer.object(),gl=call(facade,"getContext"),view=camera(),depth=depthTexture(32,32),destination=target(depth);
    clearDepth(facade,gl,destination,.25);const auto handle=depthHandle(gl);bind(facade);
    const std::array<std::pair<unsigned,std::array<int,2>>,8> cases={{{0x0200,{0,0}},{0x0201,{255,0}},{0x0202,{0,0}},{0x0203,{255,0}},{0x0204,{0,255}},{0x0205,{255,255}},{0x0206,{0,255}},{0x0207,{255,255}}}};
    for(const auto& [function,expected]:cases){setNativeCompare(gl,handle,function);sample(renderer,shadow(depth,.5,true),view);expect(renderer,{expected[0],expected[0],expected[0]},"native depth comparison function controls ref .1",16);expect(renderer,{expected[1],expected[1],expected[1]},"native depth comparison function controls ref .9",48);}
    setNativeCompare(gl,handle,0x0203);
    auto left=mesh(shadow(depth,.1),1,2,-.5),right=mesh(shadow(depth,.1),1,2,.5);left.set("renderOrder",0);right.set("renderOrder",1);
    right.set("onBeforeRender",JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue {setNativeCompare(captures[0],captures[1],0x0204);return {};},{gl,handle}));
    render(renderer,scene({left,right}),view);
    expect(renderer,{255,255,255},"earlier draw retains the old LEQUAL Vulkan sampler through same-frame replacement",16);
    expect(renderer,{0,0,0},"later draw sees the newly committed GREATER sampler in the same frame",48);
    // Exercise the canonical property/update route independently of direct GL.
    depth.set("compareFunction",LessEqualCompare);depth.set("needsUpdate",true);sample(renderer,shadow(depth,.1),view);expect(renderer,{255,255,255},"canonical compareFunction needsUpdate commits LEQUAL sampling");
    depth.set("compareFunction",GreaterCompare);depth.set("needsUpdate",true);sample(renderer,shadow(depth,.1),view);expect(renderer,{0,0,0},"canonical compareFunction needsUpdate commits GREATER sampling");
    noError(gl,"comparison functions and sampler replacement preserve a clean native error state");layer.verify();
}

void meshWritesDepthWithoutColor(bool validation){
    std::cout<<"meshWritesDepthWithoutColor"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    auto facade=renderer.object(),gl=call(facade,"getContext"),view=camera(),depth=depthTexture(32,32),destination=target(depth),writer=paint();
    call(facade,"setClearColor",{0x0000ff,1});clearDepth(facade,gl,destination,1);writer.set("colorWrite",false);
    // Orthographic depth is (distance-near)/(far-near): camera z=3,
    // plane z=.425, near=.1 and far=10 give exactly .25.
    render(renderer,scene({mesh(writer,1,2,-.5,.425)}),view);
    sample(renderer,shadow(depth,.5),view);
    expect(renderer,{0,0,0},"real mesh writes .25 depth into the left half and rejects ref .5",16);
    expect(renderer,{255,255,255},"uncovered right half keeps the real depth clear of one",48);
    sample(renderer,paint(0xffffff,destination.get("texture")),view);expect(renderer,{0,0,255},"colorWrite false preserves blue color while the same draw updates the depth texture",16);
    // A target with depthBuffer=false follows submission order, with no hidden
    // depth attachment or rejection of the later farther surface.
    const auto noDepth=nativeWebGLRenderTargetConstructor().construct({32,32,fields({{"depthBuffer",false}})});
    auto near=mesh(paint(0xff0000),2,2,0,.5),far=mesh(paint(0x00ff00));near.set("renderOrder",0);far.set("renderOrder",1);
    bind(facade,noDepth);render(renderer,scene({near,far}),view);require(call(gl,"getParameter",{0x0d56}).strictEquals(0),"depthBuffer false exposes zero depth bits");
    sample(renderer,paint(0xffffff,noDepth.get("texture")),view);expect(renderer,{0,255,0},"depthBuffer false lets the farther later mesh overwrite color");
    noError(gl,"mesh depth writes and depthBuffer=false color rendering finish without GL errors");layer.verify();
}

void invalidSamplerModesAndFeedback(bool validation){
    std::cout<<"invalidSamplerModesAndFeedback"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    auto facade=renderer.object(),gl=call(facade,"getContext"),view=camera(),depth=depthTexture(32,32),destination=target(depth);
    call(facade,"setClearColor",{0x00ff00,1});clearDepth(facade,gl,destination,.25);const auto handle=depthHandle(gl);
    // A feedback error must preserve both the existing color and depth data.
    call(facade,"clear",{true,false,false});facade.set("autoClear",false);render(renderer,scene({mesh(shadow(depth,.1))}),view);
    require(call(gl,"getError").strictEquals(0x0502)&&call(gl,"getError").strictEquals(0),"sampling the attached depth texture reports INVALID_OPERATION exactly once");
    facade.set("autoClear",true);sample(renderer,paint(0xffffff,destination.get("texture")),view);expect(renderer,{0,255,0},"depth attachment feedback rejects the draw before any color write");
    sample(renderer,shadow(depth,.5,true),view);expect(renderer,{255,255,255},"feedback rejection preserves the depth clear for later legal sampling",16);expect(renderer,{0,0,0},"feedback rejection preserves the depth clear's failed comparison",48);
    render(renderer,scene({mesh(paint(0x00ff00))}),view);facade.set("autoClear",false);
    const auto rejected=[&](JSValue material,const char* message){render(renderer,scene({mesh(material)}),view);require(call(gl,"getError").strictEquals(0x0502)&&call(gl,"getError").strictEquals(0),message);expect(renderer,{0,255,0},"invalid sampler mode/type leaves the actual screen pixels unchanged");};
    rejected(ordinary(depth),"sampler2D with depth comparison enabled reports INVALID_OPERATION/no draw");
    setNativeCompare(gl,handle,0x0203,0);rejected(shadow(depth,.1),"sampler2DShadow with comparison disabled reports INVALID_OPERATION/no draw");
    rejected(shadow(destination.get("texture"),.1),"sampler2DShadow bound to color storage reports INVALID_OPERATION/no draw");
    setNativeCompare(gl,handle,0x0203);facade.set("autoClear",true);sample(renderer,shadow(depth,.1),view);expect(renderer,{255,255,255},"valid depth comparison recovers after invalid mode/type draws");
    noError(gl,"valid recovery consumes all sampler mismatch errors");layer.verify();
}

void sourcePCFShadow(bool validation,bool spot){
    std::cout<<(spot?"sourceSpotPCFShadow":"sourceDirectionalPCFShadow")<<std::endl;
    VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    constexpr double pi=3.14159265358979323846;
    auto facade=renderer.object(),gl=call(facade,"getContext"),view=camera(),shadowMap=facade.get("shadowMap");
    require(shadowMap.get("type").strictEquals(PCFShadowMap),"production source shadow map retains the pinned default PCF type");
    shadowMap.set("enabled",true);
    auto light=(spot?nativeSpotLightConstructor():nativeDirectionalLightConstructor()).construct({0xffffff,pi});
    call(light.get("position"),"set",{-1,0,2});light.set("castShadow",true);if(spot)light.set("decay",0);
    auto sourceShadow=light.get("shadow"),sourceCamera=sourceShadow.get("camera"),lightTarget=light.get("target");
    call(sourceShadow.get("mapSize"),"set",{128,128});sourceCamera.set("near",.1);sourceCamera.set("far",10);
    if(!spot){sourceCamera.set("left",-2);sourceCamera.set("right",2);sourceCamera.set("top",2);sourceCamera.set("bottom",-2);}
    call(sourceCamera,"updateProjectionMatrix");
    auto receiverMaterial=nativeMeshLambertMaterialConstructor().construct(),casterMaterial=paint(0xff0000);
    casterMaterial.set("shadowSide",DoubleSide);
    auto receiver=mesh(receiverMaterial),caster=mesh(casterMaterial,.3,1.2,-.6,1);receiver.set("receiveShadow",true);caster.set("castShadow",true);
    const auto seen=fields({{"before",0},{"after",0}});
    caster.set("onBeforeShadow",JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue {
        require(self.sameValue(captures[0])&&args.size()==7&&args[0].sameValue(captures[1])&&args[1].sameValue(self),"source shadow callback retains object/renderer identities and original argument shape");
        require(args[2].sameValue(captures[2])&&args[3].sameValue(captures[3]),"source shadow callback passes the original main and light shadow cameras");
        auto state=captures[4];state.set("before",state.get("before").number()+1);
        if(state.get("depthMaterial").isUndefined())state.set("depthMaterial",args[5]);else require(state.get("depthMaterial").sameValue(args[5]),"source reuses its internal shadow depth material across map updates");
        return {};
    },{caster,facade,view,sourceCamera,seen}));
    caster.set("onAfterShadow",JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue {
        require(self.sameValue(captures[0])&&args.size()==7&&args[5].sameValue(captures[1].get("depthMaterial")),"after-shadow callback sees the same selected source depth material");
        auto state=captures[1];state.set("after",state.get("after").number()+1);return {};
    },{caster,seen}));
    const auto world=scene({receiver,caster,light,lightTarget});render(renderer,world,view);
    expect(renderer,{0,0,0},"default PCF uses the actual caster depth to shadow the receiver",28);
    const double sampleX=2.0*(48.5/64.0)-1.0,sampleY=1.0-2.0*(32.5/64.0);
    const double cosine=spot?2/std::sqrt(4+(sampleX+1)*(sampleX+1)+sampleY*sampleY):2/std::sqrt(5.0);
    const int direct=int(std::lround(255*cosine));
    expect(renderer,{direct,direct,direct},"default PCF preserves the analytic unoccluded Lambert direct lighting",48);
    const auto map=sourceShadow.get("map"),depth=map.get("depthTexture");
    require(map.get("isWebGLRenderTarget").strictEquals(true)&&depth.get("isDepthTexture").strictEquals(true),"source shadow rendering allocates canonical render target and DepthTexture objects");
    require(depth.get("compareFunction").strictEquals(LessEqualCompare)&&depth.get("minFilter").strictEquals(LinearFilter)&&depth.get("magFilter").strictEquals(LinearFilter),"default source PCF chooses hardware LEQUAL comparison and linear filters");
    require(depth.get("renderTarget").sameValue(map)&&sourceShadow.get("camera").sameValue(sourceCamera)&&light.get("target").sameValue(lightTarget),"shadow allocation preserves the original target link and light camera/target identities");
    require(caster.get("material").sameValue(casterMaterial)&&receiver.get("material").sameValue(receiverMaterial),"source shadow depth pass preserves both original visible materials");
    require(seen.get("before").strictEquals(1)&&seen.get("after").strictEquals(1),"one source caster depth pass issues one before/after shadow callback pair");
    require(call(facade,"getRenderTarget").isNull()&&call(gl,"getParameter",{0x8ca6}).isNull(),"source shadow pass restores both the public target and actual screen framebuffer");
    shadowMap.set("autoUpdate",false);caster.get("position").set("x",.6);render(renderer,world,view);
    expect(renderer,{0,0,0},"source autoUpdate=false preserves the earlier real shadow map after the caster moves",28);
    require(seen.get("before").strictEquals(1)&&seen.get("after").strictEquals(1),"frozen source shadow map skips the depth draw and its callbacks");
    shadowMap.set("needsUpdate",true);render(renderer,world,view);
    const double shadowX=2.0*(28.5/64.0)-1.0;
    const double refreshedCosine=spot?2/std::sqrt(4+(shadowX+1)*(shadowX+1)+sampleY*sampleY):2/std::sqrt(5.0);
    const int refreshed=int(std::lround(255*refreshedCosine));
    expect(renderer,{refreshed,refreshed,refreshed},"source needsUpdate regenerates actual depth and releases the old receiver shadow",28);
    require(shadowMap.get("needsUpdate").strictEquals(false)&&seen.get("before").strictEquals(2)&&seen.get("after").strictEquals(2),"source consumes one forced shadow update and replays its callbacks once");
    require(sourceShadow.get("map").sameValue(map)&&map.get("depthTexture").sameValue(depth)&&sourceShadow.get("camera").sameValue(sourceCamera),"source map refresh retains target/depthTexture/camera identity");
    noError(gl,"default source PCF shadow rendering and update flags finish without GL errors");layer.verify();
}

const std::array<std::array<double,3>,6> cubeDirections={{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}};
JSValue cubeSample(JSValue depth,unsigned face,bool comparison=true,double reference=.5){
    const auto direction=nativeVector3Constructor().construct({cubeDirections[face][0],cubeDirections[face][1],cubeDirections[face][2]});
    return raw(comparison?
        "precision highp float;uniform highp samplerCubeShadow depthMap;uniform vec3 direction;uniform float reference;out vec4 result;void main(){float d=texture(depthMap,vec4(direction,reference));result=vec4(vec3(d),1.);}":
        "precision highp float;uniform highp samplerCube depthMap;uniform vec3 direction;out vec4 result;void main(){float d=texture(depthMap,direction).r;result=vec4(vec3(d),1.);}",
        fields({{"depthMap",uniform(depth)},{"direction",uniform(direction)},{"reference",uniform(reference)}}));
}
void cubeDepthFacesAndSampling(bool validation){
    std::cout<<"cubeDepthFacesAndSampling"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    auto facade=renderer.object(),gl=call(facade,"getContext"),view=camera(),depth=nativeCubeDepthTextureConstructor().construct({16});
    depth.set("compareFunction",LessEqualCompare);
    auto destination=nativeWebGLCubeRenderTargetConstructor().construct({16});destination.set("depthTexture",depth);
    const auto source=depth.get("source"),images=depth.get("image");
    require(destination.get("isWebGLCubeRenderTarget").strictEquals(true)&&depth.get("isCubeDepthTexture").strictEquals(true)&&depth.get("isDepthTexture").strictEquals(true)&&depth.get("isCubeTexture").strictEquals(true),"six-face fixture uses canonical cube target and cube depth texture classes");
    require(depth.get("renderTarget").sameValue(destination)&&images.length()==6&&depth.get("images").sameValue(images),"canonical cube depth attachment preserves its target link and six image descriptors");
    constexpr std::array<double,6> depths={.1,.2,.3,.6,.7,.8};std::array<JSValue,6> framebuffers;JSValue handle;
    for(unsigned face=0;face<6;++face){
        call(facade,"setRenderTarget",{destination,face});call(gl,"clearDepth",{depths[face]});call(facade,"clear",{false,true,false});call(gl,"clearDepth",{1});
        framebuffers[face]=call(gl,"getParameter",{0x8ca6});const auto current=depthHandle(gl);if(face==0)handle=current;else require(current.sameValue(handle),"all six framebuffer faces refer to one actual cube depth texture handle");
        require(call(gl,"getFramebufferAttachmentParameter",{0x8d40,0x8d00,0x8cd3}).strictEquals(0x8515+face),"native depth attachment query exposes the currently selected cube face");
        require(call(gl,"checkFramebufferStatus",{0x8d40}).strictEquals(0x8cd5),"each cube face has a complete real color/depth framebuffer");
        for(unsigned previous=0;previous<face;++previous)require(!framebuffers[face].sameValue(framebuffers[previous]),"source cube target owns a distinct framebuffer for each selected face");
    }
    for(unsigned face=0;face<6;++face){sample(renderer,cubeSample(depth,face),view);const int expected=depths[face]>=.5?255:0;expect(renderer,{expected,expected,expected},"samplerCubeShadow compares the independently cleared depth of the selected face");}
    call(gl,"activeTexture",{0x84c0});call(gl,"bindTexture",{0x8513,handle});call(gl,"texParameteri",{0x8513,0x884c,0});
    for(unsigned face=0;face<6;++face){sample(renderer,cubeSample(depth,face,false),view);const int expected=int(std::lround(255*depths[face]));expect(renderer,{expected,expected,expected},"ordinary samplerCube reads the actual independently stored depth of each face");}
    require(depth.get("source").sameValue(source)&&depth.get("image").sameValue(images)&&destination.get("depthTexture").sameValue(depth),"six-face rendering and both sampling modes retain canonical source/image/depth identities");
    require(call(facade,"getRenderTarget").isNull()&&call(gl,"getParameter",{0x8ca6}).isNull(),"cube depth sampling restores both source and native default targets");noError(gl,"six-face cube depth clear and sampling produce no GL errors");layer.verify();
}

void sourcePointPCFShadow(bool validation){
    std::cout<<"sourcePointPCFShadow"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    constexpr double pi=3.14159265358979323846;
    auto facade=renderer.object(),gl=call(facade,"getContext"),view=camera(),shadowMap=facade.get("shadowMap");shadowMap.set("enabled",true);
    require(shadowMap.get("type").strictEquals(PCFShadowMap),"point light uses the original source default PCF shadow type");
    auto light=nativePointLightConstructor().construct({0xffffff,pi});call(light.get("position"),"set",{-1,0,2});light.set("decay",0);light.set("castShadow",true);
    const auto sourceShadow=light.get("shadow");auto sourceCamera=sourceShadow.get("camera");sourceCamera.set("near",.1);sourceCamera.set("far",10);call(sourceCamera,"updateProjectionMatrix");call(sourceShadow.get("mapSize"),"set",{128,128});
    auto receiverMaterial=nativeMeshLambertMaterialConstructor().construct(),casterMaterial=paint(0xff0000);casterMaterial.set("shadowSide",DoubleSide);
    auto receiver=mesh(receiverMaterial),caster=mesh(casterMaterial,.3,1.2,-.6,1);receiver.set("receiveShadow",true);caster.set("castShadow",true);
    const auto seen=fields({{"before",0},{"after",0}});
    caster.set("onBeforeShadow",JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue {
        require(self.sameValue(captures[0])&&args.size()==7&&args[0].sameValue(captures[1])&&args[1].sameValue(self)&&args[2].sameValue(captures[2])&&args[3].sameValue(captures[3]),"point shadow callback passes the original object, renderer, main camera and rotated source shadow camera");
        const auto face=unsigned(call(captures[1],"getActiveCubeFace").number());require(face<6,"point callback runs on an actual cube face");auto state=captures[4];const auto count=unsigned(state.get("before").number());require(face==count%6,"point source visits cube depth faces in its pinned zero-through-five order");
        const auto up=args[3].get("up");constexpr std::array<std::array<int,3>,6> ups={{{0,-1,0},{0,-1,0},{0,0,1},{0,0,-1},{0,-1,0},{0,-1,0}}};
        require(up.get("x").strictEquals(ups[face][0])&&up.get("y").strictEquals(ups[face][1])&&up.get("z").strictEquals(ups[face][2]),"each point shadow face retains the original cube camera up vector");
        require(args[3].get("position").get("x").strictEquals(-1)&&args[3].get("position").get("z").strictEquals(2),"each point shadow camera face stays at the real light position");
        const auto map=call(captures[1],"getRenderTarget");require(map.get("isWebGLCubeRenderTarget").strictEquals(true),"point shadow callback sees its real canonical cube render target");
        if(state.get("depthMaterial").isUndefined())state.set("depthMaterial",args[5]);else require(state.get("depthMaterial").sameValue(args[5]),"all point faces reuse the source-selected depth material");
        state.set("before",count+1);return {};
    },{caster,facade,view,sourceCamera,seen}));
    caster.set("onAfterShadow",JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue {
        require(self.sameValue(captures[0])&&args.size()==7&&args[3].sameValue(captures[1])&&args[5].sameValue(captures[2].get("depthMaterial")),"point after-shadow callback retains the original face camera and depth material");auto state=captures[2];state.set("after",state.get("after").number()+1);return {};
    },{caster,sourceCamera,seen}));
    const auto world=scene({receiver,caster,light});render(renderer,world,view);
    expect(renderer,{0,0,0},"default point PCF cube depth casts a real shadow onto the receiver",28);
    const double directX=2*(48.5/64)-1,y=1-2*(32.5/64);const int direct=int(std::lround(255*2/std::sqrt(4+(directX+1)*(directX+1)+y*y)));
    expect(renderer,{direct,direct,direct},"unoccluded point Lambert lighting matches the analytic direct-light golden",48);
    const auto map=sourceShadow.get("map"),depth=map.get("depthTexture"),images=depth.get("image");
    require(map.get("isWebGLCubeRenderTarget").strictEquals(true)&&depth.get("isCubeDepthTexture").strictEquals(true)&&depth.get("isCubeTexture").strictEquals(true)&&images.length()==6,"default point PCF allocates canonical six-face cube target/depth objects");
    require(depth.get("compareFunction").strictEquals(LessEqualCompare)&&depth.get("minFilter").strictEquals(LinearFilter)&&depth.get("renderTarget").sameValue(map),"point PCF retains source hardware-comparison/filter defaults and depth target identity");
    require(seen.get("before").strictEquals(6)&&seen.get("after").strictEquals(6),"one real point shadow update draws and calls back on all six cube depth faces");
    require(sourceShadow.get("camera").sameValue(sourceCamera)&&receiver.get("material").sameValue(receiverMaterial)&&caster.get("material").sameValue(casterMaterial),"point depth rendering retains the original source camera and visible materials");
    require(call(facade,"getRenderTarget").isNull()&&call(gl,"getParameter",{0x8ca6}).isNull()&&call(facade,"getActiveCubeFace").strictEquals(0),"source point shadow restores the actual default framebuffer and original active cube face");
    shadowMap.set("autoUpdate",false);caster.get("position").set("x",.6);render(renderer,world,view);expect(renderer,{0,0,0},"frozen point cube depth map preserves the earlier receiver shadow",28);require(seen.get("before").strictEquals(6)&&seen.get("after").strictEquals(6),"frozen point map skips all six depth draws and callbacks");
    shadowMap.set("needsUpdate",true);render(renderer,world,view);const double shadowX=2*(28.5/64)-1;const int refreshed=int(std::lround(255*2/std::sqrt(4+(shadowX+1)*(shadowX+1)+y*y)));
    expect(renderer,{refreshed,refreshed,refreshed},"forced source point update rewrites the six-face map and releases the old shadow",28);
    require(seen.get("before").strictEquals(12)&&seen.get("after").strictEquals(12)&&shadowMap.get("needsUpdate").strictEquals(false),"forced point update consumes needsUpdate and issues one more six-face callback sequence");
    require(sourceShadow.get("map").sameValue(map)&&map.get("depthTexture").sameValue(depth)&&depth.get("image").sameValue(images)&&sourceShadow.get("camera").sameValue(sourceCamera),"point cube shadow refresh preserves target/depth/images/camera identities");
    noError(gl,"default source point PCF rendering and forced six-face updates finish without GL errors");layer.verify();
}
}
int main(int argc,char** argv){
    try{
        bool validation=false;std::string only;for(int i=1;i<argc;++i){const std::string arg=argv[i];if(arg=="--validation")validation=true;else if(arg.starts_with("--only="))only=arg.substr(7);}
        const auto run=[&](const char* name,auto function){if(only.empty()||only==name)function(validation);};
        run("clear",canonicalClearAndCompare);run("upload",nativeDepthUploadAndOrdinarySampling);run("compare",comparisonFunctionsAndSamplerLifetime);run("mesh",meshWritesDepthWithoutColor);run("invalid",invalidSamplerModesAndFeedback);
        run("directional",[](bool enabled){sourcePCFShadow(enabled,false);});run("spot",[](bool enabled){sourcePCFShadow(enabled,true);});
        run("cube",cubeDepthFacesAndSampling);run("point",sourcePointPCFShadow);
        require(only.empty()||only=="clear"||only=="upload"||only=="compare"||only=="mesh"||only=="invalid"||only=="directional"||only=="spot"||only=="cube"||only=="point","--only selects an existing depth test scenario");
        std::cout<<"managed depth textures assertions="<<assertions<<" renders="<<renders<<" validation_requested="<<validationRequested<<" validation_enabled="<<validationEnabled<<" validation_messages="<<validationMessages<<'\n';return 0;
    }catch(const std::exception& error){std::cerr<<"Managed depth textures failed: "<<error.what()<<'\n';return 1;}
}
