#include "nodes/utils/FunctionOverloadingNode.h"

bool FunctionOverloadingNode::isConsidered(const Candidate& candidate, std::size_t argumentCount) {
    // The counts must be equal; a different length is not scored at all.
    return candidate.inputTypes.size() == argumentCount;
}

int FunctionOverloadingNode::score(const Candidate& candidate,
                                   const std::vector<std::string>& argumentTypes) {
    int total = 0;
    for (std::size_t i = 0; i < argumentTypes.size(); ++i) {
        if (i < candidate.inputTypes.size() && argumentTypes[i] == candidate.inputTypes[i]) {
            total += 1;
        }
    }
    return total;
}

int FunctionOverloadingNode::choose(const std::vector<Candidate>& candidates,
                                    const std::vector<std::string>& argumentTypes) {
    int best = -1;
    int bestScore = kStartingScore;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (!isConsidered(candidates[i], argumentTypes.size())) continue;
        const int current = score(candidates[i], argumentTypes);
        // Strictly greater, so a tie keeps the earlier one.
        if (current > bestScore) {
            best = static_cast<int>(i);
            bestScore = current;
        }
    }
    return best;
}

bool FunctionOverloadingNode::remembersChoice() {
    return true;
}

// Full fixed-source FunctionOverloadingNode and overloadingFn; AST is development-only.
#include "nodes/utils/FunctionOverloadingNode.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLCore.h"
#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include "core/JSModuleObject.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSSourceTrace.h"
#include "math/JSNumberMath.h"
#include <unordered_set>
#include <cmath>
#include <exception>
namespace FunctionOverloadingManaged {

using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const JSValue& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
void setReference(JSValue base,const JSValue& key,const JSValue& value){base.set(key,value);}
JSValue deleteReference(JSValue base,const JSValue& key){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot delete property of null/undefined");const auto property=key.toPropertyKey();auto object=base.toObject();if(!object.erase(property))throw JSTypeError("Cannot delete nonconfigurable property");return true;}
JSValue argumentObject(const Args& args){auto object=JSValue::object();JSPropertyDescriptor length;length.value=args.size();length.writable=length.configurable=true;object.defineProperty("length",length);for(unsigned i=0;i<args.size();++i)own(object,JSValue(i),args[i]);return object;}
JSValue typeOf(const JSValue& value){if(value.isUndefined())return "undefined";if(value.isCallable())return "function";if(value.isSymbol())return "symbol";switch(value.type()){case JSValue::Type::Boolean:return "boolean";case JSValue::Type::Number:return "number";case JSValue::Type::String:return "string";default:return "object";}}
JSValue binary(std::string_view op,const JSValue& left,const JSValue& right){
 if(op=="in")return right.has(left);if(op=="===")return left.strictEquals(right);if(op=="!==")return !left.strictEquals(right);
 if(op=="+"){const auto a=left.toPrimitive();const auto b=right.toPrimitive();if(a.type()==JSValue::Type::String||b.type()==JSValue::Type::String){const auto x=a.toString();const auto y=b.toString();return x+y;}const auto x=a.toNumber();const auto y=b.toNumber();return x+y;}
 if(op=="<"||op==">"||op=="<="||op==">="){const auto a=left.toPrimitive(JSValue::PreferredType::Number);const auto b=right.toPrimitive(JSValue::PreferredType::Number);if(a.type()==JSValue::Type::String&&b.type()==JSValue::Type::String){const auto x=jsUTF16(a.string()),y=jsUTF16(b.string());if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}const auto x=a.toNumber();const auto y=b.toNumber();if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}
 const auto a=left.toNumber();const auto b=right.toNumber();if(op=="-")return a-b;if(op=="*")return a*b;if(op=="/")return a/b;if(op=="**")return JSNumberMath::pow(a,b);throw std::logic_error("Untranslated arithmetic operator");
}
void appendSpread(Args& values,const JSValue& iterable){auto iterator=getJSIterator(iterable);while(const auto value=stepJSIterator(iterator))values.push_back(*value);}
void copyDataProperties(JSValue target,JSValue source){if(source.isNull()||source.isUndefined())return;if(!source.isObject())source=source.toObject();for(const auto& key:source.ownPropertyKeys()){const auto d=source.getOwnPropertyDescriptor(key);if(d&&d->enumerable.value_or(false)){const auto value=source.get(key);own(target,key,value);}}}
class ForInCursor{JSValue object;std::vector<JSValue> keys;std::unordered_set<std::string> visited;std::size_t index=0;bool loaded=false;public:explicit ForInCursor(JSValue source):object(source.isNull()||source.isUndefined()?JSValue(nullptr):source.toObject()){}std::optional<JSValue> next(){while(!object.isNull()){if(!loaded){keys=object.ownPropertyKeys();index=0;loaded=true;}while(index<keys.size()){const auto key=keys[index++];if(key.isSymbol())continue;const auto name=key.string();if(visited.contains(name))continue;const auto d=object.getOwnPropertyDescriptor(key);if(!d)continue;visited.insert(name);if(d->enumerable.value_or(false))return key;}object=object.getPrototype();loaded=false;}return std::nullopt;}};

JSValue restArgs(const Args& args,std::size_t start){auto array=JSValue::array();for(std::size_t i=start;i<args.size();++i)own(array,JSValue(i-start),args[i]);return array;}
JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
JSValue nested0(const JSValue&,const Args&,const Args&);
JSValue nested1(const JSValue&,const Args&,const Args&);
JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
JSSourceTrace::FrameScope frame({"FunctionOverloadingNode.type",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/FunctionOverloadingNode.js",13,17,373,"8b125b3d9f68a1588cb25911912a05b9bef39c48af76b0a4b468689113d9a6f6"},JSSourceTrace::receiverTypeName(nativeStatementTemp0),"",false,false,false});
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
return JSValue("FunctionOverloadingNode");
return JSValue{};
}
JSValue method1(const JSValue& nativeStatementTemp6,const Args& nativeStatementTemp7,const Args& nativeStatementTemp8){
JSSourceTrace::FrameScope frame({"FunctionOverloadingNode",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/FunctionOverloadingNode.js",25,13,653,"8b125b3d9f68a1588cb25911912a05b9bef39c48af76b0a4b468689113d9a6f6"},std::string{},"",true,false,false});
auto nativeStatementTemp9=nativeStatementTemp8[0];auto nativeStatementTemp10=nativeStatementTemp6;
const auto nativeStatementTemp11=nativeStatementTemp6;nativeStatementTemp10={};
JSValue nativeStatementTemp12=arg(nativeStatementTemp7,0);if(nativeStatementTemp12.isUndefined())nativeStatementTemp12=([&]()->JSValue{auto nativeStatementTemp13=JSValue::array();return nativeStatementTemp13;}());
JSValue functionNodes=nativeStatementTemp12;
JSValue parametersNodes=restArgs(nativeStatementTemp7,1);
([&]()->JSValue{const auto nativeStatementTemp17=nativeStatementTemp9.get("FunctionOverloadingNode").getPrototype();Args nativeStatementTemp16;nativeStatementTemp10=nativeStatementTemp17.construct(nativeStatementTemp16,nativeStatementTemp11);return nativeStatementTemp10;}());
([&]()->JSValue{auto nativeStatementTemp18=nativeStatementTemp10;const auto nativeStatementTemp19=JSValue("functionNodes");const auto nativeStatementTemp20=functionNodes;setReference(nativeStatementTemp18,nativeStatementTemp19,nativeStatementTemp20);return nativeStatementTemp20;}());
([&]()->JSValue{auto nativeStatementTemp23=nativeStatementTemp10;const auto nativeStatementTemp24=JSValue("parametersNodes");const auto nativeStatementTemp25=parametersNodes;setReference(nativeStatementTemp23,nativeStatementTemp24,nativeStatementTemp25);return nativeStatementTemp25;}());
([&]()->JSValue{auto nativeStatementTemp28=nativeStatementTemp10;const auto nativeStatementTemp29=JSValue("_candidateFn");const auto nativeStatementTemp30=JSValue(nullptr);setReference(nativeStatementTemp28,nativeStatementTemp29,nativeStatementTemp30);return nativeStatementTemp30;}());
([&]()->JSValue{auto nativeStatementTemp33=nativeStatementTemp10;const auto nativeStatementTemp34=JSValue("global");const auto nativeStatementTemp35=JSValue(true);setReference(nativeStatementTemp33,nativeStatementTemp34,nativeStatementTemp35);return nativeStatementTemp35;}());
return nativeStatementTemp10;
}
JSValue method2(const JSValue& nativeStatementTemp38,const Args& nativeStatementTemp39,const Args& nativeStatementTemp40){
JSSourceTrace::FrameScope frame({"FunctionOverloadingNode.isCacheable",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/FunctionOverloadingNode.js",61,13,1262,"8b125b3d9f68a1588cb25911912a05b9bef39c48af76b0a4b468689113d9a6f6"},JSSourceTrace::receiverTypeName(nativeStatementTemp38),"",false,false,false});
auto nativeStatementTemp41=nativeStatementTemp40[0];auto nativeStatementTemp42=nativeStatementTemp38;
return JSValue(false);
return JSValue{};
}
JSValue method3(const JSValue& nativeStatementTemp44,const Args& nativeStatementTemp45,const Args& nativeStatementTemp46){
JSSourceTrace::FrameScope frame({"FunctionOverloadingNode.generateNodeType",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/FunctionOverloadingNode.js",74,18,1543,"8b125b3d9f68a1588cb25911912a05b9bef39c48af76b0a4b468689113d9a6f6"},JSSourceTrace::receiverTypeName(nativeStatementTemp44),"",false,false,false});
auto nativeStatementTemp47=nativeStatementTemp46[0];auto nativeStatementTemp48=nativeStatementTemp44;
JSValue builder=arg(nativeStatementTemp45,0);
JSValue candidateFn=([&]()->JSValue{auto nativeStatementTemp53=nativeStatementTemp48;const auto nativeStatementTemp54=JSValue("getCandidateFn");const auto nativeStatementTemp50=nativeStatementTemp53.get(nativeStatementTemp54);const auto nativeStatementTemp51=nativeStatementTemp53;Args nativeStatementTemp52;nativeStatementTemp52.push_back(builder);return nativeStatementTemp50.call(nativeStatementTemp51,nativeStatementTemp52);}());
return ([&]()->JSValue{const auto nativeStatementTemp55=([&]()->JSValue{const auto nativeStatementTemp57=([&]()->JSValue{const auto nativeStatementTemp59=candidateFn;const auto nativeStatementTemp60=JSValue("shaderNode");return nativeStatementTemp59.get(nativeStatementTemp60);}());const auto nativeStatementTemp58=JSValue("layout");return nativeStatementTemp57.get(nativeStatementTemp58);}());const auto nativeStatementTemp56=JSValue("type");return nativeStatementTemp55.get(nativeStatementTemp56);}());
return JSValue{};
}
JSValue method4(const JSValue& nativeStatementTemp61,const Args& nativeStatementTemp62,const Args& nativeStatementTemp63){
JSSourceTrace::FrameScope frame({"FunctionOverloadingNode.getCandidateFn",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/FunctionOverloadingNode.js",88,16,1881,"8b125b3d9f68a1588cb25911912a05b9bef39c48af76b0a4b468689113d9a6f6"},JSSourceTrace::receiverTypeName(nativeStatementTemp61),"",false,false,false});
auto nativeStatementTemp64=nativeStatementTemp63[0];auto nativeStatementTemp65=nativeStatementTemp61;
JSValue builder=arg(nativeStatementTemp62,0);
JSValue params=([&]()->JSValue{const auto nativeStatementTemp67=nativeStatementTemp65;const auto nativeStatementTemp68=JSValue("parametersNodes");return nativeStatementTemp67.get(nativeStatementTemp68);}());
JSValue candidateFn=([&]()->JSValue{const auto nativeStatementTemp69=nativeStatementTemp65;const auto nativeStatementTemp70=JSValue("_candidateFn");return nativeStatementTemp69.get(nativeStatementTemp70);}());
if(([&]()->JSValue{const auto nativeStatementTemp71=candidateFn;const auto nativeStatementTemp72=JSValue(nullptr);return binary("===",nativeStatementTemp71,nativeStatementTemp72);}()).toBoolean()){
JSValue bestCandidateFn=JSValue(nullptr);
JSValue bestScore=([&]()->JSValue{const auto nativeStatementTemp73=JSValue(1);return -nativeStatementTemp73.toNumber();}());
{auto nativeStatementTemp74=getJSIterator(([&]()->JSValue{const auto nativeStatementTemp122=nativeStatementTemp65;const auto nativeStatementTemp123=JSValue("functionNodes");return nativeStatementTemp122.get(nativeStatementTemp123);}()));bool nativeStatementTemp76=true;while(const auto nativeStatementTemp75=stepJSIterator(nativeStatementTemp74)){try{JSValue functionNode=*nativeStatementTemp75;
{
JSValue shaderNode=([&]()->JSValue{const auto nativeStatementTemp77=functionNode;const auto nativeStatementTemp78=JSValue("shaderNode");return nativeStatementTemp77.get(nativeStatementTemp78);}());
JSValue layout=([&]()->JSValue{const auto nativeStatementTemp79=shaderNode;const auto nativeStatementTemp80=JSValue("layout");return nativeStatementTemp79.get(nativeStatementTemp80);}());
if(([&]()->JSValue{const auto nativeStatementTemp81=layout;const auto nativeStatementTemp82=JSValue(nullptr);return binary("===",nativeStatementTemp81,nativeStatementTemp82);}()).toBoolean()){
throw JSThrownValue(([&]()->JSValue{const auto nativeStatementTemp83=nativeGlobalBinding("Error");Args nativeStatementTemp84;nativeStatementTemp84.push_back(JSValue("THREE.FunctionOverloadingNode: FunctionNode must be a layout."));return nativeStatementTemp83.construct(nativeStatementTemp84);}()));

}
JSValue inputs=([&]()->JSValue{const auto nativeStatementTemp85=layout;const auto nativeStatementTemp86=JSValue("inputs");return nativeStatementTemp85.get(nativeStatementTemp86);}());
if(([&]()->JSValue{const auto nativeStatementTemp87=([&]()->JSValue{const auto nativeStatementTemp89=params;const auto nativeStatementTemp90=JSValue("length");return nativeStatementTemp89.get(nativeStatementTemp90);}());const auto nativeStatementTemp88=([&]()->JSValue{const auto nativeStatementTemp91=inputs;const auto nativeStatementTemp92=JSValue("length");return nativeStatementTemp91.get(nativeStatementTemp92);}());return binary("===",nativeStatementTemp87,nativeStatementTemp88);}()).toBoolean()){
JSValue currentScore=JSValue(0);
{JSValue i=JSValue(0);
for(;([&]()->JSValue{const auto nativeStatementTemp93=i;const auto nativeStatementTemp94=([&]()->JSValue{const auto nativeStatementTemp95=params;const auto nativeStatementTemp96=JSValue("length");return nativeStatementTemp95.get(nativeStatementTemp96);}());return binary("<",nativeStatementTemp93,nativeStatementTemp94);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp97=i.toNumber();const auto nativeStatementTemp98=nativeStatementTemp97+1;i=nativeStatementTemp98;return nativeStatementTemp97;}())){
JSValue param=([&]()->JSValue{const auto nativeStatementTemp99=params;const auto nativeStatementTemp100=i;return nativeStatementTemp99.get(nativeStatementTemp100);}());
JSValue input=([&]()->JSValue{const auto nativeStatementTemp101=inputs;const auto nativeStatementTemp102=i;return nativeStatementTemp101.get(nativeStatementTemp102);}());
if(([&]()->JSValue{const auto nativeStatementTemp103=([&]()->JSValue{auto nativeStatementTemp108=param;const auto nativeStatementTemp109=JSValue("getNodeType");const auto nativeStatementTemp105=nativeStatementTemp108.get(nativeStatementTemp109);const auto nativeStatementTemp106=nativeStatementTemp108;Args nativeStatementTemp107;nativeStatementTemp107.push_back(builder);return nativeStatementTemp105.call(nativeStatementTemp106,nativeStatementTemp107);}());const auto nativeStatementTemp104=([&]()->JSValue{const auto nativeStatementTemp110=input;const auto nativeStatementTemp111=JSValue("type");return nativeStatementTemp110.get(nativeStatementTemp111);}());return binary("===",nativeStatementTemp103,nativeStatementTemp104);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp112=currentScore.toNumber();const auto nativeStatementTemp113=nativeStatementTemp112+1;currentScore=nativeStatementTemp113;return nativeStatementTemp112;}());

}

}
}
if(([&]()->JSValue{const auto nativeStatementTemp114=currentScore;const auto nativeStatementTemp115=bestScore;return binary(">",nativeStatementTemp114,nativeStatementTemp115);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp116=functionNode;bestCandidateFn=nativeStatementTemp116;return nativeStatementTemp116;}());
([&]()->JSValue{const auto nativeStatementTemp119=currentScore;bestScore=nativeStatementTemp119;return nativeStatementTemp119;}());

}

}

}
}catch(...){if(nativeStatementTemp76){nativeStatementTemp76=false;closeJSIterator(nativeStatementTemp74,std::current_exception());}throw;}}}
([&]()->JSValue{auto nativeStatementTemp124=nativeStatementTemp65;const auto nativeStatementTemp125=JSValue("_candidateFn");const auto nativeStatementTemp126=([&]()->JSValue{const auto nativeStatementTemp129=bestCandidateFn;candidateFn=nativeStatementTemp129;return nativeStatementTemp129;}());setReference(nativeStatementTemp124,nativeStatementTemp125,nativeStatementTemp126);return nativeStatementTemp126;}());

}
return candidateFn;
return JSValue{};
}
JSValue method5(const JSValue& nativeStatementTemp132,const Args& nativeStatementTemp133,const Args& nativeStatementTemp134){
JSSourceTrace::FrameScope frame({"FunctionOverloadingNode.setup",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/FunctionOverloadingNode.js",154,7,3113,"8b125b3d9f68a1588cb25911912a05b9bef39c48af76b0a4b468689113d9a6f6"},JSSourceTrace::receiverTypeName(nativeStatementTemp132),"",false,false,false});
auto nativeStatementTemp135=nativeStatementTemp134[0];auto nativeStatementTemp136=nativeStatementTemp132;
JSValue builder=arg(nativeStatementTemp133,0);
JSValue candidateFn=([&]()->JSValue{auto nativeStatementTemp141=nativeStatementTemp136;const auto nativeStatementTemp142=JSValue("getCandidateFn");const auto nativeStatementTemp138=nativeStatementTemp141.get(nativeStatementTemp142);const auto nativeStatementTemp139=nativeStatementTemp141;Args nativeStatementTemp140;nativeStatementTemp140.push_back(builder);return nativeStatementTemp138.call(nativeStatementTemp139,nativeStatementTemp140);}());
return ([&]()->JSValue{const auto nativeStatementTemp143=candidateFn;const JSValue nativeStatementTemp144;Args nativeStatementTemp145;appendSpread(nativeStatementTemp145,([&]()->JSValue{const auto nativeStatementTemp146=nativeStatementTemp136;const auto nativeStatementTemp147=JSValue("parametersNodes");return nativeStatementTemp146.get(nativeStatementTemp147);}()));return nativeStatementTemp143.call(nativeStatementTemp144,nativeStatementTemp145);}());
return JSValue{};
}
JSValue nested0(const JSValue& nativeStatementTemp148,const Args& nativeStatementTemp149,const Args& nativeStatementTemp150){
auto nativeStatementTemp151=nativeStatementTemp150[0];
auto nativeStatementTemp154=JSValue::object(nullptr);nativeStatementTemp154.set("value",{});
nativeStatementTemp154.set("value",arg(nativeStatementTemp149,0));
return ([&]()->JSValue{auto nativeStatementTemp164=JSValue::function(nested1,{nativeStatementTemp151,nativeStatementTemp154},"",0);nativeStatementTemp164.setFunctionSourceText("( ...params ) => overloadingBaseFn( functionNodes, ...params )");return nativeStatementTemp164;}());
}

JSValue nested1(const JSValue& nativeStatementTemp155,const Args& nativeStatementTemp156,const Args& nativeStatementTemp157){
auto nativeStatementTemp158=nativeStatementTemp157[0];
JSValue params=restArgs(nativeStatementTemp156,0);
return ([&]()->JSValue{const auto nativeStatementTemp161=nativeStatementTemp158.get("overloadingBaseFn");const JSValue nativeStatementTemp162;Args nativeStatementTemp163;nativeStatementTemp163.push_back(nativeStatementTemp157[1].get("value"));appendSpread(nativeStatementTemp163,params);return nativeStatementTemp161.call(nativeStatementTemp162,nativeStatementTemp163);}());
}
}
JSValue nativeFunctionOverloadingNodeModule(){using namespace FunctionOverloadingManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject()){const auto status=cached->getInternalSlot("status");if(status.strictEquals("evaluated"))return cached->getInternalSlot("facade");if(status.strictEquals("failed"))throw JSThrownValue(cached->getInternalSlot("error"));throw JSModuleInitializationError("FunctionOverloadingNode module");}auto module=JSValue::object(nullptr);module.retainForHeapLifetime();weak=JSWeakValue(module);lexical()=weak;module.setInternalSlot("status","initializing");try{
own(module,"Node",nativeNodeConstructor());own(module,"nodeProxy",nativeTSLCoreBinding("nodeProxy"));
auto constructor=JSValue::builtinConstructor(method1,{module},"FunctionOverloadingNode",0);constructor.setFunctionSourceText("class FunctionOverloadingNode extends Node {\r\n\r\n\tstatic get type() {\r\n\r\n\t\treturn 'FunctionOverloadingNode';\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Constructs a new function overloading node.\r\n\t *\r\n\t * @param {Array<Function>} functionNodes - Array of `Fn` function definitions.\r\n\t * @param {...Node} parametersNodes - A list of parameter nodes.\r\n\t */\r\n\tconstructor( functionNodes = [], ...parametersNodes ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\t/**\r\n\t\t * Array of `Fn` function definitions.\r\n\t\t *\r\n\t\t * @type {Array<Function>}\r\n\t\t */\r\n\t\tthis.functionNodes = functionNodes;\r\n\r\n\t\t/**\r\n\t\t * A list of parameter nodes.\r\n\t\t *\r\n\t\t * @type {Array<Node>}\r\n\t\t */\r\n\t\tthis.parametersNodes = parametersNodes;\r\n\r\n\t\t/**\r\n\t\t * The selected overloaded function call.\r\n\t\t *\r\n\t\t * @private\r\n\t\t * @type {ShaderCallNodeInternal}\r\n\t\t */\r\n\t\tthis._candidateFn = null;\r\n\r\n\t\t/**\r\n\t\t * This node is marked as global.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.global = true;\r\n\r\n\t}\r\n\r\n\tisCacheable( /*builder*/ ) {\r\n\r\n\t\treturn false;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * This method is overwritten since the node type is inferred from\r\n\t * the function's return type.\r\n\t *\r\n\t * @param {NodeBuilder} builder - The current node builder.\r\n\t * @return {string} The node type.\r\n\t */\r\n\tgenerateNodeType( builder ) {\r\n\r\n\t\tconst candidateFn = this.getCandidateFn( builder );\r\n\r\n\t\treturn candidateFn.shaderNode.layout.type;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns the candidate function for the current parameters.\r\n\t *\r\n\t * @param {NodeBuilder} builder - The current node builder.\r\n\t * @return {FunctionNode} The candidate function.\r\n\t */\r\n\tgetCandidateFn( builder ) {\r\n\r\n\t\tconst params = this.parametersNodes;\r\n\r\n\t\tlet candidateFn = this._candidateFn;\r\n\r\n\t\tif ( candidateFn === null ) {\r\n\r\n\t\t\tlet bestCandidateFn = null;\r\n\t\t\tlet bestScore = - 1;\r\n\r\n\t\t\tfor ( const functionNode of this.functionNodes ) {\r\n\r\n\t\t\t\tconst shaderNode = functionNode.shaderNode;\r\n\t\t\t\tconst layout = shaderNode.layout;\r\n\r\n\t\t\t\tif ( layout === null ) {\r\n\r\n\t\t\t\t\tthrow new Error( 'THREE.FunctionOverloadingNode: FunctionNode must be a layout.' );\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t\tconst inputs = layout.inputs;\r\n\r\n\t\t\t\tif ( params.length === inputs.length ) {\r\n\r\n\t\t\t\t\tlet currentScore = 0;\r\n\r\n\t\t\t\t\tfor ( let i = 0; i < params.length; i ++ ) {\r\n\r\n\t\t\t\t\t\tconst param = params[ i ];\r\n\t\t\t\t\t\tconst input = inputs[ i ];\r\n\r\n\t\t\t\t\t\tif ( param.getNodeType( builder ) === input.type ) {\r\n\r\n\t\t\t\t\t\t\tcurrentScore ++;\r\n\r\n\t\t\t\t\t\t}\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t\tif ( currentScore > bestScore ) {\r\n\r\n\t\t\t\t\t\tbestCandidateFn = functionNode;\r\n\t\t\t\t\t\tbestScore = currentScore;\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t\tthis._candidateFn = candidateFn = bestCandidateFn;\r\n\r\n\t\t}\r\n\r\n\t\treturn candidateFn;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Sets up the node for the current parameters.\r\n\t *\r\n\t * @param {NodeBuilder} builder - The current node builder.\r\n\t * @return {Node} The setup node.\r\n\t */\r\n\tsetup( builder ) {\r\n\r\n\t\tconst candidateFn = this.getCandidateFn( builder );\r\n\r\n\t\treturn candidateFn( ...this.parametersNodes );\r\n\r\n\t}\r\n\r\n}");constructor.setPrototype(module.get("Node"));auto prototype=constructor.get("prototype");prototype.setPrototype(module.get("Node").get("prototype"));own(module,"FunctionOverloadingNode",constructor);
{auto function=JSValue::function(method0,{module},"get type",0);function.setFunctionSourceText("get type() {\r\n\r\n\t\treturn 'FunctionOverloadingNode';\r\n\r\n\t}");JSPropertyDescriptor d;d.getter=function;d.configurable=true;constructor.defineProperty("type",d);}
{auto function=JSValue::function(method2,{module},"isCacheable",0);function.setFunctionSourceText("isCacheable( /*builder*/ ) {\r\n\r\n\t\treturn false;\r\n\r\n\t}");JSPropertyDescriptor d;d.value=function;d.configurable=true;d.writable=true;prototype.defineProperty("isCacheable",d);}
{auto function=JSValue::function(method3,{module},"generateNodeType",1);function.setFunctionSourceText("generateNodeType( builder ) {\r\n\r\n\t\tconst candidateFn = this.getCandidateFn( builder );\r\n\r\n\t\treturn candidateFn.shaderNode.layout.type;\r\n\r\n\t}");JSPropertyDescriptor d;d.value=function;d.configurable=true;d.writable=true;prototype.defineProperty("generateNodeType",d);}
{auto function=JSValue::function(method4,{module},"getCandidateFn",1);function.setFunctionSourceText("getCandidateFn( builder ) {\r\n\r\n\t\tconst params = this.parametersNodes;\r\n\r\n\t\tlet candidateFn = this._candidateFn;\r\n\r\n\t\tif ( candidateFn === null ) {\r\n\r\n\t\t\tlet bestCandidateFn = null;\r\n\t\t\tlet bestScore = - 1;\r\n\r\n\t\t\tfor ( const functionNode of this.functionNodes ) {\r\n\r\n\t\t\t\tconst shaderNode = functionNode.shaderNode;\r\n\t\t\t\tconst layout = shaderNode.layout;\r\n\r\n\t\t\t\tif ( layout === null ) {\r\n\r\n\t\t\t\t\tthrow new Error( 'THREE.FunctionOverloadingNode: FunctionNode must be a layout.' );\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t\tconst inputs = layout.inputs;\r\n\r\n\t\t\t\tif ( params.length === inputs.length ) {\r\n\r\n\t\t\t\t\tlet currentScore = 0;\r\n\r\n\t\t\t\t\tfor ( let i = 0; i < params.length; i ++ ) {\r\n\r\n\t\t\t\t\t\tconst param = params[ i ];\r\n\t\t\t\t\t\tconst input = inputs[ i ];\r\n\r\n\t\t\t\t\t\tif ( param.getNodeType( builder ) === input.type ) {\r\n\r\n\t\t\t\t\t\t\tcurrentScore ++;\r\n\r\n\t\t\t\t\t\t}\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t\tif ( currentScore > bestScore ) {\r\n\r\n\t\t\t\t\t\tbestCandidateFn = functionNode;\r\n\t\t\t\t\t\tbestScore = currentScore;\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t\tthis._candidateFn = candidateFn = bestCandidateFn;\r\n\r\n\t\t}\r\n\r\n\t\treturn candidateFn;\r\n\r\n\t}");JSPropertyDescriptor d;d.value=function;d.configurable=true;d.writable=true;prototype.defineProperty("getCandidateFn",d);}
{auto function=JSValue::function(method5,{module},"setup",1);function.setFunctionSourceText("setup( builder ) {\r\n\r\n\t\tconst candidateFn = this.getCandidateFn( builder );\r\n\r\n\t\treturn candidateFn( ...this.parametersNodes );\r\n\r\n\t}");JSPropertyDescriptor d;d.value=function;d.configurable=true;d.writable=true;prototype.defineProperty("setup",d);}
own(module,"overloadingBaseFn",([&]()->JSValue{const auto nativeStatementTemp166=module.get("nodeProxy");const JSValue nativeStatementTemp167;Args nativeStatementTemp168;nativeStatementTemp168.push_back(module.get("FunctionOverloadingNode"));return nativeStatementTemp166.call(nativeStatementTemp167,nativeStatementTemp168);}()));own(module,"overloadingFn",([&]()->JSValue{auto nativeStatementTemp165=JSValue::function(nested0,{module},"overloadingFn",1);nativeStatementTemp165.setFunctionSourceText("( functionNodes ) => ( ...params ) => overloadingBaseFn( functionNodes, ...params )");return nativeStatementTemp165;}()));auto facade=JSValue::object();own(facade,"default",constructor);own(facade,"overloadingFn",module.get("overloadingFn"));module.setInternalSlot("facade",facade);module.setInternalSlot("status","evaluated");return facade;}catch(...){module.setInternalSlot("error",nativeExceptionValue(std::current_exception()));module.setInternalSlot("status","failed");throw JSThrownValue(module.getInternalSlot("error"));}}
JSValue nativeFunctionOverloadingNodeBinding(const std::string& name){const auto keeper=nativeFunctionOverloadingNodeModule();const auto module=FunctionOverloadingManaged::lexical().lock();return module->get(name=="default"?"FunctionOverloadingNode":name);}
JSValue nativeFunctionOverloadingNodeConstructor(){return nativeFunctionOverloadingNodeBinding("default");}
