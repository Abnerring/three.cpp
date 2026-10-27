#include "core/NativeRenderingObject.h"
#include "core/NativeObject.h"
#include "core/BufferGeometry.h"
#include "core/EventDispatcher.h"
#include "objects/InstancedMesh.h"
#include "objects/Skeleton.h"
#include <algorithm>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSPropertyDescriptor data(JSValue value, bool writable = true, bool enumerable = true, bool configurable = true) {
    JSPropertyDescriptor d; d.value = std::move(value); d.writable = writable; d.enumerable = enumerable; d.configurable = configurable; return d;
}
template<class T> std::shared_ptr<T> payload(const JSValue& object, const char* tag) { return std::static_pointer_cast<T>(object.hostPayload(tag)); }
std::shared_ptr<InstancedMesh> checkedInstanced(const JSValue& object) {
    const auto mesh = std::dynamic_pointer_cast<InstancedMesh>(payload<Object3D>(object, "three.Object3D"));
    if (!mesh) throw JSTypeError("Native instanced operation requires InstancedMesh receiver");
    return mesh;
}
void accessor(JSValue object, const char* name, JSValue::NativeFunction get, JSValue::NativeFunction set = nullptr) {
    JSPropertyDescriptor d; if (get) d.getter = JSValue::function(get, {name}, std::string("get ") + name, 0);
    if (set) d.setter = JSValue::function(set, {name}, std::string("set ") + name, 1);
    d.enumerable = d.configurable = true; object.defineProperty(name, d);
}
struct DisposeBridge {
    std::weak_ptr<EventDispatcher> owner;
    EventDispatcher::Listener listener;
    ~DisposeBridge() { if (const auto object = owner.lock()) object->removeEventListener("dispose", listener); }
};
void installDisposeBridge(JSValue wrapper, const std::shared_ptr<EventDispatcher>& owner) {
    auto bridge = std::make_shared<DisposeBridge>(); bridge->owner = owner;
    const auto weak = JSWeakValue(wrapper);
    bridge->listener = owner->addEventListener("dispose", [weak](Event&) {
        if (const auto wrapper = weak.lock()) {
            auto event = JSValue::object(); event.defineProperty("type", data("dispose"));
            const auto dispatch = wrapper->get("dispatchEvent"); dispatch.call(*wrapper, {event});
        }
    });
    wrapper.setHostSlot("nativeDisposeBridge", JSValue::hostObject(bridge, bridge.get(), "three.NativeDisposeBridge"));
}
JSValue eventPrototype(JSValue oldPrototype, const char* tag) {
    auto prototype = JSValue::object(oldPrototype); const auto events = EventDispatcher::runtimePrototype();
    for (const char* name : {"addEventListener", "hasEventListener", "removeEventListener", "dispatchEvent"})
        prototype.defineProperty(name, data(events.get(name), true, false, true));
    prototype.defineProperty("dispose", data(JSValue::function(+[](const JSValue& self, const Args&, const Args& cap)->JSValue {
        if (cap[0].strictEquals("geometry")) payload<BufferGeometry>(self, "three.BufferGeometry")->dispose();
        else checkedInstanced(self)->dispose();
        return {};
    }, {tag}, "dispose", 0), true, false, true));
    return prototype;
}
JSHostPropertyResult attributeOwn(const JSValue& self, const std::string& key) {
    const auto geometry = payload<BufferGeometry>(self, "three.BufferGeometry");
    const auto found = geometry->attributes.find(key); if (found == geometry->attributes.end()) return {};
    auto descriptor = data(nativeAttribute(found->second));
    const auto flags = self.getHostSlot("descriptorFlags").get(key);
    if (!flags.isUndefined()) { descriptor.writable = flags.get("writable").toBoolean(); descriptor.enumerable = flags.get("enumerable").toBoolean(); descriptor.configurable = flags.get("configurable").toBoolean(); }
    return {true, descriptor};
}
std::optional<bool> attributeDefine(const JSValue& self, const std::string& key, const JSPropertyDescriptor& descriptor) {
    auto geometry = payload<BufferGeometry>(self, "three.BufferGeometry"); const auto previous = attributeOwn(self, key).descriptor;
    if (descriptor.getter || descriptor.setter) throw JSTypeError("Native geometry attribute maps require data properties");
    if (previous && !previous->configurable.value_or(false)) {
        if (descriptor.configurable.value_or(false) || (descriptor.enumerable && descriptor.enumerable != previous->enumerable)) return false;
        if (!previous->writable.value_or(false) && (descriptor.writable.value_or(false) || (descriptor.value && !descriptor.value->sameValue(*previous->value)))) return false;
    }
    const auto value = descriptor.value.value_or(previous ? *previous->value : JSValue{});
    // A native attribute slot has an explicit typed native boundary. Null is a
    // supported empty slot; an ordinary arbitrary JS object cannot become one.
    auto attribute = vertexAttribute(value);
    geometry->attributes[key] = std::move(attribute);
    auto flags = JSValue::object(nullptr);
    flags.set("writable", descriptor.writable.value_or(previous ? previous->writable.value_or(false) : false));
    flags.set("enumerable", descriptor.enumerable.value_or(previous ? previous->enumerable.value_or(false) : false));
    flags.set("configurable", descriptor.configurable.value_or(previous ? previous->configurable.value_or(false) : false));
    auto slots = self.getHostSlot("descriptorFlags"); slots.set(key, flags); return true;
}
std::optional<bool> attributeErase(const JSValue& self, const std::string& key) {
    auto geometry = payload<BufferGeometry>(self, "three.BufferGeometry"); const auto previous = attributeOwn(self, key).descriptor;
    if (!previous) return {}; if (!previous->configurable.value_or(false)) return false;
    geometry->attributes.erase(key); auto flags = self.getHostSlot("descriptorFlags"); flags.erase(key); return true;
}
std::vector<std::string> attributeNames(const JSValue& self) {
    std::vector<std::string> names;
    for (const auto& [name, _] : payload<BufferGeometry>(self, "three.BufferGeometry")->attributes) names.push_back(name);
    return names;
}
JSValue attributeMap(const std::shared_ptr<BufferGeometry>& geometry, const std::shared_ptr<const JSHostObjectHooks>& geometryHooks) {
    auto result = JSValue::hostObject(geometry, &geometry->attributes, "three.BufferGeometry");
    if (result.getHostSlot("descriptorFlags").isUndefined()) {
        result.setHostSlot("descriptorFlags", JSValue::object(nullptr));
        auto hooks = std::make_shared<JSHostObjectHooks>(*geometryHooks);
        hooks->getOwnProperty = attributeOwn; hooks->defineOwnProperty = attributeDefine; hooks->erase = attributeErase; hooks->ownPropertyNames = attributeNames;
        result.setHostHooks(hooks);
    }
    return result;
}
}
void installNativeGeometryRenderingBridge(JSValue object, const std::shared_ptr<BufferGeometry>& geometry,
                                          const std::shared_ptr<const JSHostObjectHooks>& geometryHooks) {
    object.defineProperty("id", data(geometry->id, false, false, false));
    accessor(object, "index", +[](const JSValue& self, const Args&, const Args&)->JSValue { return nativeAttribute(payload<BufferGeometry>(self, "three.BufferGeometry")->index); },
        +[](const JSValue& self, const Args& args, const Args&)->JSValue {
            auto value = arg(args, 0); auto attribute = value.isNull() ? nullptr : std::dynamic_pointer_cast<BufferAttribute>(vertexAttribute(value));
            if (!value.isNull() && !attribute) throw JSTypeError("Native geometry index requires BufferAttribute");
            payload<BufferGeometry>(self, "three.BufferGeometry")->index = std::move(attribute); return {};
        });
    const auto map = attributeMap(geometry, geometryHooks);
    // A stable data property holds the live map. Replacement must use native
    // attributes, whose actual shared_ptr slots remain the ownership authority.
    object.defineProperty("attributes", data(map, false, true, true));
    auto range = JSValue::hostObject(geometry, &geometry->drawRange, "three.BufferGeometry");
    for (const char* name : {"start", "count"}) {
        accessor(range, name, +[](const JSValue& self, const Args&, const Args& captures)->JSValue {
            const auto geometry = payload<BufferGeometry>(self, "three.BufferGeometry");
            return captures[0].strictEquals("start") ? geometry->drawRange.start : geometry->drawRange.count;
        }, +[](const JSValue& self, const Args& args, const Args& captures)->JSValue {
            const auto geometry = payload<BufferGeometry>(self, "three.BufferGeometry");
            const auto value = arg(args, 0).toNumber();
            (captures[0].strictEquals("start") ? geometry->drawRange.start : geometry->drawRange.count) = value;
            return {};
        });
    }
    object.defineProperty("drawRange", data(range));
    object.setPrototype(eventPrototype(object.getPrototype(), "geometry")); installDisposeBridge(object, geometry);
}
void installNativeObjectRenderingBridge(JSValue object, const std::shared_ptr<Object3D>& owner) {
    if (!std::dynamic_pointer_cast<InstancedMesh>(owner)) return;
    for (const char* name : {"instanceMatrix", "instanceColor"}) {
        accessor(object, name, +[](const JSValue& self, const Args&, const Args& cap)->JSValue {
            const auto mesh = checkedInstanced(self);
            return nativeAttribute(cap[0].strictEquals("instanceMatrix") ? mesh->instanceMatrix : mesh->instanceColor);
        }, +[](const JSValue& self, const Args& args, const Args& cap)->JSValue {
            const auto mesh = checkedInstanced(self);
            const auto value = arg(args, 0); const auto attribute = value.isNull() ? nullptr : std::dynamic_pointer_cast<InstancedBufferAttribute>(vertexAttribute(value));
            if (!value.isNull() && !attribute) throw JSTypeError("Native instanced fields require InstancedBufferAttribute");
            (cap[0].strictEquals("instanceMatrix") ? mesh->instanceMatrix : mesh->instanceColor) = attribute; return {};
        });
    }
    object.setPrototype(eventPrototype(object.getPrototype(), "object")); installDisposeBridge(object, owner);
}
void installNativeSkeletonRenderingBridge(JSValue object, const std::shared_ptr<Skeleton>&) {
    auto prototype = JSValue::object(object.getPrototype());
    prototype.defineProperty("update", data(JSValue::function(+[](const JSValue& self, const Args&, const Args&)->JSValue {
        payload<Skeleton>(self, "three.Skeleton")->update(); return {};
    }, {}, "update", 0), true, false, true)); object.setPrototype(prototype);
}
