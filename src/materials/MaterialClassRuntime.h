#pragma once
#include "core/JSValue.h"
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "core/JSString.h"
#include "core/JSNumber.h"
#include "core/JSBoolean.h"
#include <unordered_set>
namespace MaterialClassRuntime {
using Args=std::vector<JSValue>;
inline JSValue arg(const Args& args,unsigned index){return index<args.size()?args[index]:JSValue{};}
inline void own(JSValue object,const std::string& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
inline JSValue add(const JSValue& a,const JSValue& b){const auto left=a.toPrimitive();const auto right=b.toPrimitive();if(left.type()==JSValue::Type::String||right.type()==JSValue::Type::String)return left.toString()+right.toString();return left.toNumber()+right.toNumber();}
inline JSValue numeric(const JSValue& a,const JSValue& b,char operation){const auto left=a.toNumber();const auto right=b.toNumber();if(operation=='-')return left-right;if(operation=='*')return left*right;return left/right;}
inline bool greater(const JSValue& a,const JSValue& b){const auto left=a.toPrimitive(JSValue::PreferredType::Number);const auto right=b.toPrimitive(JSValue::PreferredType::Number);return left.toNumber()>right.toNumber();}
template<class F>void forIn(const JSValue& object,F body){
    if(object.isNull()||object.isUndefined())return;
    auto receiver=object;
    if(!receiver.isObject()){
        if(receiver.type()==JSValue::Type::String)receiver=createNativeStringObject(receiver);
        else if(receiver.type()==JSValue::Type::Number)receiver=createNativeNumberObject(receiver);
        else if(receiver.type()==JSValue::Type::Boolean)receiver=createNativeBooleanObject(receiver);
        else return; // The runtime does not yet expose a Symbol wrapper intrinsic.
    }
    // V8 snapshots candidates across the chain. Proxy descriptor checks belong
    // to the per-key filter, after collection and the preceding loop body.
    std::unordered_set<std::string> visited;std::vector<std::string> keys;
    auto level=receiver;
    while(!level.isNull()){
        const bool proxy=level.isProxy();
        for(const auto& key:level.ownPropertyNames()){
            if(visited.contains(key))continue;
            if(proxy){visited.insert(key);keys.push_back(key);}
            else{
                const auto d=level.getOwnPropertyDescriptor(key);
                if(!d)continue;
                visited.insert(key);if(d->enumerable.value_or(false))keys.push_back(key);
            }
        }
        level=level.getPrototype();
    }
    for(const auto& key:keys){
        auto owner=receiver;
        while(!owner.isNull()){
            const auto d=owner.getOwnPropertyDescriptor(key);
            // Ordinary properties remain candidates when their enumerable flag
            // changes. V8 applies the current flag only at a Proxy boundary.
            if(d){if(!owner.isProxy()||d->enumerable.value_or(false))body(key);break;}
            // Integer-indexed exotic lookup stops at an invalid numeric index;
            // it does not fall through to an inherited numeric property.
            if(owner.isHostObject("three.AnimationArray")&&
               (key=="-0"||JSValue(JSValue(key).toNumber()).toString()==key))break;
            owner=owner.getPrototype();
        }
    }
}
}
