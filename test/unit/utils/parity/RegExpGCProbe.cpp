#include "core/JSIterator.h"
#include "core/JSRegExp.h"
#include <iostream>
int main() {
    auto realm = JSValue::object();
    JsonValue::Array snapshots;
    auto take = [&](const char* name, const JSWeakValue& weak) {
        realm.collectGarbage();
        snapshots.emplace_back(JsonValue::Object{{"name", name}, {"alive", !weak.expired()}});
    };
    JSWeakValue cycle;
    {
        auto rx = createNativeRegExp("a");
        auto value = JSValue::object();
        value.set("rx", rx);
        rx.set("lastIndex", value);
        cycle = JSWeakValue(rx);
    }
    take("instance-cycle", cycle);
    auto rx = createNativeRegExp("a");
    JSWeakValue slot;
    {
        auto value = JSValue::object();
        slot = JSWeakValue(value);
        rx.set("lastIndex", value);
    }
    take("lastIndex-retained", slot);
    rx.set("lastIndex", 0);
    take("lastIndex-released", slot);
    JSWeakValue intrinsic;
    {
        auto value = JSValue::object();
        intrinsic = JSWeakValue(value);
        auto method = nativeRegExpPrototype().get("exec");
        method.set("payload", value);
        nativeRegExpPrototype().erase("exec");
    }
    take("deleted-exec-intrinsic", intrinsic);
    JSWeakValue stringIntrinsic;
    {
        auto value = JSValue::object();
        stringIntrinsic = JSWeakValue(value);
        auto method = nativeStringPrototype().get("replace");
        method.set("payload", value);
        nativeStringPrototype().erase("replace");
    }
    take("deleted-string-intrinsic", stringIntrinsic);
    JSWeakValue ordinary;
    {
        auto fn = JSValue::function(+[](const JSValue&, const std::vector<JSValue>&,
                                        const std::vector<JSValue>&) -> JSValue { return {}; });
        auto value = JSValue::object();
        fn.set("payload", value);
        ordinary = JSWeakValue(value);
    }
    take("ordinary-function-control", ordinary);
    std::cout << JsonValue(snapshots).dump() << '\n';
}
