// Native adaptation of the ECMAScript Promise algorithms; no source evaluator.
// https://tc39.es/ecma262/multipage/control-abstraction-objects.html#sec-promise-objects
#include "core/JSPromise.h"
#include "core/AbortController.h"
#include "core/JSRegExp.h"
#include <exception>

namespace {
using Args = std::vector<JSValue>;
constexpr const char* promiseTag = "runtime.Promise";
struct PromiseIdentity {};
constexpr const char* stateSlot = "Promise.state";
constexpr const char* resultSlot = "Promise.result";
constexpr const char* fulfillSlot = "Promise.fulfill";
constexpr const char* rejectSlot = "Promise.reject";
JSValue arg(const Args& args, std::size_t index) { return index < args.size() ? args[index] : JSValue{}; }
JSValue reactionList() {
    auto values=JSValue::array(); values.setPrototype(nullptr); return values;
}
void createDataProperty(JSValue object, const char* name, JSValue value) {
    JSPropertyDescriptor d; d.value=std::move(value); d.writable=d.enumerable=d.configurable=true;
    object.defineProperty(name,d);
}
JSValue errorValue(const char* name, const std::string& message) {
    auto error = JSValue::object();
    for (const auto& entry : {std::pair<std::string,JSValue>{"name", name}, {"message", message}}) {
        JSPropertyDescriptor d; d.value=entry.second; d.writable=d.configurable=true; d.enumerable=false;
        error.defineProperty(entry.first,d);
    }
    return error;
}
JSValue caughtValue() {
    try { throw; }
    catch (const JSThrownValue& thrown) { return thrown.value; }
    catch (const JSAbortException& aborted) { return aborted.reason; }
    catch (const JSValue& thrown) { return thrown; }
    catch (const JSTypeError& error) { return errorValue("TypeError",error.what()); }
    catch (const JSRangeError& error) { return errorValue("RangeError",error.what()); }
    catch (const JSSyntaxError& error) { return errorValue("SyntaxError",error.what()); }
    catch (const std::exception& error) { return errorValue("Error",error.what()); }
}
void install(JSValue object, const char* name, JSValue::NativeFunction callback, std::uint32_t length) {
    JSPropertyDescriptor d; d.value=JSValue::function(callback,{},name,length);
    d.writable=d.configurable=true; d.enumerable=false; object.defineProperty(name,d);
}
JSPromiseCapability capability(JSValue constructor);
JSValue promiseResolve(JSValue constructor, JSValue resolution);
JSPromiseCapability resolvingFunctions(JSValue promise);

void enqueueReaction(JSValue reaction, JSValue argument) {
    queueMicrotask(JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue {
        const auto reaction=captures[0],argument=captures[1];
        const auto handler=reaction.get("handler"),resolve=reaction.get("resolve"),reject=reaction.get("reject");
        JSValue result; bool abrupt=false;
        if (!handler.isCallable()) {
            result=argument; abrupt=reaction.get("rejectReaction").toBoolean();
        } else {
            try { result=handler.call({}, {argument}); }
            catch (...) { result=caughtValue(); abrupt=true; }
        }
        const auto continuation = abrupt ? reject : resolve;
        if (!continuation.isUndefined()) continuation.call({}, {result});
        return {};
    },{reaction,argument}));
}
void settle(JSValue promise, JSValue value, bool rejected) {
    if (!promise.getHostSlot(stateSlot).strictEquals("pending")) return;
    const auto reactions=promise.getHostSlot(rejected?rejectSlot:fulfillSlot);
    promise.setHostSlot(resultSlot,value);
    promise.setHostSlot(fulfillSlot,{}); promise.setHostSlot(rejectSlot,{});
    promise.setHostSlot(stateSlot,rejected?"rejected":"fulfilled");
    for (std::uint32_t i=0;i<reactions.length();++i) enqueueReaction(reactions.get(std::to_string(i)),value);
}
void resolvePromise(JSValue promise, JSValue resolution) {
    if (resolution.sameValue(promise)) {
        settle(promise,errorValue("TypeError","Chaining cycle detected for promise"),true); return;
    }
    if (!resolution.isObject()) { settle(promise,resolution,false); return; }
    JSValue then;
    try { then=resolution.get("then"); }
    catch (...) { settle(promise,caughtValue(),true); return; }
    if (!then.isCallable()) { settle(promise,resolution,false); return; }
    queueMicrotask(JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue {
        const auto functions=resolvingFunctions(captures[0]);
        try { captures[2].call(captures[1], {functions.resolve,functions.reject}); }
        catch (...) { functions.reject.call({}, {caughtValue()}); }
        return {};
    },{promise,resolution,then}));
}
JSPromiseCapability resolvingFunctions(JSValue promise) {
    auto already=JSValue::object(nullptr); already.set("value",false);
    auto resolve=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
        auto already=captures[1];
        if (already.get("value").toBoolean()) return {};
        already.set("value",true); resolvePromise(captures[0],arg(args,0)); return {};
    },{promise,already},"",1);
    auto reject=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
        auto already=captures[1];
        if (already.get("value").toBoolean()) return {};
        already.set("value",true); settle(captures[0],arg(args,0),true); return {};
    },{promise,already},"",1);
    return {promise,resolve,reject};
}
JSValue initializePromise(const JSValue& newTarget,const Args& args,const Args&) {
    const auto executor=arg(args,0);
    if (!executor.isCallable()) throw JSTypeError("Promise resolver is not a function");
    auto prototype=newTarget.get("prototype");
    if (!prototype.isObject()) prototype=nativePromiseConstructor().get("prototype");
    const auto owner=std::make_shared<PromiseIdentity>();
    auto promise=JSValue::hostObject(owner,owner.get(),promiseTag);
    promise.setPrototype(prototype);
    promise.setHostSlot(stateSlot,"pending"); promise.setHostSlot(resultSlot,{});
    promise.setHostSlot(fulfillSlot,reactionList()); promise.setHostSlot(rejectSlot,reactionList());
    promise.setHostSlot("Promise.handled",false);
    const auto functions=resolvingFunctions(promise);
    try { executor.call({}, {functions.resolve,functions.reject}); }
    catch (...) { functions.reject.call({}, {caughtValue()}); }
    return promise;
}
JSPromiseCapability capability(JSValue constructor) {
    if (!constructor.isConstructible()) throw JSTypeError("Promise constructor is not a constructor");
    auto state=JSValue::object(nullptr); state.set("resolve",{}); state.set("reject",{});
    const auto executor=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
        auto state=captures[0];
        if (!state.get("resolve").isUndefined() || !state.get("reject").isUndefined())
            throw JSTypeError("Promise executor has already been invoked");
        state.set("resolve",arg(args,0)); state.set("reject",arg(args,1)); return {};
    },{state},"",2);
    const auto promise=constructor.construct({executor});
    const auto resolve=state.get("resolve"),reject=state.get("reject");
    if (!resolve.isCallable() || !reject.isCallable()) throw JSTypeError("Promise resolve or reject is not callable");
    return {promise,resolve,reject};
}
JSValue promiseResolve(JSValue constructor, JSValue resolution) {
    if (!constructor.isObject()) throw JSTypeError("Promise.resolve receiver must be an object");
    if (isNativePromise(resolution) && resolution.get("constructor").sameValue(constructor)) return resolution;
    auto result=capability(constructor); result.resolve.call({}, {resolution}); return result.promise;
}
JSValue speciesConstructor(JSValue promise) {
    const auto constructor=promise.get("constructor");
    if (constructor.isUndefined()) return nativePromiseConstructor();
    if (!constructor.isObject()) throw JSTypeError("Promise constructor property must be an object");
    const auto species=constructor.get(JSValue::wellKnownSymbol("species"));
    if (species.isUndefined() || species.isNull()) return nativePromiseConstructor();
    if (!species.isConstructible()) throw JSTypeError("Promise species is not a constructor");
    return species;
}
JSValue thenMethod(const JSValue& promise,const Args& args,const Args&) {
    if (!isNativePromise(promise)) throw JSTypeError("Promise.then receiver is not a promise");
    const auto result=capability(speciesConstructor(promise));
    auto reaction=[&](bool rejected) {
        auto value=JSValue::object(nullptr); value.set("handler",arg(args,rejected?1:0));
        value.set("rejectReaction",rejected); value.set("resolve",result.resolve); value.set("reject",result.reject);
        return value;
    };
    const auto state=promise.getHostSlot(stateSlot);
    if (state.strictEquals("pending")) {
        auto fulfilled=promise.getHostSlot(fulfillSlot),rejected=promise.getHostSlot(rejectSlot);
        fulfilled.set(std::to_string(fulfilled.length()),reaction(false));
        rejected.set(std::to_string(rejected.length()),reaction(true));
    } else enqueueReaction(reaction(state.strictEquals("rejected")),promise.getHostSlot(resultSlot));
    auto self=promise; self.setHostSlot("Promise.handled",true);
    return result.promise;
}
JSValue finallyMethod(const JSValue& promise,const Args& args,const Args&) {
    if (!promise.isObject()) throw JSTypeError("Promise.finally receiver must be an object");
    const auto constructor=speciesConstructor(promise),handler=arg(args,0);
    auto onFulfilled=handler,onRejected=handler;
    if (handler.isCallable()) {
        const auto make=[&](bool rejected) {
            return JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
                const auto result=captures[0].call();
                const auto promise=promiseResolve(captures[1],result);
                const auto thunk=JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue {
                    if (captures[1].toBoolean()) throw JSThrownValue(captures[0]);
                    return captures[0];
                },{arg(args,0),captures[2]});
                return promise.get("then").call(promise,{thunk});
            },{handler,constructor,rejected},"",1);
        };
        onFulfilled=make(false); onRejected=make(true);
    }
    return promise.get("then").call(promise,{onFulfilled,onRejected});
}
}

