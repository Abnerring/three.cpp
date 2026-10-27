#include "catch.hpp"
#include "core/NativeObject.h"
#include "core/NativeRenderingObject.h"
#include "core/BufferGeometry.h"
#include "core/JSJobs.h"
#include "core/JSWeakMap.h"
#include "objects/InstancedMesh.h"
#include "objects/SkinnedMesh.h"
#include "objects/Skeleton.h"
#include "math/Vector2.h"
#include "math/Euler.h"
#include "renderers/webgl/WebGLGeometries.h"
#include "renderers/webgl/WebGLObjects.h"
#include <cmath>
#include <limits>

namespace {
using Args = std::vector<JSValue>;
JSValue call(const JSValue& object, const char* name, const Args& args = {}) {
    return object.get(name).call(object, args);
}
void collect(const JSValue& witness) { endJSJob(); witness.collectGarbage(); }
JSValue noop(const JSValue&, const Args&, const Args&) { return {}; }
std::shared_ptr<Float32BufferAttribute> position() {
    return std::make_shared<Float32BufferAttribute>(std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0}, 3);
}
struct Commands {
    JSValue gl = JSValue::object(), attributes = JSValue::object(), bindings = JSValue::object(), info = JSValue::object();
    JSValue updates = JSValue::array(), removes = JSValue::array(), geometryReleases = JSValue::array(), objectReleases = JSValue::array();
    Commands() {
        gl.set("ARRAY_BUFFER", 34962);
        attributes.set("update", JSValue::function(+[](const JSValue&, const Args& args, const Args& captures)->JSValue {
            auto values = captures[0]; auto entry = JSValue::array();
            entry.set("0", args[0]); entry.set("1", args[1]); values.get("push").call(values, {entry}); return {};
        }, {updates}));
        attributes.set("remove", JSValue::function(+[](const JSValue&, const Args& args, const Args& captures)->JSValue {
            auto values = captures[0]; values.get("push").call(values, {args[0]}); return {};
        }, {removes}));
        bindings.set("releaseStatesOfGeometry", JSValue::function(+[](const JSValue&, const Args& args, const Args& captures)->JSValue {
            auto values = captures[0]; values.get("push").call(values, {args[0]}); return {};
        }, {geometryReleases}));
        bindings.set("releaseStatesOfObject", JSValue::function(+[](const JSValue&, const Args& args, const Args& captures)->JSValue {
            auto values = captures[0]; values.get("push").call(values, {args[0]}); return {};
        }, {objectReleases}));
        auto memory = JSValue::object(), render = JSValue::object();
        memory.set("geometries", 0); render.set("frame", 0); info.set("memory", memory); info.set("render", render);
    }
    void clearReferences() {
        for (auto value : Args{updates, removes, geometryReleases, objectReleases}) value.set("length", 0);
    }
};
}
TEST_CASE("Native draw ranges remain live through wrapper and native updates and retain their owner", "[renderaudit][native][gc]") {
    auto witness = JSValue::object(); auto geometry = std::make_shared<BufferGeometry>();
    auto wrapper = nativeGeometry(geometry), range = wrapper.get("drawRange");
    REQUIRE(range.sameValue(nativeGeometry(geometry).get("drawRange")));
    geometry->setDrawRange(2.5, 7.25);
    REQUIRE(range.get("start").strictEquals(2.5)); REQUIRE(range.get("count").strictEquals(7.25));
    range.set("start", 4.5); range.set("count", std::numeric_limits<double>::infinity());
    REQUIRE(geometry->drawRange.start == 4.5); REQUIRE(std::isinf(geometry->drawRange.count));
    std::weak_ptr<BufferGeometry> weak = geometry;
    geometry.reset(); wrapper = {}; collect(witness);
    REQUIRE_FALSE(weak.expired());
    weak.lock()->setDrawRange(8, 13); REQUIRE(range.get("count").strictEquals(13));
    range = {}; collect(witness); REQUIRE(weak.expired());
}
TEST_CASE("Native Vector2 size aliases share fields and preserve dynamic borrowed accessors", "[renderaudit][native]") {
    auto vector = std::make_shared<Vector2>(512, 256); auto wrapper = nativeVector(vector);
    REQUIRE(wrapper.get("width").strictEquals(512)); REQUIRE(wrapper.get("height").strictEquals(256));
    wrapper.set("width", 1024); wrapper.set("height", 128);
    REQUIRE(vector->x == 1024); REQUIRE(vector->y == 128);
    vector->set(80, 40); REQUIRE(wrapper.get("width").strictEquals(80));
    const auto width = wrapper.getPrototype().getOwnPropertyDescriptor("width");
    REQUIRE(width.has_value()); REQUIRE_FALSE(width->enumerable.value_or(false)); REQUIRE(width->configurable.value_or(false));
    auto borrowed = JSValue::object(); borrowed.set("x", "raw");
    REQUIRE(width->getter->call(borrowed).strictEquals("raw"));
    width->setter->call(borrowed, {nullptr}); REQUIRE(borrowed.get("x").isNull());
}
TEST_CASE("Native Euler equality compares live private values and short circuits in source order", "[renderaudit][native]") {
    auto euler = std::make_shared<Euler>(1,2,3); auto wrapper = nativeEuler(euler);
    auto other = JSValue::object(); other.set("_x",1); other.set("_y",2); other.set("_z",3); other.set("_order","XYZ");
    REQUIRE(call(wrapper,"equals",{other}).strictEquals(true));
    other.set("_order","YXZ"); REQUIRE(call(wrapper,"equals",{other}).strictEquals(false));
    other.set("_x",9); JSPropertyDescriptor d;d.configurable = true;
    d.getter = JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue {throw JSTypeError("unreached");});
    other.defineProperty("_y",d); REQUIRE(call(wrapper,"equals",{other}).strictEquals(false));
    other.set("_x",1); REQUIRE_THROWS_AS(call(wrapper,"equals",{other}),JSTypeError);
}
TEST_CASE("Native geometry external ownership retains wrapper map attribute and managed extension graphs", "[renderaudit][gc]") {
    auto witness = JSValue::object(); auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", position());
    JSWeakValue weakWrapper, weakMap, weakAttribute, weakExtra, weakMapExtra;
    {
        auto wrapper = nativeGeometry(geometry), map = wrapper.get("attributes"), attribute = map.get("position");
        auto extra = JSValue::object(), mapExtra = JSValue::object(), prototype = JSValue::object(map.getPrototype());
        extra.set("geometry", wrapper); extra.set("map", map); attribute.set("auditExtra", extra); wrapper.set("auditExtra", extra);
        mapExtra.set("back", map); prototype.set("auditExtra", mapExtra); map.setPrototype(prototype);
        weakWrapper = JSWeakValue(wrapper); weakMap = JSWeakValue(map); weakAttribute = JSWeakValue(attribute);
        weakExtra = JSWeakValue(extra); weakMapExtra = JSWeakValue(mapExtra);
        REQUIRE(nativeGeometry(geometry).sameValue(wrapper)); REQUIRE(nativeGeometry(geometry).get("attributes").sameValue(map));
    }
    collect(witness);
    REQUIRE_FALSE(weakWrapper.expired()); REQUIRE_FALSE(weakMap.expired()); REQUIRE_FALSE(weakAttribute.expired());
    REQUIRE_FALSE(weakExtra.expired()); REQUIRE_FALSE(weakMapExtra.expired());
    REQUIRE(weakWrapper.lock()->get("auditExtra").sameValue(*weakExtra.lock()));
    REQUIRE(weakMap.lock()->get("auditExtra").sameValue(*weakMapExtra.lock()));
    REQUIRE(weakAttribute.lock()->get("auditExtra").sameValue(*weakExtra.lock()));
    geometry.reset(); collect(witness);
    REQUIRE(weakWrapper.expired()); REQUIRE(weakMap.expired()); REQUIRE(weakAttribute.expired());
    REQUIRE(weakExtra.expired()); REQUIRE(weakMapExtra.expired());
}
TEST_CASE("A parent-only native mesh keeps its geometry alias wrappers and live attribute map", "[renderaudit][gc]") {
    auto witness = JSValue::object(); auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", position()); auto mesh = std::make_shared<Mesh>(geometry);
    std::weak_ptr<BufferGeometry> weakNativeGeometry = geometry; JSWeakValue weakMesh, weakGeometry, weakMap, weakPosition;
    {
        auto object = nativeObject(mesh), wrapper = object.get("geometry"), map = wrapper.get("attributes"), attribute = map.get("position");
        weakMesh = JSWeakValue(object); weakGeometry = JSWeakValue(wrapper); weakMap = JSWeakValue(map); weakPosition = JSWeakValue(attribute);
        wrapper.set("parentBack", object); attribute.set("geometryBack", wrapper);
    }
    geometry.reset(); collect(witness);
    REQUIRE_FALSE(weakNativeGeometry.expired()); REQUIRE_FALSE(weakMesh.expired());
    REQUIRE_FALSE(weakGeometry.expired()); REQUIRE_FALSE(weakMap.expired()); REQUIRE_FALSE(weakPosition.expired());
    REQUIRE(weakMesh.lock()->get("geometry").sameValue(*weakGeometry.lock()));
    mesh->geometry->attributes["position"]->setX(0, 8);
    REQUIRE(call(*weakPosition.lock(), "getX", {0}).number() == 8);
    mesh.reset(); collect(witness);
    REQUIRE(weakNativeGeometry.expired()); REQUIRE(weakMesh.expired()); REQUIRE(weakGeometry.expired());
    REQUIRE(weakMap.expired()); REQUIRE(weakPosition.expired());
}
TEST_CASE("An attribute-map root shares geometry control-block ownership without duplicating native slots", "[renderaudit][gc]") {
    auto witness = JSValue::object(); auto geometry = std::make_shared<BufferGeometry>(); auto attribute = position();
    geometry->setAttribute("first", attribute).setAttribute("second", attribute); geometry->setIndex(attribute);
    auto wrapper = nativeGeometry(geometry), map = wrapper.get("attributes");
    std::weak_ptr<BufferGeometry> weakNativeGeometry = geometry; std::weak_ptr<BufferAttribute> weakNativeAttribute = attribute;
    JSWeakValue weakWrapper(wrapper), weakAttribute(nativeAttribute(attribute));
    wrapper.set("mapBack", map); auto current = map.get("first"); current.set("mapBack", map); current = {};
    geometry.reset(); attribute.reset(); wrapper = {}; collect(witness);
    REQUIRE_FALSE(weakNativeGeometry.expired()); REQUIRE_FALSE(weakNativeAttribute.expired());
    REQUIRE_FALSE(weakWrapper.expired()); REQUIRE_FALSE(weakAttribute.expired());
    REQUIRE(map.get("first").sameValue(map.get("second"))); REQUIRE(map.get("first").sameValue(weakWrapper.lock()->get("index")));
    map = {}; collect(witness);
    REQUIRE(weakNativeGeometry.expired()); REQUIRE(weakNativeAttribute.expired()); REQUIRE(weakWrapper.expired()); REQUIRE(weakAttribute.expired());
}
TEST_CASE("Native geometry dispose forwards once to managed listeners with target receiver and event lifetime", "[renderaudit][event]") {
    auto witness = JSValue::object(), events = JSValue::array(); auto geometry = std::make_shared<BufferGeometry>();
    auto wrapper = nativeGeometry(geometry);
    auto listener = JSValue::function(+[](const JSValue& self, const Args& args, const Args& captures)->JSValue {
        auto events = captures[0], expected = captures[1], event = args[0];
        REQUIRE(self.sameValue(expected)); REQUIRE(event.get("target").sameValue(expected)); REQUIRE(event.get("type").strictEquals("dispose"));
        events.get("push").call(events, {event}); return {};
    }, {events, wrapper});
    call(wrapper, "addEventListener", {"dispose", listener});
    REQUIRE(call(wrapper, "hasEventListener", {"dispose", listener}).toBoolean());
    nativeGeometry(geometry); collect(witness); geometry->dispose();
    REQUIRE(events.length() == 1); REQUIRE(events.get("0").get("target").isNull());
    call(wrapper, "dispose"); REQUIRE(events.length() == 2); REQUIRE(events.get("1").get("target").isNull());
    call(wrapper, "removeEventListener", {"dispose", listener}); geometry->dispose(); REQUIRE(events.length() == 2);
}
TEST_CASE("Unrooted geometry map listener and native attribute back-reference cycles collect", "[renderaudit][gc]") {
    auto witness = JSValue::object(); JSWeakValue weakWrapper, weakMap, weakAttribute, weakListener, weakBridge;
    std::weak_ptr<BufferGeometry> weakNative; std::weak_ptr<BufferAttribute> weakNativeAttribute;
    {
        auto geometry = std::make_shared<BufferGeometry>(); auto attribute = position(); geometry->setAttribute("position", attribute);
        auto wrapper = nativeGeometry(geometry), map = wrapper.get("attributes"), value = nativeAttribute(attribute);
        auto listener = JSValue::function(noop, {wrapper, map, value});
        call(wrapper, "addEventListener", {"dispose", listener});
        value.set("geometry", wrapper); value.set("map", map); wrapper.set("self", wrapper);
        weakNative = geometry; weakNativeAttribute = attribute;
        weakWrapper = JSWeakValue(wrapper); weakMap = JSWeakValue(map); weakAttribute = JSWeakValue(value);
        weakListener = JSWeakValue(listener); weakBridge = JSWeakValue(wrapper.getHostSlot("nativeDisposeBridge"));
    }
    collect(witness);
    REQUIRE(weakNative.expired()); REQUIRE(weakNativeAttribute.expired()); REQUIRE(weakWrapper.expired());
    REQUIRE(weakMap.expired()); REQUIRE(weakAttribute.expired()); REQUIRE(weakListener.expired()); REQUIRE(weakBridge.expired());
}
TEST_CASE("Replacing actual native attribute slots releases old managed resources and preserves the map", "[renderaudit][gc]") {
    auto witness = JSValue::object(); auto geometry = std::make_shared<BufferGeometry>(); auto first = position();
    geometry->setAttribute("position", first);
    auto wrapper = nativeGeometry(geometry), map = wrapper.get("attributes"), previous = map.get("position");
    auto oldExtra = JSValue::object(); oldExtra.set("old", previous); previous.set("audit", oldExtra);
    JSWeakValue weakPrevious(previous), weakOldExtra(oldExtra); std::weak_ptr<BufferAttribute> weakNativePrevious = first;
    auto next = position(); next->setX(0, 9); geometry->setAttribute("position", next);
    first.reset(); previous = {}; oldExtra = {}; collect(witness);
    REQUIRE(weakNativePrevious.expired()); REQUIRE(weakPrevious.expired()); REQUIRE(weakOldExtra.expired());
    REQUIRE(wrapper.get("attributes").sameValue(map)); REQUIRE(map.get("position").sameValue(nativeAttribute(next)));
    REQUIRE(call(map.get("position"), "getX", {0}).number() == 9);
    auto latest = position(); map.set("position", nativeAttribute(latest));
    REQUIRE(geometry->getAttribute("position") == latest);
    JSPropertyDescriptor readonly; readonly.value = nativeAttribute(latest); readonly.writable = false;
    readonly.enumerable = false; readonly.configurable = false; map.defineProperty("position", readonly);
    const auto descriptor = map.getOwnPropertyDescriptor("position");
    REQUIRE(descriptor->writable == false); REQUIRE(descriptor->enumerable == false); REQUIRE(descriptor->configurable == false);
    REQUIRE_FALSE(map.erase("position")); REQUIRE_THROWS_AS(map.set("position", nativeAttribute(next)), JSTypeError);
}
TEST_CASE("Retiring a dispose bridge removes its native listener and does not retain native owners", "[renderaudit][event][gc]") {
    auto witness = JSValue::object(), forwarded = JSValue::array(); auto geometry = std::make_shared<BufferGeometry>();
    auto wrapper = nativeGeometry(geometry), bridge = wrapper.getHostSlot("nativeDisposeBridge"); JSWeakValue weakBridge(bridge);
    unsigned ordinaryCalls = 0;
    auto ordinaryListener = geometry->addEventListener("dispose", [&](Event&) { ++ordinaryCalls; });
    wrapper.set("dispatchEvent", JSValue::function(+[](const JSValue&, const Args&, const Args& captures)->JSValue {
        auto log = captures[0]; log.get("push").call(log, {1}); return {};
    }, {forwarded}));
    geometry->dispose(); REQUIRE(forwarded.length() == 1); REQUIRE(ordinaryCalls == 1);
    bridge = {}; wrapper.setHostSlot("nativeDisposeBridge", {}); collect(witness);
    REQUIRE(weakBridge.expired()); REQUIRE(geometry->hasEventListener("dispose", ordinaryListener));
    geometry->dispose(); REQUIRE(forwarded.length() == 1); REQUIRE(ordinaryCalls == 2);
    {
        auto other = std::make_shared<BufferGeometry>(); auto otherWrapper = nativeGeometry(other);
        auto retainedBridge = otherWrapper.getHostSlot("nativeDisposeBridge").hostPayload("three.NativeDisposeBridge");
        std::weak_ptr<BufferGeometry> weakOther = other; JSWeakValue weakOtherWrapper(otherWrapper);
        other.reset(); otherWrapper = {}; collect(witness);
        REQUIRE(weakOther.expired()); REQUIRE(weakOtherWrapper.expired()); REQUIRE(retainedBridge.use_count() == 1);
    }
}
TEST_CASE("Native-only geometry disposal reaches WebGLGeometries and releases actual native attributes", "[renderaudit][native][event]") {
    Commands commands; auto geometry = std::make_shared<BufferGeometry>(); auto attribute = position();
    geometry->setAttribute("position", attribute);
    WebGLGeometries geometries(commands.gl, commands.attributes, commands.info, commands.bindings);
    auto wrapper = nativeGeometry(geometry);
    REQUIRE(geometries.get({}, wrapper).sameValue(wrapper)); REQUIRE(commands.info.get("memory").get("geometries").number() == 1);
    const auto wire = geometries.getWireframeAttribute(wrapper); REQUIRE(vertexAttribute(wire) != nullptr);
    geometry->dispose();
    REQUIRE(commands.info.get("memory").get("geometries").number() == 0);
    REQUIRE(commands.removes.length() == 2); REQUIRE(commands.removes.get("0").sameValue(nativeAttribute(attribute)));
    REQUIRE(commands.removes.get("1").sameValue(wire)); REQUIRE(commands.geometryReleases.get("0").sameValue(wrapper));
    geometry->dispose(); REQUIRE(commands.removes.length() == 2); REQUIRE(commands.geometryReleases.length() == 1);
    geometries.get({}, wrapper); REQUIRE(commands.info.get("memory").get("geometries").number() == 1);
    geometries.dispose(); REQUIRE(commands.info.get("memory").get("geometries").number() == 0);
}
TEST_CASE("Native instanced fields and disposal connect WebGLObjects to the actual native storage", "[renderaudit][native][event]") {
    Commands commands; auto geometry = std::make_shared<BufferGeometry>(); geometry->setAttribute("position", position());
    auto mesh = std::make_shared<InstancedMesh>(geometry, std::make_shared<MeshBasicMaterial>(), 2);
    mesh->setColorAt(0, Color(1, 0, 0)); auto wrapper = nativeObject(mesh);
    REQUIRE(wrapper.get("instanceMatrix").sameValue(nativeAttribute(mesh->instanceMatrix)));
    REQUIRE(wrapper.get("instanceColor").sameValue(nativeAttribute(mesh->instanceColor)));
    auto replacement = std::make_shared<InstancedBufferAttribute>(Float32Array(32), 16);
    wrapper.set("instanceMatrix", nativeAttribute(replacement)); REQUIRE(mesh->instanceMatrix == replacement);
    WebGLGeometries geometries(commands.gl, commands.attributes, commands.info, commands.bindings);
    WebGLObjects objects(commands.gl, geometries.object(), commands.attributes, commands.bindings, commands.info);
    REQUIRE(objects.update(wrapper).sameValue(nativeGeometry(geometry))); REQUIRE(commands.updates.length() == 3);
    REQUIRE(commands.updates.get("1").get("0").sameValue(nativeAttribute(replacement)));
    REQUIRE(commands.updates.get("2").get("0").sameValue(nativeAttribute(mesh->instanceColor)));
    nativeAttribute(replacement).get("array").set("0", 17); REQUIRE(mesh->instanceMatrix->getX(0) == 17);
    objects.update(wrapper); REQUIRE(commands.updates.length() == 3);
    commands.info.get("render").set("frame", 1); objects.update(wrapper); REQUIRE(commands.updates.length() == 6);
    mesh->dispose(); REQUIRE(commands.objectReleases.length() == 1); REQUIRE(commands.objectReleases.get("0").sameValue(wrapper));
    REQUIRE(commands.removes.length() == 2); REQUIRE(commands.removes.get("0").sameValue(nativeAttribute(replacement)));
    REQUIRE(commands.removes.get("1").sameValue(nativeAttribute(mesh->instanceColor)));
    mesh->dispose(); REQUIRE(commands.objectReleases.length() == 1); REQUIRE(commands.removes.length() == 2);
}
TEST_CASE("Skinned WebGLObjects updates actual native skeleton matrices and bone textures once per frame", "[renderaudit][native]") {
    Commands commands; auto geometry = std::make_shared<BufferGeometry>();
    auto bone = std::make_shared<Bone>(); auto mesh = std::make_shared<SkinnedMesh>(geometry);
    auto skeleton = std::make_shared<Skeleton>(Skeleton::Bones{bone}); skeleton->computeBoneTexture(); mesh->skeleton = skeleton;
    bone->matrixWorld.makeTranslation(3, 4, 5);
    auto wrapper = nativeObject(mesh), skeletonWrapper = wrapper.get("skeleton");
    auto matrices = skeletonWrapper.get("boneMatrices");
    const auto before = nativeTexture(skeleton->boneTexture).get("version").number();
    WebGLGeometries geometries(commands.gl, commands.attributes, commands.info, commands.bindings);
    WebGLObjects objects(commands.gl, geometries.object(), commands.attributes, commands.bindings, commands.info);
    objects.update(wrapper); REQUIRE(skeleton->boneMatrices.get(12).number() == 3);
    REQUIRE(matrices.get("13").number() == 4); REQUIRE(matrices.get("14").number() == 5);
    REQUIRE(nativeTexture(skeleton->boneTexture).get("version").number() == before + 1);
    bone->matrixWorld.makeTranslation(7, 8, 9); objects.update(wrapper);
    REQUIRE(skeleton->boneMatrices.get(12).number() == 3);
    REQUIRE(nativeTexture(skeleton->boneTexture).get("version").number() == before + 1);
    commands.info.get("render").set("frame", 1); objects.update(wrapper);
    REQUIRE(skeleton->boneMatrices.get(12).number() == 7); REQUIRE(matrices.get("13").number() == 8);
    REQUIRE(nativeTexture(skeleton->boneTexture).get("version").number() == before + 2);
    REQUIRE(commands.info.get("memory").get("geometries").number() == 1);
}
TEST_CASE("Parent-only native skinned mesh keeps skeleton and matrix extension graphs then releases them", "[renderaudit][native][gc]") {
    auto witness = JSValue::object(); auto mesh = std::make_shared<SkinnedMesh>();
    mesh->skeleton = std::make_shared<Skeleton>(Skeleton::Bones{std::make_shared<Bone>()});
    JSWeakValue weakObject, weakSkeleton, weakMatrices, weakExtra; std::weak_ptr<Skeleton> weakNativeSkeleton = mesh->skeleton;
    {
        auto object = nativeObject(mesh), skeleton = object.get("skeleton"), matrices = skeleton.get("boneMatrices"), extra = JSValue::object();
        extra.set("mesh", object); extra.set("skeleton", skeleton); matrices.set("auditExtra", extra);
        weakObject = JSWeakValue(object); weakSkeleton = JSWeakValue(skeleton); weakMatrices = JSWeakValue(matrices); weakExtra = JSWeakValue(extra);
    }
    collect(witness);
    REQUIRE_FALSE(weakObject.expired()); REQUIRE_FALSE(weakSkeleton.expired()); REQUIRE_FALSE(weakMatrices.expired()); REQUIRE_FALSE(weakExtra.expired());
    mesh->skeleton->boneMatrices.setIndex(0, 23); REQUIRE(weakMatrices.lock()->get("0").number() == 23);
    mesh.reset(); collect(witness);
    REQUIRE(weakNativeSkeleton.expired()); REQUIRE(weakObject.expired()); REQUIRE(weakSkeleton.expired()); REQUIRE(weakMatrices.expired()); REQUIRE(weakExtra.expired());
}
TEST_CASE("Instanced bridge methods reject incompatible real native receivers without null dereferences", "[renderaudit][receiver]") {
    auto instanced = std::make_shared<InstancedMesh>(std::make_shared<BufferGeometry>(), std::make_shared<MeshBasicMaterial>(), 1);
    const auto wrapper = nativeObject(instanced), incompatible = nativeObject(std::make_shared<Mesh>());
    REQUIRE_THROWS_AS(wrapper.get("dispose").call(incompatible), JSTypeError);
    for (const char* name : {"instanceMatrix", "instanceColor"}) {
        const auto descriptor = wrapper.getOwnPropertyDescriptor(name);
        REQUIRE_THROWS_AS(descriptor->getter->call(incompatible), JSTypeError);
        REQUIRE_THROWS_AS(descriptor->setter->call(incompatible, {nullptr}), JSTypeError);
    }
}
TEST_CASE("Live geometry module holds native geometry only until the final native owner and job keepalive end", "[renderaudit][gc][native]") {
    Commands commands; auto witness = JSValue::object(); auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute("position", position());
    WebGLGeometries geometries(commands.gl, commands.attributes, commands.info, commands.bindings);
    JSWeakValue weakGeometry, weakMap, weakPosition, weakWire; std::weak_ptr<BufferGeometry> weakNativeGeometry = geometry;
    {
        auto wrapper = nativeGeometry(geometry), map = wrapper.get("attributes"), attribute = map.get("position");
        geometries.get({}, wrapper); auto wire = geometries.getWireframeAttribute(wrapper);
        wire.set("back", wrapper); attribute.set("back", wrapper);
        weakGeometry = JSWeakValue(wrapper); weakMap = JSWeakValue(map); weakPosition = JSWeakValue(attribute); weakWire = JSWeakValue(wire);
    }
    collect(witness);
    REQUIRE_FALSE(weakGeometry.expired()); REQUIRE_FALSE(weakMap.expired()); REQUIRE_FALSE(weakPosition.expired()); REQUIRE_FALSE(weakWire.expired());
    geometry.reset(); collect(witness);
    REQUIRE(weakNativeGeometry.expired()); REQUIRE(weakGeometry.expired()); REQUIRE(weakMap.expired()); REQUIRE(weakPosition.expired()); REQUIRE(weakWire.expired());
    REQUIRE(hasPendingFinalizationJobs()); REQUIRE(commands.removes.length() == 0);
    REQUIRE(commands.info.get("memory").get("geometries").number() == 1);
    REQUIRE(drainFinalizationJobs() == 1); REQUIRE_FALSE(hasPendingFinalizationJobs());
    geometries.dispose();
    REQUIRE(commands.removes.length() == 0); REQUIRE(commands.geometryReleases.length() == 0);
    // Upstream registry cleanup deletes the weak bookkeeping entry only.
    REQUIRE(commands.info.get("memory").get("geometries").number() == 1);
}
TEST_CASE("Native-only instanced mesh roots its extended attributes and releases listener cycles on native death", "[renderaudit][native][gc]") {
    auto witness = JSValue::object(); auto mesh = std::make_shared<InstancedMesh>(
        std::make_shared<BufferGeometry>(), std::make_shared<MeshBasicMaterial>(), 1);
    mesh->setColorAt(0, Color(0, 1, 0));
    JSWeakValue weakObject, weakMatrix, weakColor, weakListener; std::weak_ptr<InstancedMesh> weakNative = mesh;
    {
        auto object = nativeObject(mesh), matrix = object.get("instanceMatrix"), color = object.get("instanceColor");
        auto listener = JSValue::function(noop, {object, matrix, color});
        call(object, "addEventListener", {"dispose", listener}); matrix.set("meshBack", object); color.set("matrixBack", matrix);
        weakObject = JSWeakValue(object); weakMatrix = JSWeakValue(matrix); weakColor = JSWeakValue(color); weakListener = JSWeakValue(listener);
    }
    collect(witness);
    REQUIRE_FALSE(weakObject.expired()); REQUIRE_FALSE(weakMatrix.expired()); REQUIRE_FALSE(weakColor.expired()); REQUIRE_FALSE(weakListener.expired());
    mesh->instanceMatrix->setX(0, 31); REQUIRE(call(*weakMatrix.lock(), "getX", {0}).number() == 31);
    mesh.reset(); collect(witness);
    REQUIRE(weakNative.expired()); REQUIRE(weakObject.expired()); REQUIRE(weakMatrix.expired()); REQUIRE(weakColor.expired()); REQUIRE(weakListener.expired());
}
