#include "nodes/parsers/GLSLNodeParser.h"
#include "nodes/core/NodeParser.h"
#include "nodes/parsers/GLSLNodeFunction.h"
#include "core/JSModuleObject.h"

JSValue nativeGLSLNodeParserConstructor() {
    static thread_local JSWeakValue weak;
    if (const auto live = weak.lock(); live && live->isConstructible()) return *live;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "GLSLNodeParser");
    auto holder = JSValue::object(nullptr); const auto base = nativeNodeParserConstructor(), function = nativeGLSLNodeFunctionConstructor();
    auto constructor = JSValue::builtinConstructor([](const JSValue& newTarget, const std::vector<JSValue>& args, const std::vector<JSValue>& captures) -> JSValue {
        return captures[0].get("constructor").getPrototype().construct(args, newTarget);
    }, {holder}, "GLSLNodeParser", 0);
    holder.set("constructor", constructor); constructor.setPrototype(base); auto prototype = constructor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor method; method.value = JSValue::function([](const JSValue&, const std::vector<JSValue>& args, const std::vector<JSValue>& captures) -> JSValue {
        return captures[0].construct({args.empty() ? JSValue{} : args[0]});
    }, {function}, "parseFunction", 1); method.writable = method.configurable = true; prototype.defineProperty("parseFunction", method);
    constructor.retainForHeapLifetime(); weak = JSWeakValue(constructor); return constructor;
}
GLSLNodeParser::GLSLNodeParser() : object_(constructor().construct()) {}
JSValue GLSLNodeParser::parseFunction(const JSValue& source) const { return object_.get("parseFunction").call(object_, {source}); }
