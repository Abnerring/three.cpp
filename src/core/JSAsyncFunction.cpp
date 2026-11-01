#include "core/JSAsyncFunction.h"
#include "core/JSFunction.h"
#include <exception>
#include <memory>
#include <utility>

namespace {
using Args = std::vector<JSValue>;
struct FunctionData { NativeAsyncBody body; };
struct StateFunctionData { JSAsyncStateBody body; JSAsyncInitializer initialize; };
enum class State { Executing, Suspended, Completed };
struct FrameData { JSAsyncStateBody body; State state = State::Executing; };
constexpr auto functionTag = "runtime.AsyncFunctionData";
constexpr auto stateFunctionTag = "runtime.AsyncStateFunctionData";
constexpr auto frameTag = "runtime.AsyncFrame";
constexpr const char* contextSlots[] = {
    "Async.receiver", "Async.arguments", "Async.captures", "Async.locals",
    "Async.promise", "Async.resolve", "Async.reject"
};
JSPropertyDescriptor data(JSValue value, bool writable,
                          bool configurable = true, bool enumerable = false) {
    JSPropertyDescriptor d; d.value=std::move(value); d.writable=writable;
    d.enumerable=enumerable; d.configurable=configurable; return d;
}
JSValue list(const Args& values) {
    auto result=JSValue::array();
    for (std::size_t i=0;i<values.size();++i)
        result.defineProperty(std::to_string(i),data(values[i],true,true,true));
    return result;
}
Args values(const JSValue& array) {
    Args result; result.reserve(array.length());
    for (std::uint32_t i=0;i<array.length();++i)
        result.push_back(array.get(std::to_string(i)));
    return result;
}
void finish(JSValue frame, const std::shared_ptr<FrameData>& state) {
    state->state=State::Completed;
    for (const auto* key:contextSlots) frame.setInternalSlot(key,{});
}
void resume(JSValue frame, JSAsyncCompletion completion) {
    const auto state=std::static_pointer_cast<FrameData>(frame.hostPayload(frameTag));
    if (state->state==State::Completed) return;
    state->state=State::Executing;
    const auto resolve=frame.getInternalSlot("Async.resolve");
    const auto reject=frame.getInternalSlot("Async.reject");
    try {
      for (;;) {
        const auto step=state->body(frame.getInternalSlot("Async.locals"),completion,
            frame.getInternalSlot("Async.receiver"),
            values(frame.getInternalSlot("Async.arguments")),
            values(frame.getInternalSlot("Async.captures")));
        if (step.kind==JSAsyncStep::Kind::Await) {
            const auto continuation=[&](bool rejected) {
                return JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
                    resume(captures[0],{captures[1].toBoolean()?JSAsyncCompletionKind::Throw:
                        JSAsyncCompletionKind::Normal,args.empty()?JSValue{}:args[0]});
                    return {};
                },{frame,rejected},"",1);
            };
            state->state=State::Suspended;
            try {
                awaitNativePromise(step.value,continuation(false),continuation(true));
            } catch (...) {
                // PromiseResolve may fail before suspension (for example an
                // actual promise's constructor getter). Deliver that abrupt
                // Await completion to the source catch/finally immediately.
                completion={JSAsyncCompletionKind::Throw,
                    nativeExceptionValue(std::current_exception())};
                state->state=State::Executing;
                continue;
            }
            return;
        }
        finish(frame,state);
        (step.kind==JSAsyncStep::Kind::Throw?reject:resolve).call({}, {step.value});
        return;
      }
    } catch (...) {
        const auto reason=nativeExceptionValue(std::current_exception());
        finish(frame,state); reject.call({}, {reason});
    }
}
void invokeState(const JSValue& receiver,const Args& args,const Args& captures,
                 const JSPromiseCapability& capability) {
    const auto metadata=std::static_pointer_cast<StateFunctionData>(
        captures[0].hostPayload(stateFunctionTag));
    const Args environment(captures.begin()+1,captures.end());
    const auto payload=std::make_shared<FrameData>(); payload->body=metadata->body;
    auto frame=JSValue::hostObject(payload,payload.get(),frameTag);
    auto locals=JSValue::object(nullptr);
    frame.setInternalSlot("Async.receiver",receiver);
    frame.setInternalSlot("Async.arguments",list(args));
    frame.setInternalSlot("Async.captures",list(environment));
    frame.setInternalSlot("Async.locals",locals);
    frame.setInternalSlot("Async.promise",capability.promise);
    frame.setInternalSlot("Async.resolve",capability.resolve);
    frame.setInternalSlot("Async.reject",capability.reject);
    try {
        if (metadata->initialize) metadata->initialize(locals,receiver,args,environment);
        resume(frame,{});
    } catch (...) {
        const auto reason=nativeExceptionValue(std::current_exception());
        finish(frame,payload); capability.reject.call({}, {reason});
    }
}
JSValue invoke(const JSValue& receiver,const Args& args,const Args& captures) {
    const auto metadata=std::static_pointer_cast<FunctionData>(
        captures[0].hostPayload(functionTag));
    const Args environment(captures.begin()+1,captures.end());
    const auto capability=createNativePromiseCapability();
    try { metadata->body(receiver,args,environment,capability); }
    catch (...) {
        capability.reject.call({}, {nativeExceptionValue(std::current_exception())});
    }
    return capability.promise;
}
void empty(const JSValue&,const Args&,const Args&,const JSPromiseCapability& capability) {
    capability.resolve.call();
}
JSValue dynamicConstructor(const JSValue&,const JSValue& newTarget,
                           const Args& args,const Args&) {
    if (!args.empty()) throw JSAsyncFunctionDynamicSourceUnsupported();
    const auto target=newTarget.isUndefined()?nativeAsyncFunctionConstructor():newTarget;
    const auto prototype=target.getPrototypeFromConstructor(nativeAsyncFunctionPrototype());
    auto function=createNativeAsyncFunction(empty,{},"anonymous",0,
        "async function anonymous(\n) {\n\n}");
    function.setPrototype(prototype); return function;
}
}
JSAsyncStep JSAsyncStep::awaitValue(JSValue value) { return {Kind::Await,std::move(value)}; }
JSAsyncStep JSAsyncStep::complete(JSValue value) { return {Kind::Return,std::move(value)}; }
JSAsyncStep JSAsyncStep::abrupt(JSValue value) { return {Kind::Throw,std::move(value)}; }
JSValue nativeAsyncFunctionConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto value=weak.lock();value&&value->isConstructible()) return *value;
    const auto functionConstructor=nativeFunctionConstructor();
    auto prototype=JSValue::object(functionConstructor.get("prototype"));
    auto constructor=JSValue::callableBuiltinConstructor(dynamicConstructor,{},
        "AsyncFunction",1,prototype);
    constructor.setPrototype(functionConstructor);
    prototype.defineProperty("constructor",data(constructor,false));
    prototype.defineProperty(JSValue::wellKnownSymbol("toStringTag"),data("AsyncFunction",false));
    constructor.retainForHeapLifetime(); weak=JSWeakValue(constructor); return constructor;
}
JSValue nativeAsyncFunctionPrototype() {
    return nativeAsyncFunctionConstructor().get("prototype");
}
JSValue createNativeAsyncFunction(NativeAsyncBody body,const Args& captures,
    const std::string& name,std::uint32_t arity,const std::string& source) {
    if (!body) throw JSTypeError("A native async function requires a body");
    const auto payload=std::make_shared<FunctionData>(FunctionData{body});
    const auto holder=JSValue::hostObject(payload,payload.get(),functionTag);
    Args environment{holder}; environment.insert(environment.end(),captures.begin(),captures.end());
    auto function=JSValue::function(invoke,environment,name,arity);
    function.setPrototype(nativeAsyncFunctionPrototype());
    if (!source.empty()) function.setFunctionSourceText(source);
    return function;
}
JSValue createNativeAsyncStateFunction(JSAsyncStateBody body,const Args& captures,
    const std::string& name,std::uint32_t arity,const std::string& source,
    JSAsyncInitializer initialize) {
    if (!body) throw JSTypeError("A native async state function requires a body");
    const auto payload=std::make_shared<StateFunctionData>(StateFunctionData{body,initialize});
    const auto holder=JSValue::hostObject(payload,payload.get(),stateFunctionTag);
    Args environment{holder}; environment.insert(environment.end(),captures.begin(),captures.end());
    return createNativeAsyncFunction(invokeState,environment,name,arity,source);
}
