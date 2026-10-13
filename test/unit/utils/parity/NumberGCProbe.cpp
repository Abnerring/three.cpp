#include "core/JSNumber.h"
#include <iostream>
void own(JSValue object, const char* key, const JSValue& value) {
    JSPropertyDescriptor d;
    d.value = value;
    d.writable = d.enumerable = d.configurable = true;
    object.defineProperty(key, d);
}
int main() {
    const auto realm = JSValue::object();
    JsonValue::Array result;
    JSWeakValue weakBox, weakPayload;
    JSValue kept;
    {
        auto box = createNativeNumberObject(1.25), payload = JSValue::object();
        own(box, "self", box);
        own(box, "payload", payload);
        weakBox = JSWeakValue(box);
        weakPayload = JSWeakValue(payload);
        kept = box;
    }
    realm.collectGarbage();
    result.emplace_back(JsonValue::Object{
        {"step", "boxed-kept"}, {"box", !weakBox.expired()}, {"payload", !weakPayload.expired()}});
    kept = {};
    realm.collectGarbage();
    result.emplace_back(JsonValue::Object{
        {"step", "boxed-drop"}, {"box", !weakBox.expired()}, {"payload", !weakPayload.expired()}});
    {
        auto payload = JSValue::object();
        weakPayload = JSWeakValue(payload);
        own(nativeNumberPrototype(), "payload", payload);
    }
    realm.collectGarbage();
    result.emplace_back(
        JsonValue::Object{{"step", "prototype-kept"}, {"payload", !weakPayload.expired()}});
    nativeNumberPrototype().erase("payload");
    realm.collectGarbage();
    result.emplace_back(
        JsonValue::Object{{"step", "prototype-delete"}, {"payload", !weakPayload.expired()}});
    JSWeakValue weakMethod;
    {
        auto method = nativeNumberPrototype().get("toFixed"), payload = JSValue::object();
        weakMethod = JSWeakValue(method);
        weakPayload = JSWeakValue(payload);
        own(method, "payload", payload);
        kept = method;
        nativeNumberPrototype().erase("toFixed");
    }
    realm.collectGarbage();
    result.emplace_back(JsonValue::Object{{"step", "method-kept"},
                                          {"method", !weakMethod.expired()},
                                          {"payload", !weakPayload.expired()},
                                          {"result", kept.call(1.25, {1}).string()}});
    kept = {};
    realm.collectGarbage();
    result.emplace_back(JsonValue::Object{{"step", "method-drop"},
                                          {"method", !weakMethod.expired()},
                                          {"payload", !weakPayload.expired()}});
    std::cout << JsonValue(std::move(result)).dump();
}
