// Direct port of three.js WebGLObjects.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See THREEJS-LICENSE.
#include "renderers/webgl/WebGLObjects.h"
#include "core/JSWeakMap.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
void literal(JSValue object, const char* name, JSValue value) {
    JSPropertyDescriptor d; d.value = std::move(value); d.writable = d.enumerable = d.configurable = true;
    object.defineProperty(name, d);
}
JSValue method(const JSValue&, const Args& args, const Args& captures) {
    const auto& name = captures[0].string();
    auto state = captures[1]; const auto gl = captures[2], geometries = captures[3], attributes = captures[4], bindingStates = captures[5], info = captures[6];
    if (name == "dispose") { state.set("updateMap", createNativeWeakMap()); return {}; }
    if (name == "onInstancedMeshDispose") {
        const auto object = arg(args, 0).get("target"), removeListener = object.get("removeEventListener");
        removeListener.call(object, {"dispose", state.get("onInstancedMeshDispose")});
        const auto release = bindingStates.get("releaseStatesOfObject"); release.call(bindingStates, {object});
        const auto removeMatrix = attributes.get("remove"), matrix = object.get("instanceMatrix"); removeMatrix.call(attributes, {matrix});
        if (!object.get("instanceColor").isNull()) {
            const auto removeColor = attributes.get("remove"), color = object.get("instanceColor"); removeColor.call(attributes, {color});
        }
        return {};
    }
    const auto object = arg(args, 0), frame = info.get("render").get("frame"), geometry = object.get("geometry");
    const auto getGeometry = geometries.get("get"), buffergeometry = getGeometry.call(geometries, {object, geometry});
    auto map = state.get("updateMap"); const auto get = map.get("get");
    if (!get.call(map, {buffergeometry}).strictEquals(frame)) {
        const auto update = geometries.get("update"); update.call(geometries, {buffergeometry});
        map = state.get("updateMap"); const auto set = map.get("set"); set.call(map, {buffergeometry, frame});
    }
    if (object.get("isInstancedMesh").toBoolean()) {
        const auto has = object.get("hasEventListener");
        if (has.call(object, {"dispose", state.get("onInstancedMeshDispose")}).strictEquals(false)) {
            const auto add = object.get("addEventListener"); add.call(object, {"dispose", state.get("onInstancedMeshDispose")});
        }
        map = state.get("updateMap"); const auto getObject = map.get("get");
        if (!getObject.call(map, {object}).strictEquals(frame)) {
            const auto updateMatrix = attributes.get("update"), matrix = object.get("instanceMatrix"), target = gl.get("ARRAY_BUFFER");
            updateMatrix.call(attributes, {matrix, target});
            if (!object.get("instanceColor").isNull()) {
                const auto updateColor = attributes.get("update"), color = object.get("instanceColor"), colorTarget = gl.get("ARRAY_BUFFER");
                updateColor.call(attributes, {color, colorTarget});
            }
            map = state.get("updateMap"); const auto set = map.get("set"); set.call(map, {object, frame});
        }
    }
    if (object.get("isSkinnedMesh").toBoolean()) {
        const auto skeleton = object.get("skeleton"); map = state.get("updateMap"); const auto getSkeleton = map.get("get");
        if (!getSkeleton.call(map, {skeleton}).strictEquals(frame)) {
            const auto update = skeleton.get("update"); update.call(skeleton);
            map = state.get("updateMap"); const auto set = map.get("set"); set.call(map, {skeleton, frame});
        }
    }
    return buffergeometry;
}
}
WebGLObjects::WebGLObjects(JSValue gl, JSValue geometries, JSValue attributes, JSValue bindingStates, JSValue info) : object_(JSValue::object()) {
    auto state = JSValue::object(nullptr); state.set("updateMap", createNativeWeakMap());
    state.set("onInstancedMeshDispose", JSValue::constructor(method, {"onInstancedMeshDispose", state, gl, geometries, attributes, bindingStates, info}, "onInstancedMeshDispose", 1, JSValue::ConstructorKind::Callable));
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"update",1},{"dispose",0}})
        literal(object_, name, JSValue::constructor(method, {name, state, gl, geometries, attributes, bindingStates, info}, name, arity, JSValue::ConstructorKind::Callable));
}
JSValue WebGLObjects::update(JSValue object) { return object_.get("update").call(object_, {object}); }
JSValue WebGLObjects::dispose() { return object_.get("dispose").call(object_); }
