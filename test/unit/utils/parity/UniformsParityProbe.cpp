#include "core/Uniform.h"
#include "core/UniformsGroup.h"
#include "core/EventDispatcher.h"
#include "renderers/shaders/UniformsUtils.h"
#include "math/ColorManagement.h"
#include <iostream>
#include <cmath>
#include <functional>
using Args = std::vector<JSValue>;
using A = JsonValue::Array;
using O = JsonValue::Object;
JSValue list(std::initializer_list<JSValue> values = {}) {
    auto result = JSValue::array(); for (const auto& value : values) result.get("push").call(result, {value}); return result;
}
void log(JSValue trace, JSValue value) { trace.get("push").call(trace, {value}); }
void getter(JSValue object, const char* key, JSValue function) {
    JSPropertyDescriptor descriptor; descriptor.getter = function; descriptor.configurable = true; descriptor.enumerable = true; object.defineProperty(key, descriptor);
}
std::string attempt(const std::function<void()>& f) {
    try { f(); return ""; } catch (const JSTypeError&) { return "TypeError"; } catch (const JSRangeError&) { return "RangeError"; }
}
JsonValue names(const std::vector<std::string>& keys) { A result; for (const auto& key : keys) result.emplace_back(key); return result; }
JsonValue data(JSValue value) {
    if (value.isUndefined()) return "<undefined>";
    if (value.isCallable()) return "<function>";
    if (value.type() == JSValue::Type::Array) {
        A slots; for (std::uint32_t i = 0; i < value.length(); ++i) slots.emplace_back(value.hasOwn(std::to_string(i)) ? data(value.get(std::to_string(i))) : JsonValue("<hole>"));
        return O{{"length",value.length()},{"keys",names(value.keys())},{"slots",slots}};
    }
    if (value.isObject()) {
        if (value.hasOwn("value")) return O{{"value",data(value.get("value"))}};
        if (value.hasOwn("marker")) return O{{"marker",data(value.get("marker"))}};
        return O{{"keys",names(value.keys())}};
    }
    return value.toJSON().value_or(JsonValue());
}
JSValue cloneable(JSValue result, JSValue trace = {}, const char* label = "clone") {
    auto object = JSValue::object(); object.set("marker", result);
    object.set("clone", JSValue::function([](const JSValue& self, const Args& args, const Args& captures) -> JSValue {
        if (!captures[0].isUndefined()) log(captures[0], list({captures[1], args.size(), self.get("marker")}));
        return self.get("marker");
    }, {trace, label})); return object;
}
JsonValue metadata() {
    Uniform uniform(2); UniformsGroup a,b; A methods;
    for (const auto* name : {"add","remove","setName","setUsage","dispose","copy","clone"}) {
        const auto descriptor = UniformsGroup::prototype().getOwnPropertyDescriptor(name); const auto method = UniformsGroup::prototype().get(name);
        methods.emplace_back(A{name,method.get("name").toString(),method.get("length").toNumber(),*descriptor->writable,*descriptor->enumerable,*descriptor->configurable});
    }
    const auto id = a.object().getOwnPropertyDescriptor("id");
    return O{{"audit","metadata"},{"uniformKeys",names(uniform.object().keys())},{"uniformPrototype",names(Uniform::prototype().ownPropertyNames())},
        {"groupKeys",names(a.object().keys())},{"groupNames",names(a.object().ownPropertyNames())},{"prototypeNames",names(UniformsGroup::prototype().ownPropertyNames())},
        {"id",A{b.id.toNumber()-a.id.toNumber(),*id->writable,*id->enumerable,*id->configurable}},
        {"defaults",A{a.isUniformsGroup.toBoolean(),a.name.toString(),a.usage.toNumber(),data(a.uniforms.value())}},
        {"methods",methods},{"inherits",a.object().getPrototype().getPrototype().sameValue(EventDispatcher::runtimePrototype())},
        {"constructorInherits",UniformsGroup::constructor().getPrototype().sameValue(EventDispatcher::runtimePrototype().get("constructor"))},
        {"callError",attempt([&]{UniformsGroup::constructor().call();})}};
}
JsonValue uniformCase(int mode) {
    JSValue value, trace=list();
    if(mode==1)value=nullptr;if(mode==2)value=5;if(mode==3)value="text";if(mode==4)value=false;
    if(mode==5)value=JSValue::object();if(mode==6)value=list({1,2});if(mode>=7)value=cloneable(7,trace);
    Uniform uniform(value); auto owner=uniform.object();
    if(mode==8)value.set("clone",nullptr);
    if(mode==9)getter(value,"clone",JSValue::function([](const JSValue&,const Args&,const Args& c) -> JSValue{log(c[0],"first");auto owner=c[1];owner.set("value",c[2]);return 1;},{trace,owner,cloneable(20,trace,"replacement")}));
    if(mode==10){auto counter=JSValue::object();counter.set("n",0);getter(owner,"value",JSValue::function([](const JSValue&,const Args&,const Args& c) -> JSValue{auto counter=c[0];const auto n=counter.get("n").toNumber();counter.set("n",n+1);log(c[1],n);return n==0?c[2]:c[3];},{counter,trace,value,cloneable(33,trace,"next")}));}
    if(mode==11){auto counter=JSValue::object();counter.set("n",0);getter(value,"clone",JSValue::function([](const JSValue&,const Args&,const Args& c) -> JSValue{auto counter=c[0];const auto n=counter.get("n").toNumber();counter.set("n",n+1);log(c[1],n);return n==0?JSValue(1):JSValue();},{counter,trace}));}
    if(mode==12)getter(value,"clone",JSValue::function([](const JSValue&,const Args&,const Args& c) -> JSValue{auto owner=c[0];owner.set("value",88);log(c[1],"absent");return JSValue();},{owner,trace}));
    if(mode==13)value.set("clone",JSValue::function([](const JSValue&,const Args&,const Args& c) -> JSValue{log(c[0],"throw");throw JSTypeError("clone");return JSValue();},{trace}));
    if(mode==14){owner.set("constructor",nullptr);}
    if(mode==15)value.set("clone",JSValue::function([](const JSValue& self,const Args&,const Args&) -> JSValue{return self;}));
    JSValue result;const auto error=attempt([&]{result=uniform.clone().object();});
    return O{{"audit","uniform"},{"mode",mode},{"error",error},{"result",data(result)},
        {"sameValue",!result.isUndefined()&&result.get("value").sameValue(value)},{"freshUniform",!result.isUndefined()&&!result.sameValue(owner)},
        {"prototype",!result.isUndefined()&&result.getPrototype().sameValue(Uniform::prototype())},{"trace",data(trace)}};
}
JsonValue copyCase(int mode) {
    UniformsGroup source,destination;source.setName("source");source.setUsage(27);auto trace=list();
    auto original=destination.uniforms.value(),replacement=list(),src=source.uniforms.value();
    auto callback=JSValue::object();callback.set("clone",JSValue::function([](const JSValue&,const Args&,const Args& c) -> JSValue{
        const auto mode=int(c[0].toNumber());auto source=c[1],destination=c[2],trace=c[3],replacement=c[4],src=c[5];log(trace,"clone0");
        if(mode==1)destination.set("uniforms",replacement);
        if(mode==2)src.erase("1");
        if(mode==3)src.set("2",Uniform(42));
        if(mode==4)log(src,Uniform(55));
        if(mode==5)src.set("length",1);
        if(mode==6)throw JSTypeError("clone failure");
        if(mode==7)destination.set("uniforms",nullptr);
        if(mode==8)source.set("uniforms",list());
        if(mode==9)src.set("1",nullptr);
        return JSValue(10);
    },{mode,source.object(),destination.object(),trace,replacement,src}));
    source.add(callback);auto nested=JSValue::array(3);nested.set("0",Uniform(20));nested.set("2",Uniform(30));source.add(nested);source.add(Uniform(40));
    if(mode==10)destination.uniforms=src;
    if(mode==11)nested.set("map",JSValue::function([](const JSValue& self,const Args& a,const Args& c) -> JSValue{log(c[0],list({"map",a.size(),a[0].get("length"),a[0].get("name")}));return a[0].call(nullptr,{self.get("2")});},{trace}));
    if(mode==12)nested.set("map",nullptr);
    if(mode==13){auto proto=JSValue::object(nested.getPrototype());proto.set("1",Uniform(25));nested.setPrototype(proto);}
    if(mode==14)getter(original,"push",JSValue::function([](const JSValue&,const Args&,const Args& c) -> JSValue{log(c[0],"getPush");auto destination=c[1];destination.set("uniforms",c[2]);return c[3];},{trace,destination.object(),replacement,JSValue::array().get("push")}));
    if(mode==15){JSPropertyDescriptor d;d.writable=false;original.defineProperty("length",d);}
    if(mode==16)getter(source.object(),"usage",JSValue::function([](const JSValue&,const Args&,const Args& c) -> JSValue{log(c[0],"usage");throw JSTypeError("usage");return JSValue();},{trace}));
    if(mode==17){nested.set("constructor",4);}
    if(mode==18)getter(nested,"constructor",JSValue::function([](const JSValue& self,const Args&,const Args& c) -> JSValue{log(c[0],"constructor");auto array=self;array.set("1",Uniform(25));return JSValue();},{trace}));
    if(mode==19){auto len=JSValue::object();len.set("valueOf",JSValue::function([](const JSValue&,const Args&,const Args& c) -> JSValue{log(c[0],"length");return 2;},{trace}));auto arrayLike=JSValue::object();arrayLike.set("length",len);arrayLike.set("0",callback);arrayLike.set("1",Uniform(80));source.uniforms=arrayLike;}
    JSValue result;const auto error=attempt([&]{result=destination.copy(source);});
    return O{{"audit","copy"},{"mode",mode},{"error",error},{"chain",result.sameValue(destination.object())},
        {"name",data(destination.name.value())},{"usage",data(destination.usage.value())},{"target",data(destination.uniforms.value())},
        {"original",data(original)},{"replacement",data(replacement)},{"trace",data(trace)}};
}
JsonValue operationsCase(int mode) {
    UniformsGroup group;auto a=Uniform(1).object(),b=Uniform(2).object();auto trace=list();JSValue result;
    group.add(a);group.add(a);group.add(b);
    if(mode==0)result=group.remove(a);
    if(mode==1)result=group.remove(Uniform(1));
    if(mode==2)result=group.copy(group);
    if(mode==3){auto original=group.uniforms.value();getter(original,"indexOf",JSValue::function([](const JSValue&,const Args&,const Args& c) -> JSValue{log(c[0],"indexOf");auto group=c[1];group.set("uniforms",c[2]);return JSValue::function([](const JSValue&,const Args&,const Args&) -> JSValue{return 1;});},{trace,group.object(),list({8,9,10})}));result=group.remove(a);}
    if(mode==4){auto original=group.uniforms.value();original.set("indexOf",JSValue::function([](const JSValue&,const Args&,const Args&) -> JSValue{return "-1";}));result=group.remove(a);}
    if(mode==5){group.setName(nullptr);result=group.setUsage("dynamic");}
    return O{{"audit","operations"},{"mode",mode},{"chain",result.sameValue(group.object())},{"uniforms",data(group.uniforms.value())},{"name",data(group.name.value())},{"usage",data(group.usage.value())},{"trace",data(trace)}};
}
JsonValue mapCase(int mode) {
    auto map=JSValue::array().get("map"),trace=list(),receiver=JSValue::array(4),thisArg=JSValue::object();receiver.set("0",2);receiver.set("2",6);
    if(mode==1){auto proto=JSValue::object(receiver.getPrototype());proto.set("1",4);receiver.setPrototype(proto);}
    if(mode==2)receiver="A\xF0\x9F\x98\x80";
    if(mode==3)receiver=17;
    if(mode==4)receiver=nullptr;
    if(mode==5)receiver=JSValue();
    if(mode>=6&&mode<=8){receiver=JSValue::object();receiver.set("0",3);receiver.set("2",9);receiver.set("length",mode==6?JSValue("3.8"):mode==7?JSValue(-2):JSValue(4294967296.));}
    if(mode==9||mode==10||mode==11)getter(receiver,"constructor",JSValue::function([](const JSValue&,const Args&,const Args& c) -> JSValue{log(c[0],"constructor");if(c[1].toNumber()==11)throw JSTypeError("ctor");return c[1].toNumber()==10?JSValue(1):JSValue();},{trace,mode}));
    if(mode==12)getter(receiver,"0",JSValue::function([](const JSValue& self,const Args&,const Args& c) -> JSValue{log(c[0],"get0");auto array=self;array.set("1",4);return 2;},{trace}));
    if(mode==16){receiver=JSValue::object();getter(receiver,"length",JSValue::function([](const JSValue&,const Args&,const Args& c) -> JSValue{log(c[0],"length");throw JSTypeError("length");return JSValue();},{trace}));}
    if(mode==17){getter(receiver,"constructor",JSValue::function([](const JSValue&,const Args&,const Args& c) -> JSValue{log(c[0],"unexpected constructor");throw JSTypeError("ctor");return JSValue();},{trace}));}
    if(mode==18){receiver=false;}
    if(mode==19){receiver=JSValue::object();receiver.set("length",3);receiver.set("0",5);getter(receiver,"constructor",JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{throw JSTypeError("must not read");}));}
    auto callback=JSValue::function([](const JSValue& self,const Args& a,const Args& c) -> JSValue{const auto mode=int(c[0].toNumber());auto array=a[2];log(c[1],list({a[0],a[1],self.sameValue(c[2]),array.isObject(),array.get("length")}));
        if(mode==13&&a[1].toNumber()==0){array.erase("2");array.set("1",4);array.set("4",8);}
        if(mode==14)throw JSTypeError("callback");
        if(mode==15)return JSValue();
        return a[0];
    },{mode,trace,thisArg});
    if(mode==16||mode==17)callback=nullptr;
    JSValue result;const auto error=attempt([&]{result=map.call(receiver,{callback,thisArg});});
    return O{{"audit","map"},{"mode",mode},{"error",error},{"result",data(result)},{"trace",data(trace)}};
}
JsonValue cloneCase(int mode) {
    UniformsGroup source;source.setName("source");source.add(Uniform(3));auto trace=list();const auto start=source.id.toNumber();JSValue ctor=UniformsGroup::constructor();
    if(mode==1)ctor=nullptr;
    if(mode==2)ctor=JSValue::function([](const JSValue&,const Args&,const Args&) -> JSValue{return JSValue();});
    if(mode>=3&&mode<=6){ctor=JSValue::constructor([](const JSValue& receiver,const Args& args,const Args& c) -> JSValue{log(c[0],list({"construct",args.size()}));auto self=receiver;self.set("marker",17);
        if(c[1].toNumber()==5)throw JSTypeError("construct");
        if(c[1].toNumber()==6)return JSValue(5);
        if(c[1].toNumber()==4){auto result=JSValue::object();result.set("marker",19);result.set("copy",c[2]);return result;}
        return JSValue();
    },{trace,mode,JSValue::function([](const JSValue& self,const Args& args,const Args& c) -> JSValue{log(c[0],list({"copy",self.get("marker"),args[0].get("name")}));return self.get("marker");},{trace})},"Custom",0);ctor.get("prototype").set("copy",JSValue::function([](const JSValue& self,const Args& args,const Args& c) -> JSValue{log(c[0],list({"copy",self.get("marker"),args[0].get("name")}));return self.get("marker");},{trace}));}
    if(mode==7){source.uniforms.get("0").set("value",nullptr);}
    getter(source.object(),"constructor",JSValue::function([](const JSValue&,const Args&,const Args& c) -> JSValue{log(c[0],"getConstructor");if(c[1].toNumber()==8)throw JSTypeError("get");return c[2];},{trace,mode,ctor}));
    JSValue result;const auto error=attempt([&]{result=source.clone();});UniformsGroup after;
    JsonValue packed=result.isObject()&&result.hasOwn("uniforms")?JsonValue(O{{"name",data(result.get("name"))},{"uniforms",data(result.get("uniforms"))}}):data(result);
    return O{{"audit","clone"},{"mode",mode},{"error",error},{"result",packed},{"allocated",after.id.toNumber()-start-1},{"trace",data(trace)}};
}
JsonValue eventCase(int mode) {
    UniformsGroup group;auto trace=list(),events=list();auto second=JSValue::function([](const JSValue& self,const Args& a,const Args& c) -> JSValue{log(c[0],list({"second",self.sameValue(a[0].get("target"))}));return JSValue();},{trace});
    auto first=JSValue::function([](const JSValue& self,const Args& a,const Args& c) -> JSValue{log(c[0],list({"first",self.sameValue(a[0].get("target"))}));log(c[1],a[0]);auto group=UniformsGroup::fromObject(self);if(c[2].toNumber()==1)group.removeEventListener("dispose",c[3]);if(c[2].toNumber()==2)group.addEventListener("dispose",c[3]);if(c[2].toNumber()==3)throw JSTypeError("event");return JSValue();},{trace,events,mode,second});
    group.addEventListener("dispose",first);group.addEventListener("dispose",first);if(mode!=2)group.addEventListener("dispose",second);
    if(mode==4)group.object().set("dispatchEvent",JSValue::function([](const JSValue& self,const Args& a,const Args& c) -> JSValue{log(c[0],list({"override",a[0].get("type"),a[0].get("target").isUndefined()}));return 88;},{trace}));
    JSValue result;const auto error=attempt([&]{result=group.dispose();});
    return O{{"audit","events"},{"mode",mode},{"error",error},{"undefinedReturn",result.isUndefined()},{"trace",data(trace)},
        {"retainedTarget",events.length()?JSValue(events.get("0").get("target").sameValue(group.object())).toJSON().value():JsonValue(false)},
        {"hasSecond",group.hasEventListener("dispose",second).toBoolean()}};
}
JsonValue deepData(JSValue value) {
    if (!value.isObject() || value.isCallable()) return data(value);
    if (value.type()==JSValue::Type::Array) {
        A values;for(std::uint32_t i=0;i<value.length();++i)values.push_back(value.hasOwn(std::to_string(i))?deepData(value.get(std::to_string(i))):JsonValue("<hole>"));
        return O{{"length",value.length()},{"keys",names(value.keys())},{"slots",values}};
    }
    O result;for(const auto& key:value.keys())result[key]=deepData(value.get(key));return result;
}
JSValue definition(JSValue value) { auto result=JSValue::object();result.set("value",value);return result; }
JsonValue utilityCloneCase(int mode) {
    auto trace=list(),source=JSValue::object(),ordinary=JSValue::object();ordinary.set("marker",5);
    auto three=cloneable(17,trace);three.set("isVector3",true);
    JSValue value=list({ordinary,three});
    if(mode==1)value=list({three,cloneable(18,trace)});
    if(mode==2)value=three;
    if(mode==3){three.set("isRenderTargetTexture",true);value=three;}
    if(mode==4)value=list({nullptr,three});
    if(mode==5)value=list({three,nullptr});
    if(mode==6||mode==7){value=list({three,cloneable(18,trace)});three.set("clone",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"clone");auto array=c[1];if(c[2].toNumber()==6)array.set("length",1);else log(array,cloneable(19));return 17;},{trace,value,mode}));}
    if(mode==8)value.set("slice",JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{log(c[0],list({"slice",args.size(),self.get("length")}));return "override";},{trace}));
    if(mode==9)value.set("constructor",4);
    if(mode==16){value=JSValue::array(3);value.set("2",ordinary);}
    if(mode==18){value=three;getter(three,"isRenderTargetTexture",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"targetFlag");return "truthy";},{trace}));}
    if(mode==19)value=ordinary;
    source.set("a",definition(value));
    if(mode==10){auto proto=JSValue::object();proto.set("inherited",definition(three));source.setPrototype(proto);auto inner=source.get("a"),innerProto=JSValue::object();innerProto.set("extra",7);inner.setPrototype(innerProto);}
    if(mode==11){auto counter=JSValue::object();counter.set("n",0);auto initial=definition(1);initial.set("second",2);auto replacement=definition(9);replacement.set("second",10);getter(source,"a",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{auto count=c[0];const auto n=count.get("n").toNumber();count.set("n",n+1);log(c[1],n);return n==0?c[2]:c[3];},{counter,trace,initial,replacement}));}
    if(mode==12){source.set("b",definition(9));auto entry=source.get("a");getter(entry,"value",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"delete");auto source=c[1];source.erase("b");return 3;},{trace,source}));}
    if(mode==13)source="abc";
    if(mode==14)source=nullptr;
    if(mode==15){source.set("a","ab");source.set("b",4);source.set("c",nullptr);}
    if(mode==17){auto proto=JSValue::object();proto.set("a",definition(19));proto.set("b",definition(21));source.setPrototype(proto);JSPropertyDescriptor descriptor;descriptor.enumerable=false;source.defineProperty("a",descriptor);}
    JSValue result;const auto error=attempt([&]{result=cloneUniforms(source);});
    const bool shared= result.isObject()&&result.get("a").isObject()&&result.get("a").get("value").sameValue(ordinary);
    const bool sharesElement=result.isObject()&&result.get("a").isObject()&&result.get("a").get("value").type()==JSValue::Type::Array&&result.get("a").get("value").get("0").sameValue(ordinary);
    return O{{"audit","cloneUniforms"},{"mode",mode},{"error",error},{"result",deepData(result)},{"shared",shared},{"sharesElement",sharesElement},{"trace",data(trace)}};
}
JsonValue utilityMergeCase(int mode) {
    auto trace=list(),first=JSValue::object(),second=JSValue::object(),third=JSValue::object();first.set("a",definition(1));first.set("b",definition(2));second.set("b",definition(3));second.set("c",definition(4));third.set("a",definition(8));auto source=list({first,second});
    if(mode>=1&&mode<=3){auto value=JSValue::object();value.set("isColor",true);value.set("clone",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"clone");auto array=c[1];const auto mode=c[2].toNumber();if(mode==1)log(array,c[3]);if(mode==2)array.set("length",1);if(mode==3)array.set("1",c[3]);return 7;},{trace,source,mode,third}));first.set("a",definition(value));}
    if(mode==4){auto arrayLike=JSValue::object();arrayLike.set("0",first);arrayLike.set("1",second);auto count=JSValue::object();count.set("n",0);getter(arrayLike,"length",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{auto count=c[0];const auto n=count.get("n").toNumber();count.set("n",n+1);log(c[1],n);return n==0?1:2;},{count,trace}));source=arrayLike;}
    if(mode==5)source=nullptr;
    if(mode==6){source=list({nullptr,"ab",first});}
    if(mode==7){getter(source,"1",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"second");throw JSTypeError("second");},{trace}));}
    JSValue result;const auto error=attempt([&]{result=UniformsUtils::merge(source);});return O{{"audit","mergeUniforms"},{"mode",mode},{"error",error},{"result",deepData(result)},{"trace",data(trace)}};
}
JsonValue utilityGroupsCase(int mode) {
    auto trace=list(),source=list();UniformsGroup group;group.setName("group");group.add(Uniform(1));
    auto first=JSValue::object();first.set("clone",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"first");auto source=c[1];const auto mode=c[2].toNumber();if(mode==1)log(source,cloneable(30,c[0]));if(mode==2)source.set("length",1);if(mode==3)source.erase("1");if(mode==4)source.set("1",cloneable(40,c[0]));return 10;},{trace,source,mode}));
    log(source,first);log(source,cloneable(20,trace));
    if(mode==0)source=list({group.object()});
    if(mode==5)source=list({nullptr});
    if(mode==6){auto object=JSValue::object();getter(object,"length",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"length");return 1;},{trace}));object.set("0",first);source=object;}
    JSValue result;const auto error=attempt([&]{result=cloneUniformsGroups(source);});
    if(mode==0&&!result.isUndefined()){auto clone=result.get("0");result=list({clone.get("name"),clone.get("uniforms"),clone.get("id").toNumber()-group.id.toNumber()});}
    return O{{"audit","cloneGroups"},{"mode",mode},{"error",error},{"result",data(result)},{"trace",data(trace)}};
}
JsonValue colorSpaceCase(int mode) {
    auto trace=list(),renderer=JSValue::object(),target=JSValue::object(),texture=JSValue::object();
    getter(renderer,"outputColorSpace",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"outputColorSpace");return "output";},{trace}));
    getter(texture,"colorSpace",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"colorSpace");if(c[1].toNumber()==9)throw JSTypeError("color");return "texture";},{trace,mode}));
    getter(target,"texture",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"texture");return c[1].toNumber()==5?JSValue(nullptr):c[2];},{trace,mode,texture}));
    getter(target,"isXRRenderTarget",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"isXR");const auto mode=c[1].toNumber();if(mode==7)throw JSTypeError("XR");if(mode==2)return 1;if(mode==3)return "true";if(mode==4)return JSValue();return true;},{trace,mode}));
    getter(renderer,"getRenderTarget",JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{log(c[0],"get method");return JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{log(c[0],list({"call",args.size(),self.sameValue(c[3])}));if(c[1].toNumber()==8)throw JSTypeError("getTarget");if(c[1].toNumber()==0)return nullptr;if(c[1].toNumber()==6)return JSValue();return c[2];},c);},{trace,mode,target,renderer}));
    const auto previous=ColorManagement::workingColorSpace;ColorManagement::workingColorSpace="working";
    JSValue result;const auto error=attempt([&]{result=getUnlitUniformColorSpace(renderer);});ColorManagement::workingColorSpace=previous;
    return O{{"audit","colorSpace"},{"mode",mode},{"error",error},{"result",data(result)},{"trace",data(trace)}};
}
int main() {
    A results;results.push_back(metadata());
    for(int i=0;i<16;++i)results.push_back(uniformCase(i));
    for(int i=0;i<20;++i)results.push_back(copyCase(i));
    for(int i=0;i<6;++i)results.push_back(operationsCase(i));
    for(int i=0;i<20;++i)results.push_back(mapCase(i));
    for(int i=0;i<9;++i)results.push_back(cloneCase(i));
    for(int i=0;i<5;++i)results.push_back(eventCase(i));
    for(int i=0;i<20;++i)results.push_back(utilityCloneCase(i));
    for(int i=0;i<8;++i)results.push_back(utilityMergeCase(i));
    for(int i=0;i<7;++i)results.push_back(utilityGroupsCase(i));
    for(int i=0;i<10;++i)results.push_back(colorSpaceCase(i));
    std::cout<<JsonValue(results).dump()<<'\n';
}
