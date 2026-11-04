#include "nodes/accessors/VelocityNode.h"

#include "core/JSWeakMap.h"
#include "math/Matrix4.h"
#include "nodes/accessors/ModelNode.h"
#include "nodes/accessors/Position.h"
#include "nodes/core/UniformGroupNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/utils/UtilityNodeRuntime.h"

#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <stdexcept>


namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }

JSValue dataFor(const JSValue& map, const JSValue& key, const JSValue& matrix4) {
    JSWeakMap weak(map);
    auto data = weak.get(key);
    if (data.isUndefined()) {
        data = JSValue::object(nullptr);
        weak.set(key, data);
    }
    auto matrix = data.get("previousMatrix");
    if (matrix.isUndefined()) {
        matrix = matrix4.construct();
        data.set("previousMatrix", matrix);
    }
    return data;
}

JSValue previousMatrix(const JSValue& map, const JSValue& key, const JSValue& matrix4) {
    auto data = dataFor(map, key, matrix4);
    return data.get("previousMatrix");
}

JSValue copyProjection(const JSValue& self, const JSValue& camera) {
    const auto projection = self.get("projectionMatrix");
    return projection.isNull() || projection.isUndefined() ? camera.get("projectionMatrix") : projection;
}

JSValue updateVelocity(const JSValue& self, const JSValue& frame, const JSValue& table, const JSValue& map) {
    const auto camera = frame.get("camera");
    const auto object = frame.get("object");
    const auto matrix4 = table.get("Matrix4");
    invoke(self.get("previousModelWorldMatrix").get("value"), "copy")({previousMatrix(map, object, matrix4)});

    auto cameraData = dataFor(map, camera, matrix4);
    const auto frameId = frame.get("frameId");
    if (!cameraData.get("frameId").sameValue(frameId)) {
        cameraData.set("frameId", frameId);
        auto previousProjection = cameraData.get("previousProjectionMatrix");
        auto previousView = cameraData.get("previousCameraViewMatrix");
        auto currentProjection = cameraData.get("currentProjectionMatrix");
        auto currentView = cameraData.get("currentCameraViewMatrix");
        if (previousProjection.isUndefined()) {
            previousProjection = matrix4.construct();
            previousView = matrix4.construct();
            currentProjection = matrix4.construct();
            currentView = matrix4.construct();
            invoke(previousProjection, "copy")({copyProjection(self, camera)});
            invoke(previousView, "copy")({camera.get("matrixWorldInverse")});
        } else {
            invoke(previousProjection, "copy")({currentProjection});
            invoke(previousView, "copy")({currentView});
        }
        invoke(currentProjection, "copy")({copyProjection(self, camera)});
        invoke(currentView, "copy")({camera.get("matrixWorldInverse")});
        cameraData.set("previousProjectionMatrix", previousProjection);
        cameraData.set("previousCameraViewMatrix", previousView);
        cameraData.set("currentProjectionMatrix", currentProjection);
        cameraData.set("currentCameraViewMatrix", currentView);
        invoke(self.get("previousProjectionMatrix").get("value"), "copy")({previousProjection});
        invoke(self.get("previousCameraViewMatrix").get("value"), "copy")({previousView});
        invoke(self.get("currentProjectionMatrix").get("value"), "copy")({currentProjection});
    }
    return {};
}

JSValue setupVelocity(const JSValue& self, const JSValue& table) {
    const auto previousModelView = invoke(self.get("previousCameraViewMatrix"), "mul")({self.get("previousModelWorldMatrix")});
    const auto clipCurrent = invoke(invoke(self.get("currentProjectionMatrix"), "mul")({table.get("modelViewMatrix")}), "mul")({table.get("positionLocal")});
    const auto clipPrevious = invoke(invoke(self.get("previousProjectionMatrix"), "mul")({previousModelView}), "mul")({table.get("positionPrevious")});
    const auto ndcCurrent = invoke(clipCurrent.get("xy"), "div")({clipCurrent.get("w")});
    const auto ndcPrevious = invoke(clipPrevious.get("xy"), "div")({clipPrevious.get("w")});
    return invoke(ndcCurrent, "sub")({ndcPrevious});
}

JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver;
    const auto name = captures[0].string();
    const auto table = captures[2];
    const auto map = captures[3];
    if (name == "setProjectionMatrix") {
        self.set("projectionMatrix", arg(args, 0));
        return {};
    }
    if (name == "update") return updateVelocity(self, arg(args, 0), table, map);
    if (name == "updateAfter") {
        const auto object = arg(args, 0).get("object");
        invoke(previousMatrix(map, object, table.get("Matrix4")), "copy")({object.get("matrixWorld")});
        return {};
    }
    if (name == "setup") return setupVelocity(self, table);
    throw std::logic_error("Unknown VelocityNode method: " + name);
}

} // namespace

JSValue nativeVelocityNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "VelocityNode module");
    auto table = JSValue::object(nullptr);
    own(table, "Matrix4", nativeMatrix4Constructor());
    own(table, "uniform", nativeUniformFactory());
    own(table, "renderGroup", nativeUniformGroupNodeBinding("renderGroup"));
    own(table, "modelViewMatrix", nativeModelNodeBinding("modelViewMatrix"));
    own(table, "positionLocal", nativePositionBinding("positionLocal"));
    own(table, "positionPrevious", nativePositionBinding("positionPrevious"));
    auto map = createNativeWeakMap();
    table.set("data", map);

    auto holder = JSValue::object(nullptr);
    auto constructor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args&, const Args& captures) -> JSValue {
            auto self = captures[0].get("constructor").getPrototype().construct({"vec2"}, target);
            const auto table = captures[1];
            const auto matrix = table.get("Matrix4").construct();
            self.set("projectionMatrix", nullptr);
            self.set("updateType", nativeNodeConstantsBinding("NodeUpdateType").get("OBJECT"));
            self.set("updateAfterType", nativeNodeConstantsBinding("NodeUpdateType").get("OBJECT"));
            self.set("previousModelWorldMatrix", table.get("uniform").call({}, {matrix}));
            self.set("currentProjectionMatrix", invoke(table.get("uniform").call({}, {table.get("Matrix4").construct()}), "setGroup")({table.get("renderGroup")}));
            self.set("previousProjectionMatrix", invoke(table.get("uniform").call({}, {table.get("Matrix4").construct()}), "setGroup")({table.get("renderGroup")}));
            self.set("previousCameraViewMatrix", invoke(table.get("uniform").call({}, {table.get("Matrix4").construct()}), "setGroup")({table.get("renderGroup")}));
            return self;
        },
        {holder, table, map}, "VelocityNode", 0);
    holder.set("constructor", constructor);
    constructor.setPrototype(nativeNodeConstructor());
    auto prototype = constructor.get("prototype");
    prototype.setPrototype(nativeNodeConstructor().get("prototype"));
    JSPropertyDescriptor type;
    type.getter = JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue { return "VelocityNode"; }, {}, "get type", 0);
    type.configurable = true;
    constructor.defineProperty("type", type);
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"setProjectionMatrix", 1}, {"update", 1}, {"updateAfter", 1}, {"setup", 1}}) {
        JSPropertyDescriptor descriptor;
        descriptor.value = JSValue::function(method, {name, prototype, table, map}, name, arity);
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }
    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", constructor);
    own(bindings, "velocity", nativeTSLBaseBinding("nodeImmutable").call({}, {constructor}));
    auto module = JSValue::object();
    for (const auto& name : bindings.ownPropertyNames()) own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime();
    table.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeVelocityNodeBinding(const std::string& name) {
    const auto keeper = nativeVelocityNodeModule();
    (void)keeper;
    return lexical().lock()->get(name);
}

JSValue nativeVelocityNodeConstructor() { return nativeVelocityNodeBinding("default"); }
