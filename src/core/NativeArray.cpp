#include "core/NativeArray.h"
#include "core/JSIterator.h"
#include <array>
#include <cmath>
#include <limits>

struct NativeArrayAccess {
    static std::shared_ptr<void> storage(const ArrayBuffer& value) { return value.bytes_; }
    static ArrayBuffer buffer(const std::shared_ptr<void>& payload) {
        return ArrayBuffer(std::static_pointer_cast<std::vector<std::byte>>(payload));
    }
    template<class T> static std::shared_ptr<void> storage(const T& value) { return value.view_; }
    template<class T> static T view(const std::shared_ptr<void>& payload) {
        return T(std::static_pointer_cast<typename T::View>(payload));
    }
    template<class T> static JSNativeOwnerNode owner(const std::shared_ptr<void>& payload) {
        const auto view=std::static_pointer_cast<typename T::View>(payload);
        return {payload,{},{{&view->buffer.bytes_,view->buffer.bytes_}}};
    }
    template<class T> static NativeArrayOwnership ownership(const T& value) {
        return {{{&value.view_,value.view_},{&value.buffer_.bytes_,value.buffer_.bytes_}},
            {{value.view_,{},{{&value.view_->buffer.bytes_,value.view_->buffer.bytes_}}},{value.buffer_.bytes_,{},{}}}};
    }
};
#define THREE_NATIVE_ARRAY_OWNER(Type) NativeArrayOwnership nativeArrayOwnership(const Type& array) { return NativeArrayAccess::ownership(array); }
THREE_NATIVE_ARRAY_OWNER(Int8Array) THREE_NATIVE_ARRAY_OWNER(Uint8Array)
THREE_NATIVE_ARRAY_OWNER(Int16Array) THREE_NATIVE_ARRAY_OWNER(Uint16Array)
THREE_NATIVE_ARRAY_OWNER(Int32Array) THREE_NATIVE_ARRAY_OWNER(Uint32Array)
THREE_NATIVE_ARRAY_OWNER(Float32Array) THREE_NATIVE_ARRAY_OWNER(Float64Array)
THREE_NATIVE_ARRAY_OWNER(Uint8ClampedArray)
#undef THREE_NATIVE_ARRAY_OWNER
namespace {
using Args=std::vector<JSValue>;
constexpr const char* typedTag="three.AnimationArray";
constexpr const char* bufferTag="three.ArrayBuffer";
using Arrays=InterpolantArray::Storage;
constexpr std::array<const char*,9> names{"Int8Array","Uint8Array","Int16Array","Uint16Array",
    "Int32Array","Uint32Array","Float32Array","Float64Array","Uint8ClampedArray"};
JSValue arg(const Args& args,std::size_t i) { return i<args.size()?args[i]:JSValue{}; }
double integer(JSValue value) { const auto n=value.toNumber(); return std::isnan(n)?0:std::trunc(n); }
double lengthOf(JSValue value) { return std::min(std::max(integer(value),0.0),9007199254740991.0); }
template<std::size_t I=0,class F> decltype(auto) select(std::size_t kind,F&& callback) {
    if constexpr(I<8) {
        if(kind==I) return callback.template operator()<std::variant_alternative_t<I,Arrays>>();
        return select<I+1>(kind,std::forward<F>(callback));
    } else return callback.template operator()<Uint8ClampedArray>();
}
std::size_t kindOf(const JSValue& value) {
    if(!value.isHostObject(typedTag)) throw JSTypeError("TypedArray method requires a typed array");
    return std::size_t(value.getHostSlot("kind").number());
}
JSPropertyDescriptor data(JSValue value,bool writable=true,bool enumerable=false,bool configurable=true) {
    JSPropertyDescriptor d;d.value=value;d.writable=writable;d.enumerable=enumerable;d.configurable=configurable;return d;
}
void install(JSValue object,const char* name,JSValue::NativeFunction callback,unsigned arity,const Args& captures={}) {
    object.defineProperty(name,data(JSValue::function(callback,captures,name,arity)));
}
bool numeric(const std::string& key) { return key=="-0"||JSValue(JSValue(key).toNumber()).toString()==key; }
bool valid(const std::string& key,double index,std::size_t size) {
    return key!="-0"&&std::isfinite(index)&&index>=0&&index==std::trunc(index)&&index<double(size);
}
JSHostPropertyResult own(const JSValue& self,const std::string& key) {
    if(!numeric(key)) return {};
    const auto values=NativeArray_detail::unwrap(self); const auto index=JSValue(key).toNumber();
    if(!valid(key,index,values.size())) return {true,{}};
    return {true,data(values.get(index),true,true,true)};
}
std::optional<bool> define(const JSValue& self,const std::string& key,const JSPropertyDescriptor& descriptor) {
    if(!numeric(key)) return {};
    auto values=NativeArray_detail::unwrap(self); const auto index=JSValue(key).toNumber();
    if(!valid(key,index,values.size())) return false;
    if(descriptor.getter||descriptor.setter||(descriptor.configurable&&!*descriptor.configurable)||
        (descriptor.enumerable&&!*descriptor.enumerable)||(descriptor.writable&&!*descriptor.writable)) return false;
    if(descriptor.value) values.set(index,*descriptor.value); return true;
}
std::optional<bool> erase(const JSValue& self,const std::string& key) {
    if(!numeric(key)) return {}; return !own(self,key).descriptor.has_value();
}
std::vector<std::string> ownNames(const JSValue& self) {
    const auto values=NativeArray_detail::unwrap(self);std::vector<std::string> result;
    for(std::size_t i=0;i<values.size();++i) result.push_back(std::to_string(i)); return result;
}
std::optional<bool> set(const JSValue& self,const std::string& key,const JSValue& value,const JSValue& receiver) {
    if(!numeric(key)) return {};
    auto values=NativeArray_detail::unwrap(self);const auto index=JSValue(key).toNumber();
    if(!self.sameValue(receiver)) { if(!valid(key,index,values.size())) return true;return {}; }
    const auto converted=value.toNumber(); if(valid(key,index,values.size())) values.set(index,converted);return true;
}
JSValue metadata(const JSValue& self,const Args&,const Args& captures) {
    const auto kind=kindOf(self);const auto& name=captures[0].string();
    if(name=="buffer") return self.getHostSlot("buffer");
    return select(kind,[&]<class T>()->JSValue {
        const auto array=NativeArrayAccess::view<T>(self.hostPayload(typedTag));
        return double(name=="length"?std::size_t(array.length):name=="byteLength"?std::size_t(array.byteLength):std::size_t(array.byteOffset));
    });
}
JSValue typedConstructor(std::size_t kind);
JSValue bufferConstructor();
JSValue constructTyped(const JSValue& receiver,const Args& args,const Args& captures) {
    const auto kind=std::size_t(captures[0].number()); const auto first=arg(args,0);
    const auto allocationPrototype=[&] {
        const auto prototype=receiver.get("prototype");
        return prototype.isObject()?prototype:typedConstructor(kind).get("prototype");
    };
    const auto inherit=[](const InterpolantArray& array,const JSValue& prototype) {
        auto result=NativeArray_detail::wrap(array);result.setPrototype(prototype);return result;
    };
    return select(kind,[&]<class T>()->JSValue {
        if(first.isHostObject(bufferTag)) {
            const auto prototype=allocationPrototype();
            const auto storage=arrayBuffer(first);
            const auto offset=TypedArray_detail::toIndex(arg(args,1).toNumber());
            // Misalignment is checked before converting the optional length.
            if(offset%T::BYTES_PER_ELEMENT) throw JSRangeError("TypedArray byteOffset is unaligned");
            std::optional<double> count;
            if(!arg(args,2).isUndefined()) count=double(TypedArray_detail::toIndex(arg(args,2).toNumber()));
            return inherit(T(storage,double(offset),count),prototype);
        }
        if(first.isHostObject(typedTag)) {
            const auto prototype=allocationPrototype();
            const auto source=NativeArray_detail::unwrap(first);
            return std::visit([&](const auto& values)->JSValue {
                using S=std::decay_t<decltype(values)>;
                if constexpr(std::is_same_v<S,JSValue>) throw JSTypeError("Expected typed array");
                else if constexpr(std::is_same_v<S,T>) return inherit(values.slice(),prototype);
                else return inherit(T(values),prototype);
            },source.storage());
        }
        if(first.isObject()) {
            const auto iteratorMethod=first.get(JSValue::wellKnownSymbol("iterator"));
            if(!iteratorMethod.isUndefined()&&!iteratorMethod.isNull()) {
                // TypedArray construction runs IterableToList before allocating
                // the backing store or coercing any collected element.
                const auto iterator=iteratorMethod.call(first);
                if(!iterator.isObject()) throw JSTypeError("Iterator must return an object");
                JSIteratorRecord record{iterator,iterator.get("next")};
                std::vector<JSValue> values;
                while(const auto value=stepJSIterator(record)) values.push_back(*value);
                const auto prototype=allocationPrototype();
                T array(values.size());
                for(std::size_t i=0;i<values.size();++i) array.setIndex(double(i),values[i].toNumber());
                return inherit(array,prototype);
            }
            const auto length=lengthOf(first.get("length"));const auto prototype=allocationPrototype();T array(TypedArray_detail::toIndex(length));
            for(double i=0;i<length;++i) array.setIndex(i,first.get(JSValue(i).toString()).toNumber());
            return inherit(array,prototype);
        }
        const auto length=TypedArray_detail::toIndex(first.toNumber());const auto prototype=allocationPrototype();
        return inherit(T(length),prototype);
    });
}
JSValue typedMethod(const JSValue& self,const Args& args,const Args& captures) {
    const auto kind=kindOf(self);const auto& name=captures[0].string();
    return select(kind,[&]<class T>()->JSValue {
        auto array=NativeArrayAccess::view<T>(self.hostPayload(typedTag));
        if(name=="set") {
            const auto offset=integer(arg(args,1));
            if(offset<0) throw JSRangeError("TypedArray.set offset is out of bounds");
            const auto source=arg(args,0);
            if(source.isHostObject(typedTag)) {
                const auto values=NativeArray_detail::unwrap(source);
                std::visit([&](const auto& input) {
                    if constexpr(!std::is_same_v<std::decay_t<decltype(input)>,JSValue>) array.set(input,offset);
                },values.storage()); return {};
            }
            if(source.isNull()||source.isUndefined()) throw JSTypeError("Cannot convert undefined or null to object");
            const auto length=lengthOf(source.get("length"));
            if(!std::isfinite(offset)||offset+length>double(array.size())) throw JSRangeError("TypedArray.set source is too large");
            for(double i=0;i<length;++i) array.setIndex(offset+i,source.get(JSValue(i).toString()).toNumber());
            return {};
        }
        if(name=="slice"||name=="subarray") {
            const auto begin=integer(arg(args,0));std::optional<double> end;
            if(!arg(args,1).isUndefined()) end=integer(arg(args,1));
            const auto constructor=self.get("constructor");
            if(!constructor.isUndefined()&&!constructor.isObject()) throw JSTypeError("TypedArray constructor must be an object");
            return NativeArray_detail::wrap(name=="slice"?array.slice(begin,end):array.subarray(begin,end));
        }
        throw JSTypeError("Unknown TypedArray method");
    });
}
JSValue typedBase() {
    // Intrinsics survive GC within their heap; thread caches must not own it.
    static thread_local JSWeakValue cached;
    if(const auto value=cached.lock();value&&value->isObject())return *value;
    auto result=[] {
        auto value=JSValue::object();
        for(const char* name:{"length","buffer","byteOffset","byteLength"}) {
            JSPropertyDescriptor d;d.getter=JSValue::function(metadata,{name},std::string("get ")+name,0);
            d.configurable=true;value.defineProperty(name,d);
        }
        for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"set",1},{"slice",2},{"subarray",2}})
            install(value,name,typedMethod,arity,{name});
        for(unsigned i=0;i<3;++i){
            const char* iteratorNames[]={"keys","values","entries"};
            const auto function=JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{
                static_cast<void>(kindOf(self)); // TypedArray methods validate their receiver eagerly.
                return JSIterator::array(self,static_cast<JSIterator::Kind>(static_cast<unsigned>(c[0].number())));
            },{i},iteratorNames[i],0);
            value.defineProperty(iteratorNames[i],data(function));
            if(i==1)value.defineProperty(JSValue::wellKnownSymbol("iterator"),data(function));
        }
        auto array=JSValue::array();value.defineProperty("toString",data(array.get("toString")));
        value.defineProperty("join",data(array.get("join")));return value;
    }();
    result.retainForHeapLifetime();cached=JSWeakValue(result);return result;
}
JSValue typedConstructor(std::size_t kind) {
    static thread_local std::array<JSWeakValue,9> constructors;
    if(const auto value=constructors[kind].lock();value&&value->isObject())return *value;
    auto constructor=JSValue::builtinConstructor(constructTyped,{double(kind)},names[kind],3);
    auto prototype=constructor.get("prototype");prototype.setPrototype(typedBase());
    const auto bytes=select(kind,[]<class T>{return double(T::BYTES_PER_ELEMENT);});
    constructor.defineProperty("BYTES_PER_ELEMENT",data(bytes,false,false,false));
    prototype.defineProperty("BYTES_PER_ELEMENT",data(bytes,false,false,false));
    constructor.retainForHeapLifetime();constructors[kind]=JSWeakValue(constructor);
    return constructor;
}
JSValue bufferConstructor() {
    static thread_local JSWeakValue cached;
    if(const auto value=cached.lock();value&&value->isObject())return *value;
    auto constructor=[] {
        auto result=JSValue::constructor([](const JSValue& receiver,const Args& args,const Args&)->JSValue {
            auto value=nativeArrayBuffer(ArrayBuffer(TypedArray_detail::toIndex(arg(args,0).toNumber())));
            value.setPrototype(receiver.getPrototype());return value;
        },{},"ArrayBuffer",1);
        auto prototype=result.get("prototype");JSPropertyDescriptor d;
        d.getter=JSValue::function([](const JSValue& self,const Args&,const Args&)->JSValue {
            return double(arrayBuffer(self).byteLength);
        },{},"get byteLength",0);d.configurable=true;prototype.defineProperty("byteLength",d);
        install(prototype,"slice",[](const JSValue& self,const Args& args,const Args&)->JSValue {
            const auto buffer=arrayBuffer(self);const auto begin=integer(arg(args,0));std::optional<double> end;
            if(!arg(args,1).isUndefined()) end=integer(arg(args,1));
            const auto constructor=self.get("constructor");
            if(!constructor.isUndefined()&&!constructor.isObject()) throw JSTypeError("ArrayBuffer constructor must be an object");
            return nativeArrayBuffer(buffer.slice(begin,end));
        },2);return result;
    }();
    constructor.retainForHeapLifetime();cached=JSWeakValue(constructor);return constructor;
}
}
JSValue nativeArrayBufferConstructor() { return bufferConstructor(); }
JSValue nativeArrayBuffer(const ArrayBuffer& buffer) {
    auto result=JSValue::hostObject(NativeArrayAccess::storage(buffer),buffer.identity(),bufferTag);
    if(result.getHostSlot("initialized").isUndefined()) {
        result.setHostSlot("initialized",true);result.setPrototype(bufferConstructor().get("prototype"));
        static auto hooks=[] { auto h=std::make_shared<JSHostObjectHooks>();
            h->nativeOwnership=[](const JSValue& self) { return std::vector<JSNativeOwnerNode>{{self.hostPayload(bufferTag),{},{}}}; };return h;
        }();result.setHostHooks(hooks);
    }
    return result;
}
ArrayBuffer arrayBuffer(const JSValue& value) { return NativeArrayAccess::buffer(value.hostPayload(bufferTag)); }
JSValue NativeArray_detail::wrap(const InterpolantArray& array) {
    if(const auto ordinary=std::get_if<JSValue>(&array.storage())) return *ordinary;
    const auto kind=array.storage().index();
    return std::visit([&](const auto& value)->JSValue {
        using T=std::decay_t<decltype(value)>;
        if constexpr(std::is_same_v<T,JSValue>) return value;
        else {
            auto result=JSValue::hostObject(NativeArrayAccess::storage(value),value.identity(),typedTag);
            if(result.getHostSlot("initialized").isUndefined()) {
                result.setHostSlot("initialized",true);result.setHostSlot("kind",double(kind));
                result.setHostSlot("buffer",nativeArrayBuffer(value.buffer));result.setPrototype(typedConstructor(kind).get("prototype"));
                static auto hooks=[] {
                    auto h=std::make_shared<JSHostObjectHooks>(JSHostObjectHooks{own,define,erase,ownNames,set});
                    h->nativeOwnership=[](const JSValue& self) {
                        return std::vector<JSNativeOwnerNode>{NativeArrayAccess::owner<T>(self.hostPayload(typedTag))};
                    };return h;
                }();result.setHostHooks(hooks);
            }
            return result;
        }
    },array.storage());
}
JSValue NativeArray_detail::constructor(std::size_t kind) { return typedConstructor(kind); }
InterpolantArray NativeArray_detail::unwrap(const JSValue& value) {
    if(!value.isHostObject(typedTag)) return value;
    const auto kind=kindOf(value);
    return select(kind,[&]<class T>()->InterpolantArray {return NativeArrayAccess::view<T>(value.hostPayload(typedTag));});
}
