#include "core/JSSet.h"
#include "core/JSIterator.h"
#include "core/JSSymbol.h"

namespace {
using Args=std::vector<JSValue>;
const JSValue& arg(const Args& args,std::size_t index) {
    static const JSValue undefined;
    return index<args.size()?args[index]:undefined;
}
JSPropertyDescriptor data(const JSValue& value,bool writable=true) {
    JSPropertyDescriptor d;d.value=value;d.writable=writable;d.enumerable=false;d.configurable=true;return d;
}
void install(JSValue prototype,const char* name,JSValue::NativeFunction callback,unsigned arity) {
    prototype.defineProperty(name,data(JSValue::function(callback,{},name,arity)));
}
JSValue initialize(const JSValue& newTarget,const Args& args,const Args&) {
    const auto candidate=newTarget.get("prototype");
    const auto object=JSSet::create(candidate.isObject()?candidate:nativeSetConstructor().get("prototype"));
    const auto& iterable=arg(args,0);
    if(iterable.isNull()||iterable.isUndefined())return object;
    const auto adder=object.get("add");
    if(!adder.isCallable())throw JSTypeError("Set adder is not callable");
    auto iterator=getJSIterator(iterable);
    while(const auto item=stepJSIterator(iterator)) {
        try {
            adder.call(object,{*item});
        } catch(...) {closeJSIterator(iterator,std::current_exception());}
    }
    return object;
}
}
JSSet::JSSet():object_(createNativeSet()) {}
JSSet::JSSet(const JSValue& object) {
    if(!isCollection(object))throw JSTypeError("Set receiver lacks Set data");
    object_=object;
}
bool JSSet::isCollection(const JSValue& value) {return value.hasCollectionData(2);}
JSValue JSSet::create(const JSValue& prototype) {return JSValue::createCollectionData(prototype,2);}
std::size_t JSSet::size() const {return object_.collectionSize(2);}
void JSSet::clear() {object_.collectionClear(2);}
bool JSSet::has(const JSValue& key) const {return object_.collectionHas(key,2);}
bool JSSet::remove(const JSValue& key) {return object_.collectionErase(key,2);}
JSValue JSSet::add(const JSValue& value) {object_.collectionSet(value,{},2);return object_;}
JSValue JSSet::keys() const {return JSIterator::collection(object_,JSIterator::Brand::Set,JSIterator::Kind::Key);}
JSValue JSSet::values() const {return JSIterator::collection(object_,JSIterator::Brand::Set,JSIterator::Kind::Value);}
JSValue JSSet::entries() const {return JSIterator::collection(object_,JSIterator::Brand::Set,JSIterator::Kind::Entry);}
void JSSet::forEach(const JSValue& callback,const JSValue& thisArg) const {object_.collectionForEach(callback,thisArg,2);}
bool isNativeSet(const JSValue& value) {return JSSet::isCollection(value);}
JSValue createNativeSet() {return nativeSetConstructor().construct();}
JSValue nativeSetConstructor() {
    static thread_local JSWeakValue cached;
    if(const auto value=cached.lock();value && value->isCallable())return *value;
    auto constructor=JSValue::builtinConstructor(initialize,{},"Set",0);
    auto prototype=constructor.get("prototype");
    install(prototype,"has",+[](const JSValue& self,const Args& args,const Args&)->JSValue{return JSSet(self).has(arg(args,0));},1);
    install(prototype,"add",+[](const JSValue& self,const Args& args,const Args&)->JSValue{return JSSet(self).add(arg(args,0));},1);
    install(prototype,"delete",+[](const JSValue& self,const Args& args,const Args&)->JSValue{return JSSet(self).remove(arg(args,0));},1);
    install(prototype,"clear",+[](const JSValue& self,const Args& args,const Args&)->JSValue{JSSet(self).clear();return {};},0);
    install(prototype,"entries",+[](const JSValue& self,const Args& args,const Args&)->JSValue{return JSSet(self).entries();},0);
    install(prototype,"forEach",+[](const JSValue& self,const Args& args,const Args&)->JSValue{JSSet(self).forEach(arg(args,0),arg(args,1));return {};},1);
    JSPropertyDescriptor size; size.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{return JSSet(self).size();},{},"get size",0);
    size.enumerable=false;size.configurable=true;prototype.defineProperty("size",size);
    install(prototype,"values",+[](const JSValue& self,const Args& args,const Args&)->JSValue{return JSSet(self).values();},0);
    prototype.defineProperty("keys",data(prototype.get("values")));
    prototype.defineProperty(JSSymbol::toStringTag(),data("Set",false));
    prototype.defineProperty(JSSymbol::iterator(),data(prototype.get("values")));
    JSPropertyDescriptor species;
    species.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{return self;},{},"get [Symbol.species]",0);
    species.enumerable=false;species.configurable=true;
    constructor.defineProperty(JSSymbol::species(),species);
    constructor.retainForHeapLifetime();cached=JSWeakValue(constructor);return constructor;
}
