#include "animation/AnimationUtils.h"
#include "materials/nodes/manager/NodeMaterialObserver.h"
#include "materials/Material.h"
#include "core/NativeObject.h"
#include "core/BufferGeometry.h"
#include "core/Object3D.h"
#include "core/EventDispatcher.h"
#include "core/InterleavedBuffer.h"
#include "core/InterleavedBufferAttribute.h"
#include "core/JSWeakMap.h"
#include "math/Color.h"
#include "math/Euler.h"
#include "math/Matrix4.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "textures/Texture.h"
#include "lights/SpotLight.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeFrame.h"
#include "constants.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <set>

namespace {
using Args=std::vector<JSValue>;
bool relativeResourceIds=false,hasTextureOrigin=false;
double textureOrigin=0;
std::vector<double> actualTextureIds;
void own(JSValue o,const std::string& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;o.defineProperty(key,d);}
JSValue arr(const Args& values={}){auto a=JSValue::array();for(const auto& value:values)own(a,std::to_string(a.length()),value);return a;}
JSValue call(const JSValue& o,const char* key,const Args& args={}){return o.get(key).call(o,args);}
JSValue rec(std::initializer_list<std::pair<const char*,JSValue>> fields){auto r=JSValue::object();for(const auto& [k,v]:fields)own(r,k,v);return r;}
JSValue error(){try{throw;}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return "#Error";}}
JSValue normal(const JSValue& v,std::vector<JSValue>* seen=nullptr){
    if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";if(v.isSymbol())return "#symbol";
    if(v.type()==JSValue::Type::Number){if(std::isnan(v.number()))return "#NaN";if(std::isinf(v.number()))return v.number()>0?"#Infinity":"#-Infinity";}
    if(!v.isObject())return v;if(isNativeWeakMap(v))return "#WeakMap";
    std::vector<JSValue> local;if(!seen)seen=&local;for(unsigned i=0;i<seen->size();++i)if(v.sameValue((*seen)[i]))return arr({"#ref",i});seen->push_back(v);
    auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();for(const auto& key:v.keys())own(out,key,normal(v.get(key),seen));return out;
}
JSValue descriptors(const JSValue& o){auto out=arr();for(const auto& key:o.ownPropertyNames()){const auto d=o.getOwnPropertyDescriptor(key);auto row=rec({{"name",key},{"writable",d->writable.value_or(false)},{"enumerable",d->enumerable.value_or(false)},{"configurable",d->configurable.value_or(false)}});if(d->getter)row.set("get",d->getter->get("name"));if(d->value){const auto v=*d->value;row.set("value",v.isCallable()?JSValue("#function"):v.isObject()?JSValue("#object"):normal(v));if(v.isCallable()){row.set("functionName",v.get("name"));row.set("length",v.get("length"));row.set("constructible",v.isConstructible());}}call(out,"push",{row});}return out;}
JSValue texture(){auto value=nativeTextureConstructor().construct();if(relativeResourceIds)actualTextureIds.push_back(value.get("id").number());return value;}
JSValue attribute(){return nativeBufferAttributeConstructor().construct({NativeArray_detail::constructor(6).construct({arr({0,1,2,3,4,5})}),3});}
JSValue makeFixture(const JSValue& request){
    auto object=nativeObject(std::make_shared<Object3D>()),geometry=nativeGeometry(std::make_shared<BufferGeometry>()),material=nativeMaterialConstructor().construct();
    material.set("envMap",nullptr);material.set("map",nullptr);material.set("color",nativeColorConstructor().construct({0x123456}));material.set("normalScale",nativeVector2Constructor().construct({1,1}));material.set("opacity",1);material.set("anisotropyRotation",nullptr);
    geometry.get("attributes").set("position",attribute());
    auto scene=rec({{"environment",nullptr},{"environmentIntensity",1},{"environmentRotation",nativeEuler(std::make_shared<Euler>())}}),context=rec({{"width",640},{"height",480}}),lights=arr();
    auto lightsNode=rec({{"getBuiltinLights",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{lights},"getBuiltinLights",0)}});
    auto renderer=rec({{"getMRT",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0].get("mrt");},{request},"getMRT",0)}});
    // The renderer and scene are host interfaces. Observer state and math remain production objects.
    auto host=rec({{"mrt",nullptr}});renderer.set("getMRT",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0].get("mrt");},{host},"getMRT",0));
    object.set("material",material);auto builder=nativeNodeBuilderConstructor().construct({object,renderer,nullptr});
    auto observer=nativeNodeMaterialObserverConstructor().construct({builder});auto frame=NodeFrame().object();frame.set("renderer",renderer);frame.set("renderId",0);
    auto renderObject=rec({{"object",object},{"geometry",geometry},{"material",material},{"context",context},{"scene",scene},{"lightsNode",lightsNode},{"bundle",nullptr}});
    if(relativeResourceIds&&!hasTextureOrigin){const auto baseline=texture();textureOrigin=baseline.get("id").number();hasTextureOrigin=true;std::cerr<<"#TextureBaseline "<<textureOrigin<<'\n';}
    return rec({{"object",object},{"geometry",geometry},{"material",material},{"scene",scene},{"context",context},{"lights",lights},{"lightsNode",lightsNode},{"renderer",renderer},{"host",host},{"builder",builder},{"observer",observer},{"frame",frame},{"renderObject",renderObject}});
}
void event(const JSValue& state,const std::string& name){if(!state.get("active").toBoolean())return;call(state.get("events"),"push",{name});if(state.get("request").get("throwAt").strictEquals(name))throw JSTypeError("fixture getter");}
void observe(const JSValue& state,JSValue object,const std::string& key,const std::string& name){const auto old=object.getOwnPropertyDescriptor(key);if(old&&!old->configurable.value_or(false))return;auto slot=rec({{"value",object.get(key)}});call(state.get("slots"),"push",{arr({object,key,slot})});JSPropertyDescriptor d;d.enumerable=d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],"get "+c[2].string());return c[1].get("value");},{state,slot,name},"get fixture",0);d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],"set "+c[2].string());auto slot=c[1];slot.set("value",a.empty()?JSValue():a[0]);return {};},{state,slot,name},"set fixture",1);object.defineProperty(key,d);}
void observeMethod(const JSValue& state,JSValue object,const char* key,const std::string& name){const auto original=object.get(key);object.set(key,JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{event(c[0],"call "+c[2].string());return c[1].call(self,a);},{state,original,name},key,original.get("length").toNumber()));}
void restore(const JSValue& state){const auto slots=state.get("slots");for(unsigned i=0;i<slots.length();++i){const auto row=slots.get(std::to_string(i));own(row.get("0"),row.get("1").string(),row.get("2").get("value"));}}
JSValue metadata(const JSValue& r){auto fixture=makeFixture(r);const auto ctor=nativeNodeMaterialObserverConstructor(),observer=fixture.get("observer"),other=ctor.construct({fixture.get("builder")});auto facade=nativeNodeMaterialObserverModule(),keys=arr();for(const auto& key:facade.keys())call(keys,"push",{key});auto out=rec({{"keys",keys},{"constructor",descriptors(ctor)},{"prototype",descriptors(ctor.get("prototype"))},{"own",normal(observer)},{"sharedUniforms",observer.get("refreshUniforms").sameValue(other.get("refreshUniforms"))},{"separateMaps",!observer.get("renderObjects").sameValue(other.get("renderObjects"))},{"objectPrototype",ctor.get("prototype").getPrototype().sameValue(JSValue::intrinsicObjectConstructor().get("prototype"))}});facade.set("default",nullptr);out.set("privateBinding",nativeNodeMaterialObserverBinding("default").sameValue(ctor));out.set("privateCtor",nativeNodeMaterialObserverConstructor().sameValue(ctor));try{ctor.call({}, {fixture.get("builder")});}catch(...){out.set("callError",error());}return out;}
// SpotLight.map is a writable data field upstream. The older host setter only
// accepts native Texture payloads; define its actual dynamic data field so the
// real managed Texture and real native SpotLight can exercise this observer.
// This fixture does not claim native SpotLight's payload bridge accepts it.
void configure(JSValue fixture,const std::string& kind){auto o=fixture.get("object");if(kind=="center")o.set("center",nativeVector2Constructor().construct({.5,.5}));if(kind=="morph")o.set("morphTargetInfluences",arr({0,.25,.5}));if(kind=="instance"){own(o,"isInstancedMesh",true);o.set("instanceMatrix",attribute());o.set("instanceColor",nullptr);o.set("morphTexture",nullptr);}if(kind=="batch"){o.set("isBatchedMesh",true);o.set("_matricesTexture",texture());o.set("_colorsTexture",nullptr);o.set("_indirectTexture",texture());}if(kind=="bundle")fixture.get("renderObject").set("bundle",rec({{"version",0},{"static",true}}));if(kind=="environment")fixture.get("scene").set("environment",texture());if(kind=="lights"){auto light=nativeObject(std::make_shared<SpotLight>());own(light,"map",texture());light.set("castShadow",true);call(fixture.get("lights"),"push",{light});}if(kind=="texture")fixture.get("material").set("map",texture());}
void action(JSValue f,const std::string& a){auto object=f.get("object"),geometry=f.get("geometry"),material=f.get("material"),ro=f.get("renderObject"),frame=f.get("frame");
    if(a=="next")frame.set("renderId",frame.get("renderId").toNumber()+1);
    else if(a=="matrix")object.get("matrixWorld").get("elements").set("12",3);
    else if(a=="opacity")material.set("opacity",.25);
    else if(a=="color")call(material.get("color"),"setRGB",{.2,.3,.4});
    else if(a=="nullColor")material.set("color",nullptr);
    else if(a=="newColor")material.set("color",nativeColorConstructor().construct({0xff0000}));
    else if(a=="removeOpacity")material.erase("opacity");
    else if(a=="newOpacity")material.set("opacity",.7);
    else if(a=="width")f.get("context").set("width",800);
    else if(a=="height")f.get("context").set("height",600);
    else if(a=="geometry")ro.set("geometry",nativeGeometry(std::make_shared<BufferGeometry>()));
    else if(a=="attribute")geometry.get("attributes").get("position").set("needsUpdate",true);
    else if(a=="removeAttribute")geometry.get("attributes").erase("position");
    else if(a=="addAttribute")geometry.get("attributes").set("normal",attribute());
    else if(a=="replaceAttribute"){geometry.get("attributes").erase("position");geometry.get("attributes").set("normal",attribute());}
    else if(a=="index")geometry.set("index",attribute());
    else if(a=="indexVersion")geometry.get("index").set("needsUpdate",true);
    else if(a=="nullIndex")geometry.set("index",nullptr);
    else if(a=="range")geometry.get("drawRange").set("count",3);
    else if(a=="disposeGeometry")call(geometry,"dispatchEvent",{rec({{"type","dispose"}})});
    else if(a=="morph")object.get("morphTargetInfluences").set("1",.75);
    else if(a=="instanceMatrix")object.get("instanceMatrix").set("needsUpdate",true);
    else if(a=="instanceColor")object.set("instanceColor",attribute());
    else if(a=="instanceColorVersion")object.get("instanceColor").set("needsUpdate",true);
    else if(a=="morphTexture")object.set("morphTexture",texture());
    else if(a=="textureVersion")material.get("map").set("needsUpdate",true);
    else if(a=="newTexture")material.set("map",texture());
    else if(a=="nullTexture")material.set("map",nullptr);
    else if(a=="disposeTexture")call(material.get("map"),"dispose");
    else if(a=="matricesTexture")object.get("_matricesTexture").set("needsUpdate",true);
    else if(a=="colorsTexture")object.set("_colorsTexture",texture());
    else if(a=="indirectTexture")object.get("_indirectTexture").set("needsUpdate",true);
    else if(a=="center")call(object.get("center"),"set",{.2,.4});
    else if(a=="environmentIntensity")f.get("scene").set("environmentIntensity",2);
    else if(a=="environmentRotation")f.get("scene").get("environmentRotation").set("x",.3);
    else if(a=="lightMap")f.get("lights").get("0").get("map").set("needsUpdate",true);
    else if(a=="lightDispose")call(f.get("lights").get("0").get("map"),"dispose");
    else if(a=="shadowWidth")f.get("lights").get("0").get("shadow").get("mapSize").set("width",1024);
    else if(a=="shadowHeight")f.get("lights").get("0").get("shadow").get("mapSize").set("height",2048);
    else if(a=="bundleVersion")ro.get("bundle").set("version",1);
    else if(a=="static")object.set("static",true);
    else if(a=="dynamic")object.get("instanceMatrix").set("usage",35048);
    else if(a=="hasNode")f.get("observer").set("hasNode",true);
    else if(a=="hasAnimation")f.get("observer").set("hasAnimation",true);
    else if(a=="velocity")f.get("host").set("mrt",rec({{"has",JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue{return !a.empty()&&a[0].strictEquals("velocity");},{},"has",1)}}));
    else if(a=="noVelocity")f.get("host").set("mrt",nullptr);
}
JSValue materialSnapshot(const JSValue& data){auto out=normal(data);if(relativeResourceIds){const auto snapshot=data.get("map");if(snapshot.isObject()){
    // Only the observer's actual material.map Texture snapshot is relative.
    // Geometry/attribute IDs, arbitrary id properties and lights.map versions remain absolute.
    if(snapshot.keys()!=std::vector<std::string>{"id","version","cacheVersion"})throw std::logic_error("Unexpected material.map snapshot shape");
    const auto id=snapshot.get("id").number();if(std::find(actualTextureIds.begin(),actualTextureIds.end(),id)==actualTextureIds.end())throw std::logic_error("Snapshot did not originate from an actual fixture Texture");
    out.get("map").set("id",id-textureOrigin);
}}return out;}
JSValue state(const JSValue& f){const auto observer=f.get("observer"),ro=f.get("renderObject");auto out=rec({{"renderId",observer.get("renderId")},{"initialized",call(observer.get("renderObjects"),"has",{ro})}});if(out.get("initialized").toBoolean()){out.set("renderObject",normal(call(observer.get("renderObjects"),"get",{ro})));out.set("geometry",normal(call(observer,"getGeometryData",{ro.get("geometry")})));out.set("material",materialSnapshot(call(observer,"getMaterialData",{f.get("material")})));}return out;}
JSValue refresh(const JSValue& r){auto f=makeFixture(r);configure(f,r.get("configure").isUndefined()?"":r.get("configure").string());auto out=arr();const auto steps=r.get("steps");for(unsigned i=0;i<steps.length();++i){auto row=JSValue::object();try{action(f,steps.get(std::to_string(i)).string());row.set("result",normal(call(f.get("observer"),"needsRefresh",{f.get("renderObject"),f.get("frame")})));}catch(...){row.set("error",error());}row.set("state",state(f));call(out,"push",{row});}return out;}
JSValue protocol(const JSValue& r){auto f=makeFixture(r);const auto operation=r.get("operation").string();configure(f,r.get("configure").isUndefined()?"":r.get("configure").string());const auto observer=f.get("observer"),ro=f.get("renderObject");if(operation=="equals"||operation=="needsRefresh")call(observer,"getRenderObjectData",{ro});
    auto s=rec({{"request",r},{"events",arr()},{"slots",arr()},{"active",false}});auto out=JSValue::object();
    for(const auto* key:{"renderObjects","hasNode","hasAnimation","refreshUniforms","renderId"})observe(s,observer,key,std::string("observer.")+key);
    for(const auto* key:{"object","geometry","material","context","scene","lightsNode","bundle"})observe(s,ro,key,std::string("renderObject.")+key);
    for(const auto* key:{"isInstancedMesh","isBatchedMesh","instanceMatrix","instanceColor","morphTexture","_matricesTexture","_colorsTexture","_indirectTexture","center","morphTargetInfluences","matrixWorld","static"})observe(s,f.get("object"),key,std::string("object.")+key);
    for(const auto* key:{"id","attributes","index","drawRange"})observe(s,f.get("geometry"),key,std::string("geometry.")+key);
    for(const auto* key:{"start","count"})observe(s,f.get("geometry").get("drawRange"),key,std::string("drawRange.")+key);
    for(const auto* key:{"opacity","color","map","envMap","_renderId","_version"})observe(s,f.get("material"),key,std::string("material.")+key);
    for(const auto* key:{"width","height"})observe(s,f.get("context"),key,std::string("context.")+key);
    for(const auto* key:{"environment","environmentIntensity","environmentRotation"})observe(s,f.get("scene"),key,std::string("scene.")+key);
    for(const auto* key:{"renderId","renderer"})observe(s,f.get("frame"),key,std::string("frame.")+key);
    for(const auto* key:{"getRenderObjectData","getGeometryData","getTextureData","getMaterialData","getLightsData","getLights","getAttributesData","firstInitialization","hasDynamicInstancing","needsVelocity","equals"})observeMethod(s,observer,key,std::string("observer.")+key);
    observeMethod(s,f.get("lightsNode"),"getBuiltinLights","lightsNode.getBuiltinLights");observeMethod(s,f.get("geometry"),"addEventListener","geometry.addEventListener");observeMethod(s,f.get("renderer"),"getMRT","renderer.getMRT");
    s.set("events",arr());s.set("active",true);
    try{Args args;if(operation=="getGeometryData")args={f.get("geometry")};else if(operation=="getMaterialData")args={f.get("material")};else if(operation=="getLights")args={f.get("lightsNode"),1};else if(operation=="containsNode")args={f.get("builder")};else args={ro,operation=="equals"?arr():f.get("frame"),1};out.set("result",normal(call(observer,operation.c_str(),args)));}catch(...){out.set("error",error());}
    s.set("active",false);restore(s);out.set("events",s.get("events"));out.set("state",state(f));return out;
}
JSValue cache(const JSValue& r){auto f=makeFixture(r),observer=f.get("observer");auto out=JSValue::object();const auto kind=r.get("case").string();
    if(kind=="geometry"){const auto g=f.get("geometry"),a=call(observer,"getGeometryData",{g}),other=nativeNodeMaterialObserverConstructor().construct({f.get("builder")});out.set("initial",normal(a));out.set("shared",a.sameValue(call(other,"getGeometryData",{g})));call(g,"dispatchEvent",{rec({{"type","dispose"}})});call(g,"dispatchEvent",{rec({{"type","dispose"}})});out.set("disposed",normal(a));}
    if(kind=="texture"||kind=="renderTarget"){auto t=texture();const auto target=EventDispatcher::runtimeConstructor().construct();if(kind=="renderTarget")t.set("renderTarget",target);const auto a=call(observer,"getTextureData",{t}),other=nativeNodeMaterialObserverConstructor().construct({f.get("builder")});out.set("initial",normal(a));out.set("shared",a.sameValue(call(other,"getTextureData",{t})));call(kind=="renderTarget"?target:t,"dispatchEvent",{rec({{"type","dispose"}})});out.set("disposed",normal(a));if(kind=="renderTarget"){call(t,"dispatchEvent",{rec({{"type","dispose"}})});out.set("ownDisposeIgnored",normal(a));}}
    if(kind=="sharedMaterial"){auto ro=f.get("renderObject"),ro2=rec({{"object",f.get("object")},{"geometry",f.get("geometry")},{"material",f.get("material")},{"context",f.get("context")},{"scene",f.get("scene")},{"lightsNode",f.get("lightsNode")},{"bundle",nullptr}});const auto other=nativeNodeMaterialObserverConstructor().construct({f.get("builder")});call(observer,"getRenderObjectData",{ro});call(other,"getRenderObjectData",{ro2});f.get("material").set("opacity",.3);out.set("first",call(observer,"equals",{ro,arr(),1}));out.set("second",call(other,"equals",{ro2,arr(),1}));out.set("sameData",call(observer,"getMaterialData",{f.get("material")}).sameValue(call(other,"getMaterialData",{f.get("material")})));out.set("material",normal(call(observer,"getMaterialData",{f.get("material")})));}
    if(kind=="lightsReentry"){auto node=f.get("lightsNode"),guard=rec({{"once",true},{"nested",nullptr}});node.set("getBuiltinLights",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{auto guard=c[1];if(guard.get("once").toBoolean()){guard.set("once",false);guard.set("nested",call(c[0],"getLights",{self,9}));}return c[2];},{observer,guard,f.get("lights")},"getBuiltinLights",0));const auto a=call(observer,"getLights",{node,9});out.set("sameNested",a.sameValue(guard.get("nested")));out.set("value",normal(a));}
    if(kind=="geometryReentry"){auto g=f.get("geometry"),guard=rec({{"once",true},{"nested",nullptr}});const auto original=g.get("addEventListener");g.set("addEventListener",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{auto guard=c[2];if(guard.get("once").toBoolean()){guard.set("once",false);guard.set("nested",call(c[0],"getGeometryData",{self}));}return c[1].call(self,a);},{observer,original,guard},"addEventListener",2));const auto a=call(observer,"getGeometryData",{g});out.set("sameNested",a.sameValue(guard.get("nested")));out.set("cachedOuter",a.sameValue(call(observer,"getGeometryData",{g})));call(g,"dispatchEvent",{rec({{"type","dispose"}})});out.set("outer",normal(a));out.set("nested",normal(guard.get("nested")));}
    if(kind=="enum"){auto types=nativeConstantsBinding("RenderObjectRefreshType"),events=arr();for(const auto* key:{"NONE","SHARED","FULL"}){JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"push",{c[1]});return "enum-"+c[1].string();},{events,key},"get enum",0);d.enumerable=d.configurable=true;types.defineProperty(key,d);}auto values=arr();for(const auto* op:{"","","next","hasNode"}){action(f,op);call(values,"push",{call(observer,"needsRefresh",{f.get("renderObject"),f.get("frame")})});}out.set("values",values);out.set("events",events);}
    return out;
}
JSValue edge(const JSValue& r){auto f=makeFixture(r),o=f.get("observer");auto out=JSValue::object();const auto kind=r.get("case").string();
    if(kind=="dynamic"){auto object=f.get("object");own(object,"isInstancedMesh",r.get("instanced"));object.set("instanceMatrix",rec({{"usage",r.get("matrixUsage")}}));object.set("instanceColor",r.get("color").strictEquals("undefined")?JSValue():r.get("color").isNull()?JSValue(nullptr):rec({{"usage",r.get("color")}}));out.set("value",call(o,"hasDynamicInstancing",{object}));}
    if(kind=="velocity"){auto mrt=r.get("value"),calls=arr();out.set("calls",calls);if(mrt.strictEquals("undefined-mrt"))mrt={};else if(!mrt.isNull())mrt=rec({{"has",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{call(c[1],"push",{arr(a)});return c[0];},{mrt,calls},"has",1)}});f.get("host").set("mrt",mrt);out.set("value",normal(call(o,"needsVelocity",{f.get("renderer")})));}
    if(kind=="containsContext"){f.get("builder").get("context").set(r.get("key").string(),r.get("value"));out.set("value",call(o,"containsNode",{f.get("builder")}));}
    if(kind=="containsInherited"){auto proto=JSValue::object(f.get("material").getPrototype());proto.set("inheritedNode",nativeNodeConstructor().construct());f.get("material").setPrototype(proto);out.set("value",call(o,"containsNode",{f.get("builder")}));}
    if(kind=="firstStrict"){auto calls=arr();o.set("renderObjects",rec({{"has",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{r.get("value").strictEquals("undefined")?JSValue():r.get("value")},"has",1)}}));o.set("getRenderObjectData",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"push",{"getRenderObjectData"});return {};},{calls},"getRenderObjectData",1));out.set("value",call(o,"firstInitialization",{f.get("renderObject")}));out.set("calls",calls);}
    if(kind=="constructorOverride"){auto ctor=nativeNodeMaterialObserverConstructor(),proto=ctor.get("prototype"),calls=arr();out.set("calls",calls);proto.set("containsNode",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{call(c[0],"push",{a.empty()?JSValue(false):a[0].sameValue(c[1])});return "contains-sentinel";},{calls,f.get("builder")},"containsNode",1));out.set("own",normal(ctor.construct({f.get("builder")})));}
    if(kind=="worldStrict"){const auto ro=f.get("renderObject");auto data=call(o,"getRenderObjectData",{ro}),world=data.get("worldMatrix"),calls=arr();world.set("equals",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return c[0];},{r.get("value").strictEquals("undefined")?JSValue():r.get("value")},"equals",1));world.set("copy",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{call(c[0],"push",{"copy"});return {};},{calls},"copy",1));out.set("value",call(o,"equals",{ro,arr(),1}));out.set("calls",calls);}
    if(kind=="uniformList"){auto other=nativeNodeMaterialObserverConstructor().construct({f.get("builder")});call(o.get("refreshUniforms"),"push",{"custom"});f.get("material").set("custom",17);out.set("shared",other.get("refreshUniforms").get(std::to_string(other.get("refreshUniforms").length()-1)));out.set("material",normal(call(o,"getMaterialData",{f.get("material")})));}
    return out;
}
JSValue run(const JSValue& r){try{const auto kind=r.get("kind");if(kind.strictEquals("metadata"))return metadata(r);if(kind.strictEquals("refresh"))return refresh(r);if(kind.strictEquals("protocol"))return protocol(r);if(kind.strictEquals("cache"))return cache(r);if(kind.strictEquals("edge"))return edge(r);throw std::logic_error("Unknown observer fixture");}catch(...){return rec({{"error",error()}});}}
void lifetime(){for(unsigned round=0;round<3;++round){JSWeakValue ctor,key,data,observer,textureKey,textureData;{auto keeper=nativeNodeMaterialObserverConstructor();ctor=JSWeakValue(keeper);{auto f=makeFixture(JSValue::object());auto o=f.get("observer"),g=f.get("geometry"),t=texture();auto d=call(o,"getGeometryData",{g}),td=call(o,"getTextureData",{t});observer=JSWeakValue(o);key=JSWeakValue(g);data=JSWeakValue(d);textureKey=JSWeakValue(t);textureData=JSWeakValue(td);auto module=nativeNodeMaterialObserverModule();module.set("default",nullptr);if(!nativeNodeMaterialObserverBinding("default").sameValue(keeper))throw std::logic_error("Observer facade changed private binding");}keeper.collectGarbage();if(!key.expired()||!data.expired()||!observer.expired()||!textureKey.expired()||!textureData.expired())throw std::logic_error("Observer weak cache retained dead keys/data in live heap");}JSValue::nativeSafepoint();if(!ctor.expired())throw std::logic_error("Observer retired heap retained");}std::cout<<"3 fresh NodeMaterialObserver module/cache lifetimes and live-heap ephemeron collections passed\n";}
}
int main(int argc,char** argv){try{if(argc>1&&std::string(argv[1])=="--lifetime"){lifetime();return 0;}relativeResourceIds=argc>1&&std::string(argv[1])=="--relative-import-resource-ids";const std::string text(std::istreambuf_iterator<char>(std::cin),{});const auto request=Animation_detail::parseJSON(text);std::cout<<run(request).toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
