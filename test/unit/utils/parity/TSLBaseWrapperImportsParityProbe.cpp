#include "nodes/core/ArrayNode.h"
#include "nodes/core/IsolateNode.h"
#include "nodes/core/BypassNode.h"
#include "nodes/core/SubBuildNode.h"
#include "nodes/core/VaryingNode.h"
#include "nodes/code/ExpressionNode.h"
#include "nodes/utils/DebugNode.h"
#include "nodes/display/ColorSpaceNode.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {
using Args=std::vector<JSValue>;
void own(JSValue o,const std::string& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;o.defineProperty(key,d);}
JSValue array(const Args& args={}){auto out=JSValue::array();for(std::size_t i=0;i<args.size();++i)own(out,std::to_string(i),args[i]);return out;}
JSValue normal(const JSValue& value){if(value.isUndefined())return "#undefined";if(value.isCallable())return "#function";if(value.isSymbol())return "#symbol";if(value.type()==JSValue::Type::Number){if(std::isnan(value.number()))return "#NaN";if(std::isinf(value.number()))return value.number()>0?"#Infinity":"#-Infinity";if(value.number()==0&&std::signbit(value.number()))return "#-0";}if(!value.isObject())return value;auto out=value.type()==JSValue::Type::Array?JSValue::array(value.length()):JSValue::object();for(const auto& key:value.keys())own(out,key,normal(value.get(key)));return out;}
JSValue construct(const std::string& name){
    // Primitive arguments do not create a realm. The wrapper is the first
    // operation that can allocate a managed object in this fresh process.
    if(name=="ArrayNode")return ArrayNode("float",2).object();
    if(name=="IsolateNode")return IsolateNode(nullptr).object();
    if(name=="BypassNode")return BypassNode(nullptr,nullptr).object();
    if(name=="SubBuildNode")return SubBuildNode(nullptr,"TEST").object();
    if(name=="VaryingNode")return VaryingNode(nullptr).object();
    if(name=="ExpressionNode")return ExpressionNode("","void").object();
    if(name=="DebugNode")return DebugNode(nullptr).object();
    if(name=="ColorSpaceNode")return ColorSpaceNode(nullptr,"srgb-linear","srgb").object();
    throw std::invalid_argument("Unknown wrapper");
}
JSValue constructor(const std::string& name){
    if(name=="ArrayNode")return ArrayNode::constructor();if(name=="IsolateNode")return IsolateNode::constructor();if(name=="BypassNode")return BypassNode::constructor();if(name=="SubBuildNode")return SubBuildNode::constructor();if(name=="VaryingNode")return VaryingNode::constructor();if(name=="ExpressionNode")return ExpressionNode::constructor();if(name=="DebugNode")return DebugNode::constructor();return ColorSpaceNode::constructor();
}
std::vector<const char*> chains(const std::string& name){if(name=="ArrayNode")return {"toArray"};if(name=="IsolateNode")return {"cache","isolate"};if(name=="BypassNode")return {"bypass"};if(name=="VaryingNode")return {"toVarying","toVertexStage"};if(name=="DebugNode")return {"debug"};if(name=="ColorSpaceNode")return {"workingToColorSpace","colorSpaceToWorking"};return {};}
}
int main(int argc,char** argv){try{
    if(argc!=2)throw std::invalid_argument("Expected one wrapper name");const std::string name=argv[1];
    // Capture instance and chain state before calling any explicit constructor,
    // module or binding entry point that could repair missing initialization.
    const auto node=construct(name);auto out=JSValue::object();out.set("instance",normal(node));out.set("id",node.get("id"));auto methods=array();const auto nodePrototype=node.getPrototype().getPrototype();
    for(const auto* key:chains(name)){
        const auto fn=node.get(key);const auto d=nodePrototype.getOwnPropertyDescriptor(key);auto item=JSValue::object();item.set("name",key);item.set("callable",fn.isCallable());item.set("ownOnNodePrototype",d.has_value());item.set("sameAsNodePrototype",fn.sameValue(nodePrototype.get(key)));
        item.set("function",fn.isCallable()?array({fn.get("name"),fn.get("length"),fn.isConstructible()}):normal(fn));if(d)item.set("descriptor",array({d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)}));own(methods,std::to_string(methods.length()),item);
    }
    out.set("chains",methods);if(name=="ExpressionNode")out.set("unregisteredFactoryMethod",normal(node.get("expression")));if(name=="SubBuildNode")out.set("unregisteredFactoryMethod",normal(node.get("subBuild")));if(name=="ColorSpaceNode")out.set("unregisteredFactoryMethod",normal(node.get("convertColorSpace")));
    out.set("wrapperConstructorIdentity",node.getPrototype().get("constructor").sameValue(constructor(name)));std::cout<<out.toJSON()->dump();
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
