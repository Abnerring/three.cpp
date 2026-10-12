#pragma once
#include "core/NativeObject.h"
#include "core/Object3D.h"

namespace VertexHelper_detail {
using Args=std::vector<JSValue>;
template<class T> std::shared_ptr<T> helper(const JSValue& self) {
    auto value=std::dynamic_pointer_cast<T>(std::static_pointer_cast<Object3D>(self.hostPayload("three.Object3D")));
    if(!value)throw JSTypeError("Incompatible vertex helper receiver");return value;
}
template<class T> void initialize(JSValue wrapper) {
    for(const auto* key:{"object","size"}) {
        JSPropertyDescriptor d;d.enumerable=true;d.configurable=true;
        d.getter=JSValue::function([](const JSValue& self,const Args&,const Args& captures)->JSValue {
            auto h=helper<T>(self);return captures[0].string()=="size"?h->size:nativeObject(h->object);
        },{key});
        d.setter=JSValue::function([](const JSValue& self,const Args& args,const Args& captures)->JSValue {
            auto h=helper<T>(self);const auto value=args.empty()?JSValue():args[0];
            if(captures[0].string()=="size")h->size=value;
            else h->object=value.isNull()||value.isUndefined()?nullptr:std::static_pointer_cast<Object3D>(value.hostPayload("three.Object3D"));
            return {};
        },{key});
        wrapper.defineProperty(key,d);
    }
}
}
