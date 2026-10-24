// Native suspension states for the pinned NodeBuilder.buildAsync body.
#include "nodes/core/NodeBuilderAsync.h"
#include "core/JSIterator.h"
#include "core/JSPromise.h"

namespace {
using Args=std::vector<JSValue>;
JSValue invoke(JSValue receiver,const char* name,const Args& args={}) { return receiver.get(name).call(receiver,args); }
JSValue record(JSValue iterable) {
    const auto iterator=getJSIterator(iterable);
    auto state=JSValue::object(nullptr);
    state.set("iterator",iterator.iterator);state.set("next",iterator.nextMethod);state.set("done",false);
    return state;
}
std::optional<JSValue> step(JSValue state) {
    JSIteratorRecord iterator{state.get("iterator"),state.get("next"),state.get("done").toBoolean()};
    try {
        const auto result=stepJSIterator(iterator);state.set("done",iterator.done);return result;
    } catch(...) { state.set("done",iterator.done);throw; }
}
void close(JSValue state,std::exception_ptr error) {
    JSIteratorRecord iterator{state.get("iterator"),state.get("next"),state.get("done").toBoolean()};
    try { closeJSIterator(iterator,error); } catch(...) {}
}
template<class F> void each(JSValue iterable,F body) {
    auto iterator=getJSIterator(iterable);
    while(const auto value=stepJSIterator(iterator)) {
        try {body(*value);} catch(...) {closeJSIterator(iterator,std::current_exception());}
    }
}
void release(JSValue state) {
    for(const auto* key:{"builder","yield","resolve","reject","buildStages","shaderStages","buildIterator","shaderIterator","stage"})state.set(key,{});
    state.set("done",true);
}
void fail(JSValue state,std::exception_ptr error) {
    if(state.get("shaderBody").toBoolean())close(state.get("shaderIterator"),error);
    if(state.get("buildBody").toBoolean())close(state.get("buildIterator"),error);
    const auto reject=state.get("reject"),reason=nativeExceptionValue(error);
    release(state);reject.call({}, {reason});
}
void resume(JSValue state) {
    if(state.get("done").toBoolean())return;
    try {
        const auto builder=state.get("builder");
        while(true) {
            if(state.get("phase").strictEquals(0)) {
                state.set("buildBody",false);
                const auto stage=step(state.get("buildIterator"));
                if(!stage) {
                    invoke(builder,"setBuildStage",{nullptr});invoke(builder,"setShaderStage",{nullptr});
                    invoke(builder,"buildCode");invoke(builder,"buildUpdateNodes");
                    const auto resolve=state.get("resolve");release(state);resolve.call({}, {builder});return;
                }
                state.set("buildBody",true);state.set("stage",*stage);
                invoke(builder,"setBuildStage",{*stage});
                if(builder.get("context").get("position").toBoolean()&&builder.get("context").get("position").get("isNode").toBoolean()) {
                    const auto flow=builder.get("flowNodeFromShaderStage");
                    const auto position=builder.get("context").get("position");
                    flow.call(builder,{"vertex",position});
                }
                state.set("shaderIterator",record(state.get("shaderStages")));state.set("phase",1);
            }
            state.set("shaderBody",false);
            const auto shader=step(state.get("shaderIterator"));
            if(!shader) {state.set("phase",0);continue;}
            state.set("shaderBody",true);
            invoke(builder,"setShaderStage",{*shader});
            const auto flowNodes=builder.get("flowNodes").get(*shader);
            each(flowNodes,[&](const JSValue& node) {
                if(state.get("stage").strictEquals("generate"))invoke(builder,"flowNode",{node});
                else invoke(node,"build",{builder});
            });
            const auto yielded=state.get("yield").call();
            const auto fulfilled=JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue {
                resume(captures[0]);return {};
            },{state},"",1);
            const auto rejected=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
                fail(captures[0],std::make_exception_ptr(JSThrownValue(args.empty()?JSValue{}:args[0])));return {};
            },{state},"",1);
            awaitNativePromise(yielded,fulfilled,rejected);return;
        }
    } catch(...) {fail(state,std::current_exception());}
}
}
JSValue buildNodeBuilderAsync(JSValue builder,JSValue yieldFunction,JSValue buildStages,JSValue shaderStages) {
    const auto capability=createNativePromiseCapability();
    auto state=JSValue::object(nullptr);
    state.set("builder",builder);state.set("yield",yieldFunction);
    state.set("resolve",capability.resolve);state.set("reject",capability.reject);
    state.set("buildStages",buildStages);state.set("shaderStages",shaderStages);
    state.set("phase",0);state.set("done",false);state.set("buildBody",false);state.set("shaderBody",false);
    try {invoke(builder,"prebuild");state.set("buildIterator",record(buildStages));resume(state);}
    catch(...) {fail(state,std::current_exception());}
    return capability.promise;
}
