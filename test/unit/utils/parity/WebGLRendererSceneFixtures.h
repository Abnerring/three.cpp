#pragma once
#include "renderers/WebGLRendererScene.h"
#include "renderers/webgl/WebGLRenderLists.h"
#include "renderers/webgl/WebGLRenderStates.h"
#include "core/Layers.h"
#include "core/JSPromise.h"
#include "core/JSAsyncFunction.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Color.h"
#include "math/Sphere.h"
#include <cmath>

// CPU protocol harness. The list/state implementations and mathematical
// objects are real ports; backend services record calls without claiming GPU
// execution. The oracle injects the same boundaries into pinned source bodies.
namespace WebGLRendererSceneFixtures {
using Args=std::vector<JSValue>;
inline JSValue obj(){return JSValue::object();}
inline JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
inline void push(JSValue a,JSValue v){a.set(JSValue(a.length()),v);}
inline JSValue list(const Args& a={}){auto out=JSValue::array();for(const auto& v:a)push(out,v);return out;}
inline JSValue call(JSValue o,const char* key,const Args& a={}){return o.get(key).call(o,a);}
inline JSValue clean(JSValue v){if(v.isUndefined()){auto o=obj();o.set("$undefined",true);return o;}if(v.type()==JSValue::Type::Number&&!std::isfinite(v.number())){auto o=obj();o.set("$number",std::isnan(v.number())?"NaN":v.number()<0?"-Infinity":"Infinity");return o;}if(!v.isObject())return v;auto o=v.type()==JSValue::Type::Array?list():obj();for(const auto& k:v.keys())o.set(k,clean(v.get(k)));return o;}
inline JSValue tag(JSValue v){if(!v.isObject())return v;if(v.hasOwn("tag"))return v.get("tag");if(v.type()==JSValue::Type::Array){auto out=list();for(double i=0;i<v.length();++i)push(out,tag(v.get(JSValue(i))));return out;}return "$object";}
inline JSValue get(JSValue h,const char* name){return h.get("pipeline").getInternalSlot("WebGLRendererScene.get").call({}, {name});}
inline void set(JSValue h,const char* name,JSValue v){h.get("pipeline").getInternalSlot("WebGLRendererScene.set").call({}, {name,v});}
inline JSValue failure(){const auto v=nativeExceptionValue(std::current_exception());auto out=obj();if(v.isObject())out.set("name",v.get("name"));else{out.set("name","ThrownValue");out.set("raw",v);}return out;}
inline void event(JSValue h,JSValue v){push(h.get("events"),v);}
inline JSValue dispatch(const JSValue& self,const Args& args,const Args& captures);
inline void method(JSValue h,JSValue owner,const char* key,const std::string& label,JSValue original={}){owner.set(key,JSValue::function(dispatch,{h,label,owner,original},key,0));}
inline void instrument(JSValue h,JSValue value,const char* key,const std::string& label){method(h,value,key,label,value.get(key));}
inline JSValue camera(JSValue h,const char* name,int id){auto out=obj();out.set("tag",name);out.set("id",id);out.set("isCamera",true);out.set("parent",nullptr);out.set("matrixWorldAutoUpdate",true);out.set("coordinateSystem",2000);out.set("reversedDepth",false);out.set("projectionMatrix",nativeMatrix4Constructor().construct());out.set("matrixWorldInverse",nativeMatrix4Constructor().construct());out.set("layers",nativeLayersConstructor().construct());method(h,out,"updateMatrixWorld",std::string("camera.update:")+name);return out;}
inline JSValue material(JSValue h,const char* name,int id){auto out=obj();out.set("tag",name);out.set("id",id);out.set("visible",true);out.set("transparent",false);out.set("transmission",0);out.set("side",0);out.set("forceSinglePass",false);out.set("allowOverride",true);out.set("version",0);method(h,out,"onBeforeRender",std::string("material.before:")+name);JSPropertyDescriptor d;d.setter=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{event(c[0],list({"material.needsUpdate",self.get("tag"),arg(a,0)}));if(arg(a,0).strictEquals(true)){auto value=self;value.set("version",self.get("version").toNumber()+1);}return {};},{h});d.configurable=true;out.defineProperty("needsUpdate",d);return out;}
inline JSValue node(JSValue h,const char* name,int id,const char* kind="mesh"){auto out=obj();out.set("tag",name);out.set("id",id);out.set("visible",true);out.set("layers",nativeLayersConstructor().construct());out.set("children",list());out.set("renderOrder",0);out.set("frustumCulled",false);out.set("matrixWorld",nativeMatrix4Constructor().construct());out.set("modelViewMatrix",nativeMatrix4Constructor().construct());out.set("normalMatrix",nativeMatrix3Constructor().construct());if(std::string(kind)=="mesh")out.set("isMesh",true);if(std::string(kind)=="sprite")out.set("isSprite",true);if(std::string(kind)=="group")out.set("isGroup",true);if(std::string(kind)=="lod"){out.set("isLOD",true);out.set("autoUpdate",true);method(h,out,"update",std::string("lod.update:")+name);}method(h,out,"onBeforeRender",std::string("object.before:")+name);method(h,out,"onAfterRender",std::string("object.after:")+name);method(h,out,"intersectsFrustum",std::string("intersects:")+name);return out;}
inline JSValue scene(JSValue h,const char* name,int id){auto out=node(h,name,id,"scene");out.set("isScene",true);out.set("overrideMaterial",nullptr);out.set("matrixWorldAutoUpdate",true);method(h,out,"updateMatrixWorld",std::string("scene.update:")+name);method(h,out,"onBeforeRender",std::string("scene.before:")+name);method(h,out,"onAfterRender",std::string("scene.after:")+name);return out;}
inline JSValue geometry(JSValue h,const char* name){auto out=obj();out.set("tag",name);out.set("groups",list());out.set("boundingSphere",nullptr);method(h,out,"computeBoundingSphere",std::string("geometry.sphere:")+name);return out;}
inline void maybeReenter(JSValue h,const std::string& label){const auto r=h.get("request");if(!r.get("reentry").strictEquals(label)||h.get("reentered").toBoolean())return;h.set("reentered",true);event(h,list({"reenter",label}));h.get("renderer").get("render").call(h.get("renderer"),{h.get("inner"),h.get("camera2")});event(h,list({"resumed",label}));}
inline JSValue dispatch(const JSValue& self,const Args& args,const Args& c){
    auto h=c[0];const auto label=c[1].toString();const auto request=h.get("request"),owner=c[2],original=c[3];
    auto entry=list({label,self.sameValue(owner)});for(const auto& a:args)push(entry,tag(a));event(h,entry);
    if(label.starts_with("scene.before:")||label.starts_with("scene.after:")||label.starts_with("object.")||label.starts_with("material.before:")||label=="draw"){
        auto aliases=list();for(const auto& a:args){if(a.isObject()&&a.hasOwn("tag")){const auto known=h.get("aliases").get(a.get("tag"));push(aliases,known.isUndefined()?JSValue(nullptr):JSValue(a.sameValue(known)));}else push(aliases,nullptr);}event(h,list({"arg-identities",label,aliases}));
    }
    maybeReenter(h,label);if(request.get("throwAt").strictEquals(label))throw JSThrownValue(label);
    if(label=="objects.update"){
        auto object=arg(args,0);h.set("textureUnits",h.get("textureUnits").toNumber()+1);
        if(request.get("shrinkChildren").toBoolean()&&object.sameValue(h.get("mesh")))h.get("scene").get("children").set("length",1);
        return object.get("geometry");
    }
    if(label.starts_with("geometry.sphere:")){auto object=owner;object.set("boundingSphere",nativeSphereConstructor().construct({nativeVector3Constructor().construct(),.5}));return {};}
    if(label.starts_with("intersects:")){if(request.get("rejectFrustum").toBoolean())return false;return call(arg(args,0),"intersectsSphere",{nativeSphereConstructor().construct({nativeVector3Constructor().construct(),.5})});}
    if(label=="clipping.init")return request.get("clipping").toBoolean();
    if(label=="textures.getTextureUnits")return h.get("textureUnits");
    if(label=="textures.setTextureUnits"){h.set("textureUnits",arg(args,0));return {};}
    if(label=="xr.getCamera")return h.get("camera2");
    if(label=="xr.getDepthSensingMesh")return request.get("depthMesh").toBoolean()?h.get("depthMesh"):JSValue(nullptr);
    if(label=="xr.hasDepthSensing")return request.get("depthSensing").toBoolean();
    if(label=="output.begin")return request.hasOwn("outputValue")?request.get("outputValue"):JSValue(true);
    if(label=="output.hasRenderPass")return request.get("skipScene").toBoolean();
    if(label=="output.isCompositing")return request.get("compositing").toBoolean();
    if(label=="extensions.has")return arg(args,0).strictEquals("WEBGL_multisampled_render_to_texture")&&request.get("msrt").toBoolean();
    if(label=="renderer.getRenderTarget")return h.get("target");
    if(label=="renderer.getActiveCubeFace"||label=="renderer.getActiveMipmapLevel")return 0;
    if(label=="renderer.setRenderTarget"){auto target=arg(args,0);if(target.isObject()&&!target.hasOwn("tag"))target.set("tag","transmission-target");h.set("target",target);set(h,"_currentRenderTarget",target);return {};}
    if(label=="renderer.getClearColor"){call(arg(args,0),"copy",{h.get("clearColor")});return arg(args,0);}
    if(label=="renderer.getClearAlpha")return h.get("clearAlpha");
    if(label=="renderer.setClearColor"){call(h.get("clearColor"),"set",{arg(args,0)});if(args.size()>1)h.set("clearAlpha",arg(args,1));return {};}
    if(label=="draw"){
        event(h,list({"draw-state",arg(args,3).get("side"),arg(args,3).get("version"),arg(args,0).get("viewport").isUndefined()}));
        if(request.get("drawThrow").toBoolean()&&(!request.get("throwMesh").toBoolean()||arg(args,4).sameValue(h.get("mesh"))))throw JSThrownValue("GPU:unsupported");return {};
    }
    if(original.isCallable()){
        const auto value=original.call(self,args);
        if(label=="renderStates.get"&&!value.getInternalSlot("sceneHarnessInstrumented").toBoolean()){
            auto target=value;target.setInternalSlot("sceneHarnessInstrumented",true);for(const auto* key:{"init","pushLight","pushShadow","setupLights","setupLightsView"})instrument(h,target,key,std::string("renderState.")+key);
        }return value;
    }
    return {};
}
inline JSValue harness(JSValue request){
    auto h=obj();h.set("request",request);h.set("events",list());h.set("textureUnits",0);h.set("reentered",false);h.set("target",nullptr);h.set("clearAlpha",.25);h.set("clearColor",nativeColorConstructor().construct({0x123456}));
    auto renderer=obj(),bindings=obj();renderer.set("tag","renderer");renderer.set("sortObjects",request.hasOwn("sort")?request.get("sort"):JSValue(true));renderer.set("clippingPlanes",list());renderer.set("localClippingEnabled",request.get("clipping").toBoolean());renderer.set("transmissionResolutionScale",.5);renderer.set("toneMapping",3);
    auto info=obj(),render=obj();render.set("frame",0);info.set("render",render);info.set("autoReset",true);method(h,info,"reset","info.reset");renderer.set("info",info);h.set("renderer",renderer);bindings.set("_this",renderer);
    auto cam=camera(h,"camera",101),cam2=camera(h,"camera2",102);h.set("camera",cam);h.set("camera2",cam2);cam2.set("viewport",nativeVector4Constructor().construct({2,3,32,24}));
    auto root=scene(h,"scene",1),inner=scene(h,"inner",2);h.set("scene",root);h.set("inner",inner);auto mat=material(h,"material",7),mat2=material(h,"material2",8),over=material(h,"override",9);h.set("material",mat);
    if(request.hasOwn("transparent"))mat.set("transparent",request.get("transparent"));if(request.hasOwn("side"))mat.set("side",request.get("side"));if(request.hasOwn("allowOverride"))mat.set("allowOverride",request.get("allowOverride"));if(request.get("transmission").toBoolean()){mat.set("transmission",1);mat.set("side",2);}
    if(request.get("override").toBoolean())root.set("overrideMaterial",over);
    auto mesh=node(h,"mesh",11),mesh2=node(h,"mesh2",12),geom=geometry(h,"geometry"),geom2=geometry(h,"geometry2");h.set("mesh",mesh);mesh.set("geometry",geom);mesh.set("material",mat);mesh2.set("geometry",geom2);mesh2.set("material",mat2);call(mesh2.get("matrixWorld"),"makeTranslation",{0,0,.5});
    if(request.hasOwn("visible"))mesh.set("visible",request.get("visible"));if(request.hasOwn("materialVisible"))mat.set("visible",request.get("materialVisible"));if(request.get("cull").toBoolean())mesh.set("frustumCulled",true);if(request.get("layerMismatch").toBoolean())call(mesh.get("layers"),"set",{2});
    if(request.get("groups").toBoolean()){auto group=obj();group.set("tag","group");group.set("start",0);group.set("count",3);group.set("materialIndex",1);geom.set("groups",list({group}));mesh.set("material",list({mat2,mat}));}
    if(request.get("sprite").toBoolean()){mesh.erase("isMesh");mesh.set("isSprite",true);}
    if(request.get("line").toBoolean()){mesh.erase("isMesh");mesh.set("isLine",true);}
    if(request.get("points").toBoolean()){mesh.erase("isMesh");mesh.set("isPoints",true);}
    auto children=list({mesh,mesh2});if(request.get("group").toBoolean()||request.get("lod").toBoolean()){auto parent=node(h,"parent",13,request.get("lod").toBoolean()?"lod":"group");parent.set("renderOrder",5);parent.set("children",children);children=list({parent});}root.set("children",children);
    if(request.get("arrayCamera").toBoolean()){cam.set("isArrayCamera",true);auto a=camera(h,"array0",201),b=camera(h,"array1",202);a.set("viewport",nativeVector4Constructor().construct({0,0,16,16}));b.set("viewport",nativeVector4Constructor().construct({16,0,16,16}));cam.set("cameras",list({a,b}));}
    if(request.get("light").toBoolean()){auto light=node(h,"light",21,"light");light.set("isLight",true);light.set("isAmbientLight",true);light.set("color",nativeColorConstructor().construct({0xffffff}));light.set("intensity",2);light.set("castShadow",true);push(children,light);}
    auto extensions=obj();method(h,extensions,"has","extensions.has");bindings.set("extensions",extensions);auto capabilities=obj();capabilities.set("samples",4);bindings.set("capabilities",capabilities);
    auto lists=WebGLRenderLists().object(),states=WebGLRenderStates(extensions).object();instrument(h,lists,"get","renderLists.get");instrument(h,states,"get","renderStates.get");bindings.set("renderLists",lists);bindings.set("renderStates",states);
    const std::vector<std::pair<const char*,Args>> services={{"objects",{"update"}},{"clipping",{"init","beginShadows","endShadows","setGlobalState"}},{"background",{"addToRenderList","render"}},{"shadowMap",{"render"}},{"textures",{"getTextureUnits","setTextureUnits","updateMultisampleRenderTarget","updateRenderTargetMipmap"}},{"bindingStates",{"resetDefaultState"}}};
    for(const auto& [name,keys]:services){auto value=obj();for(const auto& key:keys)method(h,value,key.toString().c_str(),std::string(name)+"."+key.toString());bindings.set(name,value);}
    auto state=obj(),buffers=obj();for(const auto* name:{"depth","color"}){auto buffer=obj();for(const auto* key:{"setTest","setMask"})method(h,buffer,key,std::string("state.")+name+"."+key);buffers.set(name,buffer);}state.set("buffers",buffers);method(h,state,"viewport","state.viewport");method(h,state,"setPolygonOffset","state.setPolygonOffset");bindings.set("state",state);
    auto xr=obj();xr.set("enabled",request.get("xr").toBoolean());xr.set("isPresenting",request.get("xr").toBoolean());xr.set("cameraAutoUpdate",true);for(const auto* key:{"updateCamera","getCamera","getDepthSensingMesh","hasDepthSensing"})method(h,xr,key,std::string("xr.")+key);bindings.set("xr",xr);renderer.set("xr",xr);
    auto depthMesh=node(h,"depthMesh",22);depthMesh.set("geometry",geometry(h,"depth-geometry"));depthMesh.set("material",mat2);h.set("depthMesh",depthMesh);
    if(request.get("output").toBoolean()){auto output=obj();for(const auto* key:{"begin","hasRenderPass","isCompositing","end"})method(h,output,key,std::string("output.")+key);bindings.set("output",output);}
    if(request.get("nodes").toBoolean()){auto nodes=obj();for(const auto* key:{"renderStart","updateLights","setObject","renderEnd"})method(h,nodes,key,std::string("nodes.")+key);bindings.set("_nodesHandler",nodes);mat.set("isNodeMaterial",true);}
    bindings.set("_isContextLost",request.get("contextLost").toBoolean());
    for(const auto* key:{"getRenderTarget","getActiveCubeFace","getActiveMipmapLevel","setRenderTarget","getClearColor","getClearAlpha","setClearColor","clear"})method(h,renderer,key,std::string("renderer.")+key);method(h,renderer,"renderBufferDirect","draw");
    auto aliases=obj();for(const auto& value:Args{renderer,cam,cam2,root,inner,mat,mat2,over,mesh,mesh2,geom,geom2,depthMesh})aliases.set(value.get("tag"),value);if(request.get("arrayCamera").toBoolean())for(double i=0;i<cam.get("cameras").length();++i){const auto value=cam.get("cameras").get(JSValue(i));aliases.set(value.get("tag"),value);}if(request.get("groups").toBoolean()){const auto group=geom.get("groups").get("0");aliases.set("group",group);}h.set("aliases",aliases);
    auto pipeline=nativeWebGLRendererScenePipeline(bindings);h.set("pipeline",pipeline);renderer.set("render",pipeline.get("render"));call(get(h,"_currentViewport"),"set",{0,0,64,48});
    if(request.get("target").toBoolean()){auto target=get(h,"WebGLRenderTarget").construct({64,48});target.set("tag","target");h.set("target",target);set(h,"_currentRenderTarget",target);}
    if(request.get("badCamera").toBoolean())cam.set("isCamera",false);
    if(request.get("visibleGetter").toBoolean()){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],"get:visible");return c[1];},{h,request.get("visible")});mesh.defineProperty("visible",d);}
    return h;
}
inline JSValue run(JSValue request){auto h=harness(request),out=obj();if(request.get("op").strictEquals("metadata")){auto records=list();for(const auto* name:{"render","projectObject","renderScene","renderTransmissionPass","renderObjects","renderObject","WebGLRenderTarget"}){const auto fn=std::string(name)=="render"?h.get("pipeline").get("render"):std::string(name)=="WebGLRenderTarget"?get(h,name):h.get("pipeline").getInternalSlot(std::string("WebGLRendererScene.")+name);auto record=obj();record.set("key",name);record.set("name",fn.get("name"));record.set("length",fn.get("length"));record.set("source",call(fn,"toString"));record.set("constructible",fn.isConstructible());push(records,record);}return records;}try{const auto value=h.get("renderer").get("render").call(h.get("renderer"),{h.get("scene"),h.get("camera")});out.set("value",clean(value));}catch(...){out.set("error",failure());}
    out.set("events",clean(h.get("events")));auto state=obj();state.set("renderListDepth",get(h,"renderListStack").get("length"));state.set("renderStateDepth",get(h,"renderStateStack").get("length"));state.set("currentListNull",get(h,"currentRenderList").isNull());state.set("currentStateNull",get(h,"currentRenderState").isNull());state.set("materialSide",h.get("material").get("side"));state.set("materialVersion",h.get("material").get("version"));state.set("frame",h.get("renderer").get("info").get("render").get("frame"));state.set("toneMapping",h.get("renderer").get("toneMapping"));state.set("textureUnits",h.get("textureUnits"));state.set("target",tag(h.get("target")));out.set("state",state);return out;}
}
