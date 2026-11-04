#include "nodes/core/RotateNode.h"

#include <cmath>

RotateNode::Pair RotateNode::rotate2D(const Pair& point, double radians) {
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    // The matrix has columns (c, s) and (-s, c); multiplying a column vector by it gives this.
    Pair out;
    out.x = c * point.x + (-s) * point.y;
    out.y = s * point.x + c * point.y;
    return out;
}

bool RotateNode::usesSharedSineOrder(char axis) {
    // The first and third axes put the sine where the second puts its negation.
    return axis != 'Y';
}

RotateNode::Column RotateNode::firstColumn(char axis, double radians) {
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    Column out;
    if (axis == 'X') {
        // The first column of the turn about the first axis is untouched.
        out.a = 1.0; out.b = 0.0; out.c = 0.0; out.d = 0.0;
    } else if (axis == 'Y') {
        // Here the sine is negated in the first column and positive in the third, which is the
        // opposite of the other two.
        out.a = c; out.b = 0.0; out.c = -s; out.d = 0.0;
    } else {
        // The turn about the third axis puts the cosine and sine first.
        out.a = c; out.b = s; out.c = 0.0; out.d = 0.0;
    }
    return out;
}

bool RotateNode::orderIsLeftToRight() {
    // The first character names the leftmost factor.
    return true;
}

bool RotateNode::cacheKeyIncludesOrder() {
    // Two nodes differing only in order build different shaders, so the key must differ.
    return true;
}

#include "nodes/core/NodeUtils.h"
#include "nodes/math/MathNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& rotateLexical() {
    static thread_local JSWeakValue weak;
    return weak;
}

JSValue rotateMethod(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    const auto name = captures[0].string();
    if (name == "customCacheKey") {
        return nativeNodeUtilsBinding("hashString").call({}, {self.get("_order")});
    }
    if (name == "setOrder") {
        self.set("_order", arg(args, 0));
        return self;
    }
    if (name == "getOrder") return self.get("_order");
    if (name == "generateNodeType") {
        return invoke(self.get("positionNode"), "getNodeType")({arg(args, 0)});
    }
    if (name == "setup") {
        const auto builder = arg(args, 0);
        const auto position = self.get("positionNode");
        const auto rotation = self.get("rotationNode");
        const auto nodeType = invoke(self, "getNodeType")({builder});
        const auto vec4 = nativeTSLBaseBinding("vec4");
        if (nodeType.strictEquals("vec2")) {
            const auto cosAngle = nativeMathNodeBinding("cos").call({}, {rotation});
            const auto sinAngle = nativeMathNodeBinding("sin").call({}, {rotation});
            const auto rotationMatrix = nativeTSLBaseBinding("mat2").call({}, {
                cosAngle, sinAngle, invoke(sinAngle, "negate")(), cosAngle});
            return invoke(rotationMatrix, "mul")({position});
        }

        const auto cosX = nativeMathNodeBinding("cos").call({}, {rotation.get("x")});
        const auto sinX = nativeMathNodeBinding("sin").call({}, {rotation.get("x")});
        const auto cosY = nativeMathNodeBinding("cos").call({}, {rotation.get("y")});
        const auto sinY = nativeMathNodeBinding("sin").call({}, {rotation.get("y")});
        const auto cosZ = nativeMathNodeBinding("cos").call({}, {rotation.get("z")});
        const auto sinZ = nativeMathNodeBinding("sin").call({}, {rotation.get("z")});
        const auto rotationX = nativeTSLBaseBinding("mat4").call({}, {
            vec4.call({}, {1, 0, 0, 0}),
            vec4.call({}, {0, cosX, sinX, 0}),
            vec4.call({}, {0, invoke(sinX, "negate")(), cosX, 0}),
            vec4.call({}, {0, 0, 0, 1})});
        const auto rotationY = nativeTSLBaseBinding("mat4").call({}, {
            vec4.call({}, {cosY, 0, invoke(sinY, "negate")(), 0}),
            vec4.call({}, {0, 1, 0, 0}),
            vec4.call({}, {sinY, 0, cosY, 0}),
            vec4.call({}, {0, 0, 0, 1})});
        const auto rotationZ = nativeTSLBaseBinding("mat4").call({}, {
            vec4.call({}, {cosZ, sinZ, 0, 0}),
            vec4.call({}, {invoke(sinZ, "negate")(), cosZ, 0, 0}),
            vec4.call({}, {0, 0, 1, 0}),
            vec4.call({}, {0, 0, 0, 1})});

        const auto order = self.get("_order").string();
        const auto axis = [&](char value) -> JSValue {
            if (value == 'X') return rotationX;
            if (value == 'Y') return rotationY;
            return rotationZ;
        };
        JSValue matrixChain = axis(order.at(0));
        matrixChain = invoke(matrixChain, "mul")({axis(order.at(1))});
        matrixChain = invoke(matrixChain, "mul")({axis(order.at(2))});
        const auto position4 = vec4.call({}, {position, 1});
        return invoke(matrixChain, "mul")({position4}).get("xyz");
    }
    if (name == "serialize") {
        const auto data = arg(args, 0);
        superMethod(self, "serialize", captures, {data});
        data.set("order", self.get("_order"));
        return {};
    }
    if (name == "deserialize") {
        const auto data = arg(args, 0);
        superMethod(self, "deserialize", captures, {data});
        self.set("_order", data.get("order"));
        return {};
    }
    throw std::logic_error("Unknown RotateNode method: " + name);
}

} // namespace

JSValue nativeRotateNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "RotateNode module");
    nativeNodeUtils();
    nativeMathNodeModule();
    nativeTSLBase();

    auto holder = JSValue::object(nullptr);
    auto constructor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = superConstruct(target, captures);
            self.set("positionNode", arg(args, 0));
            self.set("rotationNode", arg(args, 1));
            self.set("_order", arg(args, 2, "XYZ"));
            return self;
        }, {holder}, "RotateNode", 2);
    holder.set("constructor", constructor);
    constructor.setPrototype(nativeNodeConstructor());
    auto prototype = constructor.get("prototype");
    prototype.setPrototype(nativeNodeConstructor().get("prototype"));
    JSPropertyDescriptor type;
    type.getter = JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue {
        return "RotateNode";
    }, {}, "get type", 0);
    type.configurable = true;
    constructor.defineProperty("type", type);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
             {"customCacheKey", 0}, {"setOrder", 1}, {"getOrder", 0}, {"generateNodeType", 1},
             {"setup", 1}, {"serialize", 1}, {"deserialize", 1}}) {
        JSPropertyDescriptor descriptor;
        descriptor.value = JSValue::function(rotateMethod, {name, prototype}, name, arity);
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }
    const auto factory = invoke(nativeTSLBaseBinding("nodeProxy").call({}, {constructor}),
                                "setParameterLength")({2, 3});
    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", constructor);
    own(bindings, "rotate", factory);
    bindings.retainForHeapLifetime();
    rotateLexical() = JSWeakValue(bindings);
    auto module = JSValue::object();
    own(module, "default", constructor);
    own(module, "rotate", factory);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeRotateNodeBinding(const std::string& name) {
    const auto keeper = nativeRotateNodeModule();
    (void)keeper;
    return rotateLexical().lock()->get(name);
}

JSValue nativeRotateNodeConstructor() { return nativeRotateNodeBinding("default"); }
