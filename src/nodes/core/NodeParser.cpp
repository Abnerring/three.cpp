#include "nodes/core/NodeParser.h"
#include "core/JSModuleObject.h"
#include "utils.h"

JSValue nativeNodeParserConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto live = weak.lock(); live && live->isConstructible()) return *live;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "NodeParser");
    auto constructor = JSValue::constructor([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue { return {}; }, {}, "NodeParser", 0);
    JSPropertyDescriptor method;
    method.value = JSValue::function([](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue { ThreeUtils::warn({"Abstract function."}); return {}; }, {}, "parseFunction", 0);
    method.writable = method.configurable = true; constructor.get("prototype").defineProperty("parseFunction", method);
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
NodeParser::NodeParser() : object_(constructor().construct()) {}
JSValue NodeParser::parseFunction(const JSValue& source) const { return object_.get("parseFunction").call(object_, {source}); }
