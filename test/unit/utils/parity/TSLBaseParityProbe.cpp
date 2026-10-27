#include "animation/AnimationUtils.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/core/Node.h"
#include "nodes/core/NodeBuilder.h"
#include "nodes/core/StackNode.h"
#include "nodes/core/ArrayNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/core/AssignNode.h"
#include "nodes/code/FunctionCallNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/math/MathNode.h"
#include "nodes/math/ConditionalNode.h"
#include "nodes/core/ContextNode.h"
#include "nodes/core/VarNode.h"
#include "nodes/core/VaryingNode.h"
#include "nodes/display/ColorSpaceNode.h"
#include "nodes/display/ToneMappingNode.h"
#include "nodes/accessors/BufferAttributeNode.h"
#include "nodes/gpgpu/ComputeNode.h"
#include "nodes/core/IsolateNode.h"
#include "nodes/core/BypassNode.h"
#include "nodes/utils/Remap.h"
#include "nodes/code/ExpressionNode.h"
#include "nodes/utils/Discard.h"
#include "nodes/display/RenderOutputNode.h"
#include "nodes/utils/DebugNode.h"
#include "nodes/core/SubBuildNode.h"
#include "nodes/core/InspectorNode.h"
#include "core/NativeArray.h"
#include "utils.h"
#include <cmath>
#include <iostream>
#include <regex>

