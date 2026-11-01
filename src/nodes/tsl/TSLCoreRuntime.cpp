#include "core/JSSourceTrace.h"
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
    proto.set("assign",JSValue::constructor([](const JSValue& self,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_0({"assign",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",62,25,1956,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},JSSourceTrace::receiverTypeName(self),"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",62,25,1956,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
        if(!self.get("isStackNode").strictEquals(true)) {
            const auto stack=nativeGetCurrentStack();
            if(!stack.isNull()){const auto method=invoke(stack,"assign");Args params{self};params.insert(params.end(),args.begin(),args.end());method(params);}
            else {JSSourceTrace::PositionScope sourceTraceTSLError_2122({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",72,4,2122,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"});ThreeUtils::error({"TSL: No stack defined for assign operation. Make sure the assign is inside a Fn().",([&]()->JSValue{JSSourceTrace::PositionScope sourceTraceTSLNew_2215({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",72,97,2215,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"});return nativeStackTraceConstructor().construct();}())});}
            return self;
        }
        const auto element=invoke(cap[0],"get")({"assign"});const auto add=invoke(self,"addToStack");return add({element.call({},args)});
    },{elements},"",0,JSValue::ConstructorKind::Callable));
    proto.set("toVarIntent",JSValue::constructor([](const JSValue& self,const Args&,const Args&)->JSValue{JSSourceTrace::FrameScope sourceTraceTSL_1({"toVarIntent",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",88,30,2429,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},JSSourceTrace::receiverTypeName(self),"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",88,30,2429,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});return self;},{},"",0,JSValue::ConstructorKind::Callable));
    proto.set("get",JSValue::constructor([](const JSValue& self,const Args& args,const Args&)->JSValue{JSSourceTrace::FrameScope sourceTraceTSL_2({"get",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",94,22,2490,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},JSSourceTrace::receiverTypeName(self),"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",94,22,2490,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});return nativeMemberNodeConstructor().construct({self,arg(args,0)});},{},"",1,JSValue::ConstructorKind::Callable));
    // Object.defineProperties observes numeric keys first. Gather descriptors
    // before installing them, preserving alias getter/setter identity.
    auto descriptors=JSValue::object();
    const auto swizzle=[&](const std::string& key,const std::string& aliasA,const std::string& aliasB) {
        auto descriptor=JSValue::object();
        own(descriptor,"get",JSValue::function([](const JSValue& receiver,const Args&,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_3({"get",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",112,3,2808,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},JSSourceTrace::receiverTypeName(receiver),"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",112,3,2808,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
            auto self=receiver;const auto old=self.get("_cache");self.set("_cache",old.toBoolean()?old:JSValue::object());
            auto split=self.get("_cache").get(cap[0]);
            if(split.isUndefined()){split=nativeSplitNodeConstructor().construct({self,cap[0]});auto cache=self.get("_cache");cache.set(cap[0],split);}return split;
        },{key},"get",0));
        own(descriptor,"set",JSValue::function([](const JSValue& self,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_4({"set",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",132,3,3071,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},JSSourceTrace::receiverTypeName(self),"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",132,3,3071,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
            const auto method=invoke(self.get(cap[0]),"assign");const auto value=cap[1].call({}, {arg(args,0)});method({value});return {};
        },{key,module.get("nodeObject")},"set",1));
        own(descriptors,aliasB,descriptor);own(descriptors,aliasA,descriptor);own(descriptors,key,descriptor);
        auto upper=key,upperA=aliasA,upperB=aliasB;for(auto* s:{&upper,&upperA,&upperB})for(auto& c:*s)c=static_cast<char>(c-'a'+'A');
        const auto set=JSValue::constructor([](const JSValue& self,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_5({"",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",148,116,3500,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},JSSourceTrace::receiverTypeName(self),"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",148,116,3500,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
            const auto channels=normalizedSwizzle(cap[0].string());const auto value=cap[1].call({}, {arg(args,0)});return nativeSetNodeConstructor().construct({self,channels,value});
        },{key,module.get("nodeObject")},"",1,JSValue::ConstructorKind::Callable);
        const auto flip=JSValue::constructor([](const JSValue& self,const Args&,const Args& cap)->JSValue{JSSourceTrace::FrameScope sourceTraceTSL_6({"",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",158,119,3805,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},JSSourceTrace::receiverTypeName(self),"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",158,119,3805,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});return nativeFlipNodeConstructor().construct({self,normalizedSwizzle(cap[0].string())});},{key},"",0,JSValue::ConstructorKind::Callable);
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
        own(descriptor,"get",JSValue::function([](const JSValue& receiver,const Args&,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_7({"get",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",218,3,5136,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},JSSourceTrace::receiverTypeName(receiver),"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",218,3,5136,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
            auto self=receiver;const auto old=self.get("_cache");self.set("_cache",old.toBoolean()?old:JSValue::object());auto element=self.get("_cache").get(cap[0]);
            if(element.isUndefined()){element=nativeArrayElementNodeConstructor().construct({self,nativeConstNodeConstructor().construct({cap[0],"uint"})});auto cache=self.get("_cache");cache.set(cap[0],element);}return element;
        },{i},"get",0));
        own(descriptor,"set",JSValue::function([](const JSValue& self,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_8({"set",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",238,3,5420,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},JSSourceTrace::receiverTypeName(self),"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",238,3,5420,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
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
    const auto fn=JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_9(cap[0].get("scope").isNull()?JSSourceTrace::Frame{"",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",392,8,8077,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",392,8,8077,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}}:cap[0].get("factor").isNull()?JSSourceTrace::Frame{"",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",402,8,8275,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",402,8,8275,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}}:JSSourceTrace::Frame{"",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",410,8,8429,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",410,8,8429,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
        const auto state=cap[0];auto params=rest(args);const auto name=state.get("name");JSValue tslName;
        if(name.toBoolean()){const auto re=createNativeRegExp("[a-z]","i");tslName=invoke(re,"test")({name}).toBoolean()?plus(name,"()"):name;}else tslName=state.get("constructor").get("type");
        const auto min=state.get("min"),max=state.get("max");
        if(!min.isUndefined()&&params.get("length").toNumber()<min.toNumber()) {
            {JSSourceTrace::PositionScope sourceTraceTSLError_7623({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",374,4,7623,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"});ThreeUtils::error({"TSL: \""+tslName.toString()+"\" parameter length is less than minimum required.",([&]()->JSValue{JSSourceTrace::PositionScope sourceTraceTSLNew_7701({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",374,82,7701,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"});return nativeStackTraceConstructor().construct();}())});}
            const auto concat=invoke(params,"concat");const auto missing=JSValue::intrinsicArrayConstructor().construct({min.toNumber()-params.get("length").toNumber()});
            params=concat({invoke(missing,"fill")({0})});
        } else if(!max.isUndefined()&&params.get("length").toNumber()>max.toNumber()) {
            {JSSourceTrace::PositionScope sourceTraceTSLError_7881({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",380,4,7881,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"});ThreeUtils::error({"TSL: \""+tslName.toString()+"\" parameter length exceeds limit.",([&]()->JSValue{JSSourceTrace::PositionScope sourceTraceTSLNew_7943({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",380,66,7943,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"});return nativeStackTraceConstructor().construct();}())});}params=invoke(params,"slice")({0,max});
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
    own(fn,"setParameterLength",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_10({"",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",418,26,8586,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",418,26,8586,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
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
    own(fn,"setName",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_11({"",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",427,15,8784,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",427,15,8784,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});auto state=cap[0];state.set("name",arg(args,0));return state.get("fn");},{state},"",1));
    return fn;
}
}

void installNativeTSLCoreExtensions(const JSValue& module,const JSValue& constNode,const JSValue& cacheMaps) {
    const auto elements=createNativeMap();
    const auto objectsConstructor=JSValue::constructor([](const JSValue&,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_12({"ShaderNodeObjects",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",316,27,6632,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",316,27,6632,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
        auto objects=arg(args,0);const auto altType=arg(args,1,nullptr);forIn(objects,[&](const JSValue& key){const auto value=objects.get(key);objects.set(key,cap[0].call({}, {value,altType}));});return objects;
    },{module.get("nodeObject")},"ShaderNodeObjects",1,JSValue::ConstructorKind::Callable);
    own(module,"nodeObjects",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue{JSSourceTrace::FrameScope sourceTraceTSL_13({"nodeObjects",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",994,28,20016,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",994,28,20016,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});return cap[0].construct({arg(args,0),arg(args,1,nullptr)});},{objectsConstructor},"nodeObjects",1));
    const auto arrayConstructor=JSValue::constructor([](const JSValue&,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_14({"ShaderNodeArray",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",328,25,6829,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",328,25,6829,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
        auto array=arg(args,0);const auto altType=arg(args,1,nullptr),length=array.get("length");
        for(double i=0;i<length.toNumber();++i){const auto key=JSValue(i).toString();const auto value=array.get(key);array.set(key,cap[0].call({}, {value,altType}));}return array;
    },{module.get("nodeObject")},"ShaderNodeArray",1,JSValue::ConstructorKind::Callable);
    own(module,"nodeArray",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue{JSSourceTrace::FrameScope sourceTraceTSL_15({"nodeArray",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",995,26,20108,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",995,26,20108,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});return cap[0].construct({arg(args,0),arg(args,1,nullptr)});},{arrayConstructor},"nodeArray",1));
    own(module,"nodeObjectIntent",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_16({"nodeObjectIntent",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",993,33,19909,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",993,33,19909,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
        return invoke(cap[0].call({}, {arg(args,0),arg(args,1,nullptr)}),"toVarIntent")();
    },{module.get("nodeObject")},"nodeObjectIntent",1));
    own(module,"nodeProxy",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue{JSSourceTrace::FrameScope sourceTraceTSL_17({"nodeProxy",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",996,26,20198,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",996,26,20198,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});return proxyFactory(cap[0],args);},{module},"nodeProxy",1));
    own(module,"nodeImmutable",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_18({"nodeImmutable",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",997,30,20350,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",997,30,20350,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
        const auto values=rest(Args(args.begin()+std::min<std::size_t>(1,args.size()),args.end()));return arg(args,0).construct(spread(cap[0].call({}, {values})));
    },{module.get("nodeArray")},"nodeImmutable",1));
    own(module,"nodeProxyIntent",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_19({"nodeProxyIntent",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",998,32,20459,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",998,32,20459,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
        auto settings=JSValue::object();const auto input=arg(args,3,JSValue::object());
        if(input.isObject())for(const auto& key:input.ownPropertyKeys()){const auto d=input.getOwnPropertyDescriptor(key);if(d&&d->enumerable.value_or(false))own(settings,key,input.get(key));}
        else if(input.type()==JSValue::Type::String)for(double i=0;i<input.get("length").toNumber();++i){const auto key=JSValue(i).toString();own(settings,key,input.get(key));}
        own(settings,"intent",true);return proxyFactory(cap[0],{arg(args,0),arg(args,1,nullptr),arg(args,2,nullptr),settings});
    },{module},"nodeProxyIntent",1));
    own(module,"nodeProxyConstructor",JSValue::function([](const JSValue&,const Args& args,const Args&)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_20({"nodeProxyConstructor",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",1000,37,20639,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",1000,37,20639,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
        auto handler=JSValue::object();const auto node=arg(args,1);
        own(handler,"get",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue{JSSourceTrace::FrameScope sourceTraceTSL_21({"get",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",1004,3,20732,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",1004,3,20732,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});return cap[0].get(arg(args,1),arg(args,2));},{node},"get",3));
        own(handler,"set",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue{JSSourceTrace::FrameScope sourceTraceTSL_22({"set",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",1010,3,20835,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",1010,3,20835,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});return cap[0].reflectSet(arg(args,1),arg(args,2),cap[0]);},{node},"set",3));
        return createNativeProxy(arg(args,0),handler);
    },{},"nodeProxyConstructor",2));
    own(module,"addMethodChaining",JSValue::constructor([](const JSValue&,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_23({"addMethodChaining",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",20,8,740,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",20,8,740,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
        const auto name=arg(args,0),element=arg(args,1),elements=cap[0];
        if(invoke(elements,"has")({name}).toBoolean()){ThreeUtils::warn({"TSL: Redefinition of method chaining '"+name.toString()+"'."});return {};}
        if(!element.isCallable())throw std::runtime_error("THREE.TSL: Node element "+name.toString()+" is not a function");
        invoke(elements,"set")({name,element});if(name.strictEquals("assign"))return {};
        auto proto=nativeNodeConstructor().get("prototype");
        proto.set(name,JSValue::constructor([](const JSValue& self,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_24({"",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",39,28,1247,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},JSSourceTrace::receiverTypeName(self),"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",39,28,1247,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
            if(self.get("isStackNode").toBoolean()){const auto add=invoke(self,"addToStack");return add({cap[0].call({},args)});}
            Args values{self};values.insert(values.end(),args.begin(),args.end());return cap[0].call({},values);
        },{element},"",0,JSValue::ConstructorKind::Callable));
        proto.set(plus(name,"Assign"),JSValue::constructor([](const JSValue& self,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_25({"",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",49,39,1520,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},JSSourceTrace::receiverTypeName(self),"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",49,39,1520,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
            const auto stack=self.get("isStackNode").toBoolean();const auto assign=invoke(self,"assign");
            if(stack)return assign({arg(args,0),cap[0].call({},args)});
            Args values{self};values.insert(values.end(),args.begin(),args.end());return assign({cap[0].call({},values)});
        },{element},"",0,JSValue::ConstructorKind::Callable));return {};
    },{elements},"addMethodChaining",2,JSValue::ConstructorKind::Callable));
    installPrototype(module,elements);
    own(module,"defined",JSValue::constructor([](const JSValue&,const Args& args,const Args&)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_26({"defined",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",960,8,19228,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",960,8,19228,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
        auto state=JSValue::object(nullptr);state.set("value",arg(args,0));const auto value=state.get("value");
        if(value.toBoolean()&&value.get("isNode").toBoolean())invoke(value,"traverse")({JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_27({"",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",964,19,19312,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",964,19,19312,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});const auto node=arg(args,0);if(node.get("isConstNode").toBoolean()){auto state=cap[0];state.set("value",node.get("value"));}return {};},{state},"",1)});
        return state.get("value").toBoolean();
    },{},"defined",1,JSValue::ConstructorKind::Callable));
    installNativeTSLShaderExports(module);
    const auto convertType=[&](const char* type,JSValue cache) {
        return JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_28({"",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",911,9,18158,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",911,9,18158,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});
            const auto type=cap[0],cache=cap[1];auto params=rest(args);
            auto iterator=getJSIterator(params);
            while(const auto param=stepJSIterator(iterator)) {
                if(param->isUndefined()) {
                    JSValue result;
                    try {
                        {JSSourceTrace::PositionScope sourceTraceTSLError_18256({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",917,5,18256,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"});ThreeUtils::error({"TSL: Invalid parameter for the type \""+type.toString()+"\".",([&]()->JSValue{JSSourceTrace::PositionScope sourceTraceTSLNew_18315({"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",917,64,18315,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"});return nativeStackTraceConstructor().construct();}())});}
                        result=nativeConstNodeConstructor().construct({0,type});
                    } catch(...) {closeJSIterator(iterator,std::current_exception());}
                    closeJSIterator(iterator);return result;
                }
            }
            bool makeDefault=params.get("length").strictEquals(0);
            if(!makeDefault){
                const auto scalar=invoke(rest({"bool","float","int","uint"}),"includes")({type}).toBoolean();
                if(!scalar)makeDefault=invoke(params,"every")({JSValue::function([](const JSValue&,const Args& args,const Args&)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_29({"",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",925,104,18496,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",925,104,18496,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});const auto value=arg(args,0);return !value.isObject()&&!value.isNull();},{},"",1)}).toBoolean();
            }
            if(makeDefault){const auto values=spread(params);Args callArgs{type};callArgs.insert(callArgs.end(),values.begin(),values.end());params=rest({cap[2].call({},callArgs)});}
            if(params.get("length").strictEquals(1)&&!cache.isNull()&&invoke(cache,"has")({params.get("0")}).toBoolean())return cap[4].call({}, {invoke(cache,"get")({params.get("0")})});
            if(params.get("length").strictEquals(1)) {
                const auto node=cap[3].call({}, {params.get("0"),type});
                if(node.get("nodeType").strictEquals(type))return cap[4].call({}, {node});
                return cap[4].call({}, {nativeConvertNodeConstructor().construct({node,type})});
            }
            const auto nodes=invoke(params,"map")({JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue{JSSourceTrace::FrameScope sourceTraceTSL_30({"",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",951,29,19098,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",951,29,19098,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});return cap[0].call({}, {arg(args,0)});},{cap[3]},"",1)});
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
    own(module,"convert",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_31({"convert",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",1288,24,26721,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",1288,24,26721,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});return nativeConvertNodeConstructor().construct({cap[0].call({}, {arg(args,0)}),arg(args,1)});},{module.get("nodeObject")},"convert",2));
    own(module,"split",JSValue::function([](const JSValue&,const Args& args,const Args& cap)->JSValue {JSSourceTrace::FrameScope sourceTraceTSL_32({"split",{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",1289,22,26808,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"},std::string{},"",false,false,false,{"file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/tsl/TSLCore.js",1289,22,26808,"272524b87c2ded3333c693631d8aac2000a4af14adfbdfaf975027f5036808fd"}});return nativeSplitNodeConstructor().construct({cap[0].call({}, {arg(args,0)}),arg(args,1)});},{module.get("nodeObject")},"split",2));
    module.get("addMethodChaining").call({}, {"element",module.get("element")});module.get("addMethodChaining").call({}, {"convert",module.get("convert")});
}
