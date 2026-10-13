#include "core/JSMap.h"
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
    const auto object=JSMap::create(candidate.isObject()?candidate:nativeMapConstructor().get("prototype"));
    const auto& iterable=arg(args,0);
    if(iterable.isNull()||iterable.isUndefined())return object;
    const auto adder=object.get("set");
    if(!adder.isCallable())throw JSTypeError("Map adder is not callable");
    auto iterator=getJSIterator(iterable);
    while(const auto item=stepJSIterator(iterator)) {
        try {
            if(!item->isObject())throw JSTypeError("Map iterator entry is not an object");
            const auto key=item->get("0");
            const auto value=item->get("1");
            adder.call(object,{key,value});
        } catch(...) {closeJSIterator(iterator,std::current_exception());}
    }
    return object;
}
}
JSMap::JSMap():object_(createNativeMap()) {}
JSMap::JSMap(const JSValue& object) {
    if(!isCollection(object))throw JSTypeError("Map receiver lacks Map data");
    object_=object;
}
bool JSMap::isCollection(const JSValue& value) {return value.hasCollectionData(1);}
JSValue JSMap::create(const JSValue& prototype) {return JSValue::createCollectionData(prototype,1);}
std::size_t JSMap::size() const {return object_.collectionSize(1);}
void JSMap::clear() {object_.collectionClear(1);}
bool JSMap::has(const JSValue& key) const {return object_.collectionHas(key,1);}
bool JSMap::remove(const JSValue& key) {return object_.collectionErase(key,1);}
JSValue JSMap::get(const JSValue& key) const {return object_.collectionGet(key,1);}
JSValue JSMap::set(const JSValue& key,const JSValue& value) {object_.collectionSet(key,value,1);return object_;}
JSValue JSMap::keys() const {return JSIterator::collection(object_,JSIterator::Brand::Map,JSIterator::Kind::Key);}
JSValue JSMap::values() const {return JSIterator::collection(object_,JSIterator::Brand::Map,JSIterator::Kind::Value);}
JSValue JSMap::entries() const {return JSIterator::collection(object_,JSIterator::Brand::Map,JSIterator::Kind::Entry);}
void JSMap::forEach(const JSValue& callback,const JSValue& thisArg) const {object_.collectionForEach(callback,thisArg,1);}
bool isNativeMap(const JSValue& value) {return JSMap::isCollection(value);}
JSValue createNativeMap() {return nativeMapConstructor().construct();}
JSValue nativeMapConstructor() {
    static thread_local JSWeakValue cached;
    if(const auto value=cached.lock();value && value->isCallable())return *value;
    auto constructor=JSValue::builtinConstructor(initialize,{},"Map",0);
    auto prototype=constructor.get("prototype");
    install(prototype,"get",+[](const JSValue& self,const Args& args,const Args&)->JSValue{return JSMap(self).get(arg(args,0));},1);
    install(prototype,"set",+[](const JSValue& self,const Args& args,const Args&)->JSValue{return JSMap(self).set(arg(args,0),arg(args,1));},2);
    install(prototype,"has",+[](const JSValue& self,const Args& args,const Args&)->JSValue{return JSMap(self).has(arg(args,0));},1);
    install(prototype,"delete",+[](const JSValue& self,const Args& args,const Args&)->JSValue{return JSMap(self).remove(arg(args,0));},1);
    install(prototype,"clear",+[](const JSValue& self,const Args& args,const Args&)->JSValue{JSMap(self).clear();return {};},0);
    install(prototype,"entries",+[](const JSValue& self,const Args& args,const Args&)->JSValue{return JSMap(self).entries();},0);
    install(prototype,"forEach",+[](const JSValue& self,const Args& args,const Args&)->JSValue{JSMap(self).forEach(arg(args,0),arg(args,1));return {};},1);
    install(prototype,"keys",+[](const JSValue& self,const Args& args,const Args&)->JSValue{return JSMap(self).keys();},0);
    JSPropertyDescriptor size; size.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{return JSMap(self).size();},{},"get size",0);
    size.enumerable=false;size.configurable=true;prototype.defineProperty("size",size);
    install(prototype,"values",+[](const JSValue& self,const Args& args,const Args&)->JSValue{return JSMap(self).values();},0);
    prototype.defineProperty(JSSymbol::toStringTag(),data("Map",false));
    prototype.defineProperty(JSSymbol::iterator(),data(prototype.get("entries")));
    JSPropertyDescriptor species;
    species.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{return self;},{},"get [Symbol.species]",0);
    species.enumerable=false;species.configurable=true;
    constructor.defineProperty(JSSymbol::species(),species);
    constructor.retainForHeapLifetime();cached=JSWeakValue(constructor);return constructor;
}
