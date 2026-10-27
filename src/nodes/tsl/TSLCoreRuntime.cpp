#include "nodes/tsl/TSLCoreRuntime.h"
#include "nodes/tsl/TSLShaderNodes.h"
#include "nodes/core/Node.h"
#include "nodes/core/ConstNode.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/core/StackNode.h"
#include "nodes/core/StackTrace.h"
#include "nodes/utils/ArrayElementNode.h"
#include "nodes/utils/ConvertNode.h"
#include "nodes/utils/JoinNode.h"
#include "nodes/utils/SplitNode.h"
#include "nodes/utils/SetNode.h"
#include "nodes/utils/FlipNode.h"
#include "nodes/utils/MemberNode.h"
#include "core/JSMap.h"
#include "core/JSProxy.h"
#include "core/JSIterator.h"
#include "core/JSRegExp.h"
#include "utils.h"
#include <algorithm>
#include <unordered_set>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i,const JSValue& fallback={}) { return i<args.size()&&!args[i].isUndefined()?args[i]:fallback; }
struct Invocation {JSValue object,method;JSValue operator()(const Args& args={})const{return method.call(object,args);}};
Invocation invoke(const JSValue& object,const char* key){return {object,object.get(key)};}
void own(JSValue object,const JSValue& key,const JSValue& value) {JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
JSValue rest(const Args& args){auto array=JSValue::array();for(std::size_t i=0;i<args.size();++i)own(array,std::to_string(i),args[i]);return array;}
Args spread(const JSValue& iterable){Args values;auto iterator=getJSIterator(iterable);while(const auto next=stepJSIterator(iterator))values.push_back(*next);return values;}
template<class F>void each(const JSValue& iterable,F body) {
    auto iterator=getJSIterator(iterable);while(const auto next=stepJSIterator(iterator)){try{body(*next);}catch(...){closeJSIterator(iterator,std::current_exception());}}
}
template<class F>void forIn(JSValue object,F body) {
    if(object.isUndefined()||object.isNull())return;
    if(!object.isObject()) {
        if(object.type()==JSValue::Type::String)for(double i=0;i<object.get("length").toNumber();++i)body(JSValue(i).toString());return;
    }
    std::unordered_set<std::string> seen;std::vector<std::string> candidates;
    for(auto level=object;!level.isNull();level=level.getPrototype()) {
        for(const auto& key:level.ownPropertyNames()) {
            if(seen.contains(key))continue;const auto d=level.getOwnPropertyDescriptor(key);if(!d)continue;seen.insert(key);
            if(d->enumerable.value_or(false))candidates.push_back(key);
        }
    }
    for(const auto& key:candidates)if(object.has(key))body(key);
}
JSValue plus(const JSValue& a,const JSValue& b) {
    const auto x=a.toPrimitive(),y=b.toPrimitive();
    return x.type()==JSValue::Type::String||y.type()==JSValue::Type::String?JSValue(x.toString()+y.toString()):JSValue(x.toNumber()+y.toNumber());
}
JSValue normalizedSwizzle(const std::string& text) {
    JSValue value=text;
    for(const auto& [pattern,replacement]:std::initializer_list<std::pair<const char*,const char*>>{{"r|s","x"},{"g|t","y"},{"b|p","z"},{"a|q","w"}}) {
        const auto method=invoke(value,"replace");const auto re=createNativeRegExp(pattern,"g");value=method({re,replacement});
    }
    return invoke(invoke(invoke(value,"split")({""}),"sort")(),"join")({""});
}

void installPrototype(const JSValue& module,const JSValue& elements) {
    auto proto=nativeNodeConstructor().get("prototype");
    proto.set("assign",JSValue::function([](const JSValue& self,const Args& args,const Args& cap)->JSValue {
        if(!self.get("isStackNode").strictEquals(true)) {
            const auto stack=nativeGetCurrentStack();
            if(!stack.isNull()){const auto method=invoke(stack,"assign");Args params{self};params.insert(params.end(),args.begin(),args.end());method(params);}
            else ThreeUtils::error({"TSL: No stack defined for assign operation. Make sure the assign is inside a Fn().",nativeStackTraceConstructor().construct()});
            return self;
        }
        const auto element=invoke(cap[0],"get")({"assign"});const auto add=invoke(self,"addToStack");return add({element.call({},args)});
    },{elements},"",0));
    proto.set("toVarIntent",JSValue::function([](const JSValue& self,const Args&,const Args&)->JSValue{return self;},{},"",0));
    proto.set("get",JSValue::function([](const JSValue& self,const Args& args,const Args&)->JSValue{return nativeMemberNodeConstructor().construct({self,arg(args,0)});},{},"",1));
    // Object.defineProperties observes numeric keys first. Gather descriptors
    // before installing them, preserving alias getter/setter identity.
    auto descriptors=JSValue::object();
    const auto swizzle=[&](const std::string& key,const std::string& aliasA,const std::string& aliasB) {
        auto descriptor=JSValue::object();
        own(descriptor,"get",JSValue::function([](const JSValue& receiver,const Args&,const Args& cap)->JSValue {
            auto self=receiver;const auto old=self.get("_cache");self.set("_cache",old.toBoolean()?old:JSValue::object());
            auto split=self.get("_cache").get(cap[0]);
            if(split.isUndefined()){split=nativeSplitNodeConstructor().construct({self,cap[0]});auto cache=self.get("_cache");cache.set(cap[0],split);}return split;
        },{key},"get",0));
        own(descriptor,"set",JSValue::function([](const JSValue& self,const Args& args,const Args& cap)->JSValue {
            const auto method=invoke(self.get(cap[0]),"assign");const auto value=cap[1].call({}, {arg(args,0)});method({value});return {};
        },{key,module.get("nodeObject")},"set",1));
        own(descriptors,aliasB,descriptor);own(descriptors,aliasA,descriptor);own(descriptors,key,descriptor);
        auto upper=key,upperA=aliasA,upperB=aliasB;for(auto* s:{&upper,&upperA,&upperB})for(auto& c:*s)c=static_cast<char>(c-'a'+'A');
        const auto set=JSValue::function([](const JSValue& self,const Args& args,const Args& cap)->JSValue {
            const auto channels=normalizedSwizzle(cap[0].string());const auto value=cap[1].call({}, {arg(args,0)});return nativeSetNodeConstructor().construct({self,channels,value});
        },{key,module.get("nodeObject")},"",1);
        const auto flip=JSValue::function([](const JSValue& self,const Args&,const Args& cap)->JSValue{return nativeFlipNodeConstructor().construct({self,normalizedSwizzle(cap[0].string())});},{key},"",0);
        // Assignment chains evaluate the rightmost assignment first.
        for(const auto& property:{upperB,upperA,upper})proto.set("set"+property,set);
        for(const auto& property:{upperB,upperA,upper})proto.set("flip"+property,flip);
    };
    const auto build=[&](auto&& recurse,std::string key,std::string a,std::string b,int depth)->void {
        swizzle(key,a,b);if(depth==4)return;
        for(int i=0;i<4;++i)recurse(recurse,key+"xyzw"[i],a+"rgba"[i],b+"stpq"[i],depth+1);
    };
    for(int i=0;i<4;++i)build(build,std::string(1,"xyzw"[i]),std::string(1,"rgba"[i]),std::string(1,"stpq"[i]),1);
    for(int i=0;i<32;++i) {
        auto descriptor=JSValue::object();
        own(descriptor,"get",JSValue::function([](const JSValue& receiver,const Args&,const Args& cap)->JSValue {
            auto self=receiver;const auto old=self.get("_cache");self.set("_cache",old.toBoolean()?old:JSValue::object());auto element=self.get("_cache").get(cap[0]);
            if(element.isUndefined()){element=nativeArrayElementNodeConstructor().construct({self,nativeConstNodeConstructor().construct({cap[0],"uint"})});auto cache=self.get("_cache");cache.set(cap[0],element);}return element;
        },{i},"get",0));
        own(descriptor,"set",JSValue::function([](const JSValue& self,const Args& args,const Args& cap)->JSValue {
            const auto method=invoke(self.get(cap[0]),"assign");const auto value=cap[1].call({}, {arg(args,0)});method({value});return {};
        },{i,module.get("nodeObject")},"set",1));
        own(descriptors,std::to_string(i),descriptor);
    }
    for(const auto& key:descriptors.ownPropertyNames()) {
        const auto descriptor=descriptors.get(key);JSPropertyDescriptor d;d.getter=descriptor.get("get");d.setter=descriptor.get("set");proto.defineProperty(key,d);
    }
}
JSValue proxyFactory(const JSValue& module,const Args& args) {
    const auto constructor=arg(args,0),scope=arg(args,1,nullptr);auto factor=arg(args,2,nullptr);const auto settings=arg(args,3,nullptr);
    auto state=JSValue::object(nullptr);state.set("constructor",constructor);state.set("scope",scope);state.set("settings",settings);state.set("name",scope);state.set("min",{});state.set("max",{});
    if(!scope.isNull()&&!factor.isNull())factor=module.get("nodeObject").call({}, {factor});state.set("factor",factor);
    const auto fn=JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {
        const auto state=cap[0];auto params=rest(args);const auto name=state.get("name");JSValue tslName;
        if(name.toBoolean()){const auto re=createNativeRegExp("[a-z]","i");tslName=invoke(re,"test")({name}).toBoolean()?plus(name,"()"):name;}else tslName=state.get("constructor").get("type");
        const auto min=state.get("min"),max=state.get("max");
        if(!min.isUndefined()&&params.get("length").toNumber()<min.toNumber()) {
            ThreeUtils::error({"TSL: \""+tslName.toString()+"\" parameter length is less than minimum required.",nativeStackTraceConstructor().construct()});
            const auto concat=invoke(params,"concat");const auto missing=JSValue::intrinsicArrayConstructor().construct({min.toNumber()-params.get("length").toNumber()});
            params=concat({invoke(missing,"fill")({0})});
        } else if(!max.isUndefined()&&params.get("length").toNumber()>max.toNumber()) {
            ThreeUtils::error({"TSL: \""+tslName.toString()+"\" parameter length exceeds limit.",nativeStackTraceConstructor().construct()});params=invoke(params,"slice")({0,max});
        }
        const auto converted=cap[1].call({}, {params});const auto values=spread(converted);Args ctorArgs;
        const auto scope=state.get("scope"),factor=state.get("factor");if(!scope.isNull())ctorArgs.push_back(scope);ctorArgs.insert(ctorArgs.end(),values.begin(),values.end());if(!scope.isNull()&&!factor.isNull())ctorArgs.push_back(factor);
        auto node=state.get("constructor").construct(ctorArgs);const auto settings=state.get("settings");
        if(!settings.isNull()) {
            JSValue::intrinsicObjectConstructor().get("assign").call(JSValue::intrinsicObjectConstructor(),{node,settings});
            if(settings.get("intent").strictEquals(true))node=invoke(node,"toVarIntent")();
        }
        return node;
    },{state,module.get("nodeArray")},"fn",0);
    state.set("fn",fn);
    own(fn,"setParameterLength",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {
        auto state=cap[0];
        if(args.size()==1){state.set("max",args[0]);state.set("min",args[0]);}
        else if(args.size()==2){
            auto iterator=getJSIterator(rest(args));
            const auto next=[&]()->JSValue {
                if(iterator.done)return {};
                const auto value=stepJSIterator(iterator);return value?*value:JSValue{};
            };
            try {state.set("min",next());state.set("max",next());}
            catch(...){if(!iterator.done)closeJSIterator(iterator,std::current_exception());throw;}
            if(!iterator.done)closeJSIterator(iterator);
        }
        return state.get("fn");
    },{state},"",0));
    own(fn,"setName",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {auto state=cap[0];state.set("name",arg(args,0));return state.get("fn");},{state},"",1));
    return fn;
}
}

