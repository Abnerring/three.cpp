#include "materials/nodes/ProxyNodeMaterial.h"

#include "core/JSGlobal.h"
#include "materials/Material.h"
#include "nodes/utils/UtilityNodeRuntime.h"

#include <initializer_list>
#include <string>
#include <tuple>

namespace {
using namespace UtilityNodeRuntime;

JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
// const _nodeDescriptors = {};
JSValue& nodeDescriptorsKeeper() { static thread_local JSValue value; return value; }

bool endsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() &&
        text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// function getNodeDescriptor( property ) { return _nodeDescriptors[ property ] ??= { enumerable: true, get() { return this.nodeMaterial[ property ]; } }; }
JSValue getNodeDescriptor(const JSValue& property) {
    auto& descriptors = nodeDescriptorsKeeper();
    if (!descriptors.toBoolean()) descriptors = JSValue::object(nullptr);
    const auto key = property.toString();
    const auto existing = descriptors.get(key);
    if (existing.isUndefined()) {
        auto descriptor = JSValue::object(nullptr);
        descriptor.set("enumerable", true);
        // The getter reads this.nodeMaterial[ property ]; the receiver arrives as the first argument.
        auto getter = JSValue::function(
            +[](const JSValue& self, const Args&, const Args& captures) -> JSValue {
                const auto receiver = self.toBoolean() ? self : JSValue{};
                return receiver.get("nodeMaterial").get(captures[0].string().c_str());
            },
            {property}, "get", 0);
        descriptor.set("get", getter);
        descriptors.set(key, descriptor);
    }
    return descriptors.get(key);
}

// function cloneUserData( userData ) { const result = {}; for ( const key in userData ) { const value = userData[ key ]; result[ key ] = value?.clone !== undefined && value.isTexture !== true ? value.clone() : value; } return result; }
JSValue cloneUserData(const JSValue& userData) {
    auto result = JSValue::object(nullptr);
    if (!userData.isObject()) return result;
    // The source iterates with for in, which visits enumerable own properties here.
    for (const auto& key : userData.ownPropertyNames()) {
        const auto descriptor = userData.getOwnPropertyDescriptor(key);
        if (!descriptor || !descriptor->enumerable.value_or(false)) continue;
        const auto value = userData.get(key.c_str());
        const bool hasClone = value.isObject() && !value.isNull() && !value.get("clone").isUndefined();
        const bool isTexture = value.isObject() && value.get("isTexture").strictEquals(true);
        if (hasClone && !isTexture) {
            result.set(key, invoke(value, "clone")());
        } else {
            result.set(key, value);
        }
    }
    return result;
}

// function copyProperties( target, source ) { ... }
void copyProperties(const JSValue& target, const JSValue& source) {
    auto targetRef = target;
    if (!source.isObject()) return;
    const auto objectConstructor = nativeGlobalBinding("Object");
    const auto keys = invoke(objectConstructor.get("keys"), "call")({objectConstructor, source});
    for (unsigned i = 0; i < keys.length(); ++i) {
        const auto property = keys.get(JSValue(static_cast<double>(i)));
        const auto name = property.string();
        if (name == "uuid" || name == "version" || name == "_listeners") continue;
        if (endsWith(name, "Node")) {
            // Object.hasOwn( target, property ) === false  is written as a descriptor probe here.
            if (!targetRef.getOwnPropertyDescriptor(name).has_value()) {
                auto descriptor = getNodeDescriptor(property);
                invoke(objectConstructor, "defineProperty")({targetRef, property, descriptor});
            }
        } else {
            targetRef.set(name.c_str(), source.get(name.c_str()));
        }
    }
}

// customProgramCacheKey() { return this.nodeMaterial.uuid + ',' + this.nodeMaterial.version; }
JSValue customProgramCacheKey(const JSValue& self, const Args&, const Args&) {
    const auto nodeMaterial = self.get("nodeMaterial");
    // String concatenation as the source does with +: the uuid string, a comma, then the version.
    return JSValue(nodeMaterial.get("uuid").toString() + "," + nodeMaterial.get("version").toString());
}

// build( builder ) { this.nodeMaterial.build( builder ); }
JSValue build(const JSValue& self, const Args& args, const Args&) {
    invoke(self.get("nodeMaterial"), "build")({arg(args, 0, nullptr)});
    return {};
}

// copy( source ) { copyProperties( this, source ); this.userData = cloneUserData( source.userData ); return this; }
JSValue copy(const JSValue& self, const Args& args, const Args&) {
    const auto source = arg(args, 0, nullptr);
    copyProperties(self, source);
    auto selfRef = self;
    selfRef.set("userData", cloneUserData(source.get("userData")));
    return self;
}

// clone() { return new this.constructor( this.nodeMaterial ).copy( this ); }
JSValue clone(const JSValue& self, const Args&, const Args&) {
    const auto constructor = self.get("constructor");
    // construct is an internal slot rather than a property, so it is invoked through the handle API.
    const auto created = constructor.construct({self.get("nodeMaterial")});
    return invoke(created, "copy")({self});
}

// get version() { return this.nodeMaterial.version; }  set version( value ) {}
JSValue versionGetter(const JSValue& self, const Args&, const Args&) {
    return self.get("nodeMaterial").get("version");
}
JSValue versionSetter(const JSValue&, const Args&, const Args&) { return {}; }
}  // namespace

