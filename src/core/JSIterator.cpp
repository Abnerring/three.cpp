#include "core/JSIterator.h"
#include "core/JSSymbol.h"
#include "core/JSString.h"
#include <array>

namespace {
using Args = std::vector<JSValue>;
void check(const JSValue& value) { static_cast<void>(JSWeakValue(value)); }
JSPropertyDescriptor data(const JSValue& value, bool writable = true) {
    JSPropertyDescriptor result;
    result.value=value; result.writable=writable; result.enumerable=false; result.configurable=true;
    return result;
}
JSValue getMethod(const JSValue& object, const JSValue& key) {
    const auto method=object.get(key);
    if(method.isUndefined() || method.isNull()) return {};
    if(!method.isCallable()) throw JSTypeError("Iterator method is not callable");
    return method;
}
void returnIterator(JSIteratorRecord& record) {
    const auto method=getMethod(record.iterator,"return");
    if(method.isUndefined()) return;
    const auto result=method.call(record.iterator);
    if(!result.isObject()) throw JSTypeError("Iterator return result is not an object");
}
JSValue next(const JSValue& self,const Args&,const Args& captures) {
    return JSIterator::next(self,static_cast<JSIterator::Brand>(static_cast<unsigned>(captures[0].number())));
}
JSValue arrayMethod(const JSValue& self,const Args&,const Args& captures) {
    return JSIterator::array(self,static_cast<JSIterator::Kind>(static_cast<unsigned>(captures[0].number())));
}
}
JSIteratorRecord getJSIterator(const JSValue& iterable) {
    check(iterable);
    const auto method=getMethod(iterable,JSSymbol::iterator());
    if(method.isUndefined()) throw JSTypeError("Value is not iterable");
    const auto iterator=method.call(iterable);
    if(!iterator.isObject()) throw JSTypeError("Iterator method result is not an object");
    return {iterator,iterator.get("next"),false};
}
std::optional<JSValue> stepJSIterator(JSIteratorRecord& record) {
    check(record.iterator); check(record.nextMethod);
    try {
        const auto result=record.nextMethod.call(record.iterator);
        if(!result.isObject()) throw JSTypeError("Iterator next result is not an object");
        if(result.get("done").toBoolean()) { record.done=true; return std::nullopt; }
        return result.get("value");
    } catch(...) { record.done=true; throw; }
}
void closeJSIterator(JSIteratorRecord& record) {
    check(record.iterator); returnIterator(record);
}
[[noreturn]] void closeJSIterator(JSIteratorRecord& record,std::exception_ptr abrupt) {
    // Throw completion wins even when getting/calling return also throws. The
    // lookup and call still happen, preserving observable cleanup effects.
    try { check(record.iterator); returnIterator(record); } catch(...) {}
    std::rethrow_exception(abrupt);
}
JSValue nativeIteratorPrototype() {
    static thread_local JSWeakValue cached;
    if(const auto value=cached.lock(); value && value->isObject()) return *value;
    auto prototype=JSValue::object();
    prototype.defineProperty(JSSymbol::iterator(),data(JSValue::function(
        +[](const JSValue& self,const Args&,const Args&)->JSValue{return self;},{},"[Symbol.iterator]",0)));
    prototype.retainForHeapLifetime(); cached=JSWeakValue(prototype); return prototype;
}
JSValue nativeIteratorPrototype(JSIterator::Brand brand) {
    static thread_local std::array<JSWeakValue,4> cached;
    const auto index=static_cast<unsigned>(brand)-1;
    if(index>=cached.size()) throw std::logic_error("Invalid iterator brand");
    if(const auto value=cached[index].lock(); value && value->isObject()) return *value;
    auto prototype=JSValue::object(nativeIteratorPrototype());
    prototype.defineProperty("next",data(JSValue::function(next,{index+1},"next",0)));
    const char* names[]={"Map Iterator","Set Iterator","Array Iterator","String Iterator"};
    prototype.defineProperty(JSSymbol::toStringTag(),data(names[index],false));
    prototype.retainForHeapLifetime(); cached[index]=JSWeakValue(prototype); return prototype;
}
JSValue nativeStringPrototype() {
    static thread_local JSWeakValue cached;
    if(const auto value=cached.lock();value && value->isObject())return *value;
    auto prototype=createNativeStringPrototype();
    prototype.defineProperty(JSSymbol::iterator(),data(JSValue::function(
        +[](const JSValue& self,const Args&,const Args&)->JSValue{return JSIterator::string(self);},
        {},"[Symbol.iterator]",0)));
    prototype.retainForHeapLifetime(); cached=JSWeakValue(prototype);return prototype;
}
void initializeNativeIteration(const JSValue& arrayPrototype) {
    auto prototype=arrayPrototype;
    const char* names[]={"keys","values","entries"};
    for(unsigned i=0;i<3;++i) {
        const auto method=JSValue::function(arrayMethod,{i},names[i],0);
        prototype.defineProperty(names[i],data(method));
        if(i==1)prototype.defineProperty(JSSymbol::iterator(),data(method));
    }
}
JSValue JSIterator::collection(const JSValue& value,Brand brand,Kind kind) {
    check(value);
    return JSValue::createIteratorData(nativeIteratorPrototype(brand),value,
        static_cast<unsigned>(brand),static_cast<unsigned>(kind));
}
JSValue JSIterator::array(const JSValue& receiver,Kind kind) {
    check(receiver);
    return JSValue::createIteratorData(nativeIteratorPrototype(Brand::Array),receiver,
        static_cast<unsigned>(Brand::Array),static_cast<unsigned>(kind));
}
JSValue JSIterator::string(const JSValue& receiver) {
    check(receiver);
    return JSValue::createIteratorData(nativeIteratorPrototype(Brand::String),receiver,
        static_cast<unsigned>(Brand::String),static_cast<unsigned>(Kind::Value));
}
JSValue JSIterator::next(const JSValue& receiver,Brand brand) {
    check(receiver);return receiver.iteratorNext(static_cast<unsigned>(brand));
}
