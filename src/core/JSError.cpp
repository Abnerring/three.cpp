#include "core/JSError.h"
#include "core/JSModuleObject.h"
#include "core/JSWeakMap.h"

namespace {
using Args = std::vector<JSValue>;
struct ErrorData {};
JSValue arg(const Args& a, unsigned i) { return i < a.size() ? a[i] : JSValue{}; }
JSPropertyDescriptor field(const JSValue& value, bool enumerable = false) { JSPropertyDescriptor d; d.value = value; d.writable = d.configurable = true; d.enumerable = enumerable; return d; }
JSValue state() {
    static thread_local JSWeakValue weak; if (const auto value = weak.lock(); value && value->isObject()) return *value;
    auto value = JSValue::object(nullptr); value.set("records", createNativeWeakMap()); value.set("capture", {}); value.set("formatting", false);
    value.retainForHeapLifetime(); weak = JSWeakValue(value); return value;
}
JSValue errorString(const JSValue& self, const Args&, const Args&) {
    if (!self.isObject()) throw JSTypeError("Error.prototype.toString requires an object");
    const auto rawName = self.get("name"); const auto name = rawName.isUndefined() ? std::string("Error") : rawName.toString();
    const auto rawMessage = self.get("message"); const auto message = rawMessage.isUndefined() ? std::string() : rawMessage.toString();
    return name.empty() ? message : message.empty() ? name : name + ": " + message;
}
JSValue stackGet(const JSValue& self, const Args&, const Args& captures) {
    if (!self.isObject()) return {};
    auto module = captures[0]; auto data = JSWeakMap(module.get("records")).get(self); if (data.isUndefined()) return {};
    if (data.get("ready").toBoolean()) return data.get("value");
    if (captures[0].get("formatting").toBoolean()) {
        const auto value = captures[0].get("defaultPrepare").call({}, {self, data.get("frames")});
        data.set("value", value); data.set("ready", true); return value;
    }
    module.set("formatting", true);
    try {
        const auto error = captures[0].get("constructor"); const auto formatter = error.get("prepareStackTrace");
        JSValue value;
        if (formatter.isCallable()) value = error.get("prepareStackTrace").call(error, {self, data.get("frames")});
        else {
            const auto mainFormatter = error.get("prepareStackTrace");
            value = mainFormatter.isCallable() ? error.get("prepareStackTrace").call(error, {self, data.get("frames")}) : captures[0].get("defaultPrepare").call(error, {self, data.get("frames")});
        }
        data.set("value", value); data.set("ready", true); module.set("formatting", false); return value;
    } catch (...) { module.set("formatting", false); throw; }
}
JSValue stackSet(const JSValue& self, const Args& args, const Args& captures) {
    if (!self.isObject()) return {};
    auto data = JSWeakMap(captures[0].get("records")).get(self); if (data.isUndefined()) return {};
    data.set("value", arg(args, 0)); data.set("ready", true); return {};
}
void capture(const JSValue& object, const JSValue& constructorOption, const JSValue& module) {
    if (!object.isObject()) throw JSTypeError("Error.captureStackTrace requires an object");
    if (object.isProxy()) throw JSTypeError("invalid_argument");
    auto data = JSValue::object(nullptr); data.set("ready", false); data.set("formatting", false); data.set("value", {});
    auto frames = JSValue::array(); const auto provider = module.get("capture");
    if (provider.isCallable()) { const auto value = provider.call({}, {object, constructorOption}); if (!value.isUndefined()) frames = value; }
    data.set("frames", frames);
    JSPropertyDescriptor descriptor; descriptor.getter = module.get("get"); descriptor.setter = module.get("set"); descriptor.configurable = true;
    auto target = object; target.defineProperty("stack", descriptor); JSWeakMap(module.get("records")).set(target, data);
}
}
bool isNativeError(const JSValue& value) { return value.isHostObject("runtime.Error"); }
void setNativeErrorStackCapture(const JSValue& callback) {
    if (!callback.isUndefined() && !callback.isCallable()) throw JSTypeError("Error stack capture must be callable or undefined");
    state().set("capture", callback);
}
JSValue nativeErrorConstructor() {
    static thread_local JSWeakValue weak; if (const auto value = weak.lock(); value && value->isConstructible()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "Error"); auto module = state();
    auto ctor = JSValue::callableBuiltinConstructor([](const JSValue&, const JSValue& target, const Args& args, const Args& captures) -> JSValue {
        const auto newTarget = target.isUndefined() ? captures[0].get("constructor") : target;
        auto prototype = newTarget.get("prototype"); if (!prototype.isObject()) prototype = captures[0].get("constructor").get("prototype");
        auto owner = std::make_shared<ErrorData>(); auto self = JSValue::hostObject(owner, owner.get(), "runtime.Error"); self.setPrototype(prototype);
        capture(self, newTarget, captures[0]);
        const auto message = arg(args, 0); if (!message.isUndefined()) self.defineProperty("message", field(message.toString()));
        const auto options = arg(args, 1); if (options.isObject() && options.has("cause")) self.defineProperty("cause", field(options.get("cause")));
        return self;
    }, {module}, "Error", 1);
    module.set("constructor", ctor); module.set("get", JSValue::function(stackGet, {module}, "", 0)); module.set("set", JSValue::function(stackSet, {module}, "", 1));
    auto prototype = ctor.get("prototype"); prototype.defineProperty("name", field("Error")); prototype.defineProperty("message", field(""));
    prototype.defineProperty("toString", field(JSValue::function(errorString, {}, "toString", 0)));
    ctor.defineProperty("captureStackTrace", field(JSValue::function([](const JSValue&, const Args& args, const Args& captures) -> JSValue { capture(arg(args, 0), arg(args, 1), captures[0]); return {}; }, {module}, "captureStackTrace", 2)));
    const auto join = JSValue::intrinsicArrayConstructor().get("prototype").get("join");
    const auto prepare = JSValue::constructor([](const JSValue&, const Args& args, const Args& captures) -> JSValue {
        const auto header = errorString(arg(args, 0), {}, {}).toString(); const auto frames = arg(args, 1);
        if (frames.get("length").strictEquals(0)) return header;
        const auto trace = captures[0].call(frames, {"\n    at "}); return header + "\n    at " + trace.toString();
    }, {join}, "ErrorPrepareStackTrace", 2, JSValue::ConstructorKind::Callable);
    module.set("defaultPrepare", prepare);
    ctor.defineProperty("prepareStackTrace", field(prepare));
    ctor.defineProperty("isError", field(JSValue::function([](const JSValue&, const Args& args, const Args&) -> JSValue { return isNativeError(arg(args, 0)); }, {}, "isError", 1)));
    ctor.defineProperty("stackTraceLimit", field(10, true));
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
