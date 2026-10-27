#include "animation/AnimationUtils.h"
#include "core/JSPromise.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/core/Node.h"
#include "nodes/core/ConstNode.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
JSValue call(const JSValue& o,const std::string& k,const Args& a={}){return o.get(k).call(o,a);}
void data(JSValue o,const JSValue& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue array(const Args& values={}){auto a=JSValue::array();for(const auto& v:values)data(a,std::to_string(a.length()),v);return a;}
JSValue normal(const JSValue& v,Args seen={}){
    if(v.isUndefined())return "#undefined";if(v.isSymbol())return "#symbol";if(v.isCallable())return "#function";
    if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "#NaN";if(std::isinf(n))return n>0?"#Infinity":"#-Infinity";if(n==0&&std::signbit(n))return "#-0";}
    if(!v.isObject())return v;if(v.get("fixtureTag").toBoolean())return "@"+v.get("fixtureTag").toString();for(const auto& prior:seen)if(prior.sameValue(v))return "#cycle";seen.push_back(v);
    if(v.get("isNode").strictEquals(true)){
        auto out=JSValue::object();out.set("class",v.get("constructor").get("name"));
        for(const auto* k:{"nodeType","isWeak","value","node","nodes","convertTo","components","sourceNode","targetNode","indexNode","structNode","property"})if(v.hasOwn(k))data(out,k,normal(v.get(k),seen));return out;
    }
    if(v.get("isVector2").toBoolean()||v.get("isVector3").toBoolean()||v.get("isVector4").toBoolean()){auto out=JSValue::object();for(const auto* k:{"x","y","z","w"})if(v.has(k))data(out,k,normal(v.get(k),seen));return out;}
    if(v.get("isColor").toBoolean()){auto out=JSValue::object();for(const auto* k:{"r","g","b"})data(out,k,normal(v.get(k),seen));return out;}
    if(v.get("isMatrix2").toBoolean()||v.get("isMatrix3").toBoolean()||v.get("isMatrix4").toBoolean()){auto out=JSValue::object();out.set("elements",normal(v.get("elements"),seen));return out;}
    auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();if(v.type()==JSValue::Type::Array)for(std::uint32_t i=0;i<v.length();++i)if(!v.hasOwn(std::to_string(i)))data(out,std::to_string(i),"#hole");for(const auto& k:v.keys())data(out,k,normal(v.get(k),seen));return out;
}
void event(JSValue s,const Args& values){call(s.get("events"),"push",{array(values)});}
JSValue decode(const JSValue& v){
    if(!v.isObject())return v;const auto tag=v.get("tag");if(tag.strictEquals("undefined")||tag.strictEquals("hole"))return {};if(tag.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();if(tag.strictEquals("-0"))return -0.0;if(tag.strictEquals("Infinity"))return std::numeric_limits<double>::infinity();
    if(tag.strictEquals("node")){auto n=nativeNodeConstructor().construct({v.has("nodeType")?v.get("nodeType"):JSValue("float")});n.set("fixtureTag",v.get("name").isUndefined()?JSValue("node"):v.get("name"));return n;}
    if(tag.strictEquals("const")){auto n=nativeConstNodeConstructor().construct({decode(v.get("value")),v.has("nodeType")?v.get("nodeType"):JSValue{}});n.set("id",42);return n;}
    if(v.type()==JSValue::Type::Array){auto out=JSValue::array(v.length());for(std::uint32_t i=0;i<v.length();++i){const auto value=v.get(std::to_string(i));if(!value.isObject()||!value.get("tag").strictEquals("hole"))data(out,std::to_string(i),decode(value));}return out;}
    return v;
}
JSValue attempt(const JSValue& function,const JSValue& self,const Args& args){try{return normal(function.call(self,args));}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const JSThrownValue& e){return normal(e.value);}catch(const std::exception&){return "#Error";}}
void console(JSValue s){ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{Args values{"console"};for(const auto& v:a)values.push_back(normal(v));event(c[0],values);return {};},{s},"console",0));}
JSValue conversion(JSValue m,const JSValue& r){
    auto out=JSValue::object();Args args;auto input=r.get("args");for(std::uint32_t i=0;i<input.length();++i)args.push_back(decode(input.get(std::to_string(i))));
    auto s=JSValue::object();s.set("events",array());console(s);const auto f=m.get(r.get("method"));JSValue value;
    try{value=f.call({},args);out.set("result",normal(value));out.set("repeatIdentity",value.sameValue(f.call({},args)));}catch(const JSTypeError&){out.set("result","#TypeError");}catch(const JSRangeError&){out.set("result","#RangeError");}catch(const std::exception&){out.set("result","#Error");}
    out.set("events",s.get("events"));ThreeUtils::setConsoleFunction({});return out;
}
JSValue collection(JSValue m,const JSValue& r){
    auto s=JSValue::object();s.set("events",array());s.set("request",r);auto values=decode(r.get("input"));s.set("values",values);auto object=r.get("array").toBoolean()?JSValue::array(values.length()):JSValue::object();
    for(std::uint32_t i=0;i<values.length();++i){const auto key=std::to_string(i);if(!values.hasOwn(key))continue;JSPropertyDescriptor d;d.enumerable=d.configurable=true;
        d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[1]});if(c[0].get("request").get("throwGet").strictEquals(c[1]))throw JSTypeError("get");return c[0].get("values").get(c[1]);},{s,key},"get",0);
        d.setter=JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{"set",c[1],normal(arg(a,0))});if(c[0].get("request").get("throwSet").strictEquals(c[1]))throw JSTypeError("set");auto values=c[0].get("values");values.set(c[1],arg(a,0));return {};},{s,key},"set",1);object.defineProperty(key,d);
    }
    if(!r.get("array").toBoolean())data(object,"length",values.get("length"));
    auto out=JSValue::object();try{const auto result=m.get(r.get("method")).call({}, {object,decode(r.get("altType"))});out.set("resultIdentity",result.sameValue(object));out.set("result",normal(result));}catch(const JSTypeError&){out.set("result","#TypeError");}
    auto remaining=r.get("array").toBoolean()?JSValue::array(object.length()):JSValue::object();for(const auto& key:object.keys()){const auto d=object.getOwnPropertyDescriptor(key).value();data(remaining,key,normal(d.value.value_or(values.get(key))));}
    out.set("remaining",normal(remaining));out.set("keys",array([&]{Args keys;for(const auto& k:object.ownPropertyNames())keys.push_back(k);return keys;}()));out.set("events",s.get("events"));return out;
}
JSValue proxyFactory(JSValue m,const JSValue& r){
    auto s=JSValue::object();s.set("events",array());s.set("request",r);console(s);auto C=JSValue::builtinConstructor(+[](const JSValue& newTarget,const Args& a,const Args& c)->JSValue{
        Args values{"construct"};for(const auto& v:a)values.push_back(normal(v));event(c[0],values);auto n=nativeNodeConstructor().construct({},newTarget);n.set("fixtureTag","constructed");auto state=c[0];state.set("constructed",n);
        for(const auto* key:{"foo","intent"}){JSPropertyDescriptor d;d.setter=JSValue::function(+[](const JSValue& self,const Args& a,const Args& cap)->JSValue{event(cap[0],{"write",cap[1],normal(arg(a,0)),self.sameValue(cap[0].get("constructed"))});if(cap[0].get("request").get("throwSet").strictEquals(cap[1]))throw JSTypeError("set");return {};},{c[0],key},"set",1);d.enumerable=d.configurable=true;n.defineProperty(key,d);}
        JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& cap)->JSValue{event(cap[0],{"getIntent",self.sameValue(cap[0].get("constructed"))});return JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"intent",self.sameValue(c[0].get("constructed"))});if(c[0].get("request").get("throwIntent").toBoolean())throw JSTypeError("intent");return self;},{cap[0]},"toVarIntent",0);},{c[0]},"get",0);d.configurable=true;n.defineProperty("toVarIntent",d);return n;
    },{s},"Fixture",0);C.setPrototype(nativeNodeConstructor());C.get("prototype").setPrototype(nativeNodeConstructor().get("prototype"));
    JSPropertyDescriptor type;type.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"type"});return "Fixture";},{s},"get type",0);type.configurable=true;C.defineProperty("type",type);
    auto settings=JSValue(nullptr);if(r.has("settings")){settings=JSValue::object();const auto input=r.get("settings");for(const auto& key:input.keys()){JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"settings",c[1]});if(c[0].get("request").get("throwSetting").strictEquals(c[1]))throw JSTypeError("settings");return c[2];},{s,key,decode(input.get(key))},"get",0);d.enumerable=d.configurable=true;settings.defineProperty(key,d);}}
    auto out=JSValue::object();try{
        Args args{C};if(r.get("method").strictEquals("nodeImmutable")){auto params=decode(r.get("args"));for(std::uint32_t i=0;i<params.length();++i)args.push_back(params.get(std::to_string(i)));out.set("result",normal(m.get("nodeImmutable").call({},args)));}
        else{args.push_back(r.has("scope")?decode(r.get("scope")):JSValue(nullptr));args.push_back(r.has("factor")?decode(r.get("factor")):JSValue(nullptr));args.push_back(settings);auto f=m.get(r.get("method")).call({},args);
            if(r.has("limits")){auto limits=r.get("limits");Args a;for(std::uint32_t i=0;i<limits.length();++i)a.push_back(decode(limits.get(std::to_string(i))));out.set("limitsIdentity",call(f,"setParameterLength",a).sameValue(f));}
            if(r.has("name"))out.set("nameIdentity",call(f,"setName",{decode(r.get("name"))}).sameValue(f));
            auto params=decode(r.get("args"));Args a;for(std::uint32_t i=0;i<params.length();++i)a.push_back(params.get(std::to_string(i)));out.set("result",normal(f.call({},a)));out.set("metadata",array({f.get("name"),f.get("length"),f.get("setParameterLength").get("length"),f.get("setName").get("length")}));}
    }catch(const JSTypeError&){out.set("result","#TypeError");}catch(const JSRangeError&){out.set("result","#RangeError");}catch(const std::exception&){out.set("result","#Error");}out.set("events",s.get("events"));ThreeUtils::setConsoleFunction({});return out;
}
JSValue swizzle(JSValue m,const JSValue& r){
    auto s=JSValue::object();s.set("events",array());auto n=nativeNodeConstructor().construct({"vec4"});n.set("fixtureTag","source");const auto property=r.get("property");auto out=JSValue::object();console(s);
    try{
        const auto first=n.get(property),second=n.get(r.has("alias")?r.get("alias"):property);out.set("first",normal(first));out.set("identity",first.sameValue(second));out.set("keys",array([&]{Args a;for(const auto& k:n.get("_cache").keys())a.push_back(k);return a;}()));
        const auto d=nativeNodeConstructor().get("prototype").getOwnPropertyDescriptor(property).value();const auto other=nativeNodeConstructor().get("prototype").getOwnPropertyDescriptor(r.has("alias")?r.get("alias"):property).value();out.set("descriptor",array({d.enumerable.value_or(false),d.configurable.value_or(false),d.getter->sameValue(*other.getter),d.setter->sameValue(*other.setter)}));
        if(r.get("set").toBoolean()){
            JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"assign.get"});return JSValue::function(+[](const JSValue&,const Args& a,const Args& cap)->JSValue{event(cap[0],{"assign.call",normal(arg(a,0))});return {};},{c[0]},"assign",1);},{s},"get",0);d.configurable=true;auto value=first;value.defineProperty("assign",d);n.set(property,decode(r.get("value")));}
        if(r.has("setMethod"))out.set("setNode",normal(call(n,r.get("setMethod").string(),{decode(r.get("value"))})));
        if(r.has("flipMethod"))out.set("flipNode",normal(call(n,r.get("flipMethod").string())));
        out.set("member",normal(call(n,"get",{"field"})));
    }catch(const JSTypeError&){out.set("result","#TypeError");}catch(const std::exception&){out.set("result","#Error");}out.set("events",s.get("events"));ThreeUtils::setConsoleFunction({});return out;
}
JSValue defined(JSValue m,const JSValue& r){auto root=nativeNodeConstructor().construct();root.set("first",nativeConstNodeConstructor().construct({decode(r.get("first"))}));auto child=nativeNodeConstructor().construct();child.set("leaf",nativeConstNodeConstructor().construct({decode(r.get("last"))}));root.set("child",child);return m.get("defined").call({}, {root});}
JSValue proxyConstructor(JSValue m,const JSValue& r){
    auto s=JSValue::object();s.set("events",array());auto target=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{Args values{"target",normal(self)};for(const auto& v:a)values.push_back(normal(v));event(c[0],values);return array(a);},{s},"target",2);auto node=JSValue::object();data(node,"plain",7);s.set("node",node);
    JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"get",self.sameValue(c[0].get("proxy"))});return c[0].get("stored");},{s},"get value",0);d.setter=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{event(c[0],{"set",self.sameValue(c[0].get("node")),normal(arg(a,0))});auto s=c[0];s.set("stored",arg(a,0));return {};},{s},"set value",1);node.defineProperty("value",d);
    data(node,"method",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"method",self.sameValue(c[0].get("proxy"))});return self.get("plain");},{s},"method",0));
    if(r.get("locked").toBoolean()){JSPropertyDescriptor frozen;frozen.value="target";frozen.writable=frozen.configurable=false;target.defineProperty("locked",frozen);data(node,"locked","node");}
    auto proxy=m.get("nodeProxyConstructor").call({}, {target,node});s.set("proxy",proxy);auto out=JSValue::object();auto receiver=JSValue::object();receiver.set("fixtureTag","receiver");
    out.set("call",attempt(proxy,receiver,{1,2}));out.set("name",normal(proxy.get("name")));out.set("plain",normal(proxy.get("plain")));out.set("method",normal(call(proxy,"method")));out.set("set",proxy.reflectSet("value",3,receiver));out.set("value",normal(proxy.get("value")));out.set("nodeValue",normal(node.get("value")));
    if(r.get("locked").toBoolean()){try{out.set("locked",proxy.get("locked"));}catch(const JSTypeError&){out.set("locked","#TypeError");}}
    Args keys;for(const auto& key:proxy.ownPropertyNames())keys.push_back(key);out.set("keys",array(keys));out.set("events",s.get("events"));return out;
}
JSValue chaining(JSValue m,const JSValue& r){
    auto s=JSValue::object();s.set("events",array());s.set("request",r);auto n=nativeNodeConstructor().construct();n.set("fixtureTag","node");s.set("node",n);auto value=JSValue::object();value.set("fixtureTag","element");s.set("element",value);console(s);
    JSPropertyDescriptor stack;stack.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"stack.get"});return decode(c[0].get("request").get("stack"));},{s},"get",0);stack.configurable=true;n.defineProperty("isStackNode",stack);
    for(const auto* key:{"assign","addToStack"}){JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[1]});return JSValue::function(+[](const JSValue& self,const Args& a,const Args& cap)->JSValue{Args values{"call",cap[1],self.sameValue(cap[0].get("node"))};for(const auto& v:a)values.push_back(normal(v));event(cap[0],values);return cap[0].get("element");},{c[0],c[1]},c[1].string(),0);},{s,key},"get",0);d.configurable=true;n.defineProperty(key,d);}
    auto element=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{Args values{"element",self.isUndefined()};for(const auto& v:a)values.push_back(normal(v));event(c[0],values);if(c[0].get("request").get("throwElement").toBoolean())throw JSTypeError("element");return c[0].get("element");},{s},"element",0);
    const auto name="utilityFixture"+r.get("id").toString();auto out=JSValue::object();out.set("add",attempt(m.get("addMethodChaining"),{}, {name,r.get("invalid").toBoolean()?JSValue(1):element}));
    if(r.get("duplicate").toBoolean())out.set("duplicate",attempt(m.get("addMethodChaining"),{}, {name,element}));
    out.set("result",attempt(n.get(name+(r.get("assign").toBoolean()?"Assign":"")),n,{1,2}));out.set("events",s.get("events"));ThreeUtils::setConsoleFunction({});return out;
}
JSValue conversionIterator(JSValue m,const JSValue& r){
    auto s=JSValue::object();s.set("events",array());s.set("request",r);console(s);auto p=JSValue::intrinsicArrayConstructor().get("prototype");const auto key=JSValue::wellKnownSymbol("iterator");const auto original=p.getOwnPropertyDescriptor(key).value();
    JSPropertyDescriptor descriptor=original;descriptor.value=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
        if(!self.get("0").strictEquals(421987))return c[1].call(self);event(c[0],{"iterator"});auto iterator=JSValue::object();iterator.set("position",0);
        data(iterator,"next",JSValue::function(+[](const JSValue& self,const Args&,const Args& cap)->JSValue{const auto i=self.get("position").number();auto iterator=self;iterator.set("position",i+1);event(cap[0],{"next",i});const auto r=cap[0].get("request");if(i==1&&r.get("throwAfter").toBoolean())throw JSTypeError("next");auto out=JSValue::object();out.set("done",i>=2);out.set("value",i==0&&!r.get("firstUndefined").toBoolean()?JSValue(421987):JSValue{});return out;},{c[0]},"next",0));
        data(iterator,"return",JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{event(cap[0],{"return"});if(cap[0].get("request").get("closeThrow").toBoolean())throw JSTypeError("close");return JSValue::object();},{c[0]},"return",0));return iterator;
    },{s,*original.value},"values",0);p.defineProperty(key,descriptor);JSValue value;std::string error;
    try{value=m.get(r.get("method")).call({}, {421987,1});}catch(const JSTypeError&){error="#TypeError";}catch(const std::exception&){error="#Error";}p.defineProperty(key,original);
    auto out=JSValue::object();out.set("result",error.empty()?normal(value):JSValue(error));out.set("events",s.get("events"));ThreeUtils::setConsoleFunction({});return out;
}
JSValue conversionIncludes(JSValue m,const JSValue& r){
    auto s=JSValue::object();s.set("events",array());s.set("request",r);console(s);auto p=JSValue::intrinsicArrayConstructor().get("prototype");const auto original=p.getOwnPropertyDescriptor("includes").value();
    JSPropertyDescriptor d;d.configurable=true;d.enumerable=original.enumerable;
    d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
        if(!self.get("length").strictEquals(4)||!self.get("0").strictEquals("bool")||!self.get("3").strictEquals("uint"))return c[1];
        event(c[0],{"includes.get"});if(c[0].get("request").get("throwGet").toBoolean())throw JSTypeError("get includes");
        if(c[0].get("request").get("nonCallable").toBoolean())return 1;
        return JSValue::function(+[](const JSValue& self,const Args& a,const Args& cap)->JSValue{event(cap[0],{"includes.call",normal(self),normal(array(a))});if(cap[0].get("request").get("throwCall").toBoolean())throw JSTypeError("includes");return decode(cap[0].get("request").get("returnValue"));},{c[0]},"includes",1);
    },{s,*original.value},"get includes",0);p.defineProperty("includes",d);JSValue value;std::string error;
    try{Args a;const auto input=r.get("args");for(unsigned i=0;i<input.length();++i)a.push_back(decode(input.get(std::to_string(i))));value=m.get(r.get("method")).call({},a);}catch(const JSTypeError&){error="#TypeError";}catch(const std::exception&){error="#Error";}p.defineProperty("includes",original);
    auto out=JSValue::object();out.set("result",error.empty()?normal(value):JSValue(error));out.set("events",s.get("events"));ThreeUtils::setConsoleFunction({});return out;
}
JSValue parameterIterator(JSValue m,const JSValue& r){
    auto s=JSValue::object();s.set("events",array());s.set("request",r);console(s);auto C=JSValue::constructor(+[](const JSValue&,const Args& a,const Args& c)->JSValue{Args e{"construct"};for(const auto& v:a)e.push_back(normal(v));event(c[0],e);auto o=JSValue::object();o.set("fixtureTag","constructed");return o;},{s},"Fixture",0);C.set("type","Fixture");auto f=m.get("nodeProxy").call({}, {C});call(f,"setParameterLength",{4});
    auto p=JSValue::intrinsicArrayConstructor().get("prototype");const auto key=JSValue::wellKnownSymbol("iterator");const auto original=p.getOwnPropertyDescriptor(key).value();JSPropertyDescriptor d;d.configurable=true;d.enumerable=original.enumerable;
    d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
        if(!self.get("0").strictEquals(671431)||!self.get("1").strictEquals(671432))return c[1];event(c[0],{"iterator.get"});
        return JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{
            event(cap[0],{"iterator.call"});auto iterator=JSValue::object();iterator.set("position",0);
            data(iterator,"next",JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
                const auto i=self.get("position").number();auto iterator=self;iterator.set("position",i+1);event(c[0],{"next",i});const auto request=c[0].get("request");if(request.get("throwNext").strictEquals(i))throw JSTypeError("next");auto result=JSValue::object();
                JSPropertyDescriptor done;done.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{event(cap[0],{"done",cap[1]});return cap[1].number()>=cap[0].get("request").get("doneAt").toNumber();},{c[0],i},"get done",0);done.configurable=true;result.defineProperty("done",done);
                JSPropertyDescriptor value;value.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{event(cap[0],{"value",cap[1]});if(cap[0].get("request").get("throwValue").strictEquals(cap[1]))throw JSTypeError("value");return cap[1].number()+1;},{c[0],i},"get value",0);value.configurable=true;result.defineProperty("value",value);return result;
            },{cap[0]},"next",0));
            JSPropertyDescriptor close;close.configurable=true;close.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"return.get"});if(c[0].get("request").get("throwReturnGet").toBoolean())throw JSTypeError("close get");return JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue{event(cap[0],{"return.call"});if(cap[0].get("request").get("throwReturnCall").toBoolean())throw JSTypeError("close call");return cap[0].get("request").get("primitiveReturn").toBoolean()?JSValue(1):JSValue::object();},{c[0]},"return",0);},{cap[0]},"get return",0);iterator.defineProperty("return",close);return iterator;
        },{c[0]},"values",0);
    },{s,*original.value},"get iterator",0);p.defineProperty(key,d);auto out=JSValue::object();
    try{out.set("setIdentity",call(f,"setParameterLength",{671431,671432}).sameValue(f));}catch(const JSTypeError&){out.set("set","#TypeError");}catch(const std::exception&){out.set("set","#Error");}p.defineProperty(key,original);
    Args args;for(unsigned i=0;i<r.get("count").toNumber();++i)args.push_back(i+1);out.set("call",attempt(f,{},args));out.set("events",s.get("events"));ThreeUtils::setConsoleFunction({});return out;
}
JSValue construction(JSValue m,const JSValue& r){
    auto s=JSValue::object();s.set("events",array());s.set("request",r);auto payload=JSValue::object();payload.set("fixtureTag","element");s.set("payload",payload);auto proto=nativeNodeConstructor().get("prototype");const auto target=r.get("target");const auto name="constructionFixture"+r.get("id").toString();
    const auto element=JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{Args values{"element",self.isUndefined()};for(const auto& value:args)values.push_back(normal(value));event(c[0],values);return c[0].get("payload");},{s},"element",0);JSValue f;Args args;
    if(target.strictEquals("addMethodChaining")){f=m.get("addMethodChaining");args={name,r.get("invalid").toBoolean()?JSValue(1):element};}
    else if(target.strictEquals("defined")){f=m.get("defined");auto value=decode(r.get("value"));if(r.get("traverse").toBoolean()){value=nativeNodeConstructor().construct();value.set("fixtureTag","input");value.set("child",nativeConstNodeConstructor().construct({false}));const auto original=value.get("traverse");value.set("traverse",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{event(c[0],{"traverse",self.sameValue(c[1])});if(c[0].get("request").get("throwTraverse").toBoolean())throw JSTypeError("traverse");return c[2].call(self,args);},{s,value,original},"",0));}args={value};}
    else if(target.strictEquals("chained")||target.strictEquals("chainedAssign")){m.get("addMethodChaining").call({}, {name,element});f=proto.get(name+(target.strictEquals("chainedAssign")?"Assign":""));args={1,2};}
    else if(target.toString().starts_with("getter.")||target.toString().starts_with("setter.")){const auto text=target.toString();const auto d=proto.getOwnPropertyDescriptor(text.substr(7)).value();f=text.starts_with("getter.")?*d.getter:*d.setter;args={1};}
    else if(target.strictEquals("float")||target.strictEquals("nodeProxy")){f=m.get(target);args={1};}
    else{f=proto.get(target);if(target.strictEquals("get"))args={"field"};else if(target.toString().starts_with("set"))args={1};}
    auto out=JSValue::object();Args ownNames;for(const auto& key:f.ownPropertyNames())ownNames.push_back(key);const auto d=f.getOwnPropertyDescriptor("prototype");out.set("metadata",array({f.get("name"),f.get("length"),f.isConstructible(),array(ownNames),d?array({d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false),d->value->get("constructor").sameValue(f)}):JSValue(nullptr)}));
    if(target.strictEquals("setRGB"))out.set("aliasSame",f.sameValue(proto.get("setXYZ"))&&f.get("prototype").sameValue(proto.get("setSTP").get("prototype")));if(target.strictEquals("flipRG"))out.set("aliasSame",f.sameValue(proto.get("flipXY"))&&f.get("prototype").sameValue(proto.get("flipST").get("prototype")));
    const auto previous=call(m,"getCurrentStack");auto stack=JSValue(nullptr);if(r.get("stack").toBoolean()){stack=JSValue::object();stack.set("assign",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{Args values{"stack.assign"};for(const auto& value:args)values.push_back(normal(value));event(c[0],values);return {};},{s},"assign",0));}call(m,"setCurrentStack",{stack});console(s);
    auto targetPrototype=f.get("prototype"),newTarget=f;if(r.get("custom").toBoolean()){targetPrototype=JSValue::object();targetPrototype.set("marker",1);newTarget=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};},{},"NewTarget",0,JSValue::ConstructorKind::Callable);newTarget.set("prototype",targetPrototype);}
    try{const auto value=f.construct(args,newTarget);out.set("result",normal(value));out.set("resultPrototypeMatches",value.getPrototype().sameValue(targetPrototype));Args names;for(const auto& key:value.ownPropertyNames())names.push_back(key);out.set("ownNames",array(names));auto receiver=value.get("structNode");if(!receiver.toBoolean())receiver=value.get("sourceNode");if(!receiver.toBoolean())receiver=value.get("node");if(receiver.isObject())out.set("receiverPrototypeMatches",receiver.getPrototype().sameValue(targetPrototype));}catch(const JSTypeError&){out.set("result","#TypeError");}catch(const std::exception&){out.set("result","#Error");}call(m,"setCurrentStack",{previous});ThreeUtils::setConsoleFunction({});out.set("events",s.get("events"));return out;
}
}
int main(){std::string line;while(std::getline(std::cin,line)){try{auto r=Animation_detail::parseJSON(line);auto m=nativeTSLCore();const auto kind=r.get("kind");JSValue out;if(kind.strictEquals("conversion"))out=conversion(m,r);else if(kind.strictEquals("collection"))out=collection(m,r);else if(kind.strictEquals("proxy"))out=proxyFactory(m,r);else if(kind.strictEquals("swizzle"))out=swizzle(m,r);else if(kind.strictEquals("defined"))out=defined(m,r);else if(kind.strictEquals("proxyConstructor"))out=proxyConstructor(m,r);else if(kind.strictEquals("chaining"))out=chaining(m,r);else if(kind.strictEquals("conversionIterator"))out=conversionIterator(m,r);else if(kind.strictEquals("conversionIncludes"))out=conversionIncludes(m,r);else if(kind.strictEquals("parameterIterator"))out=parameterIterator(m,r);else if(kind.strictEquals("construction"))out=construction(m,r);else throw JSTypeError("kind");std::cout<<out.toJSON()->dump()<<'\n';}catch(const std::exception& e){std::cout<<"{\"probeError\":"<<JsonValue(e.what()).dump()<<"}\n";}}}
