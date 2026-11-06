#include "renderers/webgl/WebGLShadowMap.h"
#include "renderers/webgl/WebGLProperties.h"
#include "lights/DirectionalLight.h"
#include "lights/PointLight.h"
#include "scenes/Scene.h"
#include "cameras/OrthographicCamera.h"
#include "objects/Mesh.h"
#include "core/BufferGeometry.h"
#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include "materials/MeshBasicMaterial.h"
#include "materials/MeshDepthMaterial.h"
#include "textures/Texture.h"
#include "math/Vector2.h"
#include "math/Vector4.h"
#include "utils.h"
#include "constants.h"
#include <iostream>
#include <iterator>
#include <string>

// Independent command recorder for the canonical source factory. No Vulkan,
// renderer bridge, or hand-coded shadow decisions participate in this probe.
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
JSValue object(std::initializer_list<std::pair<const char*,JSValue>> fields={}){auto v=JSValue::object();for(const auto& [k,x]:fields)v.set(k,x);return v;}
JSValue array(const Args& values={}){auto v=JSValue::array();for(const auto& x:values)v.set(JSValue(v.length()),x);return v;}
void push(JSValue values,JSValue value){values.set(JSValue(values.length()),value);}
JSValue call(JSValue self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
JSValue clean(JSValue value){return value.isUndefined()?JSValue("undefined"):value;}
JSValue option(JSValue r,const char* name,JSValue fallback){const auto value=r.get(name);return value.isUndefined()||value.isNull()?fallback:value;}
bool flag(JSValue r,const char* name){return r.get(name).toBoolean();}
void event(JSValue h,const Args& values){push(h.get("events"),array(values));}
unsigned enroll(JSValue values,JSValue value){for(unsigned i=0;i<values.length();++i)if(values.get(JSValue(i)).sameValue(value))return i;const auto i=unsigned(values.length());push(values,value);return i;}
JSValue targetKey(JSValue h,JSValue value){if(value.isNull())return nullptr;const auto tag=value.get("tag");return !tag.isUndefined()&&!tag.isNull()?tag:JSValue("target"+std::to_string(enroll(h.get("targets"),value)));}
unsigned materialKey(JSValue h,JSValue value){
    const auto values=h.get("materials");const auto old=values.length();const auto key=enroll(values,value);
    if(key==old)call(value,"addEventListener",{"dispose",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"material.dispose",c[1]});return {};},{h,key})});
    return key;
}
JSValue materialState(JSValue h,JSValue value){
    const auto key=materialKey(h,value);const auto planes=value.get("clippingPlanes");auto uniforms=JSValue(nullptr),defines=JSValue(nullptr);
    if(value.get("isShaderMaterial").toBoolean()){
        const auto u=value.get("uniforms");uniforms=object({{"resolution",call(u.get("resolution").get("value"),"toArray")},{"radius",u.get("radius").get("value")},{"pass",u.get("shadow_pass").get("value").get("isDepthTexture").toBoolean()?JSValue("depth"):JSValue("color")}});defines=value.get("defines");
    }
    return object({{"key",key},{"type",value.get("type")},{"side",clean(value.get("side"))},{"alphaTest",clean(value.get("alphaTest"))},{"visible",clean(value.get("visible"))},{"wireframe",clean(value.get("wireframe"))},{"clipShadows",clean(value.get("clipShadows"))},{"clipIntersection",clean(value.get("clipIntersection"))},{"clippingPlanes",planes.isNull()?JSValue(nullptr):JSValue(planes.length())},{"alphaMap",value.get("alphaMap").toBoolean()},{"map",value.get("map").toBoolean()},{"displacementMap",value.get("displacementMap").toBoolean()},{"displacementScale",clean(value.get("displacementScale"))},{"displacementBias",clean(value.get("displacementBias"))},{"wireframeLinewidth",clean(value.get("wireframeLinewidth"))},{"linewidth",clean(value.get("linewidth"))},{"version",value.get("version")},{"light",call(h.get("properties"),"get",{value}).get("light").sameValue(h.get("light"))},{"defines",defines},{"uniforms",uniforms}});
}
JSValue service(const JSValue&,const Args& args,const Args& captures){
    auto h=captures[0];const auto name=captures[1].toString();const auto request=h.get("request");
    if(name=="getRenderTarget"){event(h,{name});if(request.get("throwAt").strictEquals(name))throw JSThrownValue(name);return h.get("current");}
    if(name=="getActiveCubeFace"||name=="getActiveMipmapLevel"){event(h,{name});return h.get(name=="getActiveCubeFace"?"face":"mip");}
    if(name=="depth.getReversed"){event(h,{name});return option(request,"reversed",false);}
    if(name=="setRenderTarget"){
        const auto face=arg(args,1).isUndefined()?JSValue(0):arg(args,1),mip=arg(args,2).isUndefined()?JSValue(0):arg(args,2);event(h,{name,targetKey(h,arg(args,0)),args.size(),face,mip});
        if(request.get("throwAt").strictEquals(name))throw JSThrownValue(name);h.set("current",arg(args,0));h.set("face",face);h.set("mip",mip);return {};
    }
    if(name=="viewport"){event(h,{name,call(arg(args,0),"toArray")});return {};}
    if(name=="objects.update"){event(h,{name,arg(args,0).get("name")});return arg(args,0).get("geometry");}
    if(name=="draw"){
        const auto group=arg(args,5);event(h,{name,arg(args,4).get("name"),materialState(h,arg(args,3)),group.isNull()?JSValue(nullptr):group.get("materialIndex")});
        if(request.get("throwAt").strictEquals(name))throw JSThrownValue(name);return {};
    }
    Args row{name};row.insert(row.end(),args.begin(),args.end());event(h,row);if(request.get("throwAt").strictEquals(name))throw JSThrownValue(name);return {};
}
JSValue method(JSValue h,const char* name){return JSValue::function(service,{h,name});}
JSValue hook(const JSValue& self,const Args& args,const Args& captures){
    auto h=captures[0];const auto before=captures[1].toBoolean();const auto group=arg(args,6),request=h.get("request");
    event(h,{before?"before":"after",self.sameValue(h.get("mesh")),arg(args,0).sameValue(h.get("renderer")),arg(args,1).sameValue(self),arg(args,2).sameValue(h.get("camera")),arg(args,3).sameValue(h.get("shadow").get("camera")),materialKey(h,arg(args,5)),group.isNull()?JSValue(nullptr):group.get("materialIndex")});
    if(before&&request.get("throwAt").strictEquals("before"))throw JSThrownValue("before");
    if(before&&flag(request,"reenter")&&!flag(h,"once")){h.set("once",true);event(h,{"reenter"});call(h.get("manager"),"render",{array({h.get("light")}),h.get("scene"),h.get("camera")});}
    return {};
}
JSValue mapState(JSValue h,JSValue map){
    if(map.isNull())return nullptr;const auto t=map.get("texture"),d=map.get("depthTexture");auto depth=JSValue(nullptr);
    if(!d.isNull())depth=object({{"cube",d.get("isCubeDepthTexture").strictEquals(true)},{"type",d.get("type")},{"format",d.get("format")},{"compare",d.get("compareFunction")},{"min",d.get("minFilter")},{"mag",d.get("magFilter")},{"name",d.get("name")},{"owner",d.get("renderTarget").sameValue(map)}});
    return object({{"key",targetKey(h,map)},{"size",array({map.get("width"),map.get("height")})},{"cube",map.get("isWebGLCubeRenderTarget").strictEquals(true)},{"texture",array({t.get("format"),t.get("type"),t.get("minFilter"),t.get("magFilter"),t.get("name"),t.get("generateMipmaps")})},{"depth",depth}});
}
JSValue snapshot(JSValue h){
    const auto manager=h.get("manager"),shadow=h.get("shadow"),source=h.get("source"),listeners=source.get("_listeners");auto reversed=shadow.get("camera").get("_reversedDepth");if(reversed.isUndefined()||reversed.isNull())reversed=nullptr;
    unsigned disposeListeners=0;if(listeners.isObject()){const auto values=listeners.get("dispose");if(values.isObject())disposeListeners=unsigned(values.length());}
    return object({{"manager",array({manager.get("enabled"),manager.get("autoUpdate"),manager.get("needsUpdate"),manager.get("type")})},{"target",targetKey(h,h.get("current"))},{"face",h.get("face")},{"mip",h.get("mip")},{"map",mapState(h,shadow.get("map"))},{"pass",mapState(h,shadow.get("mapPass"))},{"mapSize",call(shadow.get("mapSize"),"toArray")},{"needsUpdate",shadow.get("needsUpdate")},{"reversed",reversed},{"sourceVersion",source.get("version")},{"disposeListeners",disposeListeners}});
}
void execute(JSValue h){
    auto error=JSValue(nullptr);try{call(h.get("manager"),"render",{flag(h.get("request"),"empty")?array():array({h.get("light")}),h.get("scene"),h.get("camera")});}
    catch(const JSThrownValue& e){error=e.value.isObject()?object({{"name",e.value.get("name")}}):object({{"raw",e.value}});}
    catch(const JSTypeError&){error=object({{"name","TypeError"}});}catch(const JSRangeError&){error=object({{"name","RangeError"}});}catch(const JSReferenceError&){error=object({{"name","ReferenceError"}});}catch(const std::exception&){error=object({{"name","Error"}});}
    push(h.get("rows"),object({{"error",error},{"state",snapshot(h)}}));
}
JSValue run(JSValue request){
    WebGLProperties properties;auto h=object({{"request",request},{"events",array()},{"rows",array()},{"targets",array()},{"materials",array()},{"current",object({{"tag","outer"}})},{"face",3},{"mip",2},{"once",false},{"properties",properties.object()}});
    auto depth=object({{"getReversed",method(h,"depth.getReversed")},{"setTest",method(h,"depth.setTest")}}),color=object({{"setClear",method(h,"color.setClear")}});
    auto state=object({{"buffers",object({{"depth",depth},{"color",color}})},{"setBlending",method(h,"setBlending")},{"setScissorTest",method(h,"setScissorTest")},{"viewport",method(h,"viewport")}});
    auto renderer=object({{"localClippingEnabled",request.get("variant").strictEquals("clipping")},{"properties",properties.object()},{"state",state}});
    for(const auto* name:{"getRenderTarget","getActiveCubeFace","getActiveMipmapLevel","setRenderTarget","clear"})renderer.set(name,method(h,name));renderer.set("renderBufferDirect",method(h,"draw"));h.set("renderer",renderer);
    h.set("camera",nativeOrthographicCameraConstructor().construct());h.set("scene",nativeSceneConstructor().construct());auto light=(flag(request,"point")?nativePointLightConstructor():nativeDirectionalLightConstructor()).construct();light.set("name","light");h.set("light",light);
    auto shadow=light.get("shadow");h.set("shadow",shadow);call(shadow.get("mapSize"),"set",{flag(request,"clamp")?65:32,flag(request,"clamp")?49:16});shadow.set("autoUpdate",option(request,"lightAutoUpdate",true));shadow.set("needsUpdate",option(request,"lightNeedsUpdate",false));
    if(flag(request,"point"))light.set("distance",23);if(flag(request,"shadowLayers"))shadow.get("camera").get("layers").set("mask",request.get("shadowLayers"));
    shadow.set("updateMatrices",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{event(c[0],{"shadow.updateMatrices",args.size()});return c[1].call(self,args);},{h,shadow.get("updateMatrices")}));
    if(flag(request,"atlas")){shadow.set("_frameExtents",nativeVector2Constructor().construct({2,flag(request,"clamp")?3:1}));shadow.set("_viewportCount",2);shadow.set("_viewports",array({nativeVector4Constructor().construct({0,0,1,1}),nativeVector4Constructor().construct({1,0,1,1})}));}
    auto source=nativeMeshBasicMaterialConstructor().construct();h.set("source",source);source.set("name","source");source.set("wireframe",true);source.set("wireframeLinewidth",3);source.set("side",FrontSide);
    if(!request.get("shadowSide").isUndefined())source.set("shadowSide",request.get("shadowSide"));const auto variant=request.get("variant");
    if(variant.strictEquals("alpha")){source.set("alphaMap",nativeTextureConstructor().construct());source.set("map",nativeTextureConstructor().construct());source.set("alphaTest",.25);}
    if(variant.strictEquals("coverage"))source.set("alphaToCoverage",true);
    if(variant.strictEquals("displacement")){source.set("displacementMap",nativeTextureConstructor().construct());source.set("displacementScale",2);source.set("displacementBias",.5);}
    if(variant.strictEquals("clipping")){source.set("clipShadows",true);source.set("clippingPlanes",array({object()}));source.set("clipIntersection",true);}
    auto geometry=nativeBufferGeometryConstructor().construct();if(flag(request,"groups")){call(geometry,"addGroup",{0,3,0});call(geometry,"addGroup",{3,3,1});call(geometry,"addGroup",{6,3,2});}
    auto second=nativeMeshBasicMaterialConstructor().construct();second.set("visible",false);auto mesh=nativeMeshConstructor().construct({geometry,flag(request,"groups")?array({source,second,source}):source});h.set("mesh",mesh);mesh.set("name","caster");mesh.set("castShadow",option(request,"castShadow",true));mesh.set("receiveShadow",option(request,"receiveShadow",false));mesh.set("frustumCulled",false);
    if(flag(request,"custom"))mesh.set("customDepthMaterial",nativeMeshDepthMaterialConstructor().construct());mesh.set("onBeforeShadow",JSValue::function(hook,{h,true}));mesh.set("onAfterShadow",JSValue::function(hook,{h,false}));
    if(flag(request,"nested")){auto parent=nativeMeshConstructor().construct({nativeBufferGeometryConstructor().construct(),nativeMeshBasicMaterialConstructor().construct()});parent.set("name","parent");parent.set("castShadow",false);parent.set("frustumCulled",false);call(parent,"add",{mesh});call(h.get("scene"),"add",{parent});}else call(h.get("scene"),"add",{mesh});
    if(flag(request,"hidden"))mesh.set("visible",false);
    auto manager=nativeWebGLShadowMap(renderer,object({{"update",method(h,"objects.update")}}),object({{"maxTextureSize",flag(request,"clamp")?32:4096}}));h.set("manager",manager);
    auto keys=array();for(const auto& key:manager.ownPropertyNames()){const auto descriptor=manager.getOwnPropertyDescriptor(key);if(descriptor&&descriptor->enumerable.value_or(false))push(keys,key);}
    auto defaults=object({{"enabled",manager.get("enabled")},{"autoUpdate",manager.get("autoUpdate")},{"needsUpdate",manager.get("needsUpdate")},{"type",manager.get("type")},{"keys",keys},{"renderLength",manager.get("render").get("length")}});
    manager.set("enabled",option(request,"enabled",true));manager.set("autoUpdate",option(request,"autoUpdate",true));manager.set("needsUpdate",option(request,"needsUpdate",false));if(!request.get("type").isUndefined())manager.set("type",request.get("type"));
    if(flag(request,"missingShadow"))h.set("light",object({{"name","unshadowed"}}));
    const auto previousConsole=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{auto values=array();for(unsigned i=1;i<args.size();++i)push(values,args[i].isObject()?JSValue("object"):args[i]);event(c[0],{"warn",values});return {};},{h}));
    try{
        execute(h);if(flag(request,"repeat"))execute(h);if(flag(request,"dispose")){call(source,"dispose");execute(h);}if(flag(request,"resize")){call(shadow.get("mapSize"),"set",{16,8});execute(h);}
        if(flag(request,"vsmRepeat")){shadow.set("blurSamples",5);shadow.set("radius",2);call(shadow.get("mapSize"),"set",{16,8});execute(h);}
        if(!request.get("typeChange").isUndefined()){
            const auto old=shadow.get("map");if(!old.isNull()){
                if(!old.get("depthTexture").isNull())call(old.get("depthTexture"),"addEventListener",{"dispose",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"depth.dispose",targetKey(c[0],c[1])});return {};},{h,old})});
                call(old,"addEventListener",{"dispose",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"target.dispose",targetKey(c[0],c[1]),c[1].get("depthTexture").isNull()});return {};},{h,old})});
            }manager.set("type",request.get("typeChange"));manager.set("needsUpdate",true);execute(h);
        }
    }catch(...){ThreeUtils::setConsoleFunction(previousConsole);throw;}ThreeUtils::setConsoleFunction(previousConsole);
    return object({{"defaults",defaults},{"rows",h.get("rows")},{"events",h.get("events")}});
}
}
int main(){try{const std::string text(std::istreambuf_iterator<char>(std::cin),{});auto json=nativeGlobalObject().get("JSON");const auto requests=call(json,"parse",{text});auto results=array();for(unsigned i=0;i<requests.length();++i)push(results,run(requests.get(JSValue(i))));std::cout<<results.toJSON()->dump()<<'\n';return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