namespace {
using Args=std::vector<JSValue>;
void own(JSValue o,const std::string& k,const JSValue& v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue array(const Args& a={}){auto o=JSValue::array();for(const auto& v:a)own(o,std::to_string(o.length()),v);return o;}
JSValue call(const JSValue& o,const char* key,const Args& args={}){return o.get(key).call(o,args);}
bool relativeIds=false;double baseline=0;
JSValue normal(const JSValue& v,std::vector<JSValue>* seen=nullptr){
    if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";if(v.isSymbol())return "#symbol";if(v.type()==JSValue::Type::Number){if(std::isnan(v.number()))return "#NaN";if(std::isinf(v.number()))return v.number()>0?"#Infinity":"#-Infinity";if(v.number()==0&&std::signbit(v.number()))return "#-0";}if(!v.isObject())return v;
    std::vector<JSValue> local;if(!seen)seen=&local;for(unsigned i=0;i<seen->size();++i)if(v.sameValue((*seen)[i]))return array({"#ref",i});seen->push_back(v);auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();
    for(const auto& k:v.keys()){const auto x=v.get(k);if(k=="uuid"&&x.type()==JSValue::Type::String&&std::regex_match(x.string(),std::regex("[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}")))own(out,k,"#valid-random-uuid");else if(relativeIds&&baseline&&k=="id"&&v.get("isNode").strictEquals(true)&&x.type()==JSValue::Type::Number&&x.number()>baseline)own(out,k,x.number()-baseline);else own(out,k,normal(x,seen));}return out;
}
JSValue failure(){try{throw;}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return "#Error";}}
JSValue metadata(const JSValue& value){auto out=JSValue::object();out.set("type",value.isCallable()?"function":value.isUndefined()?"undefined":value.isNull()?"object":value.type()==JSValue::Type::Number?"number":value.type()==JSValue::Type::Boolean?"boolean":value.type()==JSValue::Type::String?"string":"object");
    if(value.isCallable()){out.set("name",value.get("name"));out.set("length",value.get("length"));out.set("constructible",value.isConstructible());auto keys=array();for(const auto& k:value.ownPropertyNames())call(keys,"push",{k});out.set("ownNames",keys);}else out.set("value",normal(value));return out;}
JSValue source(const std::string& s,const std::string& name){
    if(s=="TSLCore")return nativeTSLCoreBinding(name);if(s=="ArrayNode")return nativeArrayNodeBinding(name);if(s=="UniformNode")return nativeUniformFactory();if(s=="PropertyNode")return nativePropertyNodeBinding(name);if(s=="AssignNode")return nativeAssignNodeBinding(name);if(s=="FunctionCallNode")return nativeFunctionCallNodeBinding(name);if(s=="OperatorNode")return nativeOperatorNodeBinding(name);if(s=="MathNode")return nativeMathNodeBinding(name);if(s=="ConditionalNode")return nativeConditionalNodeBinding(name);if(s=="ContextNode")return nativeContextNodeBinding(name);if(s=="VarNode")return nativeVarNodeBinding(name);if(s=="VaryingNode")return nativeVaryingNodeBinding(name);if(s=="ColorSpaceNode")return nativeColorSpaceNodeBinding(name);if(s=="ToneMappingNode")return nativeToneMappingNodeBinding(name);if(s=="BufferAttributeNode")return nativeBufferAttributeNodeBinding(name);if(s=="ComputeNode")return nativeComputeNodeBinding(name);if(s=="IsolateNode")return nativeIsolateNodeBinding(name);if(s=="BypassNode")return nativeBypassNodeBinding(name);if(s=="Remap")return nativeRemapBinding(name);if(s=="ExpressionNode")return nativeExpressionNodeBinding(name);if(s=="Discard")return nativeDiscardBinding(name);if(s=="RenderOutputNode")return nativeRenderOutputNodeBinding(name);if(s=="DebugNode")return nativeDebugNodeBinding(name);if(s=="SubBuildNode")return nativeSubBuildNodeBinding(name);if(s=="InspectorNode")return nativeInspectorNodeBinding(name);throw std::logic_error("Unknown TSLBase source");
}
JSValue prototype(){auto rows=array();const auto p=nativeNodeConstructor().get("prototype");for(const auto& k:p.ownPropertyNames()){const auto d=p.getOwnPropertyDescriptor(k);auto row=JSValue::object();row.set("name",k);row.set("writable",d->writable.value_or(false));row.set("enumerable",d->enumerable.value_or(false));row.set("configurable",d->configurable.value_or(false));if(d->value)row.set("value",metadata(*d->value));if(d->getter&&d->getter->toBoolean())row.set("get",metadata(*d->getter));if(d->setter&&d->setter->toBoolean())row.set("set",metadata(*d->setter));call(rows,"push",{row});}return rows;}
JSValue decode(const JSValue& v){if(!v.isObject())return v;const auto tag=v.get("tag");if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("typed"))return NativeArray_detail::constructor(6).construct({v.get("values")});if(tag.strictEquals("node"))return nativeNodeConstructor().construct({v.get("type")});if(v.type()==JSValue::Type::Array){auto out=JSValue::array(v.length());for(unsigned i=0;i<v.length();++i)own(out,std::to_string(i),decode(v.get(std::to_string(i))));return out;}return v;}
JSValue graph(const JSValue& r){const auto b=nativeTSLBase();baseline=nativeNodeConstructor().construct().get("id").toNumber();const auto stack=nativeStackNodeConstructor().construct(),previous=nativeGetCurrentStack();nativeSetCurrentStack(stack);auto out=JSValue::object();
    try{Args args;for(unsigned i=0;i<r.get("args").length();++i)args.push_back(decode(r.get("args").get(std::to_string(i))));const auto name=r.get("name").string();JSValue value;
        if(name=="inspect")value=b.get(name).call({},args);
        else if(name=="If"){const auto fn=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return nativeTSLBaseBinding("float").call({}, {0.25});});value=b.get(name).call({}, {true,fn});}
        else if(name=="chain"){auto n=b.get("vec3").call({}, {1,2,3});value=call(call(call(n,"add",{1}),"normalize"),"toVarying",{"result"});}
        else value=b.get(name).call({},args);
        out.set("result",normal(value));out.set("stack",normal(stack.get("nodes")));
    }catch(...){out.set("error",failure());}nativeSetCurrentStack(previous);baseline=0;return out;}