JSValue nativeProxyNodeMaterialModule() {
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;

    static thread_local bool initializing = false;
    JSModuleInitializationScope scope(initializing, "ProxyNodeMaterial module");
    nativeMaterialModule();

    const auto base = nativeMaterialConstructor();
    auto holder = JSValue::object(nullptr);
    // The class declares no static type getter, so none is registered here either.
    auto ctor = JSValue::builtinConstructor(
        +[](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
            auto self = captures[0].get("constructor").getPrototype().construct({}, target);
            const auto nodeMaterial = arg(args, 0, JSValue{});
            if (!nodeMaterial.get("isNodeMaterial").strictEquals(true)) {
                throw JSTypeError("THREE.ProxyNodeMaterial: The parameter must be a NodeMaterial.");
            }
            copyProperties(self, nodeMaterial);
            self.set("userData", cloneUserData(nodeMaterial.get("userData")));
            self.set("type", JSValue("ProxyNodeMaterial"));
            self.set("isProxyNodeMaterial", true);
            self.set("nodeMaterial", nodeMaterial);
            return self;
        },
        {holder}, "ProxyNodeMaterial", 1);
    holder.set("constructor", ctor);
    ctor.setPrototype(base);
    auto prototype = ctor.get("prototype");
    prototype.setPrototype(base.get("prototype"));
    // clone() reads this.constructor, so the prototype has to carry it back to the class.
    prototype.set("constructor", ctor);

    for (const auto& [name, arity, callback] : std::initializer_list<
             std::tuple<const char*, unsigned, JSValue::NativeFunction>>{
             {"customProgramCacheKey", 0, customProgramCacheKey},
             {"build", 1, build},
             {"copy", 1, copy},
             {"clone", 0, clone}}) {
        auto fn = JSValue::function(callback, {}, name, arity);
        JSPropertyDescriptor descriptor;
        descriptor.value = fn;
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }

    // get version() / set version( value )  — the setter deliberately does nothing.
    JSPropertyDescriptor version;
    version.configurable = true;
    version.getter = JSValue::function(versionGetter, {}, "get version", 0);
    version.setter = JSValue::function(versionSetter, {}, "set version", 1);
    prototype.defineProperty("version", version);

    auto bindings = JSValue::object(nullptr);
    own(bindings, "default", ctor);
    auto module = JSValue::object(nullptr);
    own(module, "default", ctor);
    bindings.retainForHeapLifetime();
    lexical() = JSWeakValue(bindings);
    module.retainForHeapLifetime();
    weak = JSWeakValue(module);
    return module;
}

JSValue nativeProxyNodeMaterialBinding(const std::string& name) {
    const auto keeper = nativeProxyNodeMaterialModule();
    (void) keeper;
    return lexical().lock()->get(name);
}

JSValue nativeProxyNodeMaterialConstructor() {
    return nativeProxyNodeMaterialBinding("default");
}
