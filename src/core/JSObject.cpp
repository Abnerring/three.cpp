#include "core/JSObject.h"
#include "core/JSString.h"
namespace {
using Args=std::vector<JSValue>;
JSValue assign(const JSValue&,const Args& args,const Args&) {
    auto target=args.empty()?JSValue():args[0];
    if(target.isUndefined()||target.isNull())throw JSTypeError("Cannot convert undefined or null to object");
    if(!target.isObject()) {
        if(target.type()==JSValue::Type::String)target=createNativeStringObject(target);
        else throw JSTypeError("Number, Boolean and Symbol target boxing is not implemented");
    }
    for(std::size_t i=1;i<args.size();++i) {
        auto source=args[i];
        if(source.isUndefined()||source.isNull())continue;
        if(!source.isObject()) {
            if(source.type()==JSValue::Type::String)source=createNativeStringObject(source);
            else continue; // These primitive wrappers have no own enumerable keys.
        }
        const auto keys=source.ownPropertyKeys();
        for(const auto& key:keys) {
            const auto descriptor=source.getOwnPropertyDescriptor(key);
            if(descriptor&&descriptor->enumerable.value_or(false)) {
                const auto value=source.get(key);target.set(key,value);
            }
        }
    }
    return target;
}
}
void initializeNativeObjectMethods(const JSValue& constructor) {
    JSPropertyDescriptor d;d.value=JSValue::function(assign,{},"assign",2);
    d.writable=d.configurable=true;d.enumerable=false;
    auto object=constructor;object.defineProperty("assign",d);
}
