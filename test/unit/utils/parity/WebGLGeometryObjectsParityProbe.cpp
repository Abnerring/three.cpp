#include "renderers/webgl/WebGLGeometries.h"
#include "renderers/webgl/WebGLObjects.h"
#include "core/EventDispatcher.h"
#include "core/JSWeakMap.h"
#include "core/JSWeakRef.h"
#include "core/JSFinalizationRegistry.h"
#include "core/JSJobs.h"
#include "core/NativeArray.h"
#include "core/NativeObject.h"
#include "animation/AnimationUtils.h"
#include <iostream>
#include <cmath>
#include <limits>

namespace {
using Args=std::vector<JSValue>; using O=JsonValue::Object; using A=JsonValue::Array;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
JSPropertyDescriptor descriptor(JSValue value,bool enumerable=true){JSPropertyDescriptor d;d.value=value;d.writable=d.configurable=true;d.enumerable=enumerable;return d;}
void data(JSValue o,const std::string& k,JSValue v){o.defineProperty(k,descriptor(v));}
JSValue list(const Args& a){auto out=JSValue::array();out.get("push").call(out,a);return out;}
void push(JSValue out,const Args& a){out.get("push").call(out,{list(a)});}
JSValue scalar(unsigned i){static const std::vector<JSValue> values{{},nullptr,false,true,0,-0.0,std::numeric_limits<double>::quiet_NaN(),1,2,10,"0","2","10","x",std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()};return values.at(i);}
JsonValue encode(const JSValue& v){
    if(v.isUndefined())return "undefined";if(v.isNull())return nullptr;if(v.isCallable())return "function";
    if(v.type()==JSValue::Type::Boolean)return v.toBoolean();if(v.type()==JSValue::Type::String)return v.string();
    if(v.type()==JSValue::Type::Number){double n=v.number();if(std::isnan(n))return "NaN";if(!std::isfinite(n))return n<0?"-Infinity":"Infinity";if(n==0&&std::signbit(n))return "-0";return n;}
    if(v.type()==JSValue::Type::Array){A out;for(unsigned i=0;i<v.length();++i)out.push_back(encode(v.get(std::to_string(i))));return out;}
    O out;for(auto& k:v.keys())out.emplace(k,encode(v.get(k)));return out;
}
std::string label(const JSValue& v){if(!v.isObject())return v.isNull()?"null":v.toString();auto n=v.get("label");return n.type()==JSValue::Type::String?n.string():"wire";}
JSValue watched(JSValue target,const char* field,JSValue trace,const std::string& caption){
    auto holder=JSValue::object(nullptr);holder.set("value",target.get(field));JSPropertyDescriptor d;
    d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],{"get",c[1],self.sameValue(c[2])});return c[3].get("value");},{trace,caption,target,holder});
    d.setter=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],{"set",c[1],self.sameValue(c[2]),arg(a,0).isObject()?JSValue(label(arg(a,0))):arg(a,0)});auto h=c[3];h.set("value",arg(a,0));return {};},{trace,caption,target,holder});
    d.enumerable=d.configurable=true;target.defineProperty(field,d);return holder;
}
struct PrototypeScope{JSValue target;std::vector<std::pair<std::string,JSPropertyDescriptor>> old;void save(const char* name){old.emplace_back(name,*target.getOwnPropertyDescriptor(name));}~PrototypeScope(){for(auto& [k,d]:old)target.defineProperty(k,d);}};
JSValue callback(const JSValue& self,const Args& args,const Args& cap){
    auto state=cap[0];auto log=state.get("trace");const auto name=cap[1].string();
    push(log,name=="update"?Args{name,label(arg(args,0)),arg(args,1),self.sameValue(state.get("attributes"))}:Args{name,label(arg(args,0)),self.sameValue(state.get(name=="remove"?"attributes":"binding"))});
    auto behavior=state.get("behavior").toNumber();
    if(name=="update"&&behavior==1&&label(arg(args,0))=="matrix"){state.get("objects").get("dispose").call();}
    if(name=="remove"&&behavior==2)throw JSTypeError("remove");
    if(name=="remove"&&behavior==3){auto g=state.get("geometry");auto replacement=JSValue::object();data(replacement,"position","changed");data(replacement,"extra","changed-extra");g.set("attributes",replacement);}
    return {};
}
struct Fixture{
    JSValue state=JSValue::object(nullptr),trace=JSValue::array(),gl=JSValue::object(),attributes=JSValue::object(),binding=JSValue::object(),info=JSValue::object(),g=EventDispatcher::runtimeObject(),position=JSValue::object(),index=JSValue::object();
    JSValue geometries,objects,instance;
    Fixture(const JSValue& r){
        state.set("trace",trace);state.set("attributes",attributes);state.set("binding",binding);state.set("geometry",g);state.set("behavior",0);
        data(gl,"ARRAY_BUFFER",34962);auto memory=JSValue::object();data(memory,"geometries",0);data(info,"memory",memory);auto render=JSValue::object();data(render,"frame",0);data(info,"render",render);
        for(const char* n:{"update","remove"})data(attributes,n,JSValue::function(callback,{state,n}));
        for(const char* n:{"releaseStatesOfGeometry","releaseStatesOfObject"})data(binding,n,JSValue::function(callback,{state,n}));
        data(g,"label","geometry");data(g,"id",r.get("id").isUndefined()?JSValue(7):r.get("id"));data(g,"index",nullptr);
        const auto length=r.get("length").isUndefined()?9:r.get("length").toNumber();data(position,"label","position");data(position,"array",nativeArray(Float32Array(length)));
        data(position,"count",r.get("count").isUndefined()?JSValue(length/3):r.get("count"));data(position,"version",0);auto map=JSValue::object();if(!r.get("missing").toBoolean())data(map,"position",position);data(g,"attributes",map);
        Uint32Array values(r.get("indices").isUndefined()?0:r.get("indices").length());if(!r.get("indices").isUndefined()){for(unsigned i=0;i<values.size();++i)values.setIndex(i,r.get("indices").get(std::to_string(i)).toNumber());data(index,"label","index");data(index,"array",nativeArray(values));data(index,"version",0);g.set("index",index);}
        geometries=WebGLGeometries(gl,attributes,info,binding).object();state.set("geometries",geometries);
        objects=WebGLObjects(gl,geometries,attributes,binding,info).object();state.set("objects",objects);
        instance=EventDispatcher::runtimeObject();data(instance,"geometry",g);data(instance,"isInstancedMesh",true);auto matrix=JSValue::object();data(matrix,"label","matrix");data(instance,"instanceMatrix",matrix);auto color=JSValue::object();data(color,"label","color");data(instance,"instanceColor",color);
    }
    JSValue geo(const char* n,const Args& args={}){return geometries.get(n).call(JSValue::object(),args);}
    JSValue obj(const char* n,const Args& args={}){return objects.get(n).call(nullptr,args);}
    void dispose(JSValue target){auto event=JSValue::object();data(event,"type","dispose");target.get("dispatchEvent").call(target,{event});}
    double memory(){return info.get("memory").get("geometries").toNumber();}
};
JsonValue wireState(const JSValue& value){if(value.isUndefined())return "undefined";auto array=value.get("array");A a;for(unsigned i=0;i<array.get("length").toNumber();++i)a.push_back(encode(array.get(std::to_string(i))));return O{{"type",array.get("constructor").get("name").string()},{"array",a},{"version",encode(value.get("version"))},{"count",encode(value.get("count"))},{"itemSize",encode(value.get("itemSize"))},{"usage",encode(value.get("usage"))},{"gpuType",encode(value.get("gpuType"))},{"normalized",encode(value.get("normalized"))},{"ranges",encode(value.get("updateRanges"))}};}
JsonValue wire(const JSValue& r){Fixture f(r);const auto first=scalar(unsigned(r.get("first").toNumber())),next=scalar(unsigned(r.get("next").toNumber()));if(!r.get("indices").isUndefined())f.index.set("version",first);else f.position.set("version",first);
    if(r.get("watch").toBoolean()){for(const char* k:{"index","attributes"})watched(f.g,k,f.trace,std::string("geometry.")+k);for(const char* k:{"array","count","version"})watched(f.position,k,f.trace,std::string("position.")+k);if(!r.get("indices").isUndefined())for(const char* k:{"array","version"})watched(f.index,k,f.trace,std::string("index.")+k);}
    auto a=f.geo("getWireframeAttribute",{f.g});const auto before=wireState(a);if(!a.isUndefined())data(a,"label","wire0");
    if(!r.get("indices").isUndefined())f.index.set("version",next);else f.position.set("version",next);
    auto b=f.geo("getWireframeAttribute",{f.g});return O{{"first",before},{"second",wireState(b)},{"same",a.sameValue(b)},{"trace",encode(f.trace)}};
}
JsonValue geometryCase(const JSValue& r){Fixture f(r);unsigned s=unsigned(r.get("scenario").toNumber());std::optional<PrototypeScope> scope;std::string error="none";
    if(s==2){data(f.g,"isInstancedBufferGeometry",scalar(unsigned(r.get("flag").toNumber())));data(f.g,"_maxInstanceCount",8);}
    if(s==3){auto p=JSValue::object(),map=JSValue::object(p);data(p,"inherited",f.position);data(p,"hidden",JSValue::object());map.defineProperty("hidden",descriptor(f.index,false));data(map,"own",f.position);f.g.set("attributes",map);}
    if(s==4){data(f.g.get("attributes"),"extra",f.position);f.state.set("behavior",3);}
    if(s==5){auto proto=nativeFinalizationRegistryConstructor().get("prototype");scope.emplace();scope->target=proto;scope->save("register");proto.defineProperty("register",descriptor(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],{"register-throw"});throw JSTypeError("register");},{f.trace}),false));}
    if(s==6){auto proto=nativeWeakRefConstructor().get("prototype");scope.emplace();scope->target=proto;scope->save("deref");JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],{"deref-get",self.isObject()});return c[1];},{f.trace,proto.get("deref")});d.configurable=true;proto.defineProperty("deref",d);}
    if(r.get("watch").toBoolean()){for(const char* k:{"id","index","attributes","isInstancedBufferGeometry"})watched(f.g,k,f.trace,std::string("geometry.")+k);for(const char* k:{"remove","update"})watched(f.attributes,k,f.trace,std::string("attributes.")+k);for(const char* k:{"addEventListener","removeEventListener"})watched(f.g,k,f.trace,std::string("geometry.")+k);watched(f.info,"memory",f.trace,"info.memory");watched(f.info.get("memory"),"geometries",f.trace,"memory.geometries");watched(f.gl,"ARRAY_BUFFER",f.trace,"gl.ARRAY_BUFFER");}
    try{f.geo("get",{{},f.g});f.geo("get",{{},f.g});if(s==1)f.g.set("id",8);f.geo("update",{f.g});if(s!=3&&s!=4)f.geo("getWireframeAttribute",{f.g});if(s==7)f.geo("dispose");else{f.dispose(f.g);if(s==1)f.geo("dispose");}}
    catch(const JSTypeError&){error="TypeError";}catch(const JSRangeError&){error="RangeError";}catch(const std::exception&){error="Error";}
    const auto memory=f.memory();return O{{"error",error},{"memory",memory},{"max",encode(f.g.get("_maxInstanceCount"))},{"trace",encode(f.trace)}};
}
JsonValue objectCase(const JSValue& r){Fixture f(r);unsigned s=unsigned(r.get("scenario").toNumber());auto frame=scalar(unsigned(r.get("frame").toNumber()));f.info.get("render").set("frame",frame);std::optional<PrototypeScope> scope;
    auto skeleton=JSValue::object();data(skeleton,"label","skeleton");data(skeleton,"update",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],{"skeleton.update",self.sameValue(c[1])});if(c[2].toNumber()==8)throw JSTypeError("skeleton");return {};},{f.trace,skeleton,s}));
    data(f.instance,"isSkinnedMesh",true);data(f.instance,"skeleton",skeleton);
    if(s==1)f.state.set("behavior",1);if(s==2)data(f.instance,"instanceColor",nullptr);
    if(s==3)data(f.instance,"hasEventListener",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{return scalar(unsigned(c[0].toNumber()));},{r.get("flag")}));
    if(s==4){data(f.instance,"isInstancedMesh",false);}
    if(s==5){data(f.geometries,"get",JSValue::function(+[](const JSValue&,const Args& a,const Args&)->JSValue{return arg(a,0);}));data(f.geometries,"update",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],{"geometry.update-object"});return {};},{f.trace}));}
    if(s==6||s==7){auto p=nativeWeakMapConstructor().get("prototype");scope.emplace();scope->target=p;scope->save(s==6?"get":"set");const char* n=s==6?"get":"set";const auto original=p.get(n);
        JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],{c[1],"getter"});if(c[2].toNumber()==7)c[3].get("dispose").call();return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{push(c[0],{c[1],label(arg(a,0)),self.isObject()});return c[2].call(self,a);},{c[0],c[1],c[4]});},{f.trace,std::string("WeakMap.")+n,s,f.objects,original});d.configurable=true;p.defineProperty(n,d);}
    if(r.get("watch").toBoolean()){for(const char* k:{"geometry","isInstancedMesh","hasEventListener","addEventListener","instanceMatrix","instanceColor","isSkinnedMesh","skeleton"})watched(f.instance,k,f.trace,std::string("object.")+k);for(const char* k:{"get","update"})watched(f.geometries,k,f.trace,std::string("geometries.")+k);watched(f.info,"render",f.trace,"info.render");watched(f.info.get("render"),"frame",f.trace,"render.frame");watched(f.gl,"ARRAY_BUFFER",f.trace,"gl.ARRAY_BUFFER");}
    A errors;for(unsigned i=0;i<3;++i){try{f.obj("update",{f.instance});errors.emplace_back("none");}catch(const JSTypeError&){errors.emplace_back("TypeError");}catch(const std::exception&){errors.emplace_back("Error");}}
    try{if(s==9){f.dispose(f.instance);f.obj("update",{f.instance});}else if(s==10){f.obj("dispose");f.obj("update",{f.instance});}}catch(const JSTypeError&){errors.emplace_back("TypeError");}
    return O{{"errors",errors},{"memory",f.memory()},{"trace",encode(f.trace)}};
}
JsonValue metadata(JSValue fn){A names;for(auto& k:fn.ownPropertyNames())names.emplace_back(k);A flags;for(const char* n:{"length","name","prototype"}){auto d=*fn.getOwnPropertyDescriptor(n);flags.emplace_back(A{d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)});}return O{{"name",fn.get("name").string()},{"length",fn.get("length").number()},{"own",names},{"flags",flags},{"constructible",fn.isConstructible()},{"prototypeConstructor",fn.get("prototype").get("constructor").sameValue(fn)}};}
JsonValue constructCase(const JSValue& r){Fixture f(r);const auto group=r.get("group").string(),name=r.get("method").string();unsigned s=unsigned(r.get("scenario").toNumber());auto api=group=="geometry"?f.geometries:f.objects,fn=api.get(name),custom=JSValue::object();auto meta=metadata(fn);data(custom,"marker",8);
    auto target=JSValue::constructor(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],{"targetBody"});return {};},{f.trace},"Target",0,JSValue::ConstructorKind::Callable);auto newTarget=fn;
    if(s==1){target.set("prototype",custom);newTarget=target;}if(s==2){target.set("prototype",3);newTarget=target;}if(s==3)fn.set("prototype",custom);if(s==4)newTarget=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};});
    Args args=name=="get"?Args{{},f.g}:name=="dispose"?Args{}:Args{group=="geometry"?f.g:f.instance};if(s==5)args.clear();
    JsonValue result;try{auto value=s==6?fn.call(nullptr,args):fn.construct(args,newTarget);result=O{{"object",value.isObject()},{"geometry",value.sameValue(f.g)},{"own",value.isObject()?JsonValue([&]{A a;for(auto&k:value.ownPropertyNames())a.emplace_back(k);return a;}()):encode(value)},{"fnPrototype",value.isObject()&&value.getPrototype().sameValue(fn.get("prototype"))},{"targetPrototype",value.isObject()&&value.getPrototype().sameValue(target.get("prototype"))},{"customPrototype",value.isObject()&&value.getPrototype().sameValue(custom)}};}catch(const JSTypeError&){result="TypeError";}catch(const std::exception&){result="Error";}
    return O{{"metadata",meta},{"result",result},{"memory",f.memory()},{"trace",encode(f.trace)}};
}
JsonValue listenerCase(const JSValue& r){Fixture f(r);const auto group=r.get("group").string();unsigned s=unsigned(r.get("scenario").toNumber());auto object=group=="geometry"?f.g:f.instance;
    if(group=="geometry")f.geo("get",{{},f.g});else f.obj("update",{f.instance});
    auto fn=object.get("_listeners").get("dispose").get("0");auto meta=metadata(fn);auto target=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"Target",0,JSValue::ConstructorKind::Callable),custom=JSValue::object();target.set("prototype",custom);
    auto event=JSValue::object();data(event,"target",s==3?JSValue::object():object);Args args=s==4?Args{}:Args{event};JsonValue result;
    try{auto value=s==2?fn.call(nullptr,args):fn.construct(args,s==1?target:fn);result=O{{"object",value.isObject()},{"own",value.isObject()?encode(list([&]{Args a;for(auto& k:value.ownPropertyNames())a.emplace_back(k);return a;}())):encode(value)},{"fnPrototype",value.isObject()&&value.getPrototype().sameValue(fn.get("prototype"))},{"customPrototype",value.isObject()&&value.getPrototype().sameValue(custom)}};}catch(const JSTypeError&){result="TypeError";}catch(const std::exception&){result="Error";}
    return O{{"metadata",meta},{"result",result},{"memory",f.memory()},{"trace",encode(f.trace)}};
}
JsonValue pollutionCase(const JSValue& r){Fixture f(r);const auto s=unsigned(r.get("scenario").toNumber());auto prototype=JSValue::object().getPrototype();std::vector<std::pair<std::string,std::optional<JSPropertyDescriptor>>> old;
    auto fields=s<2?std::vector<std::string>{"get","update","getWireframeAttribute","dispose"}:s==2?std::vector<std::string>{"updateMap","geometries","registry","wireframeAttributes","onGeometryDispose","onInstancedMeshDispose"}:std::vector<std::string>{"7"};
    for(auto& name:fields){old.emplace_back(name,prototype.getOwnPropertyDescriptor(name));JSPropertyDescriptor d;d.configurable=true;if(s==1||s==3){d.value=17;d.writable=false;}else d.setter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],{"polluted-set",c[1]});return {};},{f.trace,name});prototype.defineProperty(name,d);}
    JsonValue result;try{f.geometries=WebGLGeometries(f.gl,f.attributes,f.info,f.binding).object();f.objects=WebGLObjects(f.gl,f.geometries,f.attributes,f.binding,f.info).object();f.state.set("objects",f.objects);f.state.set("geometries",f.geometries);f.obj("update",{f.instance});A names;for(auto&k:f.geometries.keys())names.emplace_back(k);result=O{{"names",names},{"memory",f.memory()},{"trace",encode(f.trace)}};}catch(const JSTypeError&){result="TypeError";}
    for(auto&[name,d]:old)if(d)prototype.defineProperty(name,*d);else prototype.erase(name);return result;
}
JsonValue gcCase(const JSValue& r){Fixture f(r);const auto s=unsigned(r.get("scenario").toNumber());f.geo("get",{{},f.g});f.geo("getWireframeAttribute",{f.g});JSWeakValue weak(f.g);const auto id=f.g.get("id");
    if(s==1)f.dispose(f.g);f.state.erase("geometry");f.g={};f.position={};f.index={};f.instance={};endJSJob();f.info.collectGarbage();const bool expired=weak.expired();drainFinalizationJobs();
    auto replacement=EventDispatcher::runtimeObject();data(replacement,"label","replacement");data(replacement,"id",id);data(replacement,"index",nullptr);data(replacement,"attributes",JSValue::object());f.geo("get",{{},replacement});f.geo("get",{{},replacement});
    return O{{"expired",expired},{"memory",f.memory()},{"trace",encode(f.trace)}};
}
JsonValue run(const JSValue& r){auto mode=r.get("mode").string();if(mode=="wire")return wire(r);if(mode=="geometry")return geometryCase(r);if(mode=="objects")return objectCase(r);if(mode=="listener")return listenerCase(r);if(mode=="pollution")return pollutionCase(r);if(mode=="gc")return gcCase(r);return constructCase(r);}
}
int main(){const std::string input(std::istreambuf_iterator<char>(std::cin),{});auto requests=Animation_detail::parseJSON(input);A output;for(unsigned i=0;i<requests.length();++i){try{output.push_back(run(requests.get(std::to_string(i))));}catch(const JSTypeError&){output.emplace_back("TypeError");}catch(const JSRangeError&){output.emplace_back("RangeError");}catch(const std::exception& e){output.emplace_back(O{{"unexpected",e.what()}});}endJSJob();requests.collectGarbage();drainFinalizationJobs();}std::cout<<JsonValue(output).dump();}
