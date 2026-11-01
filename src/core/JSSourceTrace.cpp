#include "core/JSSourceTrace.h"
#include "core/JSValue.h"
#include "core/JSNumber.h"
#include "core/JSBoolean.h"
#include "core/JSString.h"
#include "core/JSSymbol.h"
#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

namespace JSSourceTrace {
namespace {
struct ActiveFrame { Frame frame; std::uint64_t serial; };
thread_local std::vector<ActiveFrame> activeFrames;
thread_local std::vector<FunctionIdentity> activeCalls;
thread_local std::uint64_t nextSerial = 0;
using Args = std::vector<JSValue>;
JSPropertyDescriptor field(JSValue value) {
    JSPropertyDescriptor d; d.value = std::move(value); d.writable = d.configurable = true; return d;
}
const Frame& unwrap(const JSValue& self) {
    if (!self.isHostObject("runtime.CallSite")) throw JSTypeError("CallSite method called on incompatible receiver");
    return *std::static_pointer_cast<Frame>(self.hostPayload("runtime.CallSite"));
}
JSValue nullableString(const std::string& text) { return text.empty() ? JSValue(nullptr) : JSValue(text); }
JSValue nullableNumber(int number) { return number <= 0 ? JSValue(nullptr) : JSValue(number); }
JSValue callSitePrototype() {
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    auto prototype = JSValue::object();
    auto constructor = JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue {
        throw JSTypeError("CallSite is not directly constructible");
    }, {}, "CallSite", 0);
    prototype.defineProperty("constructor", field(constructor));
    for (const auto* name : {"getColumnNumber", "getEnclosingColumnNumber", "getEnclosingLineNumber", "getEvalOrigin", "getFileName", "getFunction", "getFunctionName", "getLineNumber", "getMethodName", "getPosition", "getPromiseIndex", "getScriptNameOrSourceURL", "getScriptHash", "getThis", "getTypeName", "isAsync", "isConstructor", "isEval", "isNative", "isPromiseAll", "isToplevel", "toString"}) {
        auto method = JSValue::function(+[](const JSValue& self, const Args&, const Args& captures) -> JSValue {
            const auto& frame = unwrap(self); const auto& name = captures[0].string();
            if (name == "getColumnNumber") return nullableNumber(frame.location.column);
            if (name == "getEnclosingColumnNumber") return nullableNumber(frame.enclosingLocation.column);
            if (name == "getEnclosingLineNumber") return nullableNumber(frame.enclosingLocation.line);
            if (name == "getEvalOrigin" || name == "getThis" || name == "getFunction") return {};
            if (name == "getFileName" || name == "getScriptNameOrSourceURL") return nullableString(frame.location.file);
            if (name == "getFunctionName") return nullableString(frame.functionName);
            if (name == "getLineNumber") return nullableNumber(frame.location.line);
            if (name == "getMethodName") return nullableString(frame.methodName);
            if (name == "getPosition") return frame.location.position < 0 ? JSValue(nullptr) : JSValue(frame.location.position);
            if (name == "getPromiseIndex") return nullptr;
            if (name == "getScriptHash") return nullableString(frame.location.scriptHash);
            if (name == "getTypeName") return nullableString(frame.typeName);
            if (name == "isAsync") return frame.isAsync;
            if (name == "isConstructor") return frame.isConstructor;
            if (name == "isEval" || name == "isPromiseAll") return false;
            if (name == "isNative") return frame.isNative;
            if (name == "isToplevel") return frame.typeName.empty();
            return format(frame);
        }, {name}, name, 0);
        method.retainForHeapLifetime(); prototype.defineProperty(name, field(method));
    }
    prototype.retainForHeapLifetime(); weak = JSWeakValue(prototype); return prototype;
}
}
NativeCallScope::NativeCallScope(FunctionIdentity identity) : depth_(activeCalls.size()) { activeCalls.push_back(identity); }
NativeCallScope::~NativeCallScope() { activeCalls.resize(depth_); }
FrameScope::FrameScope(Frame frame) : depth_(activeFrames.size()) {
    if (!frame.functionIdentity && !activeCalls.empty()) frame.functionIdentity = activeCalls.back();
    activeFrames.push_back({std::move(frame), ++nextSerial});
}
FrameScope::~FrameScope() { activeFrames.resize(depth_); }
PositionScope::PositionScope(Location location) : index_(activeFrames.size()) {
    if (activeFrames.empty()) return;
    --index_; auto& active = activeFrames[index_]; serial_ = active.serial;
    previous_ = std::move(active.frame.location); active.frame.location = std::move(location);
}
PositionScope::~PositionScope() {
    if (serial_ && index_ < activeFrames.size() && activeFrames[index_].serial == serial_)
        activeFrames[index_].frame.location = std::move(previous_);
}
std::vector<Frame> capture(std::size_t limit, FunctionIdentity filter) {
    auto begin = activeFrames.rbegin(), end = activeFrames.rend();
    if (filter) {
        begin = std::find_if(begin, end, [&](const auto& value) { return value.frame.functionIdentity == filter; });
        if (begin != end) ++begin;
    }
    std::vector<Frame> frames; frames.reserve(std::min(limit, activeFrames.size()));
    while (begin != end && frames.size() < limit) { frames.push_back(begin->frame); ++begin; }
    return frames;
}
std::string receiverTypeName(const JSValue& receiver) {
    switch (receiver.type()) {
    case JSValue::Type::Number: return "Number";
    case JSValue::Type::Boolean: return "Boolean";
    case JSValue::Type::String: return "String";
    case JSValue::Type::Symbol: return "Symbol";
    case JSValue::Type::Array: return "Array";
    case JSValue::Type::Function: return "Function";
    case JSValue::Type::Object:
        if (isNativeNumberObject(receiver)) return "Number";
        if (isNativeBooleanObject(receiver)) return "Boolean";
        if (isNativeStringObject(receiver)) return "String";
        if (isNativeSymbolObject(receiver)) return "Symbol";
        return "Object";
    default: return "";
    }
}
std::string format(const Frame& frame) {
    std::string location = frame.isNative ? "native" : frame.location.file.empty() ? "<anonymous>" : frame.location.file;
    if (!frame.isNative && frame.location.line > 0) {
        location += ':' + std::to_string(frame.location.line);
        if (frame.location.column > 0) location += ':' + std::to_string(frame.location.column);
    }
    std::string name = frame.functionName;
    if (frame.isConstructor) name = "new " + (name.empty() ? std::string("<anonymous>") : name);
    else if (!frame.typeName.empty()) {
        if (name.empty()) name = frame.typeName + '.' + (frame.methodName.empty() ? "<anonymous>" : frame.methodName);
        else if (name.rfind(frame.typeName + '.', 0) != 0) name = frame.typeName + '.' + name;
        if (!frame.methodName.empty() && frame.methodName != frame.functionName && frame.functionName != frame.typeName + '.' + frame.methodName)
            name += " [as " + frame.methodName + ']';
    }
    if (frame.isAsync) name = "async " + name;
    return name.empty() ? location : name + " (" + location + ')';
}
JSValue callSites(const std::vector<Frame>& frames) {
    auto array = JSValue::array(); const auto prototype = callSitePrototype();
    for (const auto& frame : frames) {
        auto payload = std::make_shared<Frame>(frame);
        auto object = JSValue::hostObject(payload, payload.get(), "runtime.CallSite"); object.setPrototype(prototype);
        array.set(std::to_string(array.length()), object);
    }
    return array;
}
} // namespace JSSourceTrace
