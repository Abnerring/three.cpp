#include "animation/AnimationUtils.h"
#include "core/JSIterator.h"
#include "core/JSPromise.h"
#include "nodes/core/Node.h"
#include "nodes/utils/ArrayElementNode.h"
#include "nodes/utils/ConvertNode.h"
#include "nodes/utils/JoinNode.h"
#include "nodes/utils/SplitNode.h"
#include "nodes/utils/SetNode.h"
#include "nodes/utils/FlipNode.h"
#include "nodes/utils/MemberNode.h"
#include "utils.h"
#include <iostream>
#include <cmath>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
JSValue call(const JSValue& object,const std::string& key,const Args& args={}){return object.get(key).call(object,args);}
JSValue array(const Args& values={}){auto out=JSValue::array();for(const auto& v:values)call(out,"push",{v});return out;}
void data(JSValue object,const std::string& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
JSValue normal(const JSValue& v,Args seen={}){
    if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";if(v.isSymbol())return "#symbol";
    if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "#NaN";if(std::isinf(n))return n>0?"#Infinity":"#-Infinity";if(n==0&&std::signbit(n))return "#-0";}
    if(!v.isObject())return v;
    if(v.get("fixtureTag").toBoolean())return "@"+v.get("fixtureTag").toString();
    for(const auto& prior:seen)if(prior.sameValue(v))return "#cycle";seen.push_back(v);
    auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();
    for(const auto& key:v.keys())data(out,key,normal(v.get(key),seen));return out;
}
void event(JSValue state,const Args& values){call(state.get("events"),"push",{array(values)});}
JSValue decode(const JSValue& value){if(value.isObject()&&value.get("tag").strictEquals("undefined"))return {};return value;}
JSValue ctor(const std::string& name){
    if(name=="ArrayElementNode")return nativeArrayElementNodeConstructor();if(name=="ConvertNode")return nativeConvertNodeConstructor();if(name=="JoinNode")return nativeJoinNodeConstructor();
    if(name=="SplitNode")return nativeSplitNodeConstructor();if(name=="SetNode")return nativeSetNodeConstructor();if(name=="FlipNode")return nativeFlipNodeConstructor();if(name=="MemberNode")return nativeMemberNodeConstructor();throw JSTypeError("unknown class");
}
JSValue descriptor(const JSValue& object,const std::string& key){
    const auto d=object.getOwnPropertyDescriptor(key).value();const auto value=d.value.value_or(JSValue{});
    return array({key,d.value?"data":"accessor",normal(value),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false),
        value.isCallable()?value.get("name"):normal(d.getter.value_or(JSValue{})),value.isCallable()?value.get("length"):normal(d.setter.value_or(JSValue{}))});
}
JSValue metadata(const JSValue& r){
    auto C=ctor(r.get("class").string());Args args;auto supplied=r.get("args");for(std::uint32_t i=0;i<supplied.length();++i)args.push_back(decode(supplied.get(std::to_string(i))));
    auto n=C.construct(args);n.set("id",100);n.set("_uuid","fixture-uuid");auto out=JSValue::object(),own=array(),constructor=array(),prototype=array();
    for(const auto& key:n.ownPropertyNames())call(own,"push",{array({key,normal(n.get(key))})});
    for(const auto& key:C.ownPropertyNames())call(constructor,"push",{descriptor(C,key)});
    for(const auto& key:C.get("prototype").ownPropertyNames())call(prototype,"push",{descriptor(C.get("prototype"),key)});
    out.set("own",own);out.set("constructor",constructor);out.set("prototype",prototype);out.set("parent",C.get("prototype").getPrototype().get("constructor").get("name"));out.set("staticParent",C.getPrototype().get("name"));
    auto other=C.construct(args);out.set("freshNodes",r.get("class").strictEquals("JoinNode")?!n.get("nodes").sameValue(other.get("nodes")):false);return out;
}
JSValue constructProtocol(const JSValue& r){
    auto state=JSValue::object();state.set("request",r);state.set("events",array());auto values=JSValue::object(),result=JSValue::object();state.set("values",values);state.set("result",result);
    const char* keys[]={"node","indexNode","isArrayElementNode","convertTo","nodes","components","isSplitNode","sourceNode","targetNode","structNode","property","isMemberNode"};
    for(const auto* key:keys){JSPropertyDescriptor d;d.enumerable=d.configurable=true;d.setter=JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
        event(c[0],{"set",c[1],normal(arg(a,0)),self.sameValue(c[0].get("result"))});if(c[0].get("request").get("throwAt").strictEquals(c[1]))throw JSTypeError("setter");auto values=c[0].get("values");data(values,c[1].string(),arg(a,0));return {};
    },{state,key},"set",1);result.defineProperty(key,d);}
    auto C=ctor(r.get("class").string());const auto original=C.getPrototype();state.set("constructor",C);
    auto base=JSValue::builtinConstructor(+[](const JSValue& newTarget,const Args& a,const Args& c)->JSValue{
        Args values{"base",newTarget.sameValue(c[0].get("constructor"))};for(const auto& v:a)values.push_back(normal(v));event(c[0],values);return c[0].get("result");
    },{state},"Base",0);C.setPrototype(base);Args args;auto supplied=r.get("args");for(std::uint32_t i=0;i<supplied.length();++i)args.push_back(decode(supplied.get(std::to_string(i))));
    JSValue output;try{output=C.construct(args).sameValue(result);}catch(const JSTypeError&){output="#TypeError";}C.setPrototype(original);
    auto out=JSValue::object();out.set("result",output);out.set("events",state.get("events"));out.set("values",normal(values));return out;
}
void observe(JSValue state,JSValue object,const char* key,const std::string& label){
    auto value=object.get(key);JSPropertyDescriptor d;d.enumerable=d.configurable=true;
    d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{
        event(c[0],{"get",c[2],self.sameValue(c[3])});if(c[0].get("request").get("throwAt").strictEquals(c[2]))throw JSTypeError(c[2].string());return c[1];
    },{state,value,label,object},"get",0);object.defineProperty(key,d);
}
void fn(JSValue state,JSValue object,const char* key,const std::string& label,JSValue::NativeFunction body,unsigned arity=0){
    auto value=JSValue::function(body,{state,object,label},key,arity);data(object,key,value);observe(state,object,key,label);
}
JSValue fixtureNode(JSValue state,const char* tag,const JSValue& type,const JSValue& snippet){
    auto node=JSValue::object();node.set("fixtureTag",tag);node.set("typeValue",type);node.set("snippet",snippet);
    for(const auto* key:{"getNodeType","getElementType","getMemberType","getScope","build","hasMember"}){
        fn(state,node,key,std::string(tag)+"."+key,+[](const JSValue& self,const Args& args,const Args& c)->JSValue{
            Args values{"call",c[2],self.sameValue(c[1])};for(const auto& a:args)values.push_back(normal(a));event(c[0],values);
            if(c[0].get("request").get("throwAt").strictEquals(c[2].toString()+"()"))throw JSTypeError("call");
            const auto key=c[2].string();if(key.ends_with(".build"))return self.get("snippet");if(key.ends_with(".getScope"))return c[0].get("scope");if(key.ends_with(".hasMember"))return c[0].get("request").get("hasMember");
            if(key.ends_with(".getMemberType"))return c[0].get("request").get("memberType");return self.get("typeValue");
        });
    }
    return node;
}
double width(const JSValue& type){auto t=type.toString();if(t.size()>3&&(t.substr(0,3)=="vec"||t.substr(1,3)=="vec"))return t.back()-'0';if(t=="void")return 0;return 1;}
JSValue protocol(const JSValue& r){
    auto state=JSValue::object();state.set("request",r);state.set("events",array());auto scope=JSValue::object();scope.set("fixtureTag","scope");state.set("scope",scope);
    auto builder=JSValue::object();builder.set("fixtureTag","builder");state.set("builder",builder);
    for(const auto* key:{"getTypeLength","getTypeFromLength","getVectorType","getComponentType","format","getType","isVector","isInteger","getVarFromNode","getPropertyName","addLineFlowCode","getStructTypeNode","generateConst"}){
        fn(state,builder,key,std::string("builder.")+key,+[](const JSValue& self,const Args& a,const Args& c)->JSValue{
            Args values{"call",c[2],self.sameValue(c[1])};for(const auto& v:a)values.push_back(normal(v));event(c[0],values);
            if(c[0].get("request").get("throwAt").strictEquals(c[2].toString()+"()"))throw JSTypeError("builder call");
            const auto key=c[2].string();if(key.ends_with(".getTypeLength"))return width(arg(a,0));
            if(key.ends_with(".getTypeFromLength")){const auto n=arg(a,0).toNumber();const auto component=arg(a,1).isUndefined()?JSValue("float"):arg(a,1);if(n==1)return component;const auto p=component.strictEquals("int")?"i":component.strictEquals("uint")?"u":component.strictEquals("bool")?"b":"";return std::string(p)+"vec"+JSValue(n).toString();}
            if(key.ends_with(".getVectorType")||key.ends_with(".getType"))return arg(a,0);
            if(key.ends_with(".getComponentType")){const auto t=arg(a,0).toString();return t.starts_with("ivec")||t=="int"?JSValue("int"):t.starts_with("uvec")||t=="uint"?JSValue("uint"):t.starts_with("bvec")||t=="bool"?JSValue("bool"):JSValue("float");}
            if(key.ends_with(".format"))return "format("+arg(a,0).toString()+","+arg(a,1).toString()+","+arg(a,2).toString()+")";
            if(key.ends_with(".isVector"))return width(arg(a,0))>1;
            if(key.ends_with(".isInteger"))return arg(a,0).strictEquals("int")||arg(a,0).strictEquals("uint");
            if(key.ends_with(".getVarFromNode"))return c[0].get("cache");if(key.ends_with(".getPropertyName"))return "tmp";
            if(key.ends_with(".getStructTypeNode"))return c[0].get("source");if(key.ends_with(".generateConst"))return "zero("+arg(a,0).toString()+")";return {};
        });
    }
    auto cache=JSValue::object();cache.set("fixtureTag","cache");state.set("cache",cache);
    const auto sourceType=r.has("sourceType")?decode(r.get("sourceType")):JSValue("vec3"),indexType=r.has("indexType")?decode(r.get("indexType")):JSValue("int");
    auto source=fixtureNode(state,"source",sourceType,"SOURCE"),index=fixtureNode(state,"index",indexType,"INDEX"),target=fixtureNode(state,"target",r.has("targetType")?decode(r.get("targetType")):JSValue("float"),"TARGET");state.set("source",source);
    source.set("isMemberNode",r.get("nested"));
    const auto components=r.has("components")?decode(r.get("components")):JSValue("xy");Args args;
    const auto name=r.get("class").string();auto inputs=array({source,index,target});
    if(name=="ArrayElementNode")args={source,index};if(name=="ConvertNode")args={source,r.has("convertTo")?decode(r.get("convertTo")):JSValue("float|vec3|ivec3")};
    if(name=="JoinNode")args={inputs,r.has("nodeType")?decode(r.get("nodeType")):JSValue(nullptr)};
    if(name=="SplitNode")args={source,components};if(name=="SetNode")args={source,components,target};if(name=="FlipNode")args={source,components};if(name=="MemberNode")args={source,r.has("property")?decode(r.get("property")):JSValue("member")};
    auto n=ctor(name).construct(args);n.set("fixtureTag","main");n.set("id",100);n.set("_uuid","fixture-uuid");state.set("main",n);
    data(n,"getNodeType",JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue {Args values{"call","main.getNodeType",self.sameValue(c[1])};for(const auto& v:a)values.push_back(normal(v));event(c[0],values);if(c[0].get("request").get("throwAt").strictEquals("main.getNodeType()"))throw JSTypeError("type");const auto r=c[0].get("request");return r.has("outputType")?decode(r.get("outputType")):JSValue("vec3");},{state,n},"getNodeType",1));
    auto method=r.get("method").string();for(const auto& key:n.keys())if(key!="fixtureTag"&&key!="id"&&key!="_uuid")observe(state,n,key.c_str(),"main."+key);
    if(r.get("iterable").toBoolean()) {
        auto iterable=JSValue::object();iterable.set("fixtureTag","iterable");auto iterator=JSValue::object();iterator.set("position",0);iterator.set("fixtureTag","iterator");
        fn(state,iterator,"next","iterator.next",+[](const JSValue& self,const Args&,const Args& c)->JSValue {
            event(c[0],{"call",c[2],self.sameValue(c[1])});if(c[0].get("request").get("throwAt").strictEquals("iterator.next()"))throw JSTypeError("next");
            const auto i=self.get("position").number();auto receiver=self;receiver.set("position",i+1);auto result=JSValue::object();result.set("done",i>=3);result.set("value",c[0].get("inputs").get(JSValue(i)));return result;
        });
        fn(state,iterator,"return","iterator.return",+[](const JSValue& self,const Args&,const Args& c)->JSValue {event(c[0],{"call",c[2],self.sameValue(c[1])});if(c[0].get("request").get("closeThrow").toBoolean())throw JSTypeError("return");return JSValue::object();});
        iterable.set(JSValue::wellKnownSymbol("iterator"),JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"iterator"});return c[1];},{state,iterator},"[Symbol.iterator]",0));
        state.set("inputs",inputs);n.defineProperty("nodes",JSPropertyDescriptor{.value=iterable,.writable=true,.enumerable=true,.configurable=true});
    }
    ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{Args values{"console"};for(const auto& v:a)values.push_back(normal(v));event(c[0],values);if(c[0].get("request").get("throwAt").strictEquals("console"))throw JSTypeError("console");return {};},{state},"console",0));
    Args callArgs;if(method!="getScope"&&method!="getVectorLength"&&method!="isCacheable")callArgs.push_back(builder);if(method=="generate")callArgs.push_back(r.has("output")?decode(r.get("output")):JSValue("result"));if(method=="getMemberType")callArgs.push_back("child");
    JSValue result;try{result=normal(call(n,method,callArgs));}catch(const JSTypeError&){result="#TypeError";}catch(const JSRangeError&){result="#RangeError";}catch(const JSThrownValue& e){result=normal(e.value);}
    auto out=JSValue::object();out.set("result",result);out.set("events",state.get("events"));ThreeUtils::setConsoleFunction({});return out;
}
}
int main(){std::string line;while(std::getline(std::cin,line)){try{auto r=Animation_detail::parseJSON(line);auto out=r.get("kind").strictEquals("metadata")?metadata(r):r.get("kind").strictEquals("construct")?constructProtocol(r):protocol(r);std::cout<<out.toJSON()->dump()<<'\n';}catch(const std::exception& e){std::cout<<"{\"probeError\":"<<JsonValue(e.what()).dump()<<"}\n";}}}
