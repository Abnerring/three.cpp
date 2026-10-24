#include "nodes/core/NodeBuilder.h"
#include "animation/AnimationUtils.h"
#include "nodes/core/ConstNode.h"
#include "nodes/core/constants.h"
#include "core/JSPromise.h"
#include "core/JSIterator.h"
#include "core/JSRegExp.h"
#include <iostream>

namespace {
using Args=std::vector<JSValue>;
using O=JsonValue::Object;
JSValue invoke(JSValue self,const char* key,const Args& args={}) {return self.get(key).call(self,args);}
void log(JSValue state,JSValue value) {invoke(state.get("events"),"push",{value});}
JSValue error(JSValue value) {return value.isObject()?value.get("name"):value;}
JSValue field(JSValue value) {return value.isUndefined()?JSValue("#undefined"):value;}
JSValue context(JSValue state,const char* label) {
    auto c=JSValue::object();c.set("isContextNode",true);
    c.set("getFlowContextData",JSValue::function(+[](const JSValue&,const Args&,const Args& cap)->JSValue {
        log(cap[0],cap[1]);auto data=JSValue::object();data.set("from",cap[1]);return data;
    },{state,label}));return c;
}
JSValue iterator(JSValue state,JSValue values,const char* label) {
    return JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        log(c[0],c[2].toString()+":iterator");auto it=JSValue::object(),index=JSValue::object(nullptr);index.set("i",0);
        it.set("next",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
            auto index=c[3];const auto i=index.get("i").toNumber();index.set("i",i+1);
            log(c[0],c[2].toString()+":next:"+JSValue(i).toString());
            if(c[0].get("config").get("stepFail").strictEquals(c[2])&&i==1)throw JSThrownValue("next-stop");
            auto result=JSValue::object();result.set("done",i>=c[1].get("length").toNumber());result.set("value",c[1].get(JSValue(i)));return result;
        },{c[0],c[1],c[2],index}));
        it.set("return",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
            log(c[0],c[1].toString()+":close");
            if(c[0].get("config").get("closeFail").toBoolean())throw JSThrownValue("close-stop");return JSValue::object();
        },{c[0],c[2]}));return it;
    },{state,values,label});
}
JSValue runBuild(JSValue config) {
    auto state=JSValue::object(nullptr);state.set("config",config);state.set("events",JSValue::array());state.set("yields",0);state.set("nodeCalls",0);state.set("status","pending");
    auto r=JSValue::object();r.set("contextNode",context(state,"renderer-context"));
    auto b=NodeBuilder(nullptr,r,nullptr).object();state.set("builder",b);
    const bool real=config.get("real").toBoolean();
    auto node=real?nativeConstNodeConstructor().construct({7}):JSValue::object();
    if(!real)node.set("build",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
        const auto b=a[0];log(c[0],"node:"+b.get("buildStage").toString()+":"+b.get("shaderStage").toString());
        auto state=c[0];const auto count=state.get("nodeCalls").number()+1;state.set("nodeCalls",count);
        if(state.get("config").get("nodeFail").strictEquals(count))throw JSThrownValue("node-stop");return {};
    },{state}));
    if(!real)b.set("flowNode",JSValue::function(+[](const JSValue& b,const Args&,const Args& c)->JSValue {
        log(c[0],"flow:"+b.get("buildStage").toString()+":"+b.get("shaderStage").toString());return JSValue::object();
    },{state}));
    b.set("compute",node);
    if(config.get("material").toBoolean()) {
        auto material=JSValue::object();material.set("contextNode",context(state,"material-context"));b.set("material",material);
        auto library=JSValue::object(),mapped=JSValue::object();
        mapped.set("build",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
            log(c[0],"material-build");invoke(a[0],"addFlow",{"fragment",c[1]});return {};
        },{state,node}));
        library.set("fromMaterial",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {log(c[0],"from-material");return c[1];},{state,mapped}));r.set("library",library);
    }
    if(config.get("preFail").toBoolean())b.set("prebuild",JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSThrownValue("pre-stop");}));
    if(!config.get("positionMode").isUndefined()) {
        const auto prebuild=b.get("prebuild");
        b.set("prebuild",JSValue::function(+[](const JSValue& b,const Args&,const Args& c)->JSValue {
            c[1].call(b);auto state=c[0];state.set("positionReads",0);
            auto position=JSValue::object();position.set("isNode",true);position.set("tag","first");state.set("position",position);
            JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
                auto state=c[0];const auto count=state.get("positionReads").number()+1;state.set("positionReads",count);
                log(state,"position-get:"+JSValue(count).toString());
                if(count==3&&!state.get("config").get("positionMode").strictEquals("replace"))throw JSThrownValue("position-stop");
                return state.get("position");
            },{state});b.get("context").defineProperty("position",d);return {};
        },{state,prebuild}));
        JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
            auto state=c[0];log(state,"flow-method-get");
            if(state.get("config").get("positionMode").strictEquals("method-throw"))throw JSThrownValue("method-stop");
            auto position=JSValue::object();position.set("isNode",true);position.set("tag","second");state.set("position",position);
            return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
                log(c[0],"position-flow:"+a[1].get("tag").toString());return {};
            },{state});
        },{state});b.defineProperty("flowNodeFromShaderStage",d);
    }
    b.set("buildCode",JSValue::function(+[](const JSValue& b,const Args&,const Args& c)->JSValue {
        log(c[0],"code");if(c[0].get("config").get("codeFail").toBoolean())throw JSThrownValue("code-stop");
        if(c[1].toBoolean()) {auto self=b;self.set("computeShader",invoke(b,"getFlowData",{c[2]}).get("result"));}return {};
    },{state,real,node}));
    const auto update=b.get("buildUpdateNodes");
    b.set("buildUpdateNodes",JSValue::function(+[](const JSValue& b,const Args&,const Args& c)->JSValue {log(c[0],"updates");return c[1].call(b);},{state,update}));
    auto constants=nativeNodeConstants(),stages=constants.get("defaultBuildStages"),shaders=constants.get("shaderStages");
    const bool custom=config.get("iterators").toBoolean();
    if(custom) {
        stages.set(JSValue::wellKnownSymbol("iterator"),iterator(state,stages,"build"));
        shaders.set(JSValue::wellKnownSymbol("iterator"),iterator(state,shaders,"shader"));
    }
    const auto yield=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        auto state=c[0];auto count=state.get("yields").number()+1;state.set("yields",count);log(state,"yield:"+JSValue(count).toString());
        const auto config=state.get("config"),mode=config.get("yield");
        if(config.get("failAfter").strictEquals(count)) {
            if(mode.strictEquals("reject"))return rejectNativePromise("yield-stop");
            if(mode.strictEquals("throw"))throw JSThrownValue("yield-stop");
        }
        if(mode.strictEquals("deferred")) {const auto cap=createNativePromiseCapability();queueHostTask(cap.resolve);return cap.promise;}
        if(mode.strictEquals("species")) {
            auto promise=resolveNativePromise(count),constructor=JSValue::object();
            JSPropertyDescriptor species;species.configurable=true;
            species.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
                log(c[0],"species-get");const auto mode=c[0].get("config").get("speciesMode");
                if(mode.strictEquals("null"))return nullptr;
                if(mode.strictEquals("intrinsic"))return nativePromiseConstructor();
                if(mode.strictEquals("invalid"))return 17;
                if(mode.strictEquals("throw"))throw JSThrownValue("species-stop");
                if(mode.strictEquals("custom"))return JSValue::constructor(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
                    log(c[0],"species-construct");const auto capability=createNativePromiseCapability();
                    a[0].call({}, {capability.resolve,capability.reject});return capability.promise;
                },{c[0]},"Species",1);
                return {};
            },{state});constructor.defineProperty(JSValue::wellKnownSymbol("species"),species);
            JSPropertyDescriptor d;d.configurable=true;
            d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
                log(c[0],"promise-constructor-get");const auto mode=c[0].get("config").get("speciesMode");
                if(mode.strictEquals("constructor-throw"))throw JSThrownValue("constructor-stop");
                if(mode.strictEquals("constructor-undefined"))return {};
                if(mode.strictEquals("constructor-null"))return nullptr;
                if(mode.strictEquals("constructor-primitive"))return 17;
                return c[1];
            },{state,constructor});promise.defineProperty("constructor",d);return promise;
        }
        if(mode.strictEquals("resolved")||mode.strictEquals("poison")) {
            auto promise=resolveNativePromise(count);
            if(mode.strictEquals("poison")) {
                JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],"then-get");throw JSThrownValue("then-stop");},{state});promise.defineProperty("then",d);
            }return promise;
        }
        if(mode.strictEquals("thenable")) {
            auto value=JSValue::object();JSPropertyDescriptor d;d.configurable=true;
            d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
                log(c[0],"get-then");return JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{
                    log(c[0],"call-then");a[0].call({}, {1});a[1].call({}, {"ignored"});return {};
                },{c[0]});
            },{state});value.defineProperty("then",d);return value;
        }return {};
    },{state});
    JSValue synchronous;
    try {
        if(config.get("async").toBoolean()) {
            JSValue promise;
            if(config.get("yield").strictEquals("default"))promise=invoke(b,"buildAsync");
            else promise=invoke(b,"buildAsync",{config.get("yield").strictEquals("null")?JSValue(nullptr):yield});
            log(state,"returned");synchronous=state.get("events").toJSON()?JSValue::fromJSON(*state.get("events").toJSON()):JSValue{};
            invoke(promise,"then",{JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
                auto s=c[0];s.set("status","fulfilled");s.set("same",a[0].strictEquals(s.get("builder")));log(s,"fulfilled");return {};
            },{state}),JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue {
                auto s=c[0];s.set("status","rejected");s.set("reason",error(a[0]));log(s,"rejected");return {};
            },{state})});
            queueMicrotask(JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {log(c[0],"micro");return {};},{state}));
            drainMicrotasks();unsigned tasks=0;
            while(hasPendingHostTasks()&&tasks++<30) {log(state,"host");drainHostTasks();}
        } else {
            const auto result=invoke(b,"build");state.set("status","fulfilled");state.set("same",result.strictEquals(b));log(state,"returned");synchronous=JSValue::fromJSON(*state.get("events").toJSON());
        }
    } catch(...) {state.set("status","thrown");state.set("reason",error(nativeExceptionValue(std::current_exception())));synchronous=JSValue::fromJSON(*state.get("events").toJSON());}
    if(custom){stages.erase(JSValue::wellKnownSymbol("iterator"));shaders.erase(JSValue::wellKnownSymbol("iterator"));}
    auto result=JSValue::object();
    for(const auto* key:{"status","reason","same","events","yields"})result.set(key,field(state.get(key)));
    result.set("sync",synchronous);result.set("stage",field(b.get("buildStage")));result.set("shader",field(b.get("shaderStage")));
    result.set("context",field(b.get("context").get("from")));result.set("code",field(b.get("computeShader")));return result;
}
JSValue runString(JSValue request) {
    auto events=JSValue::array(),result=JSValue::object();
    auto decode=[](JSValue value)->JSValue {
        if(value.strictEquals("#undefined"))return {};
        if(value.strictEquals("#NaN"))return std::numeric_limits<double>::quiet_NaN();
        if(value.strictEquals("#Infinity"))return std::numeric_limits<double>::infinity();
        if(value.strictEquals("#-Infinity"))return -std::numeric_limits<double>::infinity();
        return value;
    };
    try {
        JSValue value;
        if(request.get("kind").strictEquals("substring"))value=invoke(request.get("text"),"substring",{decode(request.get("start")),decode(request.get("end"))});
        else if(request.get("kind").strictEquals("match")) {
            const auto pattern=request.get("regexp").toBoolean()?createNativeRegExp(decode(request.get("pattern")),request.get("flags")):decode(request.get("pattern"));
            value=invoke(request.get("text"),"match",{pattern});
            if(!value.isNull()) {
                result.set("index",field(value.get("index")));result.set("input",field(value.get("input")));
            }
        } else {
            auto receiver=JSValue::object(),pattern=JSValue::object();
            receiver.set("toString",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{invoke(c[0],"push",{"text"});return "abcdef";},{events}));
            if(request.get("mode").strictEquals("match-custom")) {
                JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
                    invoke(c[0],"push",{"get-match"});return JSValue::function(+[](const JSValue& self,const Args& a,const Args& c)->JSValue {
                        invoke(c[0],"push",{"call-match"});return a[0].strictEquals(c[1])&&self.strictEquals(c[2]);
                    },c);
                },{events,receiver,pattern});pattern.defineProperty(JSValue::wellKnownSymbol("match"),d);
                value=JSValue("x").get("match").call(receiver,{pattern});
            } else {
                auto start=JSValue::object(),end=JSValue::object();
                start.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{invoke(c[0],"push",{"start"});return 4;},{events}));
                end.set("valueOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{invoke(c[0],"push",{"end"});return 1;},{events}));
                value=JSValue("x").get("substring").call(receiver,{start,end});
            }
        }
        result.set("value",field(value));
    }catch(...){result.set("error",error(nativeExceptionValue(std::current_exception())));}
    result.set("events",events);return result;
}
}
int main() {
    std::string line;
    while(std::getline(std::cin,line))try{
        const auto request=Animation_detail::parseJSON(line);
        const auto result=request.get("kind").isUndefined()?runBuild(request):runString(request);
        std::cout<<result.toJSON()->dump()<<'\n';
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
