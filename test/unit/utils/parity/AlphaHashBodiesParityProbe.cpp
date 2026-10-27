#include "animation/AnimationUtils.h"
#include "core/JSIterator.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/StackNode.h"
#include "nodes/functions/material/getAlphaHashThreshold.h"
#include "nodes/tsl/TSLCore.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <iterator>

namespace {
using Args=std::vector<JSValue>;
void own(JSValue object,const std::string& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
JSValue array(const Args& values={}){auto result=JSValue::array();for(std::size_t i=0;i<values.size();++i)own(result,std::to_string(i),values[i]);return result;}
JSValue call(const JSValue& object,const char* key,const Args& args={}){const auto method=object.get(key);return method.call(object,args);}
void append(const JSValue& object,const JSValue& value){call(object,"push",{value});}
Args values(const JSValue& source){Args result;for(unsigned i=0;i<source.length();++i)result.push_back(source.get(std::to_string(i)));return result;}

class CanonicalGraph {
    bool relative_;
    double baseline_;
    Args seen_;
    JSValue objects_=array();
    JSValue encode(const JSValue& value){
        if(value.isUndefined())return array({"undefined"});if(value.isCallable())return array({"function"});if(value.isSymbol())return array({"symbol"});
        if(value.type()==JSValue::Type::Number){const auto n=value.number();if(std::isnan(n))return array({"number","NaN"});if(!std::isfinite(n))return array({"number",n>0?"Infinity":"-Infinity"});if(n==0&&std::signbit(n))return array({"number","-0"});}
        if(!value.isObject())return value;
        for(std::size_t i=0;i<seen_.size();++i)if(value.sameValue(seen_[i]))return array({"ref",static_cast<double>(i)});
        const auto index=static_cast<double>(seen_.size());seen_.push_back(value);auto record=JSValue::object(),properties=array();
        own(record,"kind",value.type()==JSValue::Type::Array?"array":"object");own(record,"properties",properties);append(objects_,record);
        if(value.type()==JSValue::Type::Array)own(record,"length",value.length());
        const auto isNode=value.get("isNode").strictEquals(true);if(isNode)own(record,"nodeType",value.get("type"));
        for(const auto& key:value.keys()){
            JSValue field;
            if(relative_&&key=="id"&&isNode){const auto id=value.get("id").toNumber();field=id>baseline_?array({"localId",id-baseline_}):array({"existingNode",index});}
            else field=encode(value.get(key));
            append(properties,array({key,field}));
        }
        return array({"ref",index});
    }
public:
    CanonicalGraph(bool relative,double baseline):relative_(relative),baseline_(baseline){}
    JSValue operator()(const JSValue& root){auto out=JSValue::object();own(out,"root",encode(root));own(out,"objects",objects_);return out;}
};

JSValue expand(const JSValue& position,const JSValue& stack,JSValue privateShaders){
    const auto result=nativeAlphaHashThreshold().get("shaderNode").get("jsFunc").call({}, {array({position})});
    Args seen,queue;auto calls=array();
    std::function<void(const JSValue&)> walk=[&](const JSValue& value){
        if(!value.isObject()||value.isCallable())return;for(const auto& old:seen)if(old.sameValue(value))return;seen.push_back(value);
        if(value.get("isShaderCallNodeInternal").strictEquals(true))queue.push_back(value);
        for(const auto& key:value.keys())walk(value.get(key));
    };
    walk(result);walk(stack.get("nodes"));unsigned count2D=0,count3D=0;
    for(std::size_t i=0;i<queue.size();++i){
        if(i>=16)throw std::logic_error("Unexpected recursive private shader graph");
        const auto node=queue[i],shader=node.get("shaderNode"),type=shader.get("layout").get("inputs").get("0").get("type");
        if(!type.strictEquals("vec2")&&!type.strictEquals("vec3"))throw std::logic_error("Unexpected private shader input type");
        const auto name=type.strictEquals("vec2")?"hash2D":"hash3D";if(type.strictEquals("vec2"))++count2D;else ++count3D;privateShaders.set(name,shader);
        const auto inputs=node.get("rawInputs"),output=shader.get("jsFunc").call({}, {inputs});auto row=JSValue::object();
        own(row,"name",name);own(row,"shader",shader);own(row,"inputs",inputs);own(row,"output",output);append(calls,row);walk(output);walk(stack.get("nodes"));
    }
    if(count2D!=4||count3D!=2)throw std::logic_error("Expected two actual hash3D and four actual hash2D body calls");
    auto out=JSValue::object();own(out,"position",position);own(out,"result",result);own(out,"calls",calls);own(out,"stack",stack.get("nodes"));return out;
}

void event(const JSValue& state,const Args& row){append(state.get("events"),array(row));}
JSValue fixture(const JSValue& state,const char* type,const JSValue& tsl){
    auto value=nativeNodeConstructor().construct({type});
    for(const auto* key:{"x","y","z","xy"}){
        JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            const auto key=c[1];event(c[0],{"get","value."+key.string()});if(c[0].get("r").get("throwAt").strictEquals("value."+key.string()))throw JSTypeError("fixture");
            return key.strictEquals("xy")?c[2].get("vec2").call({}, {.25,-.5}):c[2].get("float").call({}, {key.strictEquals("x")?.25:key.strictEquals("y")?-.5:2});
        },{state,key,tsl});value.defineProperty(key,d);
    }
    return value;
}
JSValue inputs(const JSValue& state,const JSValue& value){
    const auto mode=state.get("r").get("iterator");if(mode.strictEquals("array"))return array({value});
    auto iterator=JSValue::object();
    JSPropertyDescriptor next;next.configurable=true;next.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        event(c[0],{"get","iterator.next"});return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
            event(c[0],{"next"});const auto mode=c[0].get("r").get("iterator");if(mode.strictEquals("nextThrow"))throw JSTypeError("next");auto result=JSValue::object();
            for(const auto* key:{"done","value"}){JSPropertyDescriptor d;d.configurable=d.enumerable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
                const auto key=c[2];event(c[0],{"get","result."+key.string()});const auto mode=c[0].get("r").get("iterator");
                if(key.strictEquals("done")){if(mode.strictEquals("doneThrow"))throw JSTypeError("done");return mode.strictEquals("empty");}
                if(mode.strictEquals("valueThrow"))throw JSTypeError("value");return c[1];
            },{c[0],c[1],key});result.defineProperty(key,d);}return result;
        },c);
    },{state,value});iterator.defineProperty("next",next);
    JSPropertyDescriptor close;close.configurable=true;close.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        event(c[0],{"get","iterator.return"});const auto mode=c[0].get("r").get("iterator");if(mode.strictEquals("returnGetterThrow"))throw JSTypeError("return getter");if(mode.strictEquals("returnMissing"))return {};
        return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{"return"});const auto mode=c[0].get("r").get("iterator");if(mode.strictEquals("returnThrow"))throw JSTypeError("return");return mode.strictEquals("returnPrimitive")?JSValue(1):JSValue::object();},{c[0]});
    },{state});iterator.defineProperty("return",close);
    auto iterable=JSValue::object();iterable.set(JSValue::wellKnownSymbol("iterator"),JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{
        event(c[0],{"iterator"});if(c[0].get("r").get("iterator").strictEquals("iteratorCallThrow"))throw JSTypeError("iterator");return c[1];
    },{state,iterator}));return iterable;
}