JSValue integration(const JSValue& r){auto renderer=JSValue::object(),debug=JSValue::object(),diagnostics=JSValue::object();diagnostics.set("keywords",false);debug.set("diagnostics",diagnostics);renderer.set("debug",debug);renderer.set("coordinateSystem",2000);auto builder=nativeNodeBuilderConstructor().construct({nullptr,renderer,nullptr});builder.set("shaderStage",r.get("stage"));baseline=nativeNodeConstructor().construct().get("id").toNumber();auto out=JSValue::object();
    try{const auto b=nativeTSLBase();JSValue node;const auto name=r.get("name");if(name.strictEquals("bufferAttribute")){const auto value=NativeArray_detail::constructor(6).construct({array({1,2,3,4,5,6})});node=b.get("bufferAttribute").call({}, {value,"vec3"});}
        else if(name.strictEquals("color")){const auto color=b.get("vec4").call({}, {.1,.2,.3,1});node=b.get("convertColorSpace").call({}, {color,"srgb-linear","srgb-linear"});}
        else if(name.strictEquals("compute")){node=b.get("compute").call({}, {b.get("expression").call({}, {"computeBody();","void"}),16});}
        else node=b.get("expression").call({}, {"1.0","float"});auto results=array();for(const auto* stage:{"setup","analyze","generate"}){builder.set("buildStage",stage);call(results,"push",{normal(call(node,"build",{builder,r.get("type")}))});}out.set("results",results);out.set("flow",normal(builder.get("flow")));out.set("bufferAttributes",normal(builder.get("bufferAttributes")));out.set("varyings",normal(builder.get("varyings")));}
    catch(...){out.set("error",failure());}baseline=0;return out;}
JSValue warning(const JSValue& r){auto events=array();const auto previous=ThreeUtils::getConsoleFunction();ThreeUtils::setConsoleFunction(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{call(c[0],"push",{normal(array(a))});return {};},{events}));auto out=JSValue::object();try{const auto fn=nativeTSLBaseBinding("addNodeElement");const auto result=r.get("construct").toBoolean()?fn.construct({decode(r.get("value"))}):fn.call({}, {decode(r.get("value"))});out.set("result",normal(result));}catch(...){out.set("error",failure());}ThreeUtils::setConsoleFunction(previous);out.set("events",events);return out;}
JSValue run(const JSValue& r){const auto kind=r.get("kind"),b=nativeTSLBase();if(kind.strictEquals("keys")){auto a=array();for(const auto& k:b.keys())call(a,"push",{k});return a;}if(kind.strictEquals("metadata"))return metadata(b.get(r.get("name")));if(kind.strictEquals("identity"))return b.get(r.get("name")).sameValue(source(r.get("source").string(),r.get("name").string()));if(kind.strictEquals("prototype"))return prototype();if(kind.strictEquals("warning"))return warning(r);if(kind.strictEquals("graph"))return graph(r);if(kind.strictEquals("integration"))return integration(r);if(kind.strictEquals("aliases")){auto out=array();const auto names=b.keys();for(unsigned i=0;i<names.size();++i)for(unsigned j=i+1;j<names.size();++j)if(b.get(names[i]).sameValue(b.get(names[j])))call(out,"push",{array({names[i],names[j]})});return out;}throw std::logic_error("Unknown TSLBase fixture");}
void lifetime(){for(unsigned round=0;round<3;++round){JSWeakValue witness;{const auto fn=nativeTSLBaseBinding("array");auto b=nativeTSLBase();b.set("array",nullptr);b.set("vec3",nullptr);b.set("compute",nullptr);if(!nativeTSLBaseBinding("array").sameValue(fn)||!fn.call({}, {"float",2}).get("isArrayNode").strictEquals(true))throw std::logic_error("TSLBase facade replaced lexical factory");fn.collectGarbage();if(!nativeTSLBaseBinding("array").sameValue(fn))throw std::logic_error("TSLBase private table was collected");witness=JSWeakValue(fn);}JSValue::nativeSafepoint();if(!witness.expired())throw std::logic_error("TSLBase module retained a retired heap");}std::cout<<"3 fresh TSLBase binding lifetimes passed\n";}
}
int main(int argc,char** argv){try{if(argc>1&&std::string(argv[1])=="--lifetime"){lifetime();return 0;}relativeIds=argc>1&&std::string(argv[1])=="--relative-node-ids";const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=Animation_detail::parseJSON(input);nativeTSLBase();auto out=array();for(unsigned i=0;i<requests.length();++i)call(out,"push",{run(requests.get(std::to_string(i)))});std::cout<<out.toJSON()->dump();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
