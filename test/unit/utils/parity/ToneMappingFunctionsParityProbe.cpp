#include "animation/AnimationUtils.h"
#include "nodes/display/ToneMappingFunctions.h"
#include "nodes/core/Node.h"
#include "nodes/core/StackNode.h"
#include "nodes/tsl/TSLBase.h"
#include <cmath>
#include <functional>
#include <iostream>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,std::size_t i){return i<a.size()?a[i]:JSValue{};}
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue call(const JSValue& o,const char* k,const Args& a={}){return o.get(k).call(o,a);}
JSValue array(const Args& a={}){auto o=JSValue::array();for(const auto& v:a)call(o,"push",{v});return o;}
Args values(const JSValue& a){Args out;for(unsigned i=0;i<a.length();++i)out.push_back(a.get(std::to_string(i)));return out;}
class Graph {
    bool relative_;double baseline_;Args seen_;JSValue objects_=array();
    JSValue encode(const JSValue& v){
        if(v.isUndefined())return array({"undefined"});if(v.isCallable())return array({"function"});if(v.isSymbol())return array({"symbol"});
        if(v.type()==JSValue::Type::Number){if(std::isnan(v.number()))return array({"number","NaN"});if(!std::isfinite(v.number()))return array({"number",v.number()>0?"Infinity":"-Infinity"});if(v.number()==0&&std::signbit(v.number()))return array({"number","-0"});}if(!v.isObject())return v;
        for(std::size_t i=0;i<seen_.size();++i)if(v.sameValue(seen_[i]))return array({"ref",i});const auto index=seen_.size();seen_.push_back(v);auto record=JSValue::object(),props=array();record.set("kind",v.type()==JSValue::Type::Array?"array":"object");record.set("properties",props);call(objects_,"push",{record});if(v.type()==JSValue::Type::Array)record.set("length",v.length());const bool node=v.get("isNode").strictEquals(true);if(node)record.set("nodeType",v.get("type"));
        for(const auto& k:v.keys()){auto field=relative_&&node&&k=="id"?(v.get("id").toNumber()>baseline_?array({"localId",v.get("id").toNumber()-baseline_}):array({"existingNode",index})):encode(v.get(k));call(props,"push",{array({k,field})});}return array({"ref",index});
    }
  public:Graph(bool relative,double baseline):relative_(relative),baseline_(baseline){}JSValue operator()(const JSValue& root){auto o=JSValue::object();o.set("root",encode(root));o.set("objects",objects_);return o;}
};
JSValue evaluate(const JSValue& r,JSValue shaders){
    const auto before=nativeNodeConstructor().construct(),previous=nativeTSLBaseBinding("getCurrentStack").call();auto stack=nativeStackNodeConstructor().construct();nativeTSLBaseBinding("setCurrentStack").call({}, {stack});auto out=JSValue::object();
    try{
        if(r.get("kind").strictEquals("metadata")){
            auto exports=array();const auto m=nativeToneMappingFunctions();for(const auto& k:m.ownPropertyNames()){const auto fn=m.get(k);call(exports,"push",{array({k,fn.get("name"),fn.get("length"),fn.isConstructible(),fn.get("isFn"),fn.get("shaderNode")})});}out.set("exports",exports);
        }else{
            const auto name=r.get("name");const bool isPrivate=name.strictEquals("RRTAndODTFit")||name.strictEquals("agxDefaultContrastApprox");const auto shader=isPrivate?shaders.get(name):nativeToneMappingFunctionsBinding(name.string()).get("shaderNode");
            auto color=nativeTSLBaseBinding("vec3").call({},values(r.get("color"))),exposure=nativeTSLBaseBinding("float").call({}, {r.get("exposure")});auto inputs=array({color,exposure});if(r.get("inputs").strictEquals("null"))inputs=nullptr;if(r.get("inputs").strictEquals("number"))inputs=5;if(r.get("inputs").strictEquals("empty"))inputs=array();
            const auto result=shader.get("jsFunc").call({}, {inputs});out.set("shader",shader);out.set("color",color);out.set("exposure",exposure);out.set("result",result);
            if(r.get("expand").toBoolean()){
                Args seen,queue,callbacks;auto calls=array();std::function<void(const JSValue&)> walk=[&](const JSValue& v){if(!v.isObject()||v.isCallable())return;for(const auto& x:seen)if(x.sameValue(v))return;seen.push_back(v);if(v.get("isShaderCallNodeInternal").strictEquals(true))queue.push_back(v);if(v.hasOwn("jsFunc")&&v.get("layout").isNull())callbacks.push_back(v);for(const auto& k:v.keys())walk(v.get(k));};walk(result);walk(stack.get("nodes"));
                for(std::size_t i=0;i<queue.size();++i){if(i>=12)throw std::logic_error("Unexpected recursive private graph");const auto callNode=queue[i],fn=callNode.get("shaderNode"),inputName=fn.get("layout").get("inputs").get("0").get("name");const auto privateName=inputName.strictEquals("x")?JSValue("agxDefaultContrastApprox"):JSValue("RRTAndODTFit");shaders.set(privateName,fn);const auto inputs=callNode.get("rawInputs"),result=fn.get("jsFunc").call({}, {inputs});call(calls,"push",{array({privateName,fn,inputs,result})});walk(result);walk(stack.get("nodes"));}
                for(const auto& callback:callbacks)call(calls,"push",{array({"neutralIf",callback,callback.get("jsFunc").call({}, {array()})})});out.set("calls",calls);
            }
        }
    }catch(const JSTypeError&){out.set("error","#TypeError");}catch(const JSRangeError&){out.set("error","#RangeError");}catch(const std::exception& e){out.set("error","#Error");out.set("nativeError",e.what());}
    out.set("stack",stack.get("nodes"));nativeTSLBaseBinding("setCurrentStack").call({}, {previous});const auto after=nativeNodeConstructor().construct();out.set("allocations",after.get("id").toNumber()-before.get("id").toNumber()-1);out.set("stackRestored",nativeTSLBaseBinding("getCurrentStack").call().sameValue(previous));return Graph(r.get("relativeNodeIds").toBoolean(),before.get("id").toNumber())(out);
}
}
int main(){try{const std::string input((std::istreambuf_iterator<char>(std::cin)),{});const auto requests=Animation_detail::parseJSON(input);nativeToneMappingFunctions();auto shaders=JSValue::object(nullptr),out=array();for(unsigned i=0;i<requests.length();++i)call(out,"push",{evaluate(requests.get(std::to_string(i)),shaders)});std::cout<<out.toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