bool isNativePromise(const JSValue& value) {
    return value.isHostObject(promiseTag);
}
JSValue nativePromiseConstructor() {
    static thread_local JSValue constructor=[] {
        auto result=JSValue::builtinConstructor(initializePromise,{},"Promise",1);
        JSPropertyDescriptor species;
        species.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue { return self; },{},"get [Symbol.species]",0);
        species.enumerable=false; species.configurable=true;
        result.defineProperty(JSValue::wellKnownSymbol("species"),species);
        auto prototype=result.get("prototype");
        install(prototype,"then",thenMethod,2);
        install(prototype,"catch",+[](const JSValue& self,const Args& args,const Args&)->JSValue {
            return self.get("then").call(self,{{},arg(args,0)});
        },1);
        install(prototype,"finally",finallyMethod,1);
        install(result,"resolve",+[](const JSValue& self,const Args& args,const Args&)->JSValue {
            return promiseResolve(self,arg(args,0));
        },1);
        install(result,"reject",+[](const JSValue& self,const Args& args,const Args&)->JSValue {
            auto result=capability(self); result.reject.call({}, {arg(args,0)}); return result.promise;
        },1);
        install(result,"withResolvers",+[](const JSValue& self,const Args&,const Args&)->JSValue {
            const auto value=capability(self); auto result=JSValue::object();
            createDataProperty(result,"promise",value.promise); createDataProperty(result,"resolve",value.resolve);
            createDataProperty(result,"reject",value.reject); return result;
        },0);
        install(result,"try",+[](const JSValue& self,const Args& args,const Args&)->JSValue {
            const auto result=capability(self); JSValue value; bool rejected=false;
            try { value=arg(args,0).call({},args.empty()?Args{}:Args(args.begin()+1,args.end())); }
            catch (...) { value=caughtValue(); rejected=true; }
            (rejected?result.reject:result.resolve).call({}, {value}); return result.promise;
        },1);
        return result;
    }();
    return constructor;
}
JSValue createNativePromise(JSValue executor) { return nativePromiseConstructor().construct({executor}); }
JSPromiseCapability createNativePromiseCapability() { return capability(nativePromiseConstructor()); }
JSValue resolveNativePromise(JSValue value) { return promiseResolve(nativePromiseConstructor(),std::move(value)); }
JSValue rejectNativePromise(JSValue reason) {
    const auto result=createNativePromiseCapability(); result.reject.call({}, {reason}); return result.promise;
}
void awaitNativePromise(JSValue value, JSValue onFulfilled, JSValue onRejected) {
    auto promise = promiseResolve(nativePromiseConstructor(), std::move(value));
    const auto make = [&](bool rejected) {
        auto reaction = JSValue::object(nullptr);
        reaction.set("handler", rejected ? onRejected : onFulfilled);
        reaction.set("rejectReaction", rejected);
        reaction.set("resolve", {}); reaction.set("reject", {});
        return reaction;
    };
    const auto state = promise.getHostSlot(stateSlot);
    if (state.strictEquals("pending")) {
        auto fulfilled = promise.getHostSlot(fulfillSlot), rejected = promise.getHostSlot(rejectSlot);
        fulfilled.set(std::to_string(fulfilled.length()), make(false));
        rejected.set(std::to_string(rejected.length()), make(true));
    } else enqueueReaction(make(state.strictEquals("rejected")), promise.getHostSlot(resultSlot));
    promise.setHostSlot("Promise.handled", true);
}
JSValue nativeExceptionValue(std::exception_ptr error) {
    if (!error) throw std::logic_error("No native exception to translate");
    try { std::rethrow_exception(error); }
    catch (...) { return caughtValue(); }
}
