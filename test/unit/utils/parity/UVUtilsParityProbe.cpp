#include "nodes/utils/UVUtils.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "core/JSPromise.h"

#include <iostream>

namespace {
using UtilityNodeRuntime::own;
JSValue describe(const JSValue& value) {
    auto out = JSValue::object(nullptr);
    const auto shaderNode = value.get("shaderNode");
    const auto type = value.get("type");
    own(out, "type", type.isUndefined() ? JSValue(nullptr) : type);
    own(out, "isFn", value.get("isFn").toBoolean());
    own(out, "isNode", value.get("isNode").toBoolean());
    own(out, "isContextNode", value.get("isContextNode").toBoolean());
    own(out, "source", shaderNode.isObject() ? shaderNode.get("jsFunc").toString() : "");
    return out;
}
}

int main() {
  try {
    const auto module = nativeUVUtilsModule();
    const auto uv = nativeTSLBaseBinding("vec2").call({}, {0.25, 0.75});
    const auto rotation = nativeTSLBaseBinding("float").call({}, {0.5});
    auto out = JSValue::object(nullptr);
    UtilityNodeRuntime::own(out, "moduleKeys", JSValue::array());
    for (const auto& name : module.ownPropertyNames()) UtilityNodeRuntime::invoke(out.get("moduleKeys"), "push")({name});
    UtilityNodeRuntime::own(out, "replace", describe(nativeReplaceDefaultUV()));
    UtilityNodeRuntime::own(out, "rotate", describe(nativeRotateUV()));
    UtilityNodeRuntime::own(out, "spherize", describe(nativeSpherizeUV()));
    UtilityNodeRuntime::own(out, "rotateCall", describe(nativeRotateUV().call({}, {uv, rotation})));
    UtilityNodeRuntime::own(out, "spherizeCall", describe(nativeSpherizeUV().call({}, {uv, 1.0})));
    std::cout << out.toJSON()->dump() << '\n';
  } catch (const JSThrownValue& error) {
    std::cerr << "JSThrownValue: " << error.value.toJSON()->dump() << '\n';
    return 2;
  } catch (const std::exception& error) {
    std::cerr << "exception: " << error.what() << '\n';
    return 3;
  }
}
