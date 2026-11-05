#include "lights/AmbientLight.h"
#include "lights/DirectionalLight.h"
#include "lights/HemisphereLight.h"
#include "lights/PointLight.h"
#include "lights/SpotLight.h"
#include "lights/LightShadow.h"
#include "lights/DirectionalLightShadow.h"
#include "lights/PointLightShadow.h"
#include "lights/SpotLightShadow.h"
#include "loaders/ObjectLoader.h"
#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include "core/Object3D.h"
#include "math/Color.h"
#include <array>
#include <iostream>
#include <string>

namespace {
using Args = std::vector<JSValue>;
JSValue object(std::initializer_list<std::pair<const char*, JSValue>> fields = {}) {
    auto value = JSValue::object(); for (const auto& [key, field] : fields) value.set(key, field); return value;
}
JSValue list(const Args& fields = {}) { auto value = JSValue::array(); for (const auto& field : fields) value.set(JSValue(value.length()), field); return value; }
void push(JSValue array, JSValue value) { array.set(JSValue(array.length()), value); }
JSValue call(JSValue self, const char* name, const Args& args = {}) { return self.get(name).call(self, args); }
void getter(JSValue target, const char* name, JSValue::NativeFunction callback, const Args& captures) {
    JSPropertyDescriptor descriptor; descriptor.getter = JSValue::function(callback, captures); descriptor.configurable = true; target.defineProperty(name, descriptor);
}
JSValue shadowState(JSValue shadow) {
    auto out = object();
    for (const auto* key : {"intensity", "bias", "biasNode", "normalBias", "radius", "blurSamples", "mapType", "map", "mapPass", "autoUpdate", "needsUpdate", "focus", "aspect"}) out.set(key, shadow.get(key));
    out.set("mapSize", call(shadow.get("mapSize"), "toArray")); out.set("matrix", call(shadow.get("matrix"), "toArray"));
    out.set("frameExtents", call(shadow, "getFrameExtents").get("toArray").call(call(shadow, "getFrameExtents")));
    out.set("viewports", call(shadow, "getViewportCount"));
    const auto camera = shadow.get("camera"); auto cameraState = object();
    if (camera.isObject()) {
        for (const auto* key : {"type", "near", "far", "left", "right", "top", "bottom", "fov", "aspect"}) cameraState.set(key, camera.get(key));
        cameraState.set("projection", camera.get("projectionMatrix").get("elements"));
    }
    out.set("camera", cameraState); return out;
}
JSValue state(JSValue value) {
    auto out = object();
    if (value.get("isLight").toBoolean()) {
        for (const auto* key : {"type", "intensity", "distance", "decay", "angle", "penumbra", "map", "power"}) out.set(key, value.get(key));
        out.set("position", call(value.get("position"), "toArray")); out.set("color", call(value.get("color"), "toArray"));
        if (value.get("groundColor").isObject()) out.set("groundColor", call(value.get("groundColor"), "toArray"));
        if (value.get("shadow").isObject()) out.set("shadow", shadowState(value.get("shadow")));
    } else return shadowState(value);
    return out;
}
JSValue metadata(JSValue constructor, JSValue parent) {
    auto out = object({{"name", constructor.get("name")}, {"length", constructor.get("length")}, {"source", constructor.nativeFunctionSource()}, {"parent", constructor.getPrototype().sameValue(parent)}});
    auto methods = object(); const auto prototype = constructor.get("prototype");
    for (const auto& key : prototype.ownPropertyNames()) {
        if (key == "constructor") continue;
        const auto d = *prototype.getOwnPropertyDescriptor(key); auto method = object({{"enumerable", d.enumerable.value_or(false)}, {"configurable", d.configurable.value_or(false)}});
        if (d.value) { method.set("name", d.value->get("name")); method.set("length", d.value->get("length")); method.set("source", d.value->nativeFunctionSource()); method.set("writable", d.writable.value_or(false)); }
        if (d.getter && d.getter->isCallable()) { method.set("getName", d.getter->get("name")); method.set("getSource", d.getter->nativeFunctionSource()); }
        if (d.setter && d.setter->isCallable()) { method.set("setName", d.setter->get("name")); method.set("setSource", d.setter->nativeFunctionSource()); }
        methods.set(key, method);
    }
    out.set("methods", methods); out.set("initial", state(constructor.construct())); return out;
}
JSValue dynamicConstructor(JSValue constructor, const Args& fields, JSValue fail) {
    auto data = object({{"events", list()}, {"fail", fail}});
    auto parent = JSValue::builtinConstructor(+[](const JSValue& newTarget, const Args& args, const Args& c) -> JSValue {
        auto data = c[0]; data.set("newTarget", newTarget.sameValue(c[1])); data.set("superArgs", list(args)); auto self = object();
        auto position = object(); position.set("copy", JSValue::function(+[](const JSValue& self, const Args& args, const Args& c) -> JSValue {
            push(c[0].get("events"), list({"copy position", self.sameValue(c[1]), args.at(0).sameValue(nativeObject3DConstructor().get("DEFAULT_UP"))})); return self;
        }, {data, position}));
        getter(self, "position", +[](const JSValue&, const Args&, const Args& c) -> JSValue { push(c[0].get("events"), "get position"); return c[1]; }, {data, position});
        self.set("updateMatrix", JSValue::function(+[](const JSValue& self, const Args&, const Args& c) -> JSValue { push(c[0].get("events"), "updateMatrix"); return self; }, {data}));
        for (unsigned i = 0; i < c[2].length(); ++i) {
            const auto field = c[2].get(JSValue(i));
            JSPropertyDescriptor descriptor; descriptor.configurable = true;
            descriptor.setter = JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue {
                push(c[0].get("events"), c[1]); if (c[0].get("fail").sameValue(c[1])) throw JSThrownValue("field sentinel"); return {};
            }, {data, field}); self.defineProperty(field, descriptor);
        }
        data.set("instance", self); return self;
    }, {data, constructor, list(fields)}, "Parent", 0);
    const auto saved = constructor.getPrototype(); constructor.setPrototype(parent); bool threw = false, returned = false;
    try { returned = constructor.construct({17, 23, "third", nullptr, false, "last"}).sameValue(data.get("instance")); }
    catch (const JSThrownValue& error) { if (!error.value.strictEquals("field sentinel")) { constructor.setPrototype(saved); throw; } threw = true; }
    catch (...) { constructor.setPrototype(saved); throw; }
    constructor.setPrototype(saved);
    return object({{"fail", fail}, {"events", data.get("events")}, {"newTarget", data.get("newTarget")}, {"superArgs", data.get("superArgs")}, {"threw", threw}, {"returnsParentObject", returned}});
}
JSValue dynamicCopy(JSValue constructor, const Args& fields) {
    auto target = constructor.construct(), source = constructor.construct(), events = list(), parent = object(); const auto prototype = constructor.get("prototype"), saved = prototype.getPrototype();
    getter(parent, "copy", +[](const JSValue& self, const Args&, const Args& c) -> JSValue {
        push(c[0], list({"get super.copy", self.sameValue(c[1])}));
        return JSValue::function(+[](const JSValue& self, const Args& args, const Args& c) -> JSValue { push(c[0], list({"call super.copy", self.sameValue(c[1]), args.at(0).sameValue(c[2]), args.size(), args.size() > 1 ? args[1] : JSValue{}})); return 17; }, c);
    }, {events, target, source});
    for (const auto& field : fields) {
        JSValue value = source.get(field);
        if (field.strictEquals("target") || field.strictEquals("shadow")) { value = object(); value.set("clone", JSValue::function(+[](const JSValue& self, const Args&, const Args& c) -> JSValue { push(c[0], list({"clone", c[1], self.sameValue(c[2])})); return object(); }, {events, field, value})); }
        getter(source, field.string().c_str(), +[](const JSValue&, const Args&, const Args& c) -> JSValue { push(c[0], "get source." + c[1].string()); return c[2]; }, {events, field, value});
    }
    auto mutablePrototype = prototype; mutablePrototype.setPrototype(parent); JSValue result;
    try { result = call(target, "copy", {source, false}); } catch (...) { mutablePrototype.setPrototype(saved); throw; }
    mutablePrototype.setPrototype(saved); return object({{"events", events}, {"returnsSelf", result.sameValue(target)}});
}
JSValue cloneCase(JSValue constructor, bool recursive) {
    auto source = constructor.construct(); call(source.get("color"), "setRGB", {.2, .3, .4}); source.set("name", "source"); call(source, "add", {nativeObject3DConstructor().construct()});
    if (source.get("target").isObject()) { source.get("target").set("name", "target"); call(source.get("target"), "add", {nativeObject3DConstructor().construct()}); }
    auto bias = object(), map = object(); const auto shadow = source.get("shadow");
    if (shadow.isObject()) { auto value = shadow; value.set("bias", -.2); value.set("biasNode", bias); value.set("map", map); value.set("mapType", 1015); call(value.get("mapSize"), "set", {128, 256}); }
    if (source.get("isSpotLight").toBoolean()) source.set("map", map);
    const auto copy = call(source, "clone", {recursive}); auto out = object({{"derived", copy.instanceOf(constructor)}, {"distinctColor", !copy.get("color").sameValue(source.get("color"))}, {"children", copy.get("children").get("length")}, {"state", state(copy)}});
    if (shadow.isObject()) { out.set("distinctShadow", !copy.get("shadow").sameValue(shadow)); out.set("distinctCamera", !copy.get("shadow").get("camera").sameValue(shadow.get("camera"))); out.set("biasNodeAlias", copy.get("shadow").get("biasNode").sameValue(bias)); }
    if (source.get("target").isObject()) { out.set("distinctTarget", !copy.get("target").sameValue(source.get("target"))); out.set("targetChildren", copy.get("target").get("children").get("length")); }
    if (source.get("isSpotLight").toBoolean()) out.set("mapAlias", copy.get("map").sameValue(map)); return out;
}
JSValue primitivePrototype() {
    auto newTarget = JSValue::constructor(+[](const JSValue& self, const Args&, const Args&) { return self; }, {}, "NewTarget", 0, JSValue::ConstructorKind::Callable); newTarget.set("prototype", 17);
    auto globals = nativeGlobalObject(); const auto original = globals.get("Object"); globals.set("Object", object({{"prototype", JSValue::object(nullptr)}}));
    JSValue shadow; try { shadow = nativeLightShadowConstructor().construct({}, newTarget); } catch (...) { globals.set("Object", original); throw; }
    globals.set("Object", original);
    return object({{"intrinsicPrototype", shadow.getPrototype().sameValue(JSValue::object().getPrototype())}, {"initial", object({{"intensity", shadow.get("intensity")}, {"bias", shadow.get("bias")}, {"mapType", shadow.get("mapType")}, {"map", shadow.get("map")}, {"mapPass", shadow.get("mapPass")}, {"cameraUndefined", shadow.get("camera").isUndefined()}, {"mapSize", call(shadow.get("mapSize"), "toArray")}})}});
}
JSValue disposal(bool fail) {
    auto shadow = nativeLightShadowConstructor().construct(), events = list(), data = object({{"fail", fail}});
    auto replacement = object(), old = object(), map = object();
    replacement.set("dispose", JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue { push(c[0], "new mapPass"); return {}; }, {events}));
    old.set("dispose", JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue { push(c[0], "old mapPass"); return {}; }, {events}));
    map.set("dispose", JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue { push(c[0], "map"); c[1].get("shadow").set("mapPass", c[2]); if (c[1].get("fail").toBoolean()) throw JSThrownValue("dispose sentinel"); return {}; }, {events, data, replacement}));
    data.set("pass", old); data.set("shadow", shadow);
    getter(shadow, "map", +[](const JSValue&, const Args&, const Args& c) -> JSValue { push(c[0], "get map"); return c[1]; }, {events, map});
    JSPropertyDescriptor pass;
    pass.configurable = true;
    pass.getter = JSValue::function(+[](const JSValue&, const Args&, const Args& c) -> JSValue { push(c[0], "get mapPass"); return c[1].get("pass"); }, {events, data});
    pass.setter = JSValue::function(+[](const JSValue&, const Args& a, const Args& c) -> JSValue { push(c[0], "set mapPass"); auto data = c[1]; data.set("pass", a.at(0)); return {}; }, {events, data});
    shadow.defineProperty("mapPass", pass); bool threw = false;
    try { call(shadow, "dispose"); } catch (const JSThrownValue& error) { if (!error.value.strictEquals("dispose sentinel")) throw; threw = true; }
    return object({{"events", events}, {"threw", threw}, {"replacementRetained", data.get("pass").sameValue(replacement)}});
}
}
int main() {
    try {
        const std::array names{"AmbientLight", "DirectionalLight", "HemisphereLight", "PointLight", "SpotLight", "LightShadow", "DirectionalLightShadow", "PointLightShadow", "SpotLightShadow"};
        const std::array constructors{nativeAmbientLightConstructor(), nativeDirectionalLightConstructor(), nativeHemisphereLightConstructor(), nativePointLightConstructor(), nativeSpotLightConstructor(), nativeLightShadowConstructor(), nativeDirectionalLightShadowConstructor(), nativePointLightShadowConstructor(), nativeSpotLightShadowConstructor()};
        auto result = object(), metadataRows = object(), construction = object(), copies = object(), clones = object(), loader = object();
        for (unsigned i = 0; i < constructors.size(); ++i) metadataRows.set(names[i], metadata(constructors[i], i < 5 ? nativeLightConstructor() : i == 5 ? nativeGlobalBinding("Function").get("prototype") : nativeLightShadowConstructor()));
        const std::array<Args, 5> fields{{{"isAmbientLight", "type"}, {"isDirectionalLight", "type", "target", "shadow"}, {"isHemisphereLight", "type", "groundColor"}, {"isPointLight", "type", "distance", "decay", "shadow"}, {"isSpotLight", "type", "target", "distance", "angle", "penumbra", "decay", "map", "shadow"}}};
        auto objectLoader = nativeObjectLoaderConstructor().construct();
        for (unsigned i = 0; i < 5; ++i) {
            auto rows = list(); push(rows, dynamicConstructor(constructors[i], fields[i], "")); for (const auto& field : fields[i]) push(rows, dynamicConstructor(constructors[i], fields[i], field)); construction.set(names[i], rows);
            clones.set(std::string(names[i]) + ":false", cloneCase(constructors[i], false)); clones.set(std::string(names[i]) + ":true", cloneCase(constructors[i], true));
            const auto parsed = call(objectLoader, "parseObject", {object({{"type", names[i]}, {"uuid", names[i]}}), object(), object(), object(), object()});
            loader.set(names[i], object({{"binding", nativeObjectLoaderBinding(names[i]).sameValue(constructors[i])}, {"prototype", parsed.getPrototype().sameValue(constructors[i].get("prototype"))}, {"state", state(parsed)}}));
        }
        copies.set("DirectionalLight", dynamicCopy(constructors[1], {"target", "shadow"})); copies.set("HemisphereLight", dynamicCopy(constructors[2], {"groundColor"}));
        copies.set("PointLight", dynamicCopy(constructors[3], {"distance", "decay", "shadow"})); copies.set("SpotLight", dynamicCopy(constructors[4], {"distance", "angle", "penumbra", "decay", "map", "target", "shadow"})); copies.set("SpotLightShadow", dynamicCopy(constructors[8], {"focus", "aspect"}));
        result.set("metadata", metadataRows); result.set("construction", construction); result.set("copies", copies); result.set("clones", clones); result.set("loader", loader); result.set("primitivePrototype", primitivePrototype());
        result.set("disposal", list({disposal(false), disposal(true)}));
        std::cout << result.toJSON()->dump() << '\n'; return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
