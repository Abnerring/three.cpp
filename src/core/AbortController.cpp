#include "core/AbortController.h"
#include "core/JSPromise.h"
#include <exception>

namespace {
using Args=std::vector<JSValue>;
constexpr auto signalTag="platform.AbortSignal",controllerTag="platform.AbortController";
struct Identity {};
JSValue arg(const Args& args,std::size_t index){return index<args.size()?args[index]:JSValue{};}
JSValue makeHost(const char* tag){auto owner=std::make_shared<Identity>();return JSValue::hostObject(owner,owner.get(),tag);}
JSValue list(JSValue value,const char* name){return value.getHostSlot(name);}
void push(JSValue values,JSValue value){values.get("push").call(values,{value});}
JSValue signalPrototype();
JSValue createSignal() {
    auto signal=makeHost(signalTag);signal.setPrototype(signalPrototype());
    signal.setHostSlot("aborted",false);signal.setHostSlot("reason",{});signal.setHostSlot("onabort",nullptr);
    signal.setHostSlot("listeners",JSValue::array());signal.setHostSlot("algorithms",JSValue::array());
    signal.setHostSlot("dependents",JSValue::array());return signal;
}
void requireSignal(const JSValue& signal){if(!signal.isHostObject(signalTag))throw JSTypeError("Expected AbortSignal");}
bool capture(JSValue options){return options.isObject()?options.get("capture").toBoolean():options.toBoolean();}
JSValue reasonDefault(){auto reason=JSValue::object();reason.set("name","AbortError");reason.set("message","This operation was aborted");reason.set("code",20);return reason;}
JSValue caughtError() {
    const auto error=[](const char* name,const char* message){auto result=JSValue::object();result.set("name",name);result.set("message",message);return result;};
    try{throw;}
    catch(const JSThrownValue& exception){return exception.value;}
    catch(const JSAbortException& exception){return exception.reason;}
    catch(const JSValue& value){return value;}
    catch(const JSTypeError& exception){return error("TypeError",exception.what());}
    catch(const JSRangeError& exception){return error("RangeError",exception.what());}
    catch(const std::exception& exception){return error("Error",exception.what());}
}
void markAborted(JSValue signal,JSValue reason,std::vector<JSValue>& pending) {
    if(signal.getHostSlot("aborted").toBoolean())return;
    signal.setHostSlot("aborted",true);signal.setHostSlot("reason",reason);pending.push_back(signal);
    const auto dependents=list(signal,"dependents");
    for(std::uint32_t i=0;i<dependents.length();++i)markAborted(dependents.get(std::to_string(i)),reason,pending);
}
void dispatchAbort(JSValue signal) {
    const auto algorithms=list(signal,"algorithms");signal.setHostSlot("algorithms",JSValue::array());
    // Like EventTarget, an exception from a listener does not stop cancellation
    // or the remaining listeners. Preserve errors for the owning event loop.
    auto errors=JSValue::array();
    for(std::uint32_t i=0;i<algorithms.length();++i)try{algorithms.get(std::to_string(i)).call();}catch(...){push(errors,caughtError());}
    auto event=JSValue::object(),state=JSValue::object();state.set("stopImmediate",false);
    for(const char* name:{"stopPropagation","stopImmediatePropagation","preventDefault"})event.set(name,JSValue::function([](const JSValue&,const Args&,const Args& caps)->JSValue {
        if(caps[1].strictEquals("stopImmediatePropagation")){auto state=caps[0];state.set("stopImmediate",true);}return {};
    },{state,name},name,0));
    event.set("type","abort");event.set("target",signal);event.set("currentTarget",signal);event.set("eventPhase",2);
    event.set("bubbles",false);event.set("cancelable",false);event.set("defaultPrevented",false);event.set("isTrusted",true);
    const auto listeners=list(signal,"listeners");std::vector<JSValue> snapshot;
    for(std::uint32_t i=0;i<listeners.length();++i)snapshot.push_back(listeners.get(std::to_string(i)));
    for(auto listener:snapshot) {
        if(state.get("stopImmediate").toBoolean())break;
        if(listener.get("removed").toBoolean())continue;
        auto callback=listener.get("onabort").toBoolean()?signal.getHostSlot("onabort"):listener.get("callback");
        if(callback.isNull()||callback.isUndefined())continue;
        if(listener.get("once").toBoolean())listener.set("removed",true);
        try {
            if(callback.isCallable())callback.call(signal,{event});
            else callback.get("handleEvent").call(callback,{event});
        } catch(...){push(errors,caughtError());}
    }
    event.set("currentTarget",nullptr);event.set("eventPhase",0);signal.setHostSlot("reportedErrors",errors);
}
void abortSignal(JSValue signal,JSValue reason) {
    requireSignal(signal);if(reason.isUndefined())reason=reasonDefault();std::vector<JSValue> pending;
    markAborted(signal,reason,pending);for(auto value:pending)dispatchAbort(value);
}
JSValue signalMethod(const JSValue& receiver,const Args& args,const Args& captures) {
    requireSignal(receiver);auto self=receiver;const auto& method=captures[0].string();
    if(method=="aborted"||method=="reason"||method=="onabort")return self.getHostSlot(method);
    if(method=="throwIfAborted"){if(self.getHostSlot("aborted").toBoolean())throw JSAbortException(self.getHostSlot("reason"));return {};}
    if(method=="set onabort") {
        auto callback=arg(args,0);if(!callback.isObject())callback=nullptr;
        const auto previous=self.getHostSlot("onabort");self.setHostSlot("onabort",callback);
        const auto listeners=list(self,"listeners");
        if(callback.isNull()) {
            for(std::uint32_t i=0;i<listeners.length();++i){auto value=listeners.get(std::to_string(i));if(value.get("onabort").toBoolean())value.set("removed",true);}
        } else if(previous.isNull()) {
            auto record=JSValue::object();record.set("onabort",true);record.set("removed",false);record.set("once",false);push(listeners,record);
        }
        return {};
    }
    const auto type=arg(args,0).toString();const auto callback=arg(args,1),options=arg(args,2);
    const bool capturing=capture(options);
    if(!callback.isObject()&&!callback.isNull()&&!callback.isUndefined())throw JSTypeError("Event listener must be an object");
    if(type!="abort"||callback.isNull()||callback.isUndefined())return {};
    const auto listeners=list(self,"listeners");
    if(method=="removeEventListener") {
        for(std::uint32_t i=0;i<listeners.length();++i) {
            auto record=listeners.get(std::to_string(i));
            if(!record.get("removed").toBoolean()&&record.get("callback").sameValue(callback)&&record.get("capture").strictEquals(capturing)){record.set("removed",true);break;}
        }
        return {};
    }
    const auto once=options.isObject()?options.get("once").toBoolean():false;
    if(options.isObject())options.get("passive");
    const auto optionSignal=options.isObject()?options.get("signal"):JSValue{};
    if(!optionSignal.isUndefined()){requireSignal(optionSignal);if(optionSignal.get("aborted").toBoolean())return {};}
    for(std::uint32_t i=0;i<listeners.length();++i) {
        const auto record=listeners.get(std::to_string(i));
        if(!record.get("removed").toBoolean()&&record.get("callback").sameValue(callback)&&record.get("capture").strictEquals(capturing))return {};
    }
    auto record=JSValue::object();record.set("callback",callback);record.set("capture",capturing);record.set("once",once);record.set("removed",false);push(listeners,record);
    if(!optionSignal.isUndefined())addNativeAbortAlgorithm(optionSignal,JSValue::function([](const JSValue&,const Args&,const Args& captures)->JSValue {auto record=captures[0];record.set("removed",true);return {};},{record}));
    return {};
}
JSValue signalPrototype() {
    static thread_local JSWeakValue cached;
    if(const auto value=cached.lock();value&&value->isObject())return *value;
    auto value=[] {
        auto result=JSValue::object();
        for(const char* name:{"aborted","reason","onabort"}) {
            JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.enumerable=true;
            descriptor.getter=JSValue::function(signalMethod,{name},std::string("get ")+name);
            if(std::string(name)=="onabort")descriptor.setter=JSValue::function(signalMethod,{"set onabort"},"set onabort",1);
            result.defineProperty(name,descriptor);
        }
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"addEventListener",2},{"removeEventListener",2},{"throwIfAborted",0}}) {
            JSPropertyDescriptor descriptor;descriptor.configurable=descriptor.enumerable=descriptor.writable=true;
            descriptor.value=JSValue::function(signalMethod,{name},name,arity);result.defineProperty(name,descriptor);
        }
        return result;
    }();value.retainForHeapLifetime();cached=JSWeakValue(value);return value;
}
}
JSValue createNativeAbortController() {
    auto controller=makeHost(controllerTag);controller.setHostSlot("signal",createSignal());
    static thread_local JSWeakValue cached;
    auto alive=cached.lock();auto prototype=alive?*alive:JSValue{};
    if(!prototype.isObject()){prototype=[] {
        auto result=JSValue::object();JSPropertyDescriptor signal;signal.configurable=signal.enumerable=true;
        signal.getter=JSValue::function([](const JSValue& self,const Args&,const Args&)->JSValue {
            if(!self.isHostObject(controllerTag))throw JSTypeError("Expected AbortController");return self.getHostSlot("signal");
        },{},"get signal");result.defineProperty("signal",signal);
        JSPropertyDescriptor abort;abort.configurable=abort.enumerable=abort.writable=true;
        abort.value=JSValue::function([](const JSValue& self,const Args& args,const Args&)->JSValue {
            if(!self.isHostObject(controllerTag))throw JSTypeError("Expected AbortController");abortSignal(self.getHostSlot("signal"),arg(args,0));return {};
        },{},"abort",0);result.defineProperty("abort",abort);return result;
    }();prototype.retainForHeapLifetime();cached=JSWeakValue(prototype);}controller.setPrototype(prototype);return controller;
}
JSValue createNativeAbortSignalAny(JSValue signals) {
    auto result=createSignal();std::vector<JSValue> sources;
    const auto length=signals.get("length").toNumber();
    for(double i=0;i<length;++i){auto signal=signals.get(JSValue(i).toString());requireSignal(signal);sources.push_back(signal);}
    for(const auto& signal:sources)if(signal.getHostSlot("aborted").toBoolean()){result.setHostSlot("aborted",true);result.setHostSlot("reason",signal.getHostSlot("reason"));return result;}
    auto retained=JSValue::array();for(auto signal:sources){push(list(signal,"dependents"),result);push(retained,signal);}result.setHostSlot("sources",retained);return result;
}
void addNativeAbortAlgorithm(JSValue signal,JSValue callback) {
    requireSignal(signal);if(signal.getHostSlot("aborted").toBoolean())callback.call();else push(list(signal,"algorithms"),callback);
}
