// Port of pinned src/nodes/math/MathNode.js; all callbacks are native C++.
#include "nodes/math/MathNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"
#include "core/JSSet.h"
#include "constants.h"
#include "utils.h"
#include <limits>
#include <numbers>
#include <algorithm>
#include <iterator>
#include <string_view>

namespace {
using namespace UtilityNodeRuntime;
// Literal metadata from pinned src/nodes/math/MathNode.js.
static constexpr std::pair<const char*,const char*> constants[] = {
    {"ALL","all"},
    {"ANY","any"},
    {"RADIANS","radians"},
    {"DEGREES","degrees"},
    {"EXP","exp"},
    {"EXP2","exp2"},
    {"LOG","log"},
    {"LOG2","log2"},
    {"SQRT","sqrt"},
    {"INVERSE_SQRT","inversesqrt"},
    {"FLOOR","floor"},
    {"CEIL","ceil"},
    {"NORMALIZE","normalize"},
    {"FRACT","fract"},
    {"SIN","sin"},
    {"SINH","sinh"},
    {"COS","cos"},
    {"COSH","cosh"},
    {"TAN","tan"},
    {"TANH","tanh"},
    {"ASIN","asin"},
    {"ASINH","asinh"},
    {"ACOS","acos"},
    {"ACOSH","acosh"},
    {"ATAN","atan"},
    {"ATANH","atanh"},
    {"ABS","abs"},
    {"SIGN","sign"},
    {"LENGTH","length"},
    {"NEGATE","negate"},
    {"ONE_MINUS","oneMinus"},
    {"DFDX","dFdx"},
    {"DFDY","dFdy"},
    {"ROUND","round"},
    {"RECIPROCAL","reciprocal"},
    {"TRUNC","trunc"},
    {"FWIDTH","fwidth"},
    {"TRANSPOSE","transpose"},
    {"DETERMINANT","determinant"},
    {"INVERSE","inverse"},
    {"EQUALS","equals"},
    {"MIN","min"},
    {"MAX","max"},
    {"STEP","step"},
    {"REFLECT","reflect"},
    {"DISTANCE","distance"},
    {"DIFFERENCE","difference"},
    {"DOT","dot"},
    {"CROSS","cross"},
    {"POW","pow"},
    {"TRANSFORM_DIRECTION","transformDirection"},
    {"MIX","mix"},
    {"CLAMP","clamp"},
    {"REFRACT","refract"},
    {"SMOOTHSTEP","smoothstep"},
    {"FACEFORWARD","faceforward"},
};
static constexpr const char* floatMethods[] = {
    "RADIANS",
    "DEGREES",
    "EXP",
    "EXP2",
    "LOG",
    "LOG2",
    "SQRT",
    "INVERSE_SQRT",
    "FLOOR",
    "CEIL",
    "NORMALIZE",
    "FRACT",
    "SIN",
    "SINH",
    "COS",
    "COSH",
    "TAN",
    "TANH",
    "ASIN",
    "ASINH",
    "ACOS",
    "ACOSH",
    "ATAN",
    "ATANH",
    "LENGTH",
    "DFDX",
    "DFDY",
    "ROUND",
    "TRUNC",
    "FWIDTH",
    "RECIPROCAL",
    "STEP",
    "REFLECT",
    "DISTANCE",
    "DOT",
    "CROSS",
    "POW",
    "MIX",
    "REFRACT",
    "SMOOTHSTEP",
    "FACEFORWARD",
};
struct ProxyDefinition {const char* name;const char* method;double min,max;};
static constexpr ProxyDefinition proxies[] = {
    {"all","ALL",1,1},
    {"any","ANY",1,1},
    {"radians","RADIANS",1,1},
    {"degrees","DEGREES",1,1},
    {"exp","EXP",1,1},
    {"exp2","EXP2",1,1},
    {"log","LOG",1,1},
    {"log2","LOG2",1,1},
    {"sqrt","SQRT",1,1},
    {"inverseSqrt","INVERSE_SQRT",1,1},
    {"floor","FLOOR",1,1},
    {"ceil","CEIL",1,1},
    {"normalize","NORMALIZE",1,1},
    {"fract","FRACT",1,1},
    {"sin","SIN",1,1},
    {"sinh","SINH",1,1},
    {"cos","COS",1,1},
    {"cosh","COSH",1,1},
    {"tan","TAN",1,1},
    {"tanh","TANH",1,1},
    {"asin","ASIN",1,1},
    {"asinh","ASINH",1,1},
    {"acos","ACOS",1,1},
    {"acosh","ACOSH",1,1},
    {"atan","ATAN",1,2},
    {"atanh","ATANH",1,1},
    {"abs","ABS",1,1},
    {"sign","SIGN",1,1},
    {"length","LENGTH",1,1},
    {"negate","NEGATE",1,1},
    {"oneMinus","ONE_MINUS",1,1},
    {"dFdx","DFDX",1,1},
    {"dFdy","DFDY",1,1},
    {"round","ROUND",1,1},
    {"reciprocal","RECIPROCAL",1,1},
    {"trunc","TRUNC",1,1},
    {"fwidth","FWIDTH",1,1},
    {"transpose","TRANSPOSE",1,1},
    {"determinant","DETERMINANT",1,1},
    {"inverse","INVERSE",1,1},
    {"min","MIN",2,std::numeric_limits<double>::infinity()},
    {"max","MAX",2,std::numeric_limits<double>::infinity()},
    {"step","STEP",2,2},
    {"reflect","REFLECT",2,2},
    {"distance","DISTANCE",2,2},
    {"difference","DIFFERENCE",2,2},
    {"dot","DOT",2,2},
    {"cross","CROSS",2,2},
    {"pow","POW",2,2},
    {"transformDirection","TRANSFORM_DIRECTION",2,2},
    {"mix","MIX",3,3},
    {"refract","REFRACT",3,3},
    {"smoothstep","SMOOTHSTEP",3,3},
    {"faceForward","FACEFORWARD",3,3},
};
static constexpr std::pair<const char*,const char*> chaining[] = {
    {"all","all"},
    {"any","any"},
    {"radians","radians"},
    {"degrees","degrees"},
    {"exp","exp"},
    {"exp2","exp2"},
    {"log","log"},
    {"log2","log2"},
    {"sqrt","sqrt"},
    {"inverseSqrt","inverseSqrt"},
    {"floor","floor"},
    {"ceil","ceil"},
    {"normalize","normalize"},
    {"fract","fract"},
    {"sin","sin"},
    {"sinh","sinh"},
    {"cos","cos"},
    {"cosh","cosh"},
    {"tan","tan"},
    {"tanh","tanh"},
    {"asin","asin"},
    {"asinh","asinh"},
    {"acos","acos"},
    {"acosh","acosh"},
    {"atan","atan"},
    {"atanh","atanh"},
    {"abs","abs"},
    {"sign","sign"},
    {"length","length"},
    {"lengthSq","lengthSq"},
    {"negate","negate"},
    {"oneMinus","oneMinus"},
    {"dFdx","dFdx"},
    {"dFdy","dFdy"},
    {"round","round"},
    {"reciprocal","reciprocal"},
    {"trunc","trunc"},
    {"fwidth","fwidth"},
    {"min","min"},
    {"max","max"},
    {"step","stepElement"},
    {"reflect","reflect"},
    {"distance","distance"},
    {"dot","dot"},
    {"cross","cross"},
    {"pow","pow"},
    {"pow2","pow2"},
    {"pow3","pow3"},
    {"pow4","pow4"},
    {"transformDirection","transformDirection"},
    {"transformNormalByViewMatrix","transformNormalByViewMatrix"},
    {"transformNormalByInverseViewMatrix","transformNormalByInverseViewMatrix"},
    {"mix","mixElement"},
    {"clamp","clamp"},
    {"refract","refract"},
    {"smoothstep","smoothstepElement"},
    {"faceForward","faceForward"},
    {"difference","difference"},
    {"saturate","saturate"},
    {"cbrt","cbrt"},
    {"transpose","transpose"},
    {"determinant","determinant"},
    {"inverse","inverse"},
    {"rand","rand"},
};
static constexpr const char* declarationOrder[] = {
    "all",
    "any",
    "radians",
    "degrees",
    "exp",
    "exp2",
    "log",
    "log2",
    "sqrt",
    "inverseSqrt",
    "floor",
    "ceil",
    "normalize",
    "fract",
    "sin",
    "sinh",
    "cos",
    "cosh",
    "tan",
    "tanh",
    "asin",
    "asinh",
    "acos",
    "acosh",
    "atan",
    "atanh",
    "abs",
    "sign",
    "length",
    "negate",
    "oneMinus",
    "dFdx",
    "dFdy",
    "round",
    "reciprocal",
    "trunc",
    "fwidth",
    "transpose",
    "determinant",
    "inverse",
    "min",
    "max",
    "step",
    "reflect",
    "distance",
    "difference",
    "dot",
    "cross",
    "pow",
    "pow2",
    "pow3",
    "pow4",
    "transformDirection",
    "transformNormalByViewMatrix",
    "transformNormalByInverseViewMatrix",
    "cbrt",
    "lengthSq",
    "mix",
    "clamp",
    "saturate",
    "refract",
    "smoothstep",
    "faceForward",
};
JSValue mathState();
JSValue array(const Args& args={}) {
    auto result=JSValue::array();
    for(std::size_t i=0;i<args.size();++i){JSPropertyDescriptor d;d.value=args[i];d.writable=d.enumerable=d.configurable=true;result.defineProperty(std::to_string(i),d);}
    return result;
}
JSValue bindings(const JSValue& state) {
    const auto value=state.get("bindings");
    if(!value.hasOwn("rand"))nativeMathNodeModule();
    return value;
}
JSValue function(const JSValue& state,const char* name){return bindings(state).get(name);}
JSValue method(const JSValue& self,const Args& args,const Args& c) {
    const auto name=c[0].string();const auto builder=arg(args,0),state=c[2],ctor=state.get("constructor");
    const auto is=[&](const JSValue& value,const char* key){return value.strictEquals(ctor.get(key));};
    if(name=="getInputType") {
        const auto aType=invoke(self.get("aNode"),"getNodeType")({builder});
        const auto bType=self.get("bNode").toBoolean()?invoke(self.get("bNode"),"getNodeType")({builder}):JSValue(nullptr);
        const auto cType=self.get("cNode").toBoolean()?invoke(self.get("cNode"),"getNodeType")({builder}):JSValue(nullptr);
        const auto aLen=invoke(builder,"isMatrix")({aType}).toBoolean()?JSValue(0):invoke(builder,"getTypeLength")({aType});
        const auto bLen=invoke(builder,"isMatrix")({bType}).toBoolean()?JSValue(0):invoke(builder,"getTypeLength")({bType});
        const auto cLen=invoke(builder,"isMatrix")({cType}).toBoolean()?JSValue(0):invoke(builder,"getTypeLength")({cType});
        JSValue type;
        if(greater(aLen,bLen)&&greater(aLen,cLen))type=aType;
        else if(greater(bLen,cLen))type=bType;
        else if(greater(cLen,aLen))type=cType;
        else type=aType;
        if(invoke(builder,"isMatrix")({type}).toBoolean())return type;
        const auto has=invoke(state.get("floatMethods"),"has");const auto floatOnly=has({self.get("method")}).toBoolean();JSValue promoted;
        if(floatOnly)promoted="float";
        else {const auto fn=invoke(builder,"getPromotedComponentType");const auto a=self.get("aNode"),b=self.get("bNode"),cc=self.get("cNode");promoted=fn({a,b,cc});}
        return invoke(builder,"changeComponentType")({type,promoted});
    }
    if(name=="generateNodeType") {
        const auto method=self.get("method");
        if(is(method,"LENGTH")||is(method,"DISTANCE")||is(method,"DOT")||is(method,"DETERMINANT"))return "float";
        if(is(method,"CROSS"))return "vec3";
        if(is(method,"ALL")||is(method,"ANY"))return "bool";
        if(is(method,"EQUALS")){const auto change=invoke(builder,"changeComponentType");const auto type=invoke(self.get("aNode"),"getNodeType")({builder});return change({type,"bool"});}
        return invoke(self,"getInputType")({builder});
    }
    if(name=="setup") {
        const auto a=self.get("aNode"),b=self.get("bNode"),method=self.get("method");JSValue output=nullptr;
        if(is(method,"ONE_MINUS"))output=function(state,"sub").call({}, {1,a});
        else if(is(method,"RECIPROCAL")){const auto div=function(state,"div"),value=function(state,"float").call({}, {1});output=div.call({}, {value,a});}
        else if(is(method,"DIFFERENCE")){const auto abs=function(state,"abs"),value=function(state,"sub").call({}, {a,b});output=abs.call({}, {value});}
        else if(is(method,"TRANSFORM_DIRECTION")) {
            const auto matrix=invoke(builder,"isMatrix");const auto aType=invoke(a,"getNodeType")({builder});const auto useA=matrix({aType}).toBoolean();const auto matrixNode=useA?a:b,direction=useA?b:a;
            const auto normalize=function(state,"normalize"),mul=function(state,"mul"),vec4=function(state,"vec4"),v3=function(state,"vec3").call({}, {direction});
            const auto vector=vec4.call({}, {v3,0});const auto product=mul.call({}, {matrixNode,vector});output=normalize.call({}, {product.get("xyz")});
        }
        if(!output.isNull())return output;
        return c[1].getPrototype().get("setup",self).call(self,{builder});
    }
    if(name=="generate") {
        const auto properties=invoke(builder,"getNodeProperties")({self});
        if(properties.get("outputNode").toBoolean())return c[1].getPrototype().get("generate",self).call(self,{builder,arg(args,1)});
        auto method=self.get("method");const auto type=invoke(self,"getNodeType")({builder}),inputType=invoke(self,"getInputType")({builder});
        const auto a=self.get("aNode"),b=self.get("bNode"),cc=self.get("cNode");const auto coordinate=builder.get("renderer").get("coordinateSystem");
        if(is(method,"NEGATE")) {
            const auto format=invoke(builder,"format");const auto snippet=invoke(a,"build")({builder,inputType});
            return format({add(add("( - ",snippet)," )"),type,arg(args,1)});
        }
        auto params=JSValue::array();
        if(is(method,"CROSS")) {
            const auto push=invoke(params,"push");const auto av=invoke(a,"build")({builder,type}),bv=invoke(b,"build")({builder,type});push({av,bv});
        } else if(coordinate.strictEquals(WebGLCoordinateSystem)&&is(method,"STEP")) {
            const auto push=invoke(params,"push"),buildA=invoke(a,"build"),length=invoke(builder,"getTypeLength");
            const auto at=invoke(a,"getNodeType")({builder});const auto al=length({at});
            const auto av=buildA({builder,al.strictEquals(1)?JSValue("float"):inputType}),bv=invoke(b,"build")({builder,inputType});push({av,bv});
        } else if(coordinate.strictEquals(WebGLCoordinateSystem)&&(is(method,"MIN")||is(method,"MAX"))) {
            const auto push=invoke(params,"push");const auto av=invoke(a,"build")({builder,inputType});const auto buildB=invoke(b,"build"),length=invoke(builder,"getTypeLength");
            const auto bt=invoke(b,"getNodeType")({builder});const auto bl=length({bt});const auto bType=bl.strictEquals(1)?invoke(builder,"getComponentType")({inputType}):inputType;
            const auto bv=buildB({builder,bType});push({av,bv});
        } else if(is(method,"REFRACT")) {
            const auto push=invoke(params,"push");const auto av=invoke(a,"build")({builder,inputType}),bv=invoke(b,"build")({builder,inputType}),cv=invoke(cc,"build")({builder,"float"});push({av,bv,cv});
        } else if(is(method,"MIX")) {
            const auto push=invoke(params,"push");const auto av=invoke(a,"build")({builder,inputType}),bv=invoke(b,"build")({builder,inputType});const auto buildC=invoke(cc,"build"),length=invoke(builder,"getTypeLength");
            const auto ct=invoke(cc,"getNodeType")({builder});const auto cl=length({ct});const auto cv=buildC({builder,cl.strictEquals(1)?JSValue("float"):inputType});push({av,bv,cv});
        } else {
            if(coordinate.strictEquals(WebGPUCoordinateSystem)&&is(method,"ATAN")&&!b.isNull())method="atan2";
            if(!builder.get("shaderStage").strictEquals("fragment")&&(is(method,"DFDX")||is(method,"DFDY"))) {
                const auto methodText=method.toString(),stage=builder.get("shaderStage").toString();ThreeUtils::error({"TSL: '"+methodText+"' is not supported in the "+stage+" stage.",self.get("stackTrace")});
                method=add(add("/*",method),"*/");
            }
            auto push=invoke(params,"push");push({invoke(a,"build")({builder,inputType})});
            if(!b.isNull()){push=invoke(params,"push");push({invoke(b,"build")({builder,inputType})});}
            if(!cc.isNull()){push=invoke(params,"push");push({invoke(cc,"build")({builder,inputType})});}
        }
        const auto format=invoke(builder,"format");const auto methodName=invoke(builder,"getMethod")({method,type}).toString();const auto joined=invoke(params,"join")({", "}).toString();return format({methodName+"( "+joined+" )",type,arg(args,1)});
    }
    if(name=="serialize"||name=="deserialize") {
        c[1].getPrototype().get(name,self).call(self,{builder});
        if(name=="serialize"){auto data=builder;data.set("method",self.get("method"));}
        else {auto receiver=self;receiver.set("method",builder.get("method"));}return {};
    }
    throw std::logic_error("Unknown MathNode method");
}
JSValue mathState() {
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"MathNode class");
    auto state=JSValue::object(nullptr);state.set("bindings",JSValue::object(nullptr));
    auto constructor=makeConstructor("MathNode",2,+[](const JSValue& newTarget,const Args& args,const Args& c)->JSValue {
        auto self=superConstruct(newTarget,c);const auto method=arg(args,0);auto a=arg(args,1),b=arg(args,2,nullptr),cc=arg(args,3,nullptr);const auto ctor=nativeMathNodeConstructor();
        if((method.strictEquals(ctor.get("MAX"))||method.strictEquals(ctor.get("MIN")))&&args.size()>3) {
            auto finalOp=ctor.construct({method,a,b});for(std::size_t i=3;i<args.size()-1;++i)finalOp=ctor.construct({method,finalOp,args[i]});a=finalOp;b=args.back();cc=nullptr;
        }
        self.set("method",method);self.set("aNode",a);self.set("bNode",b);self.set("cNode",cc);self.set("isMathNode",true);return self;
    },{{"getInputType",1},{"generateNodeType",1},{"setup",1},{"generate",2},{"serialize",1},{"deserialize",1}},method,{state});
    state.set("constructor",constructor);
    for(const auto& [key,value]:constants)constructor.set(key,value);
    auto values=JSValue::array();for(const auto* key:floatMethods)invoke(values,"push")({constructor.get(key)});
    state.set("floatMethods",nativeSetConstructor().construct({values}));
    weak=JSWeakValue(state);return state;
}
JSValue composite(const JSValue&,const Args& args,const Args& c) {
    const auto state=c[0],name=c[1];const auto a=arg(args,0),b=arg(args,1),cc=arg(args,2);
    const auto fn=[&](const char* name){return function(state,name);};
    if(name.strictEquals("pow2")||name.strictEquals("pow3")||name.strictEquals("pow4")) {
        Args params{a,a};if(!name.strictEquals("pow2"))params.push_back(a);if(name.strictEquals("pow4"))params.push_back(a);return fn("mul").call({},params);
    }
    if(name.strictEquals("transformNormalByViewMatrix")||name.strictEquals("transformNormalByInverseViewMatrix")) {
        const auto normalize=fn("normalize");JSValue product;
        if(name.strictEquals("transformNormalByViewMatrix")) {
            const auto mul=fn("mul"),v4=fn("vec4"),v3=fn("vec3").call({}, {a});const auto vector=v4.call({}, {v3,0});product=mul.call({}, {b,vector});
        }else {const auto v4=fn("vec4"),v3=fn("vec3").call({}, {a});const auto vector=v4.call({}, {v3,0});product=invoke(vector,"mul")({b});}
        return normalize.call({}, {product.get("xyz")});
    }
    if(name.strictEquals("cbrt")) {
        const auto mul=fn("mul"),sign=fn("sign").call({}, {a}),pow=fn("pow"),abs=fn("abs").call({}, {a});return mul.call({}, {sign,pow.call({}, {abs,1.0/3.0})});
    }
    if(name.strictEquals("lengthSq"))return fn("dot").call({}, {a,a});
    if(name.strictEquals("clamp")) {
        const auto ctor=state.get("constructor"),method=ctor.get("CLAMP"),nodeObject=fn("nodeObject");
        const auto av=nodeObject.call({}, {a}),bv=nodeObject.call({}, {arg(args,1,0)}),cv=nodeObject.call({}, {arg(args,2,1)});return ctor.construct({method,av,bv,cv});
    }
    if(name.strictEquals("saturate"))return fn("clamp").call({}, {a});
    if(name.strictEquals("mixElement"))return fn("mix").call({}, {b,cc,a});
    if(name.strictEquals("smoothstepElement"))return fn("smoothstep").call({}, {b,cc,a});
    if(name.strictEquals("stepElement"))return fn("step").call({}, {b,a});
    throw std::logic_error("Unknown MathNode factory");
}
}
JSValue nativeMathNodeConstructor(){return mathState().get("constructor");}
JSValue nativeMathNodeBinding(const std::string& name){const auto keeper=nativeMathNodeModule();return mathState().get("bindings").get(name);}
JSValue nativeMathNodeModule() {
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"MathNode exports");
    const auto operatorKeeper=nativeOperatorNodeModule(),tslKeeper=nativeTSLCore(),state=mathState();auto module=state.get("bindings");const auto ctor=state.get("constructor");
    for(const auto* name:{"sub","mul","div","mod"})module.set(name,nativeOperatorNodeBinding(name));
    for(const auto* name:{"nodeObject","float","vec2","vec3","vec4","Fn"})module.set(name,nativeTSLCoreBinding(name));
    own(module,"default",ctor);
    constexpr double pi=std::numbers::pi_v<double>;
    for(const auto& [key,value]:std::initializer_list<std::pair<const char*,double>>{{"EPSILON",1e-6},{"INFINITY",1e6},{"PI",pi},{"PI2",pi*2},{"TWO_PI",pi*2},{"HALF_PI",pi*0.5}})own(module,key,module.get("float").call({}, {value}));
    const auto make=[&](const char* name,unsigned arity){own(module,name,JSValue::function(composite,{state,name},name,arity));};
    for(const auto* name:declarationOrder) {
        const auto proxy=std::find_if(std::begin(proxies),std::end(proxies),[&](const auto& p){return std::string_view(p.name)==name;});
        if(proxy!=std::end(proxies)) {
            auto value=nativeTSLCoreBinding("nodeProxyIntent").call({}, {ctor,ctor.get(proxy->method)});
            value=invoke(value,"setParameterLength")(proxy->min==proxy->max?Args{proxy->min}:Args{proxy->min,proxy->max});own(module,name,value);
        } else make(name,std::string_view(name)=="transformNormalByViewMatrix"||std::string_view(name)=="transformNormalByInverseViewMatrix"?2:1);
    }
    auto layout=JSValue::object();own(layout,"uv","vec2");own(layout,"return","float");
    auto rand=module.get("Fn").call({}, {JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue {
        auto it=getJSIterator(arg(args,0));JSValue uv;
        try {if(const auto value=stepJSIterator(it))uv=*value;}catch(...){if(!it.done)closeJSIterator(it,std::current_exception());throw;}
        if(!it.done)closeJSIterator(it);
        const auto module=bindings(c[0]),dot=module.get("dot"),xy=uv.get("xy"),v2=module.get("vec2").call({}, {12.9898,78.233});
        const auto dt=dot.call({}, {xy,v2});const auto sn=module.get("mod").call({}, {dt,module.get("PI")});
        const auto fract=module.get("fract"),sine=module.get("sin").call({}, {sn});const auto product=invoke(sine,"mul")({43758.5453});return fract.call({}, {product});
    },{state},"",1),layout});own(module,"rand",rand);
    make("mixElement",3);make("smoothstepElement",3);make("stepElement",2);
    own(module,"faceforward",module.get("faceForward"));own(module,"inversesqrt",module.get("inverseSqrt"));
    for(const auto& [name,value]:chaining)nativeTSLCoreBinding("addMethodChaining").call({}, {name,module.get(value)});
    auto exports=JSValue::object();auto names=module.ownPropertyNames();std::sort(names.begin(),names.end());
    for(const auto& name:names)if(name!="sub"&&name!="mul"&&name!="div"&&name!="mod"&&name!="nodeObject"&&name!="float"&&name!="vec2"&&name!="vec3"&&name!="vec4"&&name!="Fn")own(exports,name.c_str(),module.get(name));
    module.retainForHeapLifetime();exports.retainForHeapLifetime();weak=JSWeakValue(exports);return exports;
}
