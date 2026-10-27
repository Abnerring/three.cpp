#include "nodes/math/OperatorNode.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/StackTrace.h"
#include "nodes/parsers/GLSLNodeParser.h"
#include "nodes/tsl/TSLCore.h"
#include "animation/AnimationUtils.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <iterator>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
JSValue array(const Args& args={}){auto out=JSValue::array();for(const auto& a:args){JSPropertyDescriptor d;d.value=a;d.writable=d.enumerable=d.configurable=true;out.defineProperty(std::to_string(out.length()),d);}return out;}
JSValue call(const JSValue& o,const char* key,const Args& a={}){const auto fn=o.get(key);return fn.call(o,a);}
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue normal(const JSValue& v){
    if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";if(v.isSymbol())return "#symbol";
    if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "#NaN";if(!std::isfinite(n))return n>0?"#Infinity":"#-Infinity";if(n==0&&std::signbit(n))return "#-0";}
    if(!v.isObject())return v;const auto tag=v.get("_tag");if(tag.toBoolean())return "@"+tag.toString();
    if(v.get("isConstNode").toBoolean())return array({"const",normal(v.get("value")),normal(v.get("nodeType"))});
    if(v.get("isOperatorNode").toBoolean())return array({"operator",normal(v.get("op")),normal(v.get("aNode")),normal(v.get("bNode"))});
    if(v.get("isVarNode").toBoolean())return array({"var",normal(v.get("node")),v.get("name"),v.get("readOnly"),v.get("intent")});
    if(v.get("isConvertNode").toBoolean())return array({"convert",normal(v.get("node")),normal(v.get("convertTo"))});
    auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();
    for(const auto& key:v.ownPropertyNames()){const auto d=v.getOwnPropertyDescriptor(key);if(d&&d->enumerable.value_or(false))own(out,key,normal(v.get(key)));}return out;
}
template<class F>JSValue attempt(F body){try{return normal(body());}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return "#Error";}}
void event(JSValue s,const Args& a){call(s.get("events"),"push",{array(a)});}
void observe(JSValue s,JSValue o,const char* key,const std::string& label){const auto v=o.get(key);JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[2]});if(c[0].get("r").get("throwAt").sameValue(c[2]))throw JSTypeError("get");return c[1];},{s,v,label});o.defineProperty(key,d);}
bool vector(const JSValue& t){if(t.type()!=JSValue::Type::String)return false;auto s=t.string();if(s.size()==5&&(s[0]=='i'||s[0]=='u'||s[0]=='b'))s.erase(0,1);return s.size()==4&&s.substr(0,3)=="vec"&&s[3]>='2'&&s[3]<='4';}
bool matrix(const JSValue& t){return t.type()==JSValue::Type::String&&t.string().starts_with("mat");}
bool scalar(const JSValue& t){return t.strictEquals("float")||t.strictEquals("int")||t.strictEquals("uint")||t.strictEquals("bool")||t.strictEquals("half");}
double length(const JSValue& t){if(t.isNull()||t.isUndefined())return 0;if(matrix(t)){const auto n=t.string()[3]-'0';return n*n;}if(vector(t))return t.string().back()-'0';return t.strictEquals("void")?0:1;}
JSValue special(JSValue s,const JSValue& label,const JSValue& text){auto o=JSValue::object();o.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{event(c[0],{"primitive",c[1],arg(a,0)});if(c[0].get("r").get("throwAt").strictEquals("primitive."+c[1].toString()))throw JSTypeError("primitive");return c[2];},{s,label,text}));return o;}
JSValue callback(const JSValue&,const Args& args,const Args& c){
    auto s=c[0];const auto r=s.get("r");const auto name=c[1].string();Args trace{c[1]};for(const auto& a:args)trace.push_back(normal(a));event(s,trace);
    if(r.get("throwAt").sameValue(c[1]))throw JSTypeError("callback");
    if(name=="a.getNodeType")return r.get("typeA");if(name=="b.getNodeType")return r.get("typeB");
    if(name=="a.build")return r.get("snippetObject").toBoolean()?special(s,"a","A"):JSValue("A");if(name=="b.build")return r.get("snippetObject").toBoolean()?special(s,"b","B"):JSValue("B");
    if(name=="getNodeType")return r.has("nodeType")?r.get("nodeType"):JSValue("vec3");
    if(name=="getTypeLength"){const auto value=length(arg(args,0));return r.get("lengthObject").toBoolean()?special(s,"length:"+arg(args,0).toString(),value):JSValue(value);}
    if(name=="getIntegerType")return "integer:"+arg(args,0).toString();if(name=="isMatrix")return matrix(arg(args,0));if(name=="isVector")return r.get("forceVectors").toBoolean()||vector(arg(args,0));if(name=="isScalar")return scalar(arg(args,0));if(name=="isInteger"){const auto type=arg(args,0);return type.strictEquals("int")||type.strictEquals("uint")||(vector(type)&&type.string().size()==5&&(type.string()[0]=='i'||type.string()[0]=='u'));}
    if(name=="getVectorFromMatrix")return "vec"+arg(args,0).string().substr(3,1);if(name=="getPromotedComponentType"){const auto p=r.get("promoted");return p.isNull()||p.isUndefined()?JSValue("float"):p;}
    if(name=="changeComponentType"){const auto a=arg(args,0).toString();return a+":"+arg(args,1).toString();}
    if(name=="getTypeFromLength"){const auto n=arg(args,0);const auto p=arg(args,1);if(n.toNumber()<=1)return p;const auto prefix=p.strictEquals("float")?"":p.strictEquals("bool")?"b":p.strictEquals("uint")?"u":"i";return std::string(prefix)+"vec"+n.toString();}
    if(name=="getFunctionOperator")return r.get("functionOperator").toBoolean()?(r.get("snippetObject").toBoolean()?special(s,"fn","customFn"):JSValue("customFn")):JSValue(nullptr);
    if(name=="getMethod"){const auto a=arg(args,0).toString();return "method:"+a+":"+arg(args,1).toString();}
    if(name=="format")return array(args);return {};
}
JSValue fn(JSValue s,const char* name){return JSValue::function(callback,{s,name});}
JSValue protocol(const JSValue& r){
    auto s=JSValue::object();s.set("r",r);s.set("events",array());
    auto a=JSValue::object(),b=JSValue::object();a.set("_tag","a");b.set("_tag","b");a.set("getNodeType",fn(s,"a.getNodeType"));a.set("build",fn(s,"a.build"));b.set("getNodeType",fn(s,"b.getNodeType"));b.set("build",fn(s,"b.build"));
    auto n=nativeOperatorNodeConstructor().construct({r.get("op"),a,r.get("noB").toBoolean()?JSValue(nullptr):b});n.set("_tag","operator");n.set("getNodeType",fn(s,"getNodeType"));
    auto renderer=JSValue::object();renderer.set("coordinateSystem",r.get("glsl").toBoolean()?2000:2001);auto builder=JSValue::object();builder.set("_tag","builder");builder.set("renderer",renderer);
    for(const auto* name:{"getMethod","getTypeLength","getIntegerType","isMatrix","isVector","isScalar","isInteger","getVectorFromMatrix","getPromotedComponentType","changeComponentType","getTypeFromLength","getFunctionOperator","format"})builder.set(name,fn(s,name));
    if(r.get("observed").toBoolean()){
        for(const auto* key:{"op","aNode","bNode","getNodeType","getOperatorMethod"})observe(s,n,key,std::string("operator.")+key);
        for(const auto* key:{"getNodeType","build"}){observe(s,a,key,std::string("a.")+key);observe(s,b,key,std::string("b.")+key);}
        observe(s,renderer,"coordinateSystem","renderer.coordinateSystem");const auto keys=builder.ownPropertyNames();for(const auto& key:keys)if(key!="_tag")observe(s,builder,key.c_str(),"builder."+key);
    }
    const auto previous=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{Args t{"console"};for(const auto& v:a)t.push_back(normal(v));event(c[0],t);return {};},{s}));
    auto out=JSValue::object();out.set("result",attempt([&]{return call(n,r.get("operation").string().c_str(),{builder,r.get("output")});}));out.set("events",s.get("events"));ThreeUtils::setConsoleFunction(previous);return out;
}
JSValue metadata(){
    const auto ctor=nativeOperatorNodeConstructor(),proto=ctor.get("prototype");auto node=ctor.construct({"+",1,2});node.set("id",100);auto out=JSValue::object(),fields=array(),methods=array();
    for(const auto& k:node.ownPropertyNames()){const auto d=node.getOwnPropertyDescriptor(k).value();call(fields,"push",{array({k,normal(node.get(k)),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)})});}
    for(const auto& k:proto.ownPropertyNames()){const auto d=proto.getOwnPropertyDescriptor(k).value();const auto v=d.value.value_or(JSValue{});call(methods,"push",{array({k,normal(v),v.isCallable()?v.get("name"):JSValue(nullptr),v.isCallable()?v.get("length"):JSValue(nullptr),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false)})});}
    out.set("arity",ctor.get("length"));out.set("name",ctor.get("name"));out.set("parent",ctor.getPrototype().get("name"));out.set("staticType",ctor.get("type"));out.set("own",fields);out.set("methods",methods);return out;
}
Args values(const JSValue& v){Args out;for(std::uint32_t i=0;i<v.length();++i)out.push_back(v.get(std::to_string(i)));return out;}
JSValue graph(const JSValue& v,double base){
    if(!v.isObject()||!v.get("isOperatorNode").toBoolean())return normal(v);
    auto out=JSValue::object();out.set("id",v.get("id").toNumber()-base);out.set("op",normal(v.get("op")));out.set("a",graph(v.get("aNode"),base));out.set("b",graph(v.get("bNode"),base));return out;
}
JSValue serial(const JSValue& r){
    auto s=JSValue::object();s.set("r",r);s.set("events",array());auto n=nativeOperatorNodeConstructor().construct({"+",1,2}),data=JSValue::object();data.set("_tag","data");data.set("op","-");n.set("_tag","node");
    const auto key=r.get("operation").string();auto proto=nativeNodeConstructor().get("prototype");const auto old=proto.getOwnPropertyDescriptor(key).value();
    proto.set(key,JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue{Args trace{"super",self.sameValue(c[1])};for(const auto& v:a)trace.push_back(normal(v));event(c[0],trace);if(c[0].get("r").get("throwAt").strictEquals("super"))throw JSTypeError("super");return {};},{s,n}));
    observe(s,n,"op","node.op");if(r.get("observeData").toBoolean())observe(s,data,"op","data.op");
    const auto result=attempt([&]{return call(n,key.c_str(),{data});});proto.defineProperty(key,old);
    auto out=JSValue::object();out.set("result",result);out.set("events",s.get("events"));out.set("op",attempt([&]{return n.get("op");}));out.set("dataOp",attempt([&]{return data.get("op");}));return out;
}
JSValue factory(const JSValue& r){
    // The JS and native hosts supply the same actual Error.stack-shaped input.
    // Keep the parsed frames and complete diagnostic text in the comparison.
    setNodeStackCaptureFunction(JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return "Error\n    at operatorFactory (OperatorFixture.js:40:2)";}));
    auto events=array();const auto previous=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{Args trace;for(const auto& v:args)trace.push_back(v.isObject()&&v.get("constructor").get("name").strictEquals("StackTrace")?array({"StackTrace",normal(v.get("stack"))}):normal(v));call(c[0],"push",{array(trace)});return {};},{events}));
    const auto module=nativeOperatorNodeModule();auto out=JSValue::object();out.set("result",attempt([&]{return module.get(r.get("name")).call({},values(r.get("args")));}));out.set("events",events);ThreeUtils::setConsoleFunction(previous);setNodeStackCaptureFunction({});return out;
}
JSValue fnBody(const JSValue& r){
    auto s=JSValue::object();s.set("events",array());s.set("r",r);auto a=nativeNodeConstructor().construct({"int"}),temp=JSValue::object();a.set("_tag","a");temp.set("_tag","temp");s.set("a",a);s.set("temp",temp);
    for(const auto* key:{"addAssign","subAssign","toConst"}){
        JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[1]});if(c[0].get("r").get("throwAt").strictEquals("get."+c[1].toString()))throw JSTypeError("get");return JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{Args trace{c[1],self.sameValue(c[0].get("a"))};for(const auto& v:args)trace.push_back(normal(v));event(c[0],trace);if(c[0].get("r").get("throwAt").sameValue(c[1]))throw JSTypeError("call");return c[1].strictEquals("toConst")?c[0].get("temp"):c[0].get("a");},{c[0],c[1]});},{s,key});a.defineProperty(key,d);
    }
    auto inputs=JSValue::object();inputs.set(JSValue::wellKnownSymbol("iterator"),JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        event(c[0],{"iterator"});auto state=JSValue::object();state.set("count",0);auto iterator=JSValue::object();
        iterator.set("next",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"next"});if(c[0].get("r").get("throwAt").strictEquals("next"))throw JSTypeError("next");auto state=c[1];const auto count=state.get("count").toNumber();state.set("count",count+1);auto result=JSValue::object();result.set("done",count!=0);if(count==0)result.set("value",c[0].get("a"));return result;},{c[0],state}));
        JSPropertyDescriptor d;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"getReturn"});if(c[0].get("r").get("throwAt").strictEquals("getReturn"))throw JSTypeError("getReturn");return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"return"});if(c[0].get("r").get("throwAt").strictEquals("return"))throw JSTypeError("return");return JSValue::object();},{c[0]});},{c[0]});iterator.defineProperty("return",d);return iterator;
    },{s}));
    auto out=JSValue::object();out.set("result",attempt([&]{return nativeOperatorNodeModule().get(r.get("name")).get("shaderNode").get("jsFunc").call({}, {inputs});}));out.set("events",s.get("events"));return out;
}
JSValue integration(const JSValue& r){
    auto renderer=JSValue::object(),debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);renderer.set("coordinateSystem",r.get("glsl").toBoolean()?2000:2001);
    auto builder=NodeBuilder(nullptr,renderer,nativeGLSLNodeParserConstructor().construct()).object();builder.set("shaderStage","fragment");
    auto args=values(r.get("args"));if(r.get("variable").toBoolean()){const auto value=nativeTSLCore().get("int").call({}, {args[0]});args={call(value,"toVar",{"inputValue"})};}
    const auto node=nativeOperatorNodeModule().get(r.get("name")).call({},args);auto results=array();
    for(const auto* stage:{"setup","analyze","generate"}){builder.set("buildStage",stage);call(results,"push",{attempt([&]{const auto value=call(node,"build",{builder,r.get("output")});return r.get("variable").toBoolean()&&value.isObject()&&value.get("isStackNode").toBoolean()?JSValue("#stack"):value;})});}
    auto out=JSValue::object();out.set("values",results);out.set("flow",normal(builder.get("flow")));out.set("stackRestored",nativeTSLCore().get("getCurrentStack").call().isNull());return out;
}
}
int main(){try{
    const auto tsl=nativeTSLCore();const auto before=nativeNodeConstructor().construct();const auto module=nativeOperatorNodeModule();const auto after=nativeNodeConstructor().construct();
    auto imports=JSValue::object(),keys=array(),ids=array();for(const auto& k:module.ownPropertyNames())call(keys,"push",{k});
    for(const auto* k:{"incrementBefore","decrementBefore","increment","decrement"}){const auto fn=module.get(k);call(ids,"push",{array({k,fn.get("id").toNumber()-before.get("id").toNumber(),fn.get("shaderNode").get("id").toNumber()-before.get("id").toNumber()})});}
    imports.set("keys",keys);imports.set("nodeCount",after.get("id").toNumber()-before.get("id").toNumber());imports.set("fnIds",ids);
    const std::string text(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(text);auto out=array();
    for(std::uint32_t i=0;i<requests.length();++i){const auto r=requests.get(std::to_string(i)),kind=r.get("kind");JSValue result;
        if(kind.strictEquals("imports"))result=imports;else if(kind.strictEquals("metadata"))result=metadata();else if(kind.strictEquals("construction")){const auto n=nativeOperatorNodeConstructor().construct(values(r.get("args")));result=graph(n,n.get("id").toNumber());}else if(kind.strictEquals("protocol"))result=protocol(r);else if(kind.strictEquals("serial"))result=serial(r);else if(kind.strictEquals("factory"))result=factory(r);else if(kind.strictEquals("fnBody"))result=fnBody(r);else result=integration(r);call(out,"push",{result});}
    std::cout<<out.toJSON()->dump();return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
