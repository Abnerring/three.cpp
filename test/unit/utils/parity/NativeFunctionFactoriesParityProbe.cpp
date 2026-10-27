#include "animation/AnimationUtils.h"
#include "core/JSIterator.h"
#include "nodes/code/CodeNode.h"
#include "nodes/code/FunctionNode.h"
#include "nodes/code/FunctionCallNode.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/parsers/GLSLNodeParser.h"
#include "renderers/webgpu/nodes/WGSLNodeParser.h"
#include "nodes/tsl/TSLCore.h"
#include "utils.h"
#include <iostream>

namespace {
using Args = std::vector<JSValue>;
bool relativeNodeIds=false;
double nodeIdBaseline=0;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue invoke(const JSValue& object, const char* key, const Args& args = {}) { return object.get(key).call(object, args); }
void own(JSValue object, const std::string& key, const JSValue& value) { JSPropertyDescriptor d; d.value=value; d.writable=d.enumerable=d.configurable=true; object.defineProperty(key,d); }
JSValue array(const Args& values = {}) { auto out=JSValue::array(); for(const auto& value:values) own(out,std::to_string(out.length()),value); return out; }
JSValue enc(const JSValue& value) {
    if(value.isUndefined()) return "#undefined"; if(value.isSymbol()) return "#symbol";
    if(value.isCallable()) return "#function"; if(value.isObject()) return "#object"; return value;
}
JSValue normal(const JSValue& value, Args seen = {}) {
    if(!value.isObject()) return enc(value);
    if(value.get("_tag").toBoolean()) return "@"+value.get("_tag").toString();
    if(value.get("isConstNode").strictEquals(true)) return array({"const",normal(value.get("value")),value.get("nodeType")});
    if(value.isCallable()) return "#function";
    for(const auto& prior:seen) if(prior.sameValue(value)) return "#cycle"; seen.push_back(value);
    auto out=value.type()==JSValue::Type::Array ? JSValue::array(value.length()) : JSValue::object();
    if(value.type()==JSValue::Type::Array) for(std::uint32_t i=0;i<value.length();++i) out.set(std::to_string(i),normal(value.get(std::to_string(i)),seen));
    else for(const auto& key:value.keys()) {
        const auto field=value.get(key);
        own(out,key,relativeNodeIds&&key=="id"&&value.get("isNode").strictEquals(true)&&field.type()==JSValue::Type::Number?JSValue(field.number()-nodeIdBaseline):normal(field,seen));
    } return out;
}
JSValue decode(const JSValue& value) { if(value.isObject()&&value.get("tag").strictEquals("undefined")) return {}; if(value.isObject()&&value.get("tag").strictEquals("symbol")) return JSValue::symbol("test"); return value; }
void event(JSValue state, const Args& values) { invoke(state.get("events"),"push",{array(values)}); }
JSValue attempt(const JSValue& object,const char* name,const Args& args={}) {
    try{return normal(invoke(object,name,args));}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::logic_error&){return "#Error";}
}
JSValue descriptor(const JSValue& object,const std::string& key) {
    const auto d=object.getOwnPropertyDescriptor(key).value(); const auto value=d.value.value_or(JSValue{});
    return array({key,d.value?"data":"accessor",enc(value),d.writable.value_or(false),d.enumerable.value_or(false),d.configurable.value_or(false),
        value.isCallable()?value.get("name"):enc(d.getter.value_or(JSValue{})),value.isCallable()?value.get("length"):enc(d.setter.value_or(JSValue{}))});
}
JSValue snapshot(const JSValue& node) {
    auto out=JSValue::object();
    for(const auto* key:{"type","nodeType","code","language","includes","functionNode","parameters","value","isNode","isCodeNode"}) {
        const auto value=node.get(key);
        if(std::string(key)=="functionNode"&&value.toBoolean()) own(out,key,array({value.get("type"),normal(value.get("code")),normal(value.get("language"))}));
        else own(out,key,normal(value));
    }
    return out;
}
JSValue metadata(const JSValue& r) {
    auto ctor=nativeFunctionCallNodeConstructor(),node=ctor.construct(),proto=ctor.get("prototype"),out=JSValue::object();
    auto names=array(),properties=array(),statics=array(),ownFields=array();
    for(const auto& key:proto.ownPropertyNames()) invoke(properties,"push",{descriptor(proto,key)});
    for(const auto& key:ctor.ownPropertyNames()) invoke(statics,"push",{descriptor(ctor,key)});
    for(const auto& key:node.ownPropertyNames()) invoke(ownFields,"push",{array({key,key=="id"?JSValue("#id"):normal(node.get(key)),descriptor(node,key).get("3"),descriptor(node,key).get("4"),descriptor(node,key).get("5")})});
    out.set("own",ownFields);out.set("prototype",properties);out.set("constructor",statics);out.set("parent",proto.getPrototype().get("constructor").get("name"));out.set("staticParent",ctor.getPrototype().get("name"));
    out.set("call",attempt(proto,"constructor"));out.set("parametersDistinct",!node.get("parameters").sameValue(ctor.construct().get("parameters")));
    auto replacement=JSValue::object();out.set("setIdentity",invoke(node,"setParameters",{replacement}).sameValue(node));out.set("getIdentity",invoke(node,"getParameters").sameValue(replacement));return out;
}
JSValue factory(const JSValue& r) {
    const auto module=r.get("module").strictEquals("code")?nativeCodeNodeModule():nativeFunctionNodeModule();
    const auto f=module.get(r.get("factory"));Args args;
    for(std::uint32_t i=0;i<r.get("args").length();++i)args.push_back(decode(r.get("args").get(std::to_string(i))));
    auto out=JSValue::object();try {
        const auto node=f.call({},args);out.set("snapshot",snapshot(node));
        out.set("callable",node.isCallable());out.set("constructible",node.isConstructible());
        if(node.isCallable()) {
            auto fields=array();for(const auto& key:node.ownPropertyNames())invoke(fields,"push",{descriptor(node,key)});out.set("own",fields);
            out.set("prototypeIsFunction",node.getPrototype().sameValue(JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};}).getPrototype()));
            out.set("constructorIsFunctionNode",node.get("constructor").sameValue(nativeFunctionNodeConstructor()));
            auto called=node.call({}, {JSValue::object()});out.set("called",snapshot(called));
            auto marker=JSValue::object();auto mutableNode=node;mutableNode.set("marker",marker);out.set("markerIdentity",node.get("marker").sameValue(marker));
            out.set("targetOwnMarker",node.hasOwn("marker"));out.set("name",node.get("name"));
        }
    }catch(const JSTypeError&){out.set("error","TypeError");}catch(const JSRangeError&){out.set("error","RangeError");}
    return out;
}
JSValue callFactory(const JSValue& r) {
    auto func=nativeFunctionNodeConstructor().construct({"float f(){return 1.;}",array(),"glsl"});
    auto node=nativeNodeConstructor().construct({"float"});node.set("_tag","argument");
    auto map=JSValue::object();map.set("a",2);map.set("b",false);map.set("node",node);
    const auto shape=r.get("shape").string();Args args{func};
    if(shape=="map")args.push_back(map);else if(shape=="node")args.push_back(node);else if(shape=="multiple")args.insert(args.end(),{3,true,node});
    else if(shape=="array")args.push_back(array({1,2}));else if(shape=="null")args.push_back(nullptr);else if(shape=="false")args.push_back(false);else if(shape=="zero")args.push_back(0);else if(shape=="string")args.push_back("ab");
    auto out=JSValue::object();try {
        const auto result=r.get("chained").toBoolean()?func.get("call").call(func,Args(args.begin()+1,args.end())):nativeFunctionCallNodeModule().get("call").call({},args);
        out.set("parameters",normal(result.get("parameters")));out.set("functionIdentity",result.get("functionNode").sameValue(func));out.set("mapIdentity",result.get("parameters").sameValue(map));
        out.set("mapAfter",normal(map));out.set("type",result.get("type"));
    }catch(const JSTypeError&){out.set("error","TypeError");}return out;
}
JSValue integration(const JSValue& r) {
    const auto language=r.get("language"),factory=nativeFunctionNodeModule().get(language.strictEquals("glsl")?"glslFn":"wgslFn");
    const auto source=language.strictEquals("glsl")?JSValue("float sumFn(float left, float right) { return left + right; }"):JSValue("fn sumFn(left: f32, right: f32) -> f32 { return left + right; }");
    auto native=factory.call({}, {source});const auto parser=language.strictEquals("glsl")?nativeGLSLNodeParserConstructor().construct():nativeWGSLNodeParserConstructor().construct();
    auto renderer=JSValue::object();renderer.set("backend",JSValue::object());auto debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);auto builder=nativeNodeBuilderConstructor().construct({nullptr,renderer,parser});invoke(builder,"setShaderStage",{"fragment"});
    auto input=JSValue::object();input.set("isNode",true);input.set("nodeType","float");input.set("_tag","input");
    input.set("build",JSValue::function([](const JSValue&,const Args&,const Args&)->JSValue{return "valueInput";}));
    auto params=array({input,input});const auto call=native.call({}, {params});
    const auto before=normal(call.get("parameters"));auto stages=array();
    // The native parsed function and real Builder generate the declaration;
    // argument nodes are injected shader inputs with explicit snippets.
    invoke(builder,"setBuildStage",{"generate"});
    auto out=JSValue::object();out.set("result",attempt(call,"generate",{builder}));out.set("parameters",before);
    auto codes=array();const auto codeList=builder.get("codes").get("fragment");
    for(std::uint32_t i=0;i<codeList.length();++i){const auto code=codeList.get(std::to_string(i));invoke(codes,"push",{array({code.get("name"),code.get("type"),code.get("code")})});}
    out.set("codes",codes);out.set("source",attempt(builder,"getCodes",{"fragment"}));return out;
}
JSValue fixtureFunction(const JSValue&,const Args& args,const Args& c) {
    auto s=c[0];const auto name=c[1].string();Args trace{name};for(const auto& value:args)trace.push_back(normal(value));event(s,trace);
    if(s.get("r").get("throwAt").strictEquals(name))throw JSTypeError(name);
    if(name=="getInputs")return s.get("inputs");if(name=="getNodeType")return "vec3";if(name=="getMemberType")return "member:"+arg(args,1).toString();
    if(name=="function.build")return "nativeFn";
    if(name=="zero.build")return "zero";
    return name+":"+(args.size()>1?arg(args,1).toString():"implicit");
}
JSValue fixture(JSValue s,const char* name) { return JSValue::function(fixtureFunction,{s,name}); }
void observe(JSValue s,JSValue object,const char* key,const std::string& label) {
    auto value=object.get(key);JSPropertyDescriptor d;d.enumerable=d.configurable=true;
    d.getter=JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"get",c[2]});if(c[0].get("r").get("throwAt").sameValue(c[2]))throw JSTypeError("get");return c[1];},{s,value,label});object.defineProperty(key,d);
}
JSValue protocol(const JSValue& r) {
    auto s=JSValue::object();s.set("r",r);s.set("events",array());
    auto builder=JSValue::object();builder.set("_tag","builder");
    auto fn=JSValue::object();fn.set("_tag","function");
    for(const auto* key:{"getInputs","getNodeType","getMemberType"})fn.set(key,fixture(s,key));fn.set("build",fixture(s,"function.build"));
    auto inputs=array(),parameters=r.get("mode").strictEquals("array")?JSValue::array():JSValue::object();s.set("inputs",inputs);
    const auto count=r.get("count").toNumber();
    for(int i=0;i<count;++i){auto input=JSValue::object();input.set("name","p"+std::to_string(i));input.set("type",r.get("pointer").toNumber()==i?JSValue("pointer"):JSValue("float"));invoke(inputs,"push",{input});if(r.get("observed").toBoolean()){observe(s,input,"name","input"+std::to_string(i)+".name");observe(s,input,"type","input"+std::to_string(i)+".type");}}
    const auto supplied=r.get("supplied").toNumber();
    for(int i=0;i<supplied;++i){auto node=JSValue::object();node.set("_tag","param"+std::to_string(i));node.set("build",fixture(s, ("p"+std::to_string(i)+".build").c_str()));
        if(r.get("observed").toBoolean())observe(s,node,"build","p"+std::to_string(i)+".getbuild");
        if(parameters.type()==JSValue::Type::Array)invoke(parameters,"push",{node});else parameters.set("p"+std::to_string(i),node);}
    if(r.get("iterable").toBoolean()) {
        auto iterable=JSValue::object();iterable.set(JSValue::wellKnownSymbol("iterator"),JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{
            auto state=JSValue::object();state.set("i",0);auto iterator=JSValue::object();
            iterator.set("next",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{auto state=c[1];const auto index=state.get("i").toNumber();state.set("i",index+1);if(c[0].get("r").get("throwAt").strictEquals("next"))throw JSTypeError("next");auto result=JSValue::object();result.set("done",index>=c[0].get("inputs").length());result.set("value",c[0].get("inputs").get(JSValue(index)));return result;},{c[0],state}));
            iterator.set("return",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"closeInputs"});if(c[0].get("r").get("throwClose").toBoolean())throw JSTypeError("close");return JSValue::object();},{c[0]}));return iterator;
        },{s}));fn.set("getInputs",JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{event(c[0],{"getInputs",normal(arg(args,0))});return c[1];},{s,iterable}));
    }
    auto node=nativeFunctionCallNodeConstructor().construct({fn,parameters});node.set("_tag","call");
    if(r.get("observed").toBoolean()){for(const auto* key:{"getInputs","getNodeType","getMemberType","build"})observe(s,fn,key,std::string("fn.")+key);observe(s,node,"functionNode","call.functionNode");observe(s,node,"parameters","call.parameters");}
    auto zero=nativeTSLCore().get("float").call({}, {0});const auto old=zero.getOwnPropertyDescriptor("build");zero.set("build",fixture(s,"zero.build"));
    const auto previousConsole=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function([](const JSValue&,const Args& args,const Args& c)->JSValue{Args trace{"console"};for(const auto& value:args)trace.push_back(normal(value));event(c[0],trace);if(c[0].get("r").get("throwAt").strictEquals("console"))throw JSTypeError("console");return {};},{s}));
    auto out=JSValue::object();out.set("result",attempt(node,r.get("operation").string().c_str(),{builder,"member"}));out.set("events",s.get("events"));out.set("parameters",normal(parameters));
    ThreeUtils::setConsoleFunction(previousConsole);if(old)zero.defineProperty("build",*old);else zero.erase("build");return out;
}
JSValue run(const JSValue& r) { if(r.get("kind").strictEquals("metadata"))return metadata(r);if(r.get("kind").strictEquals("factory"))return factory(r);if(r.get("kind").strictEquals("protocol"))return protocol(r);if(r.get("kind").strictEquals("callFactory"))return callFactory(r);if(r.get("kind").strictEquals("integration"))return integration(r);throw JSTypeError("unknown scenario"); }
}
int main(int argc,char** argv) { try {
    for(int i=1;i<argc;++i)if(std::string(argv[i])=="--relative-node-ids")relativeNodeIds=true;
    const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);nativeNodeBuilderConstructor();nativeFunctionCallNodeModule();
    if(relativeNodeIds) {
        nativeCodeNodeModule();nativeFunctionNodeModule();nativeTSLCore();nativeGLSLNodeParserConstructor();nativeWGSLNodeParserConstructor();
        nodeIdBaseline=nativeNodeConstructor().construct().get("id").toNumber();
    }
    auto out=array();for(std::uint32_t i=0;i<requests.length();++i)invoke(out,"push",{run(requests.get(std::to_string(i)))});std::cout<<out.toJSON()->dump();
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;} }
