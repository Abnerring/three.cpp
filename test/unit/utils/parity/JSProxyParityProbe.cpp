#include "animation/AnimationUtils.h"
#include "core/JSProxy.h"
#include "core/JSIterator.h"
#include "core/NativeArray.h"
#include <cmath>
#include <iostream>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const std::string& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
JSValue array(const Args& values={}){auto out=JSValue::array();for(const auto& v:values)own(out,std::to_string(out.length()),v);return out;}
void event(JSValue s,const Args& values){auto events=s.get("events");events.get("push").call(events,{array(values)});}
JSValue normal(const JSValue& v,const JSValue& s){
    if(v.isUndefined())return "#undefined";if(v.isSymbol())return "#symbol";
    if(v.type()==JSValue::Type::Number&&v.toNumber()==0&&std::signbit(v.toNumber()))return "#-0";
    for(const auto* name:{"proxy","target","handler","receiver","proto","otherProto","newTarget"})if(v.isObject()&&v.sameValue(s.get(name)))return "@"+std::string(name);
    if(v.isCallable())return "#function";if(!v.isObject())return v;
    auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();
    if(v.type()==JSValue::Type::Array)for(std::uint32_t i=0;i<v.length();++i)out.set(std::to_string(i),normal(v.get(std::to_string(i)),s));
    else for(const auto& key:v.keys())own(out,key,normal(v.get(key),s));return out;
}
JSValue decode(const JSValue& v,JSValue s){
    if(!v.isObject())return v;const auto tag=v.get("tag");
    if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("negativeZero"))return -0.0;if(tag.strictEquals("symbol"))return s.get("symbol");
    if(tag.strictEquals("target"))return s.get("target");if(tag.strictEquals("proto"))return s.get("proto");if(tag.strictEquals("otherProto"))return s.get("otherProto");
    if(tag.strictEquals("getter"))return JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return 19;});
    if(tag.strictEquals("badKeys"))return array({"fixed","fixed",3});
    return v;
}
JSPropertyDescriptor descriptor(const JSValue& object,JSValue s){JSPropertyDescriptor d;
    if(object.has("value"))d.value=decode(object.get("value"),s);if(object.has("writable"))d.writable=object.get("writable").toBoolean();
    if(object.has("enumerable"))d.enumerable=object.get("enumerable").toBoolean();if(object.has("configurable"))d.configurable=object.get("configurable").toBoolean();
    if(object.has("get"))d.getter=decode(object.get("get"),s);if(object.has("set"))d.setter=decode(object.get("set"),s);return d;
}
JSValue descriptorObject(const std::optional<JSPropertyDescriptor>& maybe,JSValue s){if(!maybe)return "#undefined";auto out=JSValue::object();const auto d=*maybe;
    if(d.value)own(out,"value",normal(*d.value,s));if(d.writable)own(out,"writable",*d.writable);if(d.getter)own(out,"get",normal(*d.getter,s));if(d.setter)own(out,"set",normal(*d.setter,s));
    if(d.enumerable)own(out,"enumerable",*d.enumerable);if(d.configurable)own(out,"configurable",*d.configurable);return out;
}
JSValue trap(const JSValue& self,const Args& args,const Args& c){auto s=c[0];const auto name=c[1].string();const auto r=s.get("r");Args trace{"trap",name,self.sameValue(s.get("handler"))};for(const auto& v:args)trace.push_back(normal(v,s));event(s,trace);
    if(r.get("mode").strictEquals("throw"))throw JSTypeError("trap");
    if(r.get("mode").strictEquals("reenter")&&!s.get("reentered").toBoolean()){
        s.set("reentered",true);auto nested=s.get("proxy").get("loose");event(s,{"nested",normal(nested,s)});
    }
    if(r.get("mode").strictEquals("revokeInTrap"))revokeNativeProxy(s.get("proxy"));
    if(r.get("mutateTarget").toBoolean()){
        auto target=s.get("target");if(name=="preventExtensions")target.preventExtensions();else if(name=="defineProperty")target.defineOwnProperty(arg(args,1),descriptor(arg(args,2),s));else if(name=="deleteProperty")target.erase(arg(args,1));
    }
    if(r.get("mode").strictEquals("forward")){
        auto target=arg(args,0);const auto key=arg(args,1);
        if(name=="get")return target.get(key,arg(args,2));if(name=="set")return target.reflectSet(key,arg(args,2),arg(args,3));
        if(name=="has")return target.has(key);if(name=="deleteProperty")return target.erase(key);
        if(name=="getOwnPropertyDescriptor"){
            const auto d=target.getOwnPropertyDescriptor(key);if(!d)return {};auto out=JSValue::object();if(d->value)out.set("value",*d->value);if(d->writable)out.set("writable",*d->writable);if(d->getter)out.set("get",*d->getter);if(d->setter)out.set("set",*d->setter);if(d->enumerable)out.set("enumerable",*d->enumerable);if(d->configurable)out.set("configurable",*d->configurable);return out;
        }
        if(name=="defineProperty")return target.defineOwnProperty(key,descriptor(arg(args,2),s));
        if(name=="ownKeys")return array(target.ownPropertyKeys());if(name=="getPrototypeOf")return target.getPrototype();if(name=="setPrototypeOf")return target.setPrototype(key);if(name=="isExtensible")return target.isExtensible();if(name=="preventExtensions"){target.preventExtensions();return true;}
        if(name=="apply"){Args values;auto a=arg(args,2);for(std::uint32_t i=0;i<a.length();++i)values.push_back(a.get(std::to_string(i)));return target.call(arg(args,1),values);}
        if(name=="construct"){Args values;auto a=arg(args,1);for(std::uint32_t i=0;i<a.length();++i)values.push_back(a.get(std::to_string(i)));return target.construct(values,arg(args,2));}
    }
    if(r.get("mode").strictEquals("keysObject")){
        auto keys=JSValue::object();keys.set("length",3);
        for(int i=0;i<3;++i){JSPropertyDescriptor d;d.enumerable=d.configurable=true;d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"key",c[1]});if(c[1].strictEquals(2)&&c[0].get("r").get("lastKeyThrows").toBoolean())throw JSTypeError("last key");return c[1].strictEquals(2)?JSValue("loose"):JSValue("fixed");},{s,i});keys.defineProperty(std::to_string(i),d);}return keys;
    }
    if(r.get("mode").strictEquals("descriptorObject")){
        auto result=JSValue::object();const auto values=r.get("result");
        for(const auto* key:{"enumerable","configurable","value","writable","get","set"})if(values.has(key)){
            JSPropertyDescriptor d;d.enumerable=d.configurable=true;d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"descriptorGet",c[1]});return decode(c[2],c[0]);},{s,key,values.get(key)});result.defineProperty(key,d);
        }return result;
    }
    return decode(r.get("result"),s);
}
JSValue operation(JSValue s){auto r=s.get("r"),proxy=s.get("proxy");const auto name=r.get("operation").string();const auto key=r.get("symbol").toBoolean()?s.get("symbol"):JSValue(r.has("key")?r.get("key"):JSValue("fixed"));const auto value=decode(r.has("value")?r.get("value"):JSValue(7),s);
    if(name=="get")return normal(proxy.get(key,r.get("receiver").toBoolean()?s.get("receiver"):proxy),s);
    if(name=="set")return proxy.reflectSet(key,value,r.get("receiver").toBoolean()?s.get("receiver"):proxy);
    if(name=="setStrict"){proxy.set(key,value);return true;}
    if(name=="has")return proxy.has(key);if(name=="deleteProperty")return proxy.erase(key);if(name=="getOwnPropertyDescriptor")return descriptorObject(proxy.getOwnPropertyDescriptor(key),s);
    if(name=="defineProperty")return proxy.defineOwnProperty(key,descriptor(r.get("descriptor"),s));
    if(name=="ownKeys"){auto out=array();for(const auto& k:proxy.ownPropertyKeys()){auto a=out.get("push");a.call(out,{normal(k,s)});}return out;}
    if(name=="getPrototypeOf")return normal(proxy.getPrototype(),s);if(name=="setPrototypeOf")return proxy.setPrototype(r.get("sameProto").toBoolean()?s.get("proto"):s.get("otherProto"));if(name=="isExtensible")return proxy.isExtensible();if(name=="preventExtensions"){proxy.preventExtensions();return true;}if(name=="reflectPreventExtensions")return JSProxyDetail::preventExtensions(proxy);
    if(name=="apply")return normal(proxy.call(s.get("receiver"),{2,3}),s);if(name=="construct")return normal(proxy.construct({2,3},r.get("newTarget").toBoolean()?s.get("newTarget"):proxy),s);
    if(name=="isArray")return isNativeArray(proxy);throw JSTypeError("unknown operation");
}
JSValue arrayMethods(const JSValue& r){
    auto s=JSValue::object();s.set("r",r);s.set("events",array());auto receiver=JSValue::object();s.set("receiver",receiver);
    auto proto=JSValue::object();s.set("proto",proto);auto target=r.get("shape").strictEquals("array")?JSValue::array(4):JSValue::object(proto);s.set("target",target);
    target.set("0",1);target.set("2",3);
    if(target.type()!=JSValue::Type::Array){JSPropertyDescriptor d;d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"length"});if(c[0].get("r").get("throwLength").toBoolean())throw JSTypeError("length");return c[0].get("r").has("length")?c[0].get("r").get("length"):JSValue(4);},{s});d.configurable=true;target.defineProperty("length",d);}
    if(r.get("inherit").toBoolean()){proto.set("1",2);if(target.type()==JSValue::Type::Array)target.setPrototype(proto);}
    if(r.get("readonly").toBoolean()){JSPropertyDescriptor d;d.value=3;d.writable=false;d.enumerable=d.configurable=true;target.defineProperty("2",d);}
    if(r.get("empty").toBoolean()){target.erase("0");target.erase("2");}
    auto actual=target;if(r.get("shape").strictEquals("string"))actual="abc";if(r.get("shape").strictEquals("null"))actual=nullptr;if(r.get("shape").strictEquals("undefined"))actual={};if(r.get("shape").strictEquals("number"))actual=3;
    const auto method=JSValue::intrinsicArrayConstructor().get("prototype").get(r.get("method"));
    const auto callback=JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{
        auto s=c[0],r=s.get("r");const auto reduce=r.get("method").strictEquals("reduce");const auto index=arg(args,reduce?2:1);Args trace{"callback",normal(self,s)};for(const auto& value:args)trace.push_back(normal(value,s));event(s,trace);
        if(r.get("throwCallback").toBoolean())throw JSTypeError("callback");auto target=s.get("target");
        if(r.get("mutate").toBoolean()){target.erase("2");target.set("3",8);if(target.type()==JSValue::Type::Array)target.set("length",6);target.set("4",99);}
        if(reduce){const auto left=arg(args,0).toPrimitive();const auto right=arg(args,1).toPrimitive();if(left.type()==JSValue::Type::String||right.type()==JSValue::Type::String)return left.toString()+right.toString();return left.toNumber()+right.toNumber();}return !(r.has("stop")&&index.sameValue(r.get("stop")));
    },{s},"callback",4);
    Args args;const auto name=r.get("method");
    if(name.strictEquals("fill")){
        const auto bound=[&](const char* key)->JSValue{if(!r.get("coerce").toBoolean())return r.has(key)?decode(r.get(key),s):JSValue{};auto value=JSValue::object();value.setPrimitiveConverter(JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"bound",c[1],arg(args,0)});if(c[0].get("r").get("throwBound").sameValue(c[1]))throw JSTypeError("bound");return c[2];},{s,key,r.has(key)?decode(r.get(key),s):JSValue{}}));return value;};
        args={9,bound("start"),bound("end")};
    }else{args.push_back(r.get("noncallable").toBoolean()?JSValue(7):callback);if(name.strictEquals("every"))args.push_back(receiver);else if(r.get("initial").toBoolean())args.push_back(10);}
    auto out=JSValue::object();try{out.set("result",normal(method.call(actual,args),s));}catch(const JSTypeError&){out.set("result","#TypeError");}catch(const JSRangeError&){out.set("result","#RangeError");}
    out.set("events",s.get("events"));auto fields=array();for(const auto& key:target.ownPropertyNames())if(key!="length"){
        const auto d=target.getOwnPropertyDescriptor(key).value();auto push=fields.get("push");push.call(fields,{array({key,normal(target.get(key),s),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)})});}
    out.set("fields",fields);out.set("arity",method.get("length"));return out;
}
JSValue ordinarySetPrototype(const JSValue& r){
    auto out=JSValue::object(),s=JSValue::object(),target=JSValue::object(nullptr);s.set("events",array());
    const auto scenario=r.get("scenario");
    if(scenario.strictEquals("ordinaryCycle")){
        auto child=JSValue::object(target);
        out.set("result",target.setPrototype(child));out.set("selfCycle",target.setPrototype(target));
        out.set("parentUnchanged",target.getPrototype().isNull());out.set("childBacklink",child.getPrototype().sameValue(target));
        out.set("cleanup",child.setPrototype(nullptr));
    }else{
        auto protoTarget=JSValue::object(scenario.strictEquals("proxyCycle")?target:JSValue(nullptr)),handler=JSValue::object();
        if(scenario.strictEquals("proxyTrapThrows"))handler.set("getPrototypeOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"getPrototypeOf"});throw JSTypeError("prototype trap must not be called");},{s}));
        const auto proxy=createNativeProxy(protoTarget,handler);
        try{out.set("result",target.setPrototype(proxy));}catch(const JSTypeError&){out.set("result","#TypeError");}
        out.set("linked",target.getPrototype().sameValue(proxy));
        // Break the indirect cycle before reading through the Proxy or encoding
        // any graph. The ordinary identity check above never traverses it.
        out.set("cleanup",target.setPrototype(nullptr));
        if(scenario.strictEquals("proxyCycle")){
            out.set("targetBacklink",protoTarget.getPrototype().sameValue(target));
            out.set("proxyBacklink",proxy.getPrototype().sameValue(target));
        }
    }
    out.set("events",s.get("events"));return out;
}
JSValue typedArrayLookup(const JSValue& r){
    auto s=JSValue::object(),proto=JSValue::object(nullptr);s.set("events",array());
    const auto mode=r.get("mode");
    auto target=mode.strictEquals("spoof")?JSValue::object():NativeArray_detail::constructor(std::size_t(r.get("arrayKind").toNumber())).construct({array({17})});
    if(mode.strictEquals("spoof")){target.set("isTypedArray",true);target.set("type","Uint8Array");}
    s.set("target",target);target.setPrototype(proto);
    const std::vector<std::string> keys{"0","1","2","-0","NaN","Infinity","-Infinity","-1","0.5","9007199254740992","1e+21","1e-7","01","+0","1.0","1e0","1e21","0.0000001"," 1 ","","foo"};
    for(const auto& key:keys){JSPropertyDescriptor d;d.enumerable=d.configurable=true;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"inheritedGet",c[1],normal(self,c[0])});return "inherited:"+c[1].string();},{s,key});proto.defineProperty(key,d);}
    auto actual=target;
    if(mode.strictEquals("proxy")||mode.strictEquals("forward")){
        auto handler=JSValue::object();
        if(mode.strictEquals("forward")){
            handler.set("get",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"forwardGet",arg(args,1),normal(arg(args,2),c[0])});return arg(args,0).get(arg(args,1),arg(args,2));},{s}));
            handler.set("has",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"forwardHas",arg(args,1)});return arg(args,0).has(arg(args,1));},{s}));
        }
        actual=createNativeProxy(target,handler);s.set("proxy",actual);
    }else if(mode.strictEquals("child")){actual=JSValue::object(target);s.set("receiver",actual);}
    auto rows=array();for(const auto& key:keys){JSValue value;try{value=normal(actual.get(key),s);}catch(const JSTypeError&){value="#TypeError";}rows.get("push").call(rows,{array({key,value,actual.has(key),actual.hasOwn(key)})});}
    auto out=JSValue::object();out.set("rows",rows);out.set("events",s.get("events"));return out;
}
JSValue run(const JSValue& r){
    if(r.get("kind").strictEquals("typedArrayLookup"))return typedArrayLookup(r);
    if(r.get("kind").strictEquals("ordinarySetPrototype"))return ordinarySetPrototype(r);
    if(r.get("kind").strictEquals("arrayMethods"))return arrayMethods(r);
    auto s=JSValue::object();s.set("r",r);s.set("events",array());s.set("symbol",JSValue::symbol("key"));
    auto proto=JSValue::object(),otherProto=JSValue::object(),receiver=JSValue::object();s.set("proto",proto);s.set("otherProto",otherProto);s.set("receiver",receiver);
    auto target=JSValue::object(proto);const auto targetKind=r.get("target");
    if(targetKind.strictEquals("array")){target=JSValue::array();target.setPrototype(proto);target.set("0",1);}
    if(targetKind.strictEquals("function"))target=JSValue::callableBuiltinConstructor([](const JSValue& self,const JSValue& newTarget,const Args& args,const Args& c)->JSValue{
        event(c[0],{"targetCall",!newTarget.isUndefined(),normal(self,c[0]),normal(newTarget,c[0]),array(args)});if(newTarget.isUndefined())return arg(args,0).toNumber()+arg(args,1).toNumber();auto out=JSValue::object();out.set("sum",arg(args,0).toNumber()+arg(args,1).toNumber());return out;
    },{s},"Target",2);
    s.set("target",target);
    const auto fresh=JSValue::constructor([](const JSValue& self,const Args&,const Args&)->JSValue{return self;},{},"NewTarget",0);s.set("newTarget",fresh);
    JSPropertyDescriptor fixed;fixed.value=r.get("negativeZero").toBoolean()?JSValue(-0.0):JSValue(7);fixed.writable=r.get("writable").toBoolean();fixed.enumerable=true;fixed.configurable=r.get("configurable").toBoolean();target.defineProperty("fixed",fixed);
    own(target,"loose",11);own(target,"receiver",19);target.set(s.get("symbol"),13);
    JSPropertyDescriptor accessor;accessor.configurable=r.get("configurable").toBoolean();accessor.enumerable=true;
    if(!r.get("emptyGetter").toBoolean())accessor.getter=JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"targetGet",normal(self,c[0])});return self.get("receiver");},{s});else accessor.getter=JSValue{};
    if(!r.get("emptySetter").toBoolean())accessor.setter=JSValue::function([](const JSValue& self,const Args& args,const Args& c)->JSValue{event(c[0],{"targetSet",normal(self,c[0]),normal(arg(args,0),c[0])});return {};},{s});else accessor.setter=JSValue{};
    target.defineProperty("accessor",accessor);
    if(!r.get("extensible").toBoolean())target.preventExtensions();
    auto handler=JSValue::object();s.set("handler",handler);const auto name=r.get("trap").toString();
    if(!r.get("mode").strictEquals("absent")){
        const auto method=r.get("mode").strictEquals("noncallable")?JSValue(3):r.get("mode").strictEquals("null")?JSValue(nullptr):JSValue::function(trap,{s,name});
        if(r.get("getter").toBoolean()){
            JSPropertyDescriptor d;d.getter=JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{event(c[0],{"trapGet",c[2],self.sameValue(c[0].get("handler"))});if(c[0].get("r").get("getterThrow").toBoolean())throw JSTypeError("get trap");if(c[0].get("r").get("revokeGetter").toBoolean())revokeNativeProxy(c[0].get("proxy"));return c[1];},{s,method,name});d.configurable=true;handler.defineProperty(name,d);
        }else handler.set(name,method);
    }
    auto proxy=createNativeProxy(target,handler);s.set("proxy",proxy);
    if(r.get("nested").toBoolean()){proxy=createNativeProxy(proxy,JSValue::object());s.set("proxy",proxy);}
    if(r.get("revoked").toBoolean())revokeNativeProxy(proxy);
    auto out=JSValue::object();try{out.set("result",operation(s));}catch(const JSTypeError&){out.set("result","#TypeError");}catch(const JSRangeError&){out.set("result","#RangeError");}
    out.set("events",s.get("events"));out.set("targetLoose",normal(target.get("loose"),s));out.set("targetFixed",normal(target.get("fixed"),s));out.set("targetExtensible",target.isExtensible());out.set("receiverFixed",normal(receiver.get("fixed"),s));return out;
}
JSValue gc(){auto keeper=JSValue::object(),out=array();std::vector<JSWeakValue> weak;JSValue live;{
    auto target=JSValue::object(),handler=JSValue::object();auto proxy=createNativeProxy(target,handler);target.set("proxy",proxy);handler.set("proxy",proxy);live=proxy;for(const auto& v:{target,handler,proxy})weak.emplace_back(v);
}const auto snapshot=[&](){auto row=array();for(const auto& v:weak){auto push=row.get("push");push.call(row,{!v.expired()});}auto push=out.get("push");push.call(out,{row});};keeper.collectGarbage();snapshot();live={};keeper.collectGarbage();snapshot();return out;}
}
int main(int argc,char** argv){try{if(argc>1&&std::string(argv[1])=="gc"){std::cout<<gc().toJSON()->dump();return 0;}const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);auto out=array();for(std::uint32_t i=0;i<requests.length();++i){auto push=out.get("push");push.call(out,{run(requests.get(std::to_string(i)))});}std::cout<<out.toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
