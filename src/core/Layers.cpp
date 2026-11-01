// Direct port of three.js Layers.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. Source license: THREEJS-LICENSE.
#include "core/Layers.h"
#include <bit>
#include <cmath>
#include <cstdint>

namespace {
using Args = std::vector<JSValue>;
void validate(const JSValue& value) {
    static_cast<void>(JSWeakValue(value));
}
const JSValue& arg(const Args& args, std::size_t index) {
    static const JSValue undefined;
    return index < args.size() ? args[index] : undefined;
}
std::uint32_t uint32(const JSValue& value) {
    const double number = value.toNumber();
    if (!std::isfinite(number) || number == 0)
        return 0;
    double reduced = std::fmod(std::trunc(number), 4294967296.0);
    if (reduced < 0)
        reduced += 4294967296.0;
    return static_cast<std::uint32_t>(reduced);
}
JSValue signedValue(std::uint32_t value) {
    return std::bit_cast<std::int32_t>(value);
}
std::uint32_t shift(const JSValue& layer) {
    return std::uint32_t{1} << (uint32(layer) & 31u);
}
JSValue initialize(const JSValue& self, const Args&, const Args&) {
    auto receiver = self;
    receiver.set("mask", 1);
    return {};
}
JSValue method(const JSValue& self, const Args& args, const Args& captures) {
    const auto& name = captures[0].string();
    auto receiver = self;
    if (name == "set")
        receiver.set("mask", shift(arg(args, 0)));
    else if (name == "enableAll")
        receiver.set("mask", -1);
    else if (name == "disableAll")
        receiver.set("mask", 0);
    else if (name == "test") {
        const auto left = self.get("mask");
        const auto right = arg(args, 0).get("mask");
        const auto a = uint32(left);
        const auto b = uint32(right);
        return (a & b) != 0;
    } else {
        // Compound assignment reads the old mask before evaluating the shift;
        // its numeric conversion occurs after the shift's conversion.
        const auto old = self.get("mask");
        const auto bit = shift(arg(args, 0));
        const auto mask = uint32(old);
        if (name == "isEnabled")
            return (mask & bit) != 0;
        const auto result = name == "enable"   ? mask | bit
                            : name == "toggle" ? mask ^ bit
                                               : mask & ~bit;
        receiver.set("mask", signedValue(result));
    }
    return {};
}
JSValue call(const JSValue& self, const char* name, const Args& args = {}) {
    validate(self);
    for (const auto& value : args)
        validate(value);
    const auto receiver = self;
    const auto method = receiver.get(name);
    return method.call(receiver, args);
}
} // namespace
JSValue nativeLayersConstructor() {
    static thread_local JSWeakValue cached;
    if (const auto value = cached.lock(); value && value->isConstructible())
        return *value;
    auto constructor = JSValue::constructor(initialize, {}, "Layers", 0);
    constructor.setFunctionSourceText("class Layers {\r\n\r\n\t/**\r\n\t * Constructs a new layers instance, with membership\r\n\t * initially set to layer `0`.\r\n\t */\r\n\tconstructor() {\r\n\r\n\t\t/**\r\n\t\t * A bit mask storing which of the 32 layers this layers object is currently\r\n\t\t * a member of.\r\n\t\t *\r\n\t\t * @type {number}\r\n\t\t */\r\n\t\tthis.mask = 1 | 0;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Sets membership to the given layer, and remove membership all other layers.\r\n\t *\r\n\t * @param {number} layer - The layer to set.\r\n\t */\r\n\tset( layer ) {\r\n\r\n\t\tthis.mask = ( 1 << layer | 0 ) >>> 0;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Adds membership of the given layer.\r\n\t *\r\n\t * @param {number} layer - The layer to enable.\r\n\t */\r\n\tenable( layer ) {\r\n\r\n\t\tthis.mask |= 1 << layer | 0;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Adds membership to all layers.\r\n\t */\r\n\tenableAll() {\r\n\r\n\t\tthis.mask = 0xffffffff | 0;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Toggles the membership of the given layer.\r\n\t *\r\n\t * @param {number} layer - The layer to toggle.\r\n\t */\r\n\ttoggle( layer ) {\r\n\r\n\t\tthis.mask ^= 1 << layer | 0;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Removes membership of the given layer.\r\n\t *\r\n\t * @param {number} layer - The layer to enable.\r\n\t */\r\n\tdisable( layer ) {\r\n\r\n\t\tthis.mask &= ~ ( 1 << layer | 0 );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Removes the membership from all layers.\r\n\t */\r\n\tdisableAll() {\r\n\r\n\t\tthis.mask = 0;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if this and the given layers object have at least one\r\n\t * layer in common.\r\n\t *\r\n\t * @param {Layers} layers - The layers to test.\r\n\t * @return {boolean } Whether this and the given layers object have at least one layer in common or not.\r\n\t */\r\n\ttest( layers ) {\r\n\r\n\t\treturn ( this.mask & layers.mask ) !== 0;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if the given layer is enabled.\r\n\t *\r\n\t * @param {number} layer - The layer to test.\r\n\t * @return {boolean } Whether the given layer is enabled or not.\r\n\t */\r\n\tisEnabled( layer ) {\r\n\r\n\t\treturn ( this.mask & ( 1 << layer | 0 ) ) !== 0;\r\n\r\n\t}\r\n\r\n}");
    auto prototype = constructor.get("prototype");
    for (const auto& [name, arity] :
         std::initializer_list<std::pair<const char*, unsigned>>{{"set", 1},
                                                                 {"enable", 1},
                                                                 {"enableAll", 0},
                                                                 {"toggle", 1},
                                                                 {"disable", 1},
                                                                 {"disableAll", 0},
                                                                 {"test", 1},
                                                                 {"isEnabled", 1}}) {
        JSPropertyDescriptor descriptor;
        descriptor.value = JSValue::function(method, {name}, name, arity);
        if(std::string(name)=="set")descriptor.value->setFunctionSourceText("set( layer ) {\r\n\r\n\t\tthis.mask = ( 1 << layer | 0 ) >>> 0;\r\n\r\n\t}");
        if(std::string(name)=="enable")descriptor.value->setFunctionSourceText("enable( layer ) {\r\n\r\n\t\tthis.mask |= 1 << layer | 0;\r\n\r\n\t}");
        if(std::string(name)=="enableAll")descriptor.value->setFunctionSourceText("enableAll() {\r\n\r\n\t\tthis.mask = 0xffffffff | 0;\r\n\r\n\t}");
        if(std::string(name)=="toggle")descriptor.value->setFunctionSourceText("toggle( layer ) {\r\n\r\n\t\tthis.mask ^= 1 << layer | 0;\r\n\r\n\t}");
        if(std::string(name)=="disable")descriptor.value->setFunctionSourceText("disable( layer ) {\r\n\r\n\t\tthis.mask &= ~ ( 1 << layer | 0 );\r\n\r\n\t}");
        if(std::string(name)=="disableAll")descriptor.value->setFunctionSourceText("disableAll() {\r\n\r\n\t\tthis.mask = 0;\r\n\r\n\t}");
        if(std::string(name)=="test")descriptor.value->setFunctionSourceText("test( layers ) {\r\n\r\n\t\treturn ( this.mask & layers.mask ) !== 0;\r\n\r\n\t}");
        if(std::string(name)=="isEnabled")descriptor.value->setFunctionSourceText("isEnabled( layer ) {\r\n\r\n\t\treturn ( this.mask & ( 1 << layer | 0 ) ) !== 0;\r\n\r\n\t}");
        descriptor.writable = descriptor.configurable = true;
        prototype.defineProperty(name, descriptor);
    }
    constructor.retainForHeapLifetime();
    cached = JSWeakValue(constructor);
    return constructor;
}
Layers::Layers() : object_(constructor().construct()) {}
Layers::Layers(const JSValue& object, Existing) {
    validate(object);
    object_ = object;
}
Layers::Layers(const Layers& other) {
    validate(other.object_);
    object_ = other.object_;
}
Layers::Layers(Layers&& other) {
    validate(other.object_);
    object_ = std::move(other.object_);
}
Layers& Layers::operator=(const Layers& other) {
    validate(object_);
    validate(other.object_);
    object_ = other.object_;
    return *this;
}
Layers& Layers::operator=(Layers&& other) {
    validate(object_);
    validate(other.object_);
    object_ = std::move(other.object_);
    return *this;
}
JSValue Layers::constructor() {
    return nativeLayersConstructor();
}
JSValue Layers::prototype() {
    return constructor().get("prototype");
}
JSValue Layers::object() const {
    validate(object_);
    return object_;
}
JSValue Layers::Mask::value() const {
    validate(owner_->object_);
    return owner_->object_.get("mask");
}
Layers::Mask& Layers::Mask::operator=(const JSValue& value) {
    validate(owner_->object_);
    validate(value);
    owner_->object_.set("mask", value);
    return *this;
}
void Layers::invoke(const char* name, const JSValue& value) {
    validate(object_);
    validate(value);
    const auto receiver = object_;
    const auto method = receiver.get(name);
    validate(value);
    method.call(receiver, {value});
}
void Layers::set(const JSValue& layer) {
    invoke("set", layer);
}
void Layers::enable(const JSValue& layer) {
    invoke("enable", layer);
}
void Layers::toggle(const JSValue& layer) {
    invoke("toggle", layer);
}
void Layers::disable(const JSValue& layer) {
    invoke("disable", layer);
}
void Layers::enableAll() {
    call(object_, "enableAll");
}
void Layers::disableAll() {
    call(object_, "disableAll");
}
bool Layers::test(const Layers& layers) const {
    validate(object_);
    validate(layers.object_);
    const auto receiver = object_;
    const auto method = receiver.get("test");
    // A method getter can replace the native camera/object layers field. JS
    // obtains the argument after that getter, while retaining the old receiver.
    validate(layers.object_);
    return method.call(receiver, {layers.object_}).toBoolean();
}
bool Layers::isEnabled(const JSValue& layer) const {
    validate(object_);
    validate(layer);
    const auto receiver = object_;
    const auto method = receiver.get("isEnabled");
    validate(layer);
    return method.call(receiver, {layer}).toBoolean();
}
JSValue nativeLayers(const Layers& layers) {
    return layers.object();
}