JSValue evaluate(const JSValue& request,const JSValue& tsl,JSValue privateShaders){
    const auto previous=call(tsl,"getCurrentStack"),stack=nativeStackNodeConstructor().construct();auto state=JSValue::object();own(state,"r",request);own(state,"events",array());call(tsl,"setCurrentStack",{stack});
    const auto before=nativeNodeConstructor().construct();JSValue out;
    try{
        if(request.get("kind").strictEquals("expand"))out=expand(tsl.get("vec3").call({},values(request.get("values"))),stack,privateShaders);
        else{
            const auto shader=privateShaders.get(request.get("name"));if(!shader.isObject())throw std::logic_error("Private body was not discovered from the actual graph");
            const auto type=request.get("name").strictEquals("hash2D")?"vec2":"vec3";
            const auto value=request.get("fixture").toBoolean()?fixture(state,type,tsl):tsl.get(type).call({},values(request.get("values")));
            const auto result=shader.get("jsFunc").call({}, {inputs(state,value)});out=JSValue::object();own(out,"shader",shader);own(out,"value",value);own(out,"result",result);own(out,"stack",stack.get("nodes"));
        }
    }catch(const JSTypeError&){out=JSValue::object();own(out,"error","#TypeError");own(out,"stack",stack.get("nodes"));}
    catch(const JSRangeError&){out=JSValue::object();own(out,"error","#RangeError");own(out,"stack",stack.get("nodes"));}
    catch(const std::exception& e){out=JSValue::object();own(out,"error","#Error");own(out,"nativeError",e.what());own(out,"stack",stack.get("nodes"));}
    call(tsl,"setCurrentStack",{previous});const auto after=nativeNodeConstructor().construct();own(out,"events",state.get("events"));
    own(out,"allocations",after.get("id").toNumber()-before.get("id").toNumber()-1);own(out,"stackRestored",call(tsl,"getCurrentStack").sameValue(previous));
    return CanonicalGraph(request.get("relativeNodeIds").toBoolean(),before.get("id").toNumber())(out);
}
}

int main(){try{
    const std::string source(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(source);
    const auto builder=nativeNodeBuilderConstructor(),tsl=nativeTSLCore(),threshold=nativeAlphaHashThreshold();auto privateShaders=JSValue::object(nullptr),out=array();
    for(unsigned i=0;i<requests.length();++i)append(out,evaluate(requests.get(std::to_string(i)),tsl,privateShaders));
    std::cout<<out.toJSON()->dump();return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
