#include "core/NativeObject.h"
#include "core/JSValue.h"
#include "materials/MeshBasicMaterial.h"
#include "materials/MeshStandardMaterial.h"
#include <iostream>
#include <memory>
int main() {
    auto basic=nativeMaterial(std::make_shared<MeshBasicMaterial>()),standard=nativeMaterial(std::make_shared<MeshStandardMaterial>());
    const auto callback=basic.get("onBeforeCompile"),keyMethod=basic.get("customProgramCacheKey");
    auto result=JSValue::object();
    result.set("callbackName",callback.get("name"));result.set("callbackLength",callback.get("length"));result.set("callbackConstructible",callback.isConstructible());
    result.set("callbackSource",callback.get("toString").call(callback));
    result.set("keyMethodName",keyMethod.get("name"));result.set("keyMethodSource",keyMethod.get("toString").call(keyMethod));
    result.set("defaultKey",keyMethod.call(basic));
    result.set("sameDefaultCallback",callback.strictEquals(standard.get("onBeforeCompile")));
    result.set("sameKeyMethod",keyMethod.strictEquals(standard.get("customProgramCacheKey")));
    auto log=JSValue::array();auto replacement=JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&){return JSValue();});
    JSPropertyDescriptor descriptor;
    descriptor.getter=JSValue::function([](const JSValue& self,const std::vector<JSValue>&,const std::vector<JSValue>& captures){
        auto events=captures[0];events.get("push").call(events,{JSValue("get-toString")});
        return JSValue::function([](const JSValue& receiver,const std::vector<JSValue>& args,const std::vector<JSValue>& nested){
            const auto events=nested[0];events.get("push").call(events,{JSValue("call-toString"),JSValue(receiver.strictEquals(nested[1])),JSValue(double(args.size()))});return nested[2];
        },{events,self,captures[1]});
    },{log,JSValue("custom-source")});
    descriptor.configurable=true;replacement.defineProperty("toString",descriptor);
    basic.set("onBeforeCompile",replacement);result.set("dynamicKey",keyMethod.call(basic));result.set("dynamicLog",log);
    result.set("keyConstructible",keyMethod.isConstructible());
    result.set("callbackHasOwnToString",callback.getOwnPropertyDescriptor("toString").has_value());
    result.set("keyHasOwnToString",keyMethod.getOwnPropertyDescriptor("toString").has_value());
    result.set("callbackHasOwnPrototype",callback.getOwnPropertyDescriptor("prototype").has_value());
    result.set("keyHasOwnPrototype",keyMethod.getOwnPropertyDescriptor("prototype").has_value());
    auto describeName=[](const JSValue& fn,const char* name){auto d=fn.getOwnPropertyDescriptor(name);auto out=JSValue::array();out.get("push").call(out,{JSValue(*d->writable),JSValue(*d->enumerable),JSValue(*d->configurable)});return out;};
    result.set("callbackNameDescriptor",describeName(callback,"name"));result.set("keyNameDescriptor",describeName(keyMethod,"name"));
    basic.set("onBeforeCompile",callback);
    auto arbitrary=JSValue::object(),prototype=callback.getPrototype();const auto old=*prototype.getOwnPropertyDescriptor("toString");
    auto getterLog=JSValue::array();JSPropertyDescriptor d;
    d.getter=JSValue::function([](const JSValue& receiver,const std::vector<JSValue>&,const std::vector<JSValue>& captures){
        const auto events=captures[0];events.get("push").call(events,{JSValue("get-inherited-toString"),JSValue(receiver.sameValue(captures[1]))});
        return JSValue::function([](const JSValue& receiver,const std::vector<JSValue>& args,const std::vector<JSValue>& captures){const auto events=captures[0];events.get("push").call(events,{JSValue("call-inherited-toString"),JSValue(receiver.sameValue(captures[1])),JSValue(double(args.size()))});return captures[2];},captures);
    },{getterLog,callback,arbitrary});d.configurable=true;prototype.defineProperty("toString",d);
    result.set("inheritedArbitraryIdentity",keyMethod.call(basic).sameValue(arbitrary));result.set("inheritedLog",getterLog);
    prototype.defineProperty("toString",old);
    result.set("restoredDefaultKey",keyMethod.call(basic));
    auto throwLog=JSValue::array();JSPropertyDescriptor throwing;
    throwing.getter=JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>& captures)->JSValue {const auto events=captures[0];events.get("push").call(events,{JSValue("throw-getter")});throw JSTypeError("source-getter");},{throwLog});throwing.configurable=true;
    replacement.defineProperty("toString",throwing);basic.set("onBeforeCompile",replacement);
    try{keyMethod.call(basic);result.set("throwName","none");}catch(const JSTypeError&){result.set("throwName","TypeError");}
    result.set("throwLog",throwLog);
    auto reentryLog=JSValue::array();JSPropertyDescriptor reentry;
    reentry.getter=JSValue::function([](const JSValue& receiver,const std::vector<JSValue>&,const std::vector<JSValue>& captures){
        auto material=captures[0];material.set("onBeforeCompile",captures[1]);
        return JSValue::function([](const JSValue& self,const std::vector<JSValue>&,const std::vector<JSValue>& captures){const auto events=captures[0];events.get("push").call(events,{JSValue(self.sameValue(captures[1]))});return JSValue("snapshot-source");},{captures[2],receiver});
    },{basic,callback,reentryLog});reentry.configurable=true;replacement.defineProperty("toString",reentry);
    result.set("reentryKey",keyMethod.call(basic));result.set("reentryLog",reentryLog);result.set("reentryNextKey",keyMethod.call(basic));
    std::cout<<result.toJSON()->dump()<<'\n';
}