void installNativeTSLCoreExtensions(const JSValue& module,const JSValue& constNode,const JSValue& cacheMaps) {
    const auto elements=createNativeMap();
    const auto objectsConstructor=JSValue::constructor([](const JSValue&,const Args& args,const Args& cap)->JSValue {
        auto objects=arg(args,0);const auto altType=arg(args,1,nullptr);forIn(objects,[&](const JSValue& key){const auto value=objects.get(key);objects.set(key,cap[0].call({}, {value,altType}));});return objects;
    },{module.get("nodeObject")},"ShaderNodeObjects",1,JSValue::ConstructorKind::Callable);
    own(module,"nodeObjects",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue{return cap[0].construct({arg(args,0),arg(args,1,nullptr)});},{objectsConstructor},"nodeObjects",1));
    const auto arrayConstructor=JSValue::constructor([](const JSValue&,const Args& args,const Args& cap)->JSValue {
        auto array=arg(args,0);const auto altType=arg(args,1,nullptr),length=array.get("length");
        for(double i=0;i<length.toNumber();++i){const auto key=JSValue(i).toString();const auto value=array.get(key);array.set(key,cap[0].call({}, {value,altType}));}return array;
    },{module.get("nodeObject")},"ShaderNodeArray",1,JSValue::ConstructorKind::Callable);
    own(module,"nodeArray",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue{return cap[0].construct({arg(args,0),arg(args,1,nullptr)});},{arrayConstructor},"nodeArray",1));
    own(module,"nodeObjectIntent",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {
        return invoke(cap[0].call({}, {arg(args,0),arg(args,1,nullptr)}),"toVarIntent")();
    },{module.get("nodeObject")},"nodeObjectIntent",1));
    own(module,"nodeProxy",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue{return proxyFactory(cap[0],args);},{module},"nodeProxy",1));
    own(module,"nodeImmutable",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {
        const auto values=rest(Args(args.begin()+std::min<std::size_t>(1,args.size()),args.end()));return arg(args,0).construct(spread(cap[0].call({}, {values})));
    },{module.get("nodeArray")},"nodeImmutable",1));
    own(module,"nodeProxyIntent",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {
        auto settings=JSValue::object();const auto input=arg(args,3,JSValue::object());
        if(input.isObject())for(const auto& key:input.ownPropertyKeys()){const auto d=input.getOwnPropertyDescriptor(key);if(d&&d->enumerable.value_or(false))own(settings,key,input.get(key));}
        else if(input.type()==JSValue::Type::String)for(double i=0;i<input.get("length").toNumber();++i){const auto key=JSValue(i).toString();own(settings,key,input.get(key));}
        own(settings,"intent",true);return proxyFactory(cap[0],{arg(args,0),arg(args,1,nullptr),arg(args,2,nullptr),settings});
    },{module},"nodeProxyIntent",1));
    own(module,"nodeProxyConstructor",JSValue::function([](const JSValue&,const Args& args,const Args&)->JSValue {
        auto handler=JSValue::object();const auto node=arg(args,1);
        own(handler,"get",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue{return cap[0].get(arg(args,1),arg(args,2));},{node},"get",3));
        own(handler,"set",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue{return cap[0].reflectSet(arg(args,1),arg(args,2),cap[0]);},{node},"set",3));
        return createNativeProxy(arg(args,0),handler);
    },{},"nodeProxyConstructor",2));
    own(module,"addMethodChaining",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {
        const auto name=arg(args,0),element=arg(args,1),elements=cap[0];
        if(invoke(elements,"has")({name}).toBoolean()){ThreeUtils::warn({"TSL: Redefinition of method chaining '"+name.toString()+"'."});return {};}
        if(!element.isCallable())throw std::runtime_error("THREE.TSL: Node element "+name.toString()+" is not a function");
        invoke(elements,"set")({name,element});if(name.strictEquals("assign"))return {};
        auto proto=nativeNodeConstructor().get("prototype");
        proto.set(name,JSValue::constructor([](const JSValue& self,const Args& args,const Args& cap)->JSValue {
            if(self.get("isStackNode").toBoolean()){const auto add=invoke(self,"addToStack");return add({cap[0].call({},args)});}
            Args values{self};values.insert(values.end(),args.begin(),args.end());return cap[0].call({},values);
        },{element},"",0,JSValue::ConstructorKind::Callable));
        proto.set(plus(name,"Assign"),JSValue::constructor([](const JSValue& self,const Args& args,const Args& cap)->JSValue {
            const auto stack=self.get("isStackNode").toBoolean();const auto assign=invoke(self,"assign");
            if(stack)return assign({arg(args,0),cap[0].call({},args)});
            Args values{self};values.insert(values.end(),args.begin(),args.end());return assign({cap[0].call({},values)});
        },{element},"",0,JSValue::ConstructorKind::Callable));return {};
    },{elements},"addMethodChaining",2));
    installPrototype(module,elements);
    own(module,"defined",JSValue::function([](const JSValue&,const Args& args,const Args&)->JSValue {
        auto state=JSValue::object(nullptr);state.set("value",arg(args,0));const auto value=state.get("value");
        if(value.toBoolean()&&value.get("isNode").toBoolean())invoke(value,"traverse")({JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {const auto node=arg(args,0);if(node.get("isConstNode").toBoolean()){auto state=cap[0];state.set("value",node.get("value"));}return {};},{state},"",1)});
        return state.get("value").toBoolean();
    },{},"defined",1));
    installNativeTSLShaderExports(module);
    const auto convertType=[&](const char* type,JSValue cache) {
        return JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {
            const auto type=cap[0],cache=cap[1];auto params=rest(args);
            auto iterator=getJSIterator(params);
            while(const auto param=stepJSIterator(iterator)) {
                if(param->isUndefined()) {
                    JSValue result;
                    try {
                        ThreeUtils::error({"TSL: Invalid parameter for the type \""+type.toString()+"\".",nativeStackTraceConstructor().construct()});
                        result=nativeConstNodeConstructor().construct({0,type});
                    } catch(...) {closeJSIterator(iterator,std::current_exception());}
                    closeJSIterator(iterator);return result;
                }
            }
            bool makeDefault=params.get("length").strictEquals(0);
            if(!makeDefault){
                const auto scalar=invoke(rest({"bool","float","int","uint"}),"includes")({type}).toBoolean();
                if(!scalar)makeDefault=invoke(params,"every")({JSValue::function([](const JSValue&,const Args& args,const Args&)->JSValue {const auto value=arg(args,0);return !value.isObject()&&!value.isNull();},{},"",1)}).toBoolean();
            }
            if(makeDefault){const auto values=spread(params);Args callArgs{type};callArgs.insert(callArgs.end(),values.begin(),values.end());params=rest({cap[2].call({},callArgs)});}
            if(params.get("length").strictEquals(1)&&!cache.isNull()&&invoke(cache,"has")({params.get("0")}).toBoolean())return cap[4].call({}, {invoke(cache,"get")({params.get("0")})});
            if(params.get("length").strictEquals(1)) {
                const auto node=cap[3].call({}, {params.get("0"),type});
                if(node.get("nodeType").strictEquals(type))return cap[4].call({}, {node});
                return cap[4].call({}, {nativeConvertNodeConstructor().construct({node,type})});
            }
            const auto nodes=invoke(params,"map")({JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue{return cap[0].call({}, {arg(args,0)});},{cap[3]},"",1)});
            return cap[4].call({}, {nativeJoinNodeConstructor().construct({nodes,type})});
        },{type,cache,nativeNodeUtilsBinding("getValueFromType"),constNode,module.get("nodeObjectIntent")},"",0);
    };
    for(const auto* type:{"color","float","int","uint","bool","vec2","ivec2","uvec2","bvec2","vec3","ivec3","uvec3","bvec3","vec4","ivec4","uvec4","bvec4","mat2","mat3","mat4"}) {
        const auto cache=cacheMaps.get(std::string(type)=="int"?"ints":type);own(module,type,convertType(type,cache.isUndefined()?JSValue(nullptr):cache));
    }
    for(const auto* name:{"color","float","int","uint","bool","vec2","ivec2","uvec2","bvec2","vec3","ivec3","uvec3","bvec3","vec4","ivec4","uvec4","bvec4","mat2","mat3","mat4"}) {
        std::string method=name;method[0]=static_cast<char>(method[0]-'a'+'A');if(method.size()>1&&(method[0]=='I'||method[0]=='U'||method[0]=='B')&&method.substr(1,3)=="vec")method[1]='V';
        module.get("addMethodChaining").call({}, {"to"+method,module.get(name)});
    }
    const auto element=module.get("nodeProxy").call({}, {nativeArrayElementNodeConstructor()});own(module,"element",invoke(element,"setParameterLength")({2}));
    own(module,"convert",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {return nativeConvertNodeConstructor().construct({cap[0].call({}, {arg(args,0)}),arg(args,1)});},{module.get("nodeObject")},"convert",2));
    own(module,"split",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {return nativeSplitNodeConstructor().construct({cap[0].call({}, {arg(args,0)}),arg(args,1)});},{module.get("nodeObject")},"split",2));
    module.get("addMethodChaining").call({}, {"element",module.get("element")});module.get("addMethodChaining").call({}, {"convert",module.get("convert")});
}
