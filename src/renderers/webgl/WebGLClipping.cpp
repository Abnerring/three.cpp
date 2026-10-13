// Direct port of three.js WebGLClipping.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See THREEJS-LICENSE.
#include "renderers/webgl/WebGLClipping.h"
#include "core/NativeArray.h"
#include "core/NativeObject.h"
#include "math/Matrix3.h"
#include "math/Plane.h"
#include "math/Vector3.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t index) {
    return index < args.size() ? args[index] : JSValue();
}
void literal(JSValue object, const char* name, JSValue value) {
    JSPropertyDescriptor d;
    d.value = std::move(value);
    d.writable = d.enumerable = d.configurable = true;
    object.defineProperty(name, d);
}
std::string key(const JSValue& value) {
    return value.toPrimitive(JSValue::PreferredType::String).toString();
}
JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(), b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String)
        return a.toString() + b.toString();
    const auto x = a.toNumber();
    const auto y = b.toNumber();
    return x + y;
}
double product(const JSValue& left, const JSValue& right) {
    const auto x = left.toNumber();
    const auto y = right.toNumber();
    return x * y;
}
JSValue call(const JSValue& object, const char* name, Args args = {}) {
    return object.get(name).call(object, args);
}
// Internal mathematical instances use the same public prototypes and managed
// fields as the source constructors. This preserves arbitrary copied values
// until the actual arithmetic consumes them; native numeric slots stay native.
JSValue vector() {
    auto result = JSValue::object(nativeVector(std::make_shared<Vector3>()).getPrototype());
    result.set("x", 1);
    result.set("y", 0);
    result.set("z", 0);
    return result;
}
JSValue plane() {
    auto normal = vector();
    auto result = JSValue::object(nativePlanePrototype());
    result.set("isPlane", true);
    result.set("normal", normal);
    result.set("constant", 0);
    return result;
}
JSValue matrix() {
    auto result = JSValue::object(nativeMatrix(std::make_shared<Matrix3>()).getPrototype()),
         elements = JSValue::array();
    for (unsigned i = 0; i < 9; ++i) {
        JSPropertyDescriptor d;
        d.value = i % 4 == 0 ? 1 : 0;
        d.writable = d.enumerable = d.configurable = true;
        elements.defineProperty(std::to_string(i), d);
    }
    result.set("elements", elements);
    return result;
}
JSValue project(const JSValue& closure, const JSValue& planes, const JSValue& camera = {},
                const JSValue& offset = {}, const JSValue& skip = {}) {
    const auto count = planes.isNull() ? JSValue(0) : planes.get("length");
    JSValue destination = nullptr;
    auto uniform = closure.get("uniform"), scope = closure.get("scope");
    if (!count.strictEquals(0)) {
        destination = uniform.get("value");
        if (!skip.strictEquals(true) || destination.isNull()) {
            const auto flatSize = add(offset, product(count, 4));
            const auto view = camera.get("matrixWorldInverse");
            call(closure.get("viewNormalMatrix"), "getNormalMatrix", {view});
            bool allocate = destination.isNull();
            if (!allocate) {
                const auto length = destination.get("length");
                const auto a = length.toNumber();
                const auto b = flatSize.toNumber();
                allocate = a < b;
            }
            if (allocate)
                destination = NativeArray_detail::constructor(6).construct({flatSize});
            auto writeOffset = offset;
            for (double i = 0; !JSValue(i).strictEquals(count); ++i) {
                const auto p = closure.get("plane"), copy = p.get("copy");
                const auto input = planes.get(key(i));
                const auto copied = copy.call(p, {input}), apply = copied.get("applyMatrix4");
                apply.call(copied, {view, closure.get("viewNormalMatrix")});
                const auto normal = p.get("normal"), toArray = normal.get("toArray");
                toArray.call(normal, {destination, writeOffset});
                const auto targetKey = add(writeOffset, 3);
                const auto constant = p.get("constant");
                destination.set(key(targetKey), constant);
                writeOffset = add(writeOffset, 4);
            }
        }
        uniform.set("value", destination);
        uniform.set("needsUpdate", true);
    }
    scope.set("numPlanes", count);
    scope.set("numIntersection", 0);
    return destination;
}
void reset(const JSValue& closure) {
    auto uniform = closure.get("uniform");
    if (!uniform.get("value").strictEquals(closure.get("globalState"))) {
        uniform.set("value", closure.get("globalState"));
        const auto count = closure.get("numGlobalPlanes");
        uniform.set("needsUpdate", count.toNumber() > 0);
    }
    auto scope = closure.get("scope");
    scope.set("numPlanes", closure.get("numGlobalPlanes"));
    scope.set("numIntersection", 0);
}
JSValue init(const JSValue&, const Args& args, const Args& captures) {
    auto closure = captures[0];
    const auto planes = arg(args, 0), enable = arg(args, 1);
    JSValue enabled = !planes.get("length").strictEquals(0);
    if (!enabled.toBoolean()) {
        enabled = enable;
        if (!enabled.toBoolean()) {
            enabled = !closure.get("numGlobalPlanes").strictEquals(0);
            if (!enabled.toBoolean())
                enabled = closure.get("localClippingEnabled");
        }
    }
    closure.set("localClippingEnabled", enable);
    closure.set("numGlobalPlanes", planes.get("length"));
    return enabled;
}
JSValue begin(const JSValue&, const Args&, const Args& captures) {
    auto closure = captures[0];
    closure.set("renderingShadows", true);
    project(closure, nullptr);
    return {};
}
JSValue end(const JSValue&, const Args&, const Args& captures) {
    auto closure = captures[0];
    closure.set("renderingShadows", false);
    return {};
}
JSValue global(const JSValue&, const Args& args, const Args& captures) {
    auto closure = captures[0];
    const auto result = project(closure, arg(args, 0), arg(args, 1), 0);
    closure.set("globalState", result);
    return {};
}
JSValue state(const JSValue& self, const Args& args, const Args& captures) {
    auto closure = captures[0];
    const auto material = arg(args, 0), camera = arg(args, 1), cache = arg(args, 2);
    const auto planes = material.get("clippingPlanes"),
               intersection = material.get("clipIntersection"),
               shadows = material.get("clipShadows");
    const auto properties = call(closure.get("properties"), "get", {material});
    if (!closure.get("localClippingEnabled").toBoolean() || planes.isNull() ||
        planes.get("length").strictEquals(0) ||
        (closure.get("renderingShadows").toBoolean() && !shadows.toBoolean())) {
        if (closure.get("renderingShadows").toBoolean())
            project(closure, nullptr);
        else
            reset(closure);
    } else {
        const auto nGlobal = closure.get("renderingShadows").toBoolean()
                                 ? JSValue(0)
                                 : closure.get("numGlobalPlanes");
        const auto lGlobal = product(nGlobal, 4);
        auto destination = properties.get("clippingState");
        if (!destination.toBoolean())
            destination = nullptr;
        closure.get("uniform").set("value", destination);
        destination = project(closure, planes, camera, lGlobal, cache);
        for (double i = 0; i != lGlobal; ++i)
            destination.set(key(i), closure.get("globalState").get(key(i)));
        auto materialProperties = properties;
        materialProperties.set("clippingState", destination);
        auto receiver = self;
        receiver.set("numIntersection",
                     intersection.toBoolean() ? self.get("numPlanes") : JSValue(0));
        const auto current = self.get("numPlanes");
        receiver.set("numPlanes", add(current, nGlobal));
    }
    return {};
}
JSValue construct(const JSValue& self, const Args& args, const Args&) {
    auto closure = JSValue::object(nullptr);
    literal(closure, "scope", self);
    literal(closure, "properties", arg(args, 0));
    literal(closure, "globalState", nullptr);
    literal(closure, "numGlobalPlanes", 0);
    literal(closure, "localClippingEnabled", false);
    literal(closure, "renderingShadows", false);
    literal(closure, "plane", plane());
    literal(closure, "viewNormalMatrix", matrix());
    auto uniform = JSValue::object();
    literal(uniform, "value", nullptr);
    literal(uniform, "needsUpdate", false);
    literal(closure, "uniform", uniform);
    auto receiver = self;
    receiver.set("uniform", uniform);
    receiver.set("numPlanes", 0);
    receiver.set("numIntersection", 0);
    const struct {
        const char* name;
        JSValue::NativeFunction function;
        unsigned arity;
    } methods[] = {{"init", init, 2},
                   {"beginShadows", begin, 0},
                   {"endShadows", end, 0},
                   {"setGlobalState", global, 2},
                   {"setState", state, 3}};
    for (const auto& method : methods)
        receiver.set(method.name, JSValue::constructor(method.function, {closure}, "", method.arity,
                                                       JSValue::ConstructorKind::Callable));
    return {};
}
void validate(std::initializer_list<const JSValue*> values) {
    for (const auto* value : values)
        static_cast<void>(JSWeakValue(*value));
}
} // namespace
JSValue nativeWebGLClippingConstructor() {
    static thread_local JSWeakValue cached;
    if (const auto c = cached.lock(); c && c->isObject())
        return *c;
    auto c =
        JSValue::constructor(construct, {}, "WebGLClipping", 1, JSValue::ConstructorKind::Callable);
    c.retainForHeapLifetime();
    cached = JSWeakValue(c);
    return c;
}
WebGLClipping::WebGLClipping(const JSValue& properties) {
    validate({&properties});
    object_ = nativeWebGLClippingConstructor().construct({properties});
}
JSValue WebGLClipping::init(const JSValue& planes, const JSValue& enable) {
    validate({&object_, &planes, &enable});
    return call(object_, "init", {planes, enable});
}
JSValue WebGLClipping::beginShadows() {
    validate({&object_});
    return call(object_, "beginShadows");
}
JSValue WebGLClipping::endShadows() {
    validate({&object_});
    return call(object_, "endShadows");
}
JSValue WebGLClipping::setGlobalState(const JSValue& planes, const JSValue& camera) {
    validate({&object_, &planes, &camera});
    return call(object_, "setGlobalState", {planes, camera});
}
JSValue WebGLClipping::setState(const JSValue& material, const JSValue& camera,
                                const JSValue& cache) {
    validate({&object_, &material, &camera, &cache});
    return call(object_, "setState", {material, camera, cache});
}
