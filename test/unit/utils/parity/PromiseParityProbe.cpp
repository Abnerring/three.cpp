#include "core/JSPromise.h"
#include <iostream>

namespace {
using Args=std::vector<JSValue>;
void emit(JSValue events,JSValue value) { events.set(std::to_string(events.length()),value); }
JSValue normalized(JSValue value) {
    if(value.isObject() && value.get("name").type()==JSValue::Type::String) return value.get("name");
    return value;
}
JSValue observer(JSValue events,const char* label) {
    return JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
        auto value=JSValue::array();value.set("0",captures[1]);value.set("1",normalized(args[0]));
        emit(captures[0],value);return args[0];
    },{events,label});
}
void observe(JSValue promise,JSValue events,const char* label="ok") {
    promise.get("then").call(promise,{observer(events,label),observer(events,"error")});
}
JSValue thenable(JSValue events,int mode) {
    auto value=JSValue::object();value.set("value",41);
    JSPropertyDescriptor descriptor;descriptor.configurable=true;
    descriptor.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue {
        emit(captures[0],"get then");
        const auto mode=captures[1].number();
        if(mode==3) throw JSThrownValue("getter failure");
        if(mode==4) return 1;
        return JSValue::function(+[](const JSValue& receiver,const Args& args,const Args& captures)->JSValue {
            emit(captures[0],"call then"); const auto mode=captures[1].number();
            if(mode==1) {args[1].call({}, {"first rejection"});args[0].call({}, {42});}
            else if(mode==2) throw JSThrownValue("then failure");
            else {args[0].call({}, {receiver.get("value")});args[1].call({}, {"ignored"});throw JSThrownValue("ignored throw");}
            return {};
        },captures);
    },{events,mode});
    value.defineProperty("then",descriptor);return value;
}
JSValue run(int fixture) {
    auto events=JSValue::array();const auto constructor=nativePromiseConstructor();
    if(fixture==0) {
        auto executor=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
            emit(captures[0],"executor");args[0].call({}, {7});args[1].call({}, {8});
            emit(captures[0],"resolved");return {};
        },{events});
        observe(createNativePromise(executor),events);
    } else if(fixture>=1 && fixture<=5) {
        auto value=thenable(events,fixture-1),promise=resolveNativePromise(value);
        if(fixture==5) value.erase("then");
        observe(promise,events);
    } else if(fixture==6) {
        auto deferred=createNativePromiseCapability();observe(deferred.promise,events);
        deferred.resolve.call({}, {deferred.promise});deferred.reject.call({}, {9});
    } else if(fixture==7) {
        auto promise=resolveNativePromise(2);
        const auto handler=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
            emit(captures[0],args[0]);return args[0].number()+1;
        },{events});
        promise=promise.get("then").call(promise,{handler});
        observe(promise,events);
    } else if(fixture==8) {
        auto promise=resolveNativePromise(2);
        promise=promise.get("then").call(promise,{JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue {
            throw JSThrownValue("handler failure");
        })}); observe(promise,events);
    } else if(fixture==9) {
        auto promise=rejectNativePromise("original");
        promise=promise.get("catch").call(promise,{JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
            emit(captures[0],args[0]);return "recovered";
        },{events})}); observe(promise,events);
    } else if(fixture>=10 && fixture<=15) {
        auto promise=(fixture%2==0)?resolveNativePromise(17):rejectNativePromise("original");
        auto handler=JSValue::function(+[](const JSValue&,const Args& args,const Args& captures)->JSValue {
            emit(captures[0],"finally");emit(captures[0],args.size());
            const auto fixture=captures[1].number();
            if(fixture>=14) throw JSThrownValue("finally failure");
            if(fixture>=12) return rejectNativePromise("finally rejection");
            return resolveNativePromise(99);
        },{events,fixture});
        promise=promise.get("finally").call(promise,{handler}); observe(promise,events);
    } else if(fixture==16) {
        auto promise=resolveNativePromise(1);emit(events,resolveNativePromise(promise).sameValue(promise));
        promise.set("constructor",{});const auto wrapped=resolveNativePromise(promise);
        emit(events,wrapped.sameValue(promise));observe(wrapped,events);
    } else if(fixture==17) {
        auto deferred=createNativePromiseCapability();
        observe(deferred.promise,events,"first");observe(deferred.promise,events,"second");
        queueMicrotask(JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue {
            emit(captures[0],"queued before");return {};
        },{events}));
        deferred.resolve.call({}, {12});
        queueMicrotask(JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue {
            emit(captures[0],"queued after");return {};
        },{events}));
        observe(deferred.promise,events,"third");
    } else if(fixture==18) {
        auto value=thenable(events,0);
        auto deferred=createNativePromiseCapability();
        deferred.resolve.call({}, {value});deferred.reject.call({}, {"ignored before adoption"});
        value.set("value",53);observe(deferred.promise,events);
    } else if(fixture==19) {
        auto promise=resolveNativePromise(1);promise.set("constructor",nullptr);
        try { promise.get("then").call(promise); } catch(const JSTypeError&) {emit(events,"TypeError");}
    } else if(fixture==20) {
        auto generic=JSValue::object();generic.set("then",JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue {
            emit(captures[0],self.sameValue(captures[1]));emit(captures[0],args[0].isUndefined());emit(captures[0],args[1]);return 42;
        },{events,generic}));
        emit(events,constructor.get("prototype").get("catch").call(generic,{7}));
    } else if(fixture==21) {
        auto value=constructor.get("withResolvers").call(constructor);
        value.get("resolve").call({}, {61});observe(value.get("promise"),events);
    } else if(fixture==22 || fixture==23) {
        auto callback=JSValue::function(+[](const JSValue& self,const Args& args,const Args& captures)->JSValue {
            emit(captures[0],self.isUndefined());emit(captures[0],args[0]);
            if(captures[1].number()==23) throw JSThrownValue("try failure");
            return args[1];
        },{events,fixture});
        observe(constructor.get("try").call(constructor,{callback,"argument",92}),events);
    } else if(fixture==24) {
        auto promise=resolveNativePromise(1),state=JSValue::object();
        auto next=promise.get("then").call(promise,{JSValue::function(+[](const JSValue&,const Args&,const Args& captures)->JSValue {
            return captures[0].get("next");
        },{state})});state.set("next",next);observe(next,events);
    } else if(fixture==25) {
        auto promise=rejectNativePromise("reason");
        promise=promise.get("then").call(promise,{3,4});observe(promise,events);
    } else if(fixture==26) {
        auto promise=createNativePromise(JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue {
            throw JSThrownValue("executor failure");
        }));observe(promise,events);
    } else if(fixture==27) {
        auto promise=createNativePromise(JSValue::function(+[](const JSValue&,const Args& args,const Args&)->JSValue {
            args[0].call({}, {13});throw JSThrownValue("ignored executor failure");
        }));observe(promise,events);
    } else if(fixture==28 || fixture==29) {
        auto target=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue {return {};},
            {},"Target",0,JSValue::ConstructorKind::Callable);
        const auto prototype=fixture==28?constructor.get("prototype"):JSValue::object(constructor.get("prototype"));
        target.set("prototype",fixture==28?JSValue(17):prototype);
        const auto executor=JSValue::function(+[](const JSValue&,const Args& args,const Args&)->JSValue {
            args[0].call({}, {73});return {};
        });
        const auto promise=constructor.construct({executor},target);
        emit(events,promise.getPrototype().sameValue(prototype));
        emit(events,promise.ownPropertyNames().size());observe(promise,events);
    } else if(fixture>=30 && fixture<=33) {
        auto prototype=fixture==33?JSValue::array().getPrototype():JSValue::object().getPrototype();
        const char* key=fixture==30?"resolve":fixture==31?"value":fixture==32?"handler":"0";
        const auto previous=prototype.getOwnPropertyDescriptor(key);
        const auto previousLength=prototype.getOwnPropertyDescriptor("length");
        JSPropertyDescriptor poison;poison.value=17;poison.writable=false;poison.configurable=true;
        prototype.defineProperty(key,poison);
        JSValue promise,record;std::string failure;
        try {
            record=constructor.get("withResolvers").call(constructor);
            promise=record.get("promise");observe(promise,events);
            record.get("resolve").call({}, {81});
        } catch(const std::exception& error) {failure=error.what();}
        if(previous)prototype.defineProperty(key,*previous);else prototype.erase(key);
        if(previousLength)prototype.defineProperty("length",*previousLength);
        emit(events,failure.empty());
        if(!failure.empty())emit(events,failure);
        else emit(events,record.hasOwn("resolve") && record.getOwnPropertyDescriptor("resolve")->writable.value());
    }
    emit(events,"end"); drainMicrotasks();return events;
}
}
int main() {
    try {
        JsonValue::Array result;
        for(int fixture=0;fixture<34;++fixture) result.push_back(*run(fixture).toJSON());
        std::cout<<JsonValue(result).dump()<<'\n';
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
