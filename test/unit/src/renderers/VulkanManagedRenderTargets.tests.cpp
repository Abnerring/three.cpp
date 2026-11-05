#include "renderers/VulkanRenderer.h"
#include "renderers/WebGLRenderTarget.h"
#include "core/RenderTarget.h"
#include "scenes/Scene.h"
#include "cameras/OrthographicCamera.h"
#include "geometries/PlaneGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "materials/RawShaderMaterial.h"
#include "objects/Mesh.h"
#include "math/Vector2.h"
#include "math/Vector4.h"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

// Canonical WebGLRenderTarget -> production source renderer -> actual Vulkan
// attachment -> sampled screen pixels. No typed RenderTarget bridge is used.
// trace-webgl-render-targets.mjs checks the independent pinned CPU expectations.
namespace {
using Args=std::vector<JSValue>;
int assertions=0,images=0,validationRequested=0,validationEnabled=0,validationMessages=0;
void require(bool value,const char* message){++assertions;if(!value)throw std::runtime_error(message);}
JSValue call(const JSValue& self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
JSValue list(const Args& values){auto value=JSValue::array();for(const auto& item:values)value.set(JSValue(value.length()),item);return value;}
JSValue camera(){auto value=nativeOrthographicCameraConstructor().construct({-1,1,1,-1,.1,10});value.get("position").set("z",3);return value;}
JSValue paint(unsigned color=0xff0000,JSValue map={}){
    auto options=JSValue::object();options.set("color",color);if(!map.isUndefined())options.set("map",map);
    return nativeMeshBasicMaterialConstructor().construct({options});
}
JSValue scene(const Args& objects={}){auto value=nativeSceneConstructor().construct();if(!objects.empty())call(value,"add",objects);return value;}
JSValue mesh(JSValue material,double width=2,double height=2,double x=0,double y=0,double z=0){
    auto value=nativeMeshConstructor().construct({nativePlaneGeometryConstructor().construct({width,height}),material});
    call(value.get("position"),"set",{x,y,z});value.set("frustumCulled",false);return value;
}
JSValue target(int width=32,int height=32,JSValue options={}){
    return options.isUndefined()?nativeWebGLRenderTargetConstructor().construct({width,height}):nativeWebGLRenderTargetConstructor().construct({width,height,options});
}
void bind(const JSValue& renderer,JSValue destination=nullptr){call(renderer,"setRenderTarget",{destination});}
void linear(VulkanRenderer& renderer){renderer.srgbOutput=false;renderer.object().set("outputColorSpace",LinearSRGBColorSpace);}
void render(VulkanRenderer& renderer,JSValue world,JSValue view){renderer.render(world,view);++images;}
void expect(const VulkanRenderer& renderer,std::array<int,3> color,const char* message,unsigned x=32,unsigned y=32){
    const auto bytes=renderer.readPixels();require(bytes.size()==64*64*4,"screen readback remains 64x64 after target rendering");
    const auto at=(y*64+x)*4;
    for(unsigned i=0;i<3;++i)if(std::abs(int(bytes.at(at+i))-color[i])>2){std::cerr<<message<<": "<<int(bytes.at(at))<<','<<int(bytes.at(at+1))<<','<<int(bytes.at(at+2))<<'\n';require(false,message);}
    require(true,message);
}
void sample(VulkanRenderer& renderer,JSValue destination,JSValue view){
    bind(renderer.object());render(renderer,scene({mesh(paint(0xffffff,destination.get("texture")))}),view);
}
struct Validation {
    const VulkanRenderer& renderer;
    Validation(const VulkanRenderer& value,bool requested):renderer(value){if(requested)++validationRequested;if(value.getInfo().validationEnabled)++validationEnabled;}
    void verify(){const auto messages=renderer.getValidationMessages();validationMessages+=int(messages.size());for(const auto& message:messages)std::cerr<<message<<'\n';require(messages.empty(),"render-target GPU operations emit no collected validation errors");}
};
void defaultsAndSampling(bool validation){
    std::cout<<"defaultsAndSampling"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto facade=renderer.object(),view=camera(),rt=target(32,16),texture=rt.get("texture"),source=texture.get("source");
    require(rt.get("isRenderTarget").strictEquals(true)&&rt.get("isWebGLRenderTarget").strictEquals(true),"target is the actual canonical WebGLRenderTarget");
    require(rt.get("depthBuffer").strictEquals(true)&&rt.get("stencilBuffer").strictEquals(false),"canonical target retains default depth true and stencil false");
    require(rt.get("samples").strictEquals(0)&&rt.get("textures").get("0").sameValue(texture),"canonical single color attachment has zero MSAA samples");
    require(texture.get("isRenderTargetTexture").strictEquals(true)&&texture.get("renderTarget").sameValue(rt),"canonical texture retains its target identity");
    require(texture.get("format").strictEquals(RGBAFormat)&&texture.get("type").strictEquals(UnsignedByteType),"default target really uses RGBA8 color storage");
    require(texture.get("flipY").strictEquals(false)&&texture.get("generateMipmaps").strictEquals(false),"source target texture does not upload-flip or generate default mipmaps");
    bind(facade,rt);require(call(facade,"getRenderTarget").sameValue(rt),"setRenderTarget exposes the original target identity");
    require(call(facade,"getActiveCubeFace").strictEquals(0)&&call(facade,"getActiveMipmapLevel").strictEquals(0),"target face and mip default to zero");
    render(renderer,scene({mesh(paint())}),view);sample(renderer,rt,view);expect(renderer,{255,0,0},"rendered RGBA8 target is sampled by the production source material");
    require(call(facade,"getRenderTarget").isNull()&&renderer.getWidth()==64&&renderer.getHeight()==64,"screen restoration keeps renderer dimensions and null source target");
    require(rt.get("texture").sameValue(texture)&&texture.get("source").sameValue(source),"GPU allocation does not replace canonical texture or source");
    bind(facade,rt);render(renderer,scene({mesh(paint(0x0000ff))}),view);sample(renderer,rt,view);
    expect(renderer,{0,0,255},"later frame reuses the same attachment with its new blue content");layer.verify();
}
void depthAttachments(bool validation){
    std::cout<<"depthAttachments"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto view=camera();
    for(const bool depth:{true,false}){
        auto options=JSValue::object();options.set("depthBuffer",depth);const auto rt=target(24,24,options);
        auto near=mesh(paint(0xff0000),1.8,1.8,0,0,.5),far=mesh(paint(0x0000ff),1.8,1.8);
        near.set("renderOrder",0);far.set("renderOrder",1);
        bind(renderer.object(),rt);render(renderer,scene({near,far}),view);sample(renderer,rt,view);
        expect(renderer,depth?std::array<int,3>{255,0,0}:std::array<int,3>{0,0,255},depth?"default depth attachment rejects the later farther surface":"depthBuffer false keeps source order and the farther blue draw wins");
        require(rt.get("depthBuffer").strictEquals(depth),"rendering does not rewrite the public depth option");
    }
    layer.verify();
}
void quadrantOrientation(bool validation){
    std::cout<<"quadrantOrientation"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto rt=target(),view=camera();
    auto world=scene({mesh(paint(0xff0000),1,1,-.5,.5),mesh(paint(0x00ff00),1,1,.5,.5),mesh(paint(0x0000ff),1,1,-.5,-.5),mesh(paint(0xffffff),1,1,.5,-.5)});
    bind(renderer.object(),rt);render(renderer,world,view);sample(renderer,rt,view);
    expect(renderer,{255,0,0},"RT sampling preserves upper left red quadrant",16,16);
    expect(renderer,{0,255,0},"RT sampling preserves upper right green quadrant",48,16);
    expect(renderer,{0,0,255},"RT sampling preserves lower left blue quadrant",16,48);
    expect(renderer,{255,255,255},"RT sampling preserves lower right white quadrant",48,48);layer.verify();
}
void viewportScissorAndStateOrder(bool validation){
    std::cout<<"viewportScissorAndStateOrder"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    auto facade=renderer.object(),rt=target(),view=camera(),state=facade.get("state");
    bind(facade,rt);render(renderer,scene(),view);bind(facade); // Allocate outside the observed source switch.
    call(rt.get("viewport"),"set",{8,8,16,16});call(rt.get("scissor"),"set",{16,0,16,32});rt.set("scissorTest",true);
    auto events=JSValue::array();Args originals;
    for(const char* name:{"bindFramebuffer","drawBuffers","viewport","scissor","setScissorTest"}){
        const auto original=state.get(name);originals.push_back(original);
        state.set(name,JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue {
            require(self.sameValue(c[0]),"source state callback retains its original receiver");
            auto events=c[1];events.set(JSValue(events.length()),c[2]);
            if(c[2].strictEquals("drawBuffers"))require(args.size()==2&&args[0].sameValue(c[4]),"drawBuffers receives the actual target and framebuffer");
            return c[3].call(self,args);
        },{state,events,name,original,rt}));
    }
    bind(facade,rt);
    const std::array<const char*,5> expected={"bindFramebuffer","drawBuffers","viewport","scissor","setScissorTest"};
    require(events.length()==expected.size(),"warm target switch executes exactly the source state sequence");
    for(unsigned i=0;i<expected.size();++i)require(events.get(JSValue(i)).strictEquals(expected[i]),"target state methods retain source bind/drawBuffers/viewport/scissor order");
    unsigned n=0;for(const char* name:expected)state.set(name,originals[n++]);
    const auto viewport=call(facade,"getCurrentViewport",{nativeVector4Constructor().construct()});
    require(viewport.get("x").strictEquals(8)&&viewport.get("z").strictEquals(16),"public current viewport reflects the target viewport");
    render(renderer,scene({mesh(paint(0xff0000))}),view);sample(renderer,rt,view);
    expect(renderer,{255,0,0},"target viewport and scissor intersect on the GPU",40,32);
    expect(renderer,{0,0,0},"target scissor excludes the left viewport half",24,32);
    expect(renderer,{0,0,0},"target viewport excludes the bottom region",40,56);layer.verify();
}
void resizeAndDisposeIdentity(bool validation){
    std::cout<<"resizeAndDisposeIdentity"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto facade=renderer.object(),rt=target(32,16),view=camera(),texture=rt.get("texture"),source=texture.get("source"),image=texture.get("image");
    const auto viewport=rt.get("viewport"),scissor=rt.get("scissor");const auto version=texture.get("version");
    bind(facade,rt);render(renderer,scene({mesh(paint())}),view);sample(renderer,rt,view);expect(renderer,{255,0,0},"target initially renders before resize");
    auto seen=JSValue::object();seen.set("calls",0);
    call(rt,"addEventListener",{"dispose",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue {
        require(self.sameValue(c[0])&&args.size()==1&&args[0].get("target").sameValue(c[0]),"canonical target disposal keeps event receiver and target identity");
        auto s=c[1];s.set("calls",s.get("calls").number()+1);
        if(s.get("calls").strictEquals(1)){
            require(self.get("width").strictEquals(16)&&self.get("height").strictEquals(32),"resize disposal observes updated public target size");
            require(self.get("texture").get("image").get("width").strictEquals(16),"resize disposal observes updated attachment image size");
            require(self.get("viewport").get("z").strictEquals(32)&&self.get("viewport").get("w").strictEquals(16),"source emits dispose before resetting viewport dimensions");
        }return {};
    },{rt,seen})});
    call(rt,"setSize",{16,32});
    require(seen.get("calls").strictEquals(1)&&rt.get("texture").sameValue(texture)&&texture.get("source").sameValue(source)&&texture.get("image").sameValue(image),"setSize retires native resources while retaining canonical attachment aliases");
    require(rt.get("viewport").sameValue(viewport)&&rt.get("scissor").sameValue(scissor)&&viewport.get("z").strictEquals(16)&&viewport.get("w").strictEquals(32),"resize mutates original rectangle objects");
    require(texture.get("version").sameValue(version),"render-target resize does not invent a texture needsUpdate version");
    bind(facade,rt);render(renderer,scene({mesh(paint(0x0000ff))}),view);sample(renderer,rt,view);expect(renderer,{0,0,255},"resized target reallocates actual GPU storage and samples its new content");
    call(rt,"setSize",{16,32});require(seen.get("calls").strictEquals(1),"unchanged target size does not dispatch another disposal");
    call(rt,"dispose");require(seen.get("calls").strictEquals(2),"explicit target dispose dispatches its source event");
    bind(facade,rt);render(renderer,scene({mesh(paint(0x00ff00))}),view);sample(renderer,rt,view);expect(renderer,{0,255,0},"disposed target can allocate again without replacing its texture alias");
    require(rt.get("texture").sameValue(texture)&&texture.get("source").sameValue(source),"dispose and reallocation preserve source attachment identities");layer.verify();
}
void callbacksKeepTargetSwitches(bool validation){
    std::cout<<"callbacksKeepTargetSwitches"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto facade=renderer.object(),view=camera(),a=target(),b=target(),c=target();
    for(const auto rt:{a,b,c}){bind(facade,rt);render(renderer,scene(),view);}bind(facade,a);
    auto outer=mesh(paint(0xff0000)),innerScene=scene({mesh(paint(0x0000ff))}),afterScene=scene({mesh(paint(0x00ff00))}),outerScene=scene({outer});
    auto seen=JSValue::object();seen.set("before",0);seen.set("after",0);
    outer.set("onBeforeRender",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue {
        require(self.sameValue(c[0])&&args.size()==6&&args[0].sameValue(c[1])&&args[1].sameValue(c[2])&&args[2].sameValue(c[3]),"mesh before callback receives source objects and renderer");
        require(call(c[1],"getRenderTarget").sameValue(c[4]),"outer before callback begins on its original target");
        auto s=c[7];s.set("before",s.get("before").number()+1);bind(c[1],c[5]);call(c[1],"render",{c[6],c[3]});
        require(call(c[1],"getRenderTarget").sameValue(c[5]),"nested render does not restore an earlier target automatically");return {};
    },{outer,facade,outerScene,view,a,b,innerScene,seen}));
    outer.set("onAfterRender",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue {
        require(self.sameValue(c[0])&&args.size()==6&&args[0].sameValue(c[1]),"mesh after callback retains original receiver and renderer");
        require(call(c[1],"getRenderTarget").sameValue(c[2]),"outer draw remains on the target selected before its nested render");
        auto s=c[6];s.set("after",s.get("after").number()+1);bind(c[1],c[3]);call(c[1],"render",{c[4],c[5]});return {};
    },{outer,facade,b,c,afterScene,view,seen}));
    render(renderer,outerScene,view);
    require(seen.get("before").strictEquals(1)&&seen.get("after").strictEquals(1)&&call(facade,"getRenderTarget").sameValue(c),"source callbacks finish with the last selected target");
    sample(renderer,a,view);expect(renderer,{0,0,0},"outer render did not draw back into the abandoned target");
    sample(renderer,b,view);expect(renderer,{255,0,0},"outer red draw follows the nested blue draw in the selected target");
    sample(renderer,c,view);expect(renderer,{0,255,0},"onAfterRender nested draw persists in its separately selected target");layer.verify();
}
void sceneCallbacksSwitchBeforeClear(bool validation){
    std::cout<<"sceneCallbacksSwitchBeforeClear"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto facade=renderer.object(),view=camera(),a=target(),b=target(),c=target();
    bind(facade,a);render(renderer,scene({mesh(paint(0xff0000))}),view);
    for(const auto rt:{b,c}){bind(facade,rt);render(renderer,scene(),view);}bind(facade,a);
    auto world=scene({mesh(paint(0x0000ff))});auto seen=JSValue::object();seen.set("before",0);seen.set("after",0);
    world.set("onBeforeRender",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue {
        require(self.sameValue(c[0])&&args.size()==4&&args[0].sameValue(c[1])&&args[1].sameValue(c[0])&&args[2].sameValue(c[2])&&args[3].sameValue(c[3]),"scene before callback receives the target bound when the source callback begins");
        auto s=c[5];s.set("before",s.get("before").number()+1);bind(c[1],c[4]);return {};
    },{world,facade,view,a,b,seen}));
    world.set("onAfterRender",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue {
        require(self.sameValue(c[0])&&args.size()==3&&args[0].sameValue(c[1])&&args[1].sameValue(c[0])&&args[2].sameValue(c[2]),"scene after callback retains its source signature");
        require(call(c[1],"getRenderTarget").sameValue(c[3]),"scene drawing uses the target selected in onBeforeRender");
        auto s=c[5];s.set("after",s.get("after").number()+1);bind(c[1],c[4]);return {};
    },{world,facade,view,b,c,seen}));
    render(renderer,world,view);
    require(call(facade,"getRenderTarget").sameValue(c)&&seen.get("before").strictEquals(1)&&seen.get("after").strictEquals(1),"scene after callback leaves its final target selected");
    sample(renderer,a,view);expect(renderer,{255,0,0},"scene before target switch happens before autoClear and preserves the abandoned red attachment");
    sample(renderer,b,view);expect(renderer,{0,0,255},"scene before target switch directs both clear and blue rendering to its selected attachment");
    sample(renderer,c,view);expect(renderer,{0,0,0},"scene after target selection alone does not clear or draw into that target");layer.verify();
}
void sameFrameReuseAndRetirement(bool validation){
    std::cout<<"sameFrameReuseAndRetirement"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto facade=renderer.object(),view=camera(),rt=target(),texture=rt.get("texture");
    bind(facade,rt);render(renderer,scene({mesh(paint(0xff0000))}),view);bind(facade);
    auto left=mesh(paint(0xffffff,texture),1,2,-.5),right=mesh(paint(0xffffff,texture),1,2,.5);
    left.set("renderOrder",0);right.set("renderOrder",1);
    auto blue=scene({mesh(paint(0x0000ff))});
    right.set("onBeforeRender",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        const auto saved=call(c[0],"getRenderTarget");call(c[1],"dispose");bind(c[0],c[1]);call(c[0],"render",{c[2],c[3]});bind(c[0],saved);return {};
    },{facade,rt,blue,view}));
    render(renderer,scene({left,right}),view);
    expect(renderer,{255,0,0},"earlier sampled draw keeps its old attachment alive after same-frame target dispose",16,32);
    expect(renderer,{0,0,255},"later sampled draw reads the reallocated target produced by nested rendering",48,32);
    require(rt.get("texture").sameValue(texture)&&call(facade,"getRenderTarget").isNull(),"same-frame retirement preserves the public texture alias and restored screen target");
    sample(renderer,rt,view);expect(renderer,{0,0,255},"next frame samples the target's latest reallocated content");layer.verify();
}
void feedbackErrorsAndRecovery(bool validation){
    std::cout<<"feedbackErrorsAndRecovery"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    auto facade=renderer.object(),view=camera(),rt=target(),gl=call(facade,"getContext");
    bind(facade,rt);render(renderer,scene({mesh(paint(0xff0000))}),view);
    facade.set("autoClear",false);render(renderer,scene({mesh(paint(0xffffff,rt.get("texture")))}),view);
    require(call(gl,"getError").strictEquals(0x0502),"attachment/sampler feedback reports actual GL INVALID_OPERATION");
    require(call(gl,"getError").strictEquals(0),"feedback GL error is consumed once");facade.set("autoClear",true);
    sample(renderer,rt,view);expect(renderer,{255,0,0},"rejected feedback draw leaves previous target pixels intact");
    bind(facade,rt);render(renderer,scene({mesh(paint(0x00ff00))}),view);sample(renderer,rt,view);
    expect(renderer,{0,255,0},"valid rendering and sampling recover after rejected feedback");layer.verify();
}
void nativeQueriesAndDrawBufferMask(bool validation){
    std::cout<<"nativeQueriesAndDrawBufferMask"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    auto facade=renderer.object();const auto view=camera(),gl=call(facade,"getContext"),rt=target();
    const auto query=[&](unsigned name){return call(gl,"getParameter",{name});};
    bind(facade,rt);render(renderer,scene({mesh(paint(0xff0000))}),view);
    const auto framebuffer=query(0x8ca6);
    require(!framebuffer.isNull()&&query(0x8ca6).sameValue(framebuffer)&&query(0x8caa).sameValue(framebuffer),"public draw/read framebuffer queries return the exact source target handle");
    require(query(0x0d56).strictEquals(24),"canonical default target owns a real D24 depth attachment");
    require(query(0x0c02).strictEquals(0x8ce0)&&query(0x8825).strictEquals(0x8ce0),"target READ_BUFFER and DRAW_BUFFER0 report COLOR_ATTACHMENT0");
    call(gl,"readBuffer",{0});require(query(0x0c02).strictEquals(0)&&query(0x8825).strictEquals(0x8ce0),"readBuffer NONE changes the real read state independently of draw state");
    call(gl,"readBuffer",{0x8ce0});require(query(0x0c02).strictEquals(0x8ce0)&&query(0x8ca6).sameValue(framebuffer),"restored read buffer preserves the target framebuffer identity");
    call(gl,"drawBuffers",{list({0})});require(query(0x8825).strictEquals(0),"public drawBuffers NONE is visible in native command state");
    // Stay on the current target: source render does not issue another
    // setRenderTarget/drawBuffers call before its automatic clear and draw.
    call(facade,"clear");render(renderer,scene({mesh(paint(0x0000ff))}),view);sample(renderer,rt,view);
    expect(renderer,{255,0,0},"target drawBuffers NONE prevents both clearing and blue draw from overwriting red pixels");
    bind(facade,rt);
    require(query(0x8825).strictEquals(0),"source's cached COLOR_ATTACHMENT0 does not undo an external native NONE when revisiting the target");
    call(gl,"drawBuffers",{list({0x8ce0})});render(renderer,scene({mesh(paint(0x0000ff))}),view);sample(renderer,rt,view);
    expect(renderer,{0,0,255},"restoring COLOR_ATTACHMENT0 enables actual target color writes again");
    auto options=JSValue::object();options.set("depthBuffer",false);const auto noDepth=target(16,16,options);
    bind(facade,noDepth);require(query(0x0d56).strictEquals(0),"depthBuffer false reports no real depth bits");
    const auto noDepthFramebuffer=query(0x8ca6);
    require(!noDepthFramebuffer.sameValue(framebuffer)&&query(0x8caa).sameValue(noDepthFramebuffer),"another canonical target owns a distinct queried framebuffer identity");
    bind(facade);
    require(query(0x8ca6).isNull()&&query(0x8caa).isNull(),"restoring the screen clears public read and draw framebuffer bindings");
    require(query(0x0c02).strictEquals(0x0405)&&query(0x8825).strictEquals(0x0405),"screen read and draw buffer state starts at BACK");
    call(gl,"readBuffer",{0});require(query(0x0c02).strictEquals(0),"screen readBuffer NONE is queried from actual context state");call(gl,"readBuffer",{0x0405});
    render(renderer,scene({mesh(paint(0xff0000))}),view);expect(renderer,{255,0,0},"screen begins red before disabling its draw buffer");
    call(gl,"drawBuffers",{list({0})});require(query(0x8825).strictEquals(0),"screen DRAW_BUFFER0 exposes native NONE");
    call(facade,"clear");render(renderer,scene({mesh(paint(0x0000ff))}),view);
    expect(renderer,{255,0,0},"screen drawBuffers NONE prevents both clear and draw color writes");
    call(gl,"drawBuffers",{list({0x0405})});require(query(0x8825).strictEquals(0x0405),"screen draw buffer restores BACK");
    render(renderer,scene({mesh(paint(0x0000ff))}),view);expect(renderer,{0,0,255},"screen color writes recover after restoring BACK");
    require(call(gl,"getError").strictEquals(0),"legal framebuffer queries and read/draw buffer commands leave no GL error");layer.verify();
}
JSValue lodMaterial(JSValue texture,double level){
    auto uniforms=JSValue::object(),sampler=JSValue::object(),lod=JSValue::object(),options=JSValue::object();sampler.set("value",texture);lod.set("value",level);
    uniforms.set("source",sampler);uniforms.set("level",lod);options.set("uniforms",uniforms);options.set("glslVersion","300 es");
    options.set("vertexShader","precision highp float;in vec3 position;in vec2 uv;uniform mat4 projectionMatrix;uniform mat4 modelViewMatrix;out vec2 vUv;void main(){vUv=uv;gl_Position=projectionMatrix*modelViewMatrix*vec4(position,1.);}");
    options.set("fragmentShader","precision highp float;precision highp sampler2D;uniform sampler2D source;uniform float level;in vec2 vUv;out vec4 result;void main(){result=textureLod(source,vUv,level);}");
    return nativeRawShaderMaterialConstructor().construct({options});
}
void activeMipUsesScratchFramebuffer(bool validation){
    std::cout<<"activeMipUsesScratchFramebuffer"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto facade=renderer.object(),view=camera(),gl=call(facade,"getContext");auto options=JSValue::object();options.set("depthBuffer",false);
    const auto rt=target(32,32,options);auto texture=rt.get("texture");texture.set("generateMipmaps",false);texture.set("minFilter",NearestMipmapNearestFilter);texture.set("magFilter",NearestFilter);
    bind(facade,rt);const auto baseFramebuffer=call(gl,"getParameter",{0x8ca6});
    const auto handle=call(gl,"getFramebufferAttachmentParameter",{0x8d40,0x8ce0,0x8cd1});
    require(!handle.isNull()&&call(gl,"getFramebufferAttachmentParameter",{0x8d40,0x8ce0,0x8cd1}).sameValue(handle),"public attachment query returns the actual canonical texture's GL handle");
    const auto previous=call(gl,"getParameter",{0x8069});call(gl,"bindTexture",{0x0de1,handle});
    // Initial source setup allocated mutable level zero. WebGL texStorage2D
    // converts that storage to a complete immutable chain. Subsequent source
    // setRenderTarget mip handling only reattaches a level; it does not texImage.
    call(gl,"texStorage2D",{0x0de1,6,0x8058,32,32});call(gl,"bindTexture",{0x0de1,previous});
    require(call(gl,"getError").strictEquals(0),"public native texStorage2D allocates all six actual target mip levels");
    render(renderer,scene({mesh(paint(0xff0000))}),view);
    call(rt.get("viewport"),"set",{0,0,16,16});call(rt.get("scissor"),"set",{0,0,16,16});
    call(facade,"setRenderTarget",{rt,0,1});
    const auto mipFramebuffer=call(gl,"getParameter",{0x8ca6});
    require(!mipFramebuffer.isNull()&&!mipFramebuffer.sameValue(baseFramebuffer),"nonzero target mip uses the source scratch framebuffer");
    require(call(facade,"getRenderTarget").sameValue(rt)&&call(facade,"getActiveCubeFace").strictEquals(0)&&call(facade,"getActiveMipmapLevel").strictEquals(1),"original source getters expose active target mip one");
    require(call(gl,"getFramebufferAttachmentParameter",{0x8d40,0x8ce0,0x8cd1}).sameValue(handle)&&call(gl,"getFramebufferAttachmentParameter",{0x8d40,0x8ce0,0x8cd2}).strictEquals(1),"scratch framebuffer really attaches level one of the original GL texture");
    require(call(gl,"checkFramebufferStatus",{0x8d40}).strictEquals(0x8cd5)&&call(gl,"getParameter",{0x0d56}).strictEquals(0),"scratch mip framebuffer is complete and has no mismatched base-level depth attachment");
    render(renderer,scene({mesh(paint(0x0000ff))}),view);bind(facade);
    render(renderer,scene({mesh(lodMaterial(texture,1))}),view);expect(renderer,{0,0,255},"GLSL textureLod samples the blue pixels rendered to actual mip one");
    render(renderer,scene({mesh(lodMaterial(texture,0))}),view);expect(renderer,{255,0,0},"writing mip one preserves the independently rendered red base level");
    require(texture.get("generateMipmaps").strictEquals(false)&&call(gl,"getError").strictEquals(0),"source render leaves explicit mip content intact without implicit generation or GL errors");layer.verify();
}
void incompleteFramebufferNoOpAndRecovery(bool validation){
    std::cout<<"incompleteFramebufferNoOpAndRecovery"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto facade=renderer.object(),view=camera(),gl=call(facade,"getContext");
    // Warm the source's default framebuffer cache before changing raw GL state.
    render(renderer,scene({mesh(paint(0xff0000))}),view);expect(renderer,{255,0,0},"screen starts red before binding an incomplete native framebuffer");
    const auto empty=call(gl,"createFramebuffer");call(gl,"bindFramebuffer",{0x8d40,empty});
    require(call(gl,"getParameter",{0x8ca6}).sameValue(empty)&&call(gl,"getParameter",{0x8caa}).sameValue(empty),"empty framebuffer is the actual public read and draw binding");
    require(call(gl,"checkFramebufferStatus",{0x8d40}).strictEquals(0x8cd7),"empty framebuffer reports FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT");
    call(gl,"clear",{0x4100});require(call(gl,"getError").strictEquals(0x0506),"clearing incomplete framebuffer queues INVALID_FRAMEBUFFER_OPERATION");
    expect(renderer,{255,0,0},"rejected incomplete-framebuffer clear preserves the prior screen image");
    render(renderer,scene({mesh(paint(0x0000ff))}),view);
    require(call(gl,"getError").strictEquals(0x0506)&&call(gl,"getError").strictEquals(0),"source automatic clear and draw share the incomplete-framebuffer GL error flag");
    require(call(facade,"getRenderTarget").isNull()&&call(gl,"getParameter",{0x8ca6}).sameValue(empty),"public framebuffer binding changes do not rewrite the source target cell");
    expect(renderer,{255,0,0},"source drawing to incomplete framebuffer is a no-op with no exception or screen overwrite");
    call(gl,"bindFramebuffer",{0x8d40,nullptr});call(gl,"deleteFramebuffer",{empty});
    render(renderer,scene({mesh(paint(0x0000ff))}),view);expect(renderer,{0,0,255},"source drawing recovers after rebinding the complete screen framebuffer");
    require(call(gl,"getParameter",{0x8ca6}).isNull()&&call(gl,"getError").strictEquals(0),"recovery restores actual default binding without stale GL errors");layer.verify();
}
JSValue coordinateMaterial(int width,int height,bool derivativesOnly=false){
    auto options=JSValue::object();options.set("glslVersion","300 es");
    // No matrices or other vertex uniforms: the derivative-only variant needs
    // the private framebuffer height even though neither stage has a user UBO.
    options.set("vertexShader","precision highp float;in vec3 position;in vec2 uv;out vec2 vUv;void main(){vUv=uv;gl_Position=vec4(position,1.);}");
    if(derivativesOnly){
        options.set("fragmentShader","precision highp float;in vec2 vUv;out vec4 result;void main(){result=vec4(0.5+4.0*dFdy(vUv.y),0.5+4.0*dFdx(vUv.x),0.5+4.0*fwidth(vUv.y),1.);}");
    }else{
        auto uniforms=JSValue::object(),extent=JSValue::object();extent.set("value",list({width,height}));uniforms.set("extent",extent);options.set("uniforms",uniforms);
        options.set("fragmentShader","precision highp float;uniform vec2 extent;in vec2 vUv;out vec4 result;void main(){result=vec4(gl_FragCoord.xy/extent,0.5+0.25*dFdy(vUv.y)*extent.y,1.);}");
    }
    return nativeRawShaderMaterialConstructor().construct({options});
}
void fragmentCoordinatesAndDerivativeGoldens(bool validation){
    std::cout<<"fragmentCoordinatesAndDerivativeGoldens"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto facade=renderer.object(),view=camera(),gl=call(facade,"getContext"),rt=target(32,16);auto texture=rt.get("texture");
    texture.set("minFilter",NearestFilter);texture.set("magFilter",NearestFilter);
    render(renderer,scene({mesh(coordinateMaterial(64,64))}),view);
    // Readback rows are top-to-bottom; GL fragment centers are lower-left.
    // dFdy(uv.y) is +1/height in both GL framebuffer orientations.
    expect(renderer,{66,189,191},"screen raw gl_FragCoord and dFdy follow GL at the upper-left sample",16,16);
    expect(renderer,{193,62,191},"screen raw gl_FragCoord and dFdy follow GL at the lower-right sample",48,48);
    bind(facade,rt);render(renderer,scene({mesh(coordinateMaterial(32,16))}),view);sample(renderer,rt,view);
    // NEAREST samples (8,11) and (24,3) of the 32x16 target, so the
    // expected fragment centers are (8.5,11.5) and (24.5,3.5).
    expect(renderer,{68,183,191},"RT then sample preserves GL fragment centers and positive dFdy at the upper-left sample",16,16);
    expect(renderer,{195,56,191},"RT then sample preserves GL fragment centers and positive dFdy at the lower-right sample",48,48);
    render(renderer,scene({mesh(coordinateMaterial(64,64,true))}),view);
    expect(renderer,{143,143,143},"derivative-only screen shader injects its private UBO and preserves dFdy/dFdx/fwidth");
    bind(facade,rt);render(renderer,scene({mesh(coordinateMaterial(32,16,true))}),view);sample(renderer,rt,view);
    expect(renderer,{191,159,191},"derivative-only RT shader uses the target height and GL derivative sign");
    require(call(gl,"getError").strictEquals(0),"raw coordinate and derivative shaders leave no GL error");layer.verify();
}
void nativeFramebufferCompletenessBoundaries(bool validation){
    std::cout<<"nativeFramebufferCompletenessBoundaries"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto facade=renderer.object(),gl=call(facade,"getContext"),framebuffer=call(gl,"createFramebuffer");
    for(const unsigned binding:{0x8d40u,0x8ca9u,0x8ca8u})
        require(call(gl,"checkFramebufferStatus",{binding}).strictEquals(0x8cd5),"public default FRAMEBUFFER/DRAW/READ status uses the renderer's complete surface");
    call(gl,"bindFramebuffer",{0x8ca9,framebuffer});
    require(call(gl,"checkFramebufferStatus",{0x8ca9}).strictEquals(0x8cd7),"separate DRAW status checks its empty native framebuffer");
    require(call(gl,"checkFramebufferStatus",{0x8d40}).strictEquals(0x8cd7),"FRAMEBUFFER status follows the separate DRAW binding");
    require(call(gl,"checkFramebufferStatus",{0x8ca8}).strictEquals(0x8cd5),"empty DRAW binding leaves READ status on the complete default surface");
    call(gl,"bindFramebuffer",{0x8ca9,nullptr});call(gl,"bindFramebuffer",{0x8ca8,framebuffer});
    require(call(gl,"checkFramebufferStatus",{0x8ca9}).strictEquals(0x8cd5),"empty READ binding leaves DRAW status on the complete default surface");
    require(call(gl,"checkFramebufferStatus",{0x8d40}).strictEquals(0x8cd5),"FRAMEBUFFER status still follows DRAW when only READ has an empty native framebuffer");
    require(call(gl,"checkFramebufferStatus",{0x8ca8}).strictEquals(0x8cd7),"separate READ status checks its own empty native framebuffer");
    call(gl,"bindFramebuffer",{0x8ca8,nullptr});
    require(call(gl,"checkFramebufferStatus",{0x1234}).strictEquals(0)&&call(gl,"getError").strictEquals(0x0500)&&call(gl,"getError").strictEquals(0),"public status preserves the native INVALID_ENUM result and consumes it once");
    bool missingTargetRejected=false;try{call(gl,"checkFramebufferStatus");}catch(const JSTypeError&){missingTargetRejected=true;}
    require(missingTargetRejected,"public status keeps the required-target TypeError");
    require(call(gl,"getParameter",{0x8ca6}).isNull()&&call(gl,"getParameter",{0x8caa}).isNull()&&call(gl,"getError").strictEquals(0),"separate status fixtures restore both actual default bindings without a stale error");
    const auto allocate=[&](unsigned binding,int levels,int width,int height,int depth=1,unsigned format=0x8058){
        const auto texture=call(gl,"createTexture");call(gl,"bindTexture",{binding,texture});
        if(binding==0x806f||binding==0x8c1a)call(gl,"texStorage3D",{binding,levels,format,width,height,depth});
        else call(gl,"texStorage2D",{binding,levels,format,width,height});
        return texture;
    };
    const auto status=[&]{return call(gl,"checkFramebufferStatus",{0x8d40});};
    call(gl,"bindFramebuffer",{0x8d40,framebuffer});
    const auto volume=allocate(0x806f,4,8,8,8);
    call(gl,"framebufferTextureLayer",{0x8d40,0x8ce0,volume,1,3});
    require(status().strictEquals(0x8cd5),"3D mip one accepts its last z slice at depth four");
    call(gl,"framebufferTextureLayer",{0x8d40,0x8ce0,volume,1,4});
    require(status().strictEquals(0x8cd6),"3D mip one rejects a z slice outside that mip's reduced depth");
    call(gl,"framebufferTextureLayer",{0x8d40,0x8ce0,volume,3,0});
    require(status().strictEquals(0x8cd5),"last 3D mip retains exactly one complete z slice");
    call(gl,"framebufferTextureLayer",{0x8d40,0x8ce0,volume,3,1});
    require(status().strictEquals(0x8cd6),"last 3D mip rejects z slice one even when the base volume is deeper");
    const auto array=allocate(0x8c1a,4,8,8,5);
    call(gl,"framebufferTextureLayer",{0x8d40,0x8ce0,array,3,4});
    require(status().strictEquals(0x8cd5),"array mip levels preserve their full layer count");
    call(gl,"framebufferTextureLayer",{0x8d40,0x8ce0,array,3,5});
    require(status().strictEquals(0x8cd6),"array attachment rejects the first layer beyond its allocated array size");
    const auto cube=allocate(0x8513,4,8,8);
    for(unsigned face=0x8515;face<=0x851a;++face){
        call(gl,"framebufferTexture2D",{0x8d40,0x8ce0,face,cube,3});
        require(status().strictEquals(0x8cd5),"each actual cube face is complete at its last allocated mip");
        require(call(gl,"getFramebufferAttachmentParameter",{0x8d40,0x8ce0,0x8cd3}).strictEquals(face),"native cube attachment query retains the selected face");
    }
    const auto color=allocate(0x0de1,1,8,8),depth=call(gl,"createRenderbuffer");
    call(gl,"framebufferTexture2D",{0x8d40,0x8ce0,0x0de1,color,0});call(gl,"bindRenderbuffer",{0x8d41,depth});
    call(gl,"renderbufferStorage",{0x8d41,0x81a6,4,8});call(gl,"framebufferRenderbuffer",{0x8d40,0x8d00,0x8d41,depth});
    require(status().strictEquals(0x8cd9),"different native color and depth widths report FRAMEBUFFER_INCOMPLETE_DIMENSIONS");
    call(gl,"renderbufferStorage",{0x8d41,0x81a6,8,8});
    require(status().strictEquals(0x8cd5),"matching native depth storage restores framebuffer completeness");
    call(gl,"framebufferRenderbuffer",{0x8d40,0x8d00,0x8d41,nullptr});
    call(gl,"framebufferTexture2D",{0x8d40,0x8ce1,0x0de1,color,0});
    require(status().strictEquals(0x8cdd),"attaching the identical color image twice reports FRAMEBUFFER_UNSUPPORTED");
    call(gl,"framebufferTexture2D",{0x8d40,0x8ce1,0x0de1,nullptr,0});
    require(status().strictEquals(0x8cd5),"detaching duplicate color image restores completeness");
    call(gl,"framebufferTexture2D",{0x8d40,0x8d00,0x0de1,color,0});
    require(status().strictEquals(0x8cd6),"RGBA color storage is not a legal depth attachment");
    call(gl,"framebufferTexture2D",{0x8d40,0x8d00,0x0de1,nullptr,0});
    const auto half=allocate(0x0de1,1,8,8,1,0x881a);
    require(call(gl,"getExtension",{"EXT_color_buffer_float"}).isNull(),"native context does not advertise EXT_color_buffer_float");
    call(gl,"framebufferTexture2D",{0x8d40,0x8ce0,0x0de1,half,0});
    require(status().strictEquals(0x8cd6),"RGBA16F stays incomplete without the float color-buffer extension");
    call(gl,"framebufferTexture2D",{0x8d40,0x8ce0,0x0de1,color,0});
    require(status().strictEquals(0x8cd5),"legal RGBA8 attachment recovers after invalid native attachment formats");
    require(call(gl,"getError").strictEquals(0),"framebuffer completeness checks report statuses without adding GL command errors");
    call(gl,"bindFramebuffer",{0x8d40,nullptr});call(gl,"bindRenderbuffer",{0x8d41,nullptr});call(gl,"deleteFramebuffer",{framebuffer});call(gl,"deleteRenderbuffer",{depth});
    for(const auto texture:{volume,array,cube,color,half})call(gl,"deleteTexture",{texture});
    require(call(gl,"getParameter",{0x8ca6}).isNull()&&call(gl,"getError").strictEquals(0),"native completeness fixtures release their actual framebuffer binding cleanly");layer.verify();
}
void typedEntrypointsRejectRawFramebuffer(bool validation){
    std::cout<<"typedEntrypointsRejectRawFramebuffer"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto facade=renderer.object(),gl=call(facade,"getContext"),view=camera(),rt=target();
    bind(facade,rt);render(renderer,scene({mesh(paint(0x0000ff))}),view);const auto framebuffer=call(gl,"getParameter",{0x8ca6});bind(facade);
    render(renderer,scene({mesh(paint(0xff0000))}),view);
    const auto typedTarget=std::make_shared<RenderTarget>(16,16);Scene typedScene;OrthographicCamera typedView(-1,1,1,-1,.1,10);typedView.position.z=3;
    const auto rejects=[&](auto&& operation,const char* message){bool rejected=false;try{operation();}catch(const std::logic_error&){rejected=true;}require(rejected,message);};
    call(gl,"bindFramebuffer",{0x8d40,framebuffer});
    require(call(gl,"getParameter",{0x8ca6}).sameValue(framebuffer)&&call(facade,"getRenderTarget").isNull(),"raw bind changes the native framebuffer without changing the source target cell");
    rejects([&]{renderer.render(typedScene,typedView);},"typed render rejects a raw framebuffer immediately after bind");
    rejects([&]{renderer.clear();},"typed clear rejects a raw framebuffer immediately after bind");
    rejects([&]{renderer.setSize(48,32);},"typed setSize rejects a raw framebuffer immediately after bind");
    rejects([&]{renderer.setRenderTarget(typedTarget);},"typed setRenderTarget rejects a raw framebuffer immediately after bind");
    require(!renderer.getRenderTarget()&&renderer.getWidth()==64&&renderer.getHeight()==64,"rejected typed operations preserve their previous target and screen dimensions");
    expect(renderer,{255,0,0},"rejected typed operations preserve the screen's actual pixels");
    // Force the managed device active target to be the raw FBO as well; the
    // null raw unbind must let the next typed call restore the screen itself.
    call(gl,"clearColor",{0,0,1,1});call(gl,"clear",{0x4000});
    rejects([&]{renderer.render(typedScene,typedView);},"typed render also rejects an already active raw GPU target");
    rejects([&]{renderer.clear();},"typed clear also rejects an already active raw GPU target");
    rejects([&]{renderer.setSize(48,32);},"typed setSize also rejects an already active raw GPU target");
    rejects([&]{renderer.setRenderTarget(typedTarget);},"typed setRenderTarget also rejects an already active raw GPU target");
    call(gl,"bindFramebuffer",{0x8d40,nullptr});renderer.setClearColor(Color(0,1,0));renderer.clear();
    expect(renderer,{0,255,0},"raw unbind null lets typed clear restore and write the actual screen");
    renderer.setSize(48,32);require(renderer.getWidth()==48&&renderer.getHeight()==32,"typed setSize recovers after the raw null unbind");renderer.setSize(64,64);
    renderer.setRenderTarget(typedTarget);require(renderer.getRenderTarget()==typedTarget&&renderer.getWidth()==16&&renderer.getHeight()==16,"typed target selection recovers after the raw null unbind");
    renderer.setRenderTarget(nullptr);renderer.render(typedScene,typedView);++images;
    expect(renderer,{0,255,0},"typed render recovers on the screen after typed target restoration");
    require(call(gl,"getParameter",{0x8ca6}).isNull()&&call(gl,"getError").strictEquals(0),"typed/raw guards preserve a clean actual default framebuffer binding");layer.verify();
}
void crossMipFeedbackRangeAndPixels(bool validation){
    std::cout<<"crossMipFeedbackRangeAndPixels"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    auto facade=renderer.object();const auto gl=call(facade,"getContext"),view=camera();auto options=JSValue::object();options.set("depthBuffer",false);
    const auto rt=target(32,32,options);auto texture=rt.get("texture");texture.set("generateMipmaps",false);texture.set("minFilter",NearestFilter);texture.set("magFilter",NearestFilter);
    bind(facade,rt);const auto baseFramebuffer=call(gl,"getParameter",{0x8ca6}),handle=call(gl,"getFramebufferAttachmentParameter",{0x8d40,0x8ce0,0x8cd1});
    const auto previous=call(gl,"getParameter",{0x8069});call(gl,"bindTexture",{0x0de1,handle});call(gl,"texStorage2D",{0x0de1,6,0x8058,32,32});call(gl,"bindTexture",{0x0de1,previous});
    render(renderer,scene({mesh(paint(0xff0000))}),view);call(rt.get("viewport"),"set",{0,0,16,16});call(rt.get("scissor"),"set",{0,0,16,16});
    call(facade,"setRenderTarget",{rt,0,1});const auto scratch=call(gl,"getParameter",{0x8ca6});
    require(!scratch.sameValue(baseFramebuffer)&&call(gl,"getFramebufferAttachmentParameter",{0x8d40,0x8ce0,0x8cd2}).strictEquals(1),"cross-mip destination is the source scratch framebuffer attached to mip one");
    render(renderer,scene({mesh(paint(0x0000ff))}),view);facade.set("autoClear",false);
    render(renderer,scene({mesh(paint(0xffffff,texture))}),view);
    require(call(gl,"getError").strictEquals(0),"sampling base zero with non-mipmap NEAREST while writing mip one is legal");
    require(call(gl,"getParameter",{0x8ca6}).sameValue(scratch)&&call(facade,"getActiveMipmapLevel").strictEquals(1),"legal cross-mip feedback retains the original scratch framebuffer and active mip");
    // Enable access to the entire immutable chain only after the legal draw.
    // This also lets textureLod expose whether that draw replaced blue mip one.
    texture.set("minFilter",NearestMipmapNearestFilter);call(gl,"bindTexture",{0x0de1,handle});call(gl,"texParameteri",{0x0de1,0x2801,0x2700});
    bind(facade);facade.set("autoClear",true);render(renderer,scene({mesh(lodMaterial(texture,1))}),view);
    expect(renderer,{255,0,0},"legal cross-mip draw really copies red base-zero pixels into the previously blue mip one");
    call(facade,"setRenderTarget",{rt,0,1});render(renderer,scene({mesh(paint(0x0000ff))}),view);facade.set("autoClear",false);
    render(renderer,scene({mesh(paint(0xffffff,texture))}),view);
    require(call(gl,"getError").strictEquals(0x0502)&&call(gl,"getError").strictEquals(0),"mipmap filtering overlaps the attached mip and reports INVALID_OPERATION exactly once");
    bind(facade);facade.set("autoClear",true);render(renderer,scene({mesh(lodMaterial(texture,1))}),view);
    expect(renderer,{0,0,255},"rejected overlapping mip draw preserves the blue attachment pixels");
    render(renderer,scene({mesh(lodMaterial(texture,0))}),view);expect(renderer,{255,0,0},"rejected overlapping mip draw also preserves the independently rendered base level");
    require(call(gl,"getError").strictEquals(0),"valid sampling recovers after the overlapping mip feedback error");layer.verify();
}
void reducedColorFormatGoldens(bool validation){
    std::cout<<"reducedColorFormatGoldens"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto facade=renderer.object(),gl=call(facade,"getContext"),view=camera();
    for(const auto format:{RedFormat,RGFormat}){
        auto options=JSValue::object();options.set("format",format);options.set("depthBuffer",false);const auto rt=target(24,16,options);
        require(rt.get("texture").get("format").strictEquals(format)&&rt.get("depthBuffer").strictEquals(false),"canonical reduced-channel target retains its requested format and depth option");
        bind(facade,rt);require(call(gl,"checkFramebufferStatus",{0x8d40}).strictEquals(0x8cd5),"R8/RG8 managed framebuffer is complete");
        call(facade,"setClearColor",{0xffffff,1});call(facade,"clear");sample(renderer,rt,view);
        expect(renderer,format==RedFormat?std::array<int,3>{255,0,0}:std::array<int,3>{255,255,0},format==RedFormat?"cleared R8 samples red with implicit zero green and blue":"cleared RG8 samples yellow with implicit zero blue");
        bind(facade,rt);render(renderer,scene({mesh(paint(0x00ffff))}),view);sample(renderer,rt,view);
        expect(renderer,format==RedFormat?std::array<int,3>{0,0,0}:std::array<int,3>{0,255,0},format==RedFormat?"rendering cyan into R8 writes zero to its only color channel":"rendering cyan into RG8 preserves green and supplies implicit zero blue");
        require(call(gl,"getError").strictEquals(0),"reduced-channel clear/render/sample leaves no GL error");
    }
    layer.verify();
}
void typedViewportPreservesCanonicalSurfaceSize(bool validation){
    std::cout<<"typedViewportPreservesCanonicalSurfaceSize"<<std::endl;VulkanRenderer renderer({64,64,validation});Validation layer(renderer,validation);linear(renderer);
    const auto facade=renderer.object(),gl=call(facade,"getContext"),view=camera(),rt=target(32,16),texture=rt.get("texture");
    const auto surfaceSize=[&](int width,int height){
        const auto size=nativeVector2Constructor().construct(),drawing=nativeVector2Constructor().construct();
        require(call(facade,"getSize",{size}).sameValue(size)&&size.get("x").strictEquals(width)&&size.get("y").strictEquals(height),"canonical getSize retains the screen dimensions independently of the active viewport");
        require(call(facade,"getDrawingBufferSize",{drawing}).sameValue(drawing)&&drawing.get("x").strictEquals(width)&&drawing.get("y").strictEquals(height),"canonical getDrawingBufferSize agrees with the actual screen extent");
        require(gl.get("canvas").get("width").strictEquals(width)&&gl.get("canvas").get("height").strictEquals(height),"native context canvas tracks the screen size independently of render-target dimensions");
    };
    bind(facade,rt);render(renderer,scene({mesh(paint(0xff0000))}),view);renderer.setViewport(0,0,64,64);surfaceSize(64,64);
    require(call(facade,"getRenderTarget").sameValue(rt)&&rt.get("width").strictEquals(32)&&rt.get("height").strictEquals(16)&&renderer.getWidth()==32&&renderer.getHeight()==16,"typed viewport changes leave the canonical 32x16 target and active GPU extent intact");
    sample(renderer,rt,view);surfaceSize(64,64);expect(renderer,{255,0,0},"restoring the screen after typed viewport retains 64x64 sampled pixels");
    renderer.setSize(48,24);surfaceSize(48,24);
    require(renderer.getWidth()==48&&renderer.getHeight()==24&&call(facade,"getRenderTarget").isNull(),"typed setSize on the default framebuffer resizes the actual GPU surface");
    renderer.setSize(64,64);surfaceSize(64,64);sample(renderer,rt,view);expect(renderer,{255,0,0},"resizing the default surface and restoring 64x64 preserves existing target pixels");
    require(renderer.getWidth()==64&&renderer.getHeight()==64,"typed size restoration agrees with the actual default framebuffer extent");
    require(rt.get("texture").sameValue(texture)&&rt.get("width").strictEquals(32)&&rt.get("height").strictEquals(16),"screen viewport and size updates preserve the target texture identity and dimensions");
    require(call(gl,"getError").strictEquals(0),"typed viewport and size synchronization leave no GL error");layer.verify();
}
}
int main(int argc,char** argv){
    try{bool validation=false;for(int i=1;i<argc;++i)if(std::string(argv[i])=="--validation")validation=true;
        defaultsAndSampling(validation);depthAttachments(validation);quadrantOrientation(validation);viewportScissorAndStateOrder(validation);
        resizeAndDisposeIdentity(validation);callbacksKeepTargetSwitches(validation);sceneCallbacksSwitchBeforeClear(validation);sameFrameReuseAndRetirement(validation);feedbackErrorsAndRecovery(validation);
        nativeQueriesAndDrawBufferMask(validation);activeMipUsesScratchFramebuffer(validation);incompleteFramebufferNoOpAndRecovery(validation);
        fragmentCoordinatesAndDerivativeGoldens(validation);nativeFramebufferCompletenessBoundaries(validation);typedEntrypointsRejectRawFramebuffer(validation);crossMipFeedbackRangeAndPixels(validation);
        reducedColorFormatGoldens(validation);typedViewportPreservesCanonicalSurfaceSize(validation);
        std::cout<<"managed render targets assertions="<<assertions<<" renders="<<images<<" validation_requested="<<validationRequested<<" validation_enabled="<<validationEnabled<<" validation_messages="<<validationMessages<<'\n';
        return 0;
    }catch(const std::exception& error){std::cerr<<"Managed render targets failed: "<<error.what()<<'\n';return 1;}
}
