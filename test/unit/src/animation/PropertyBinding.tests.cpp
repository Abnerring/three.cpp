#include "catch.hpp"
#include "animation/PropertyBinding.h"
#include "core/NativeObject.h"

namespace {
JSValue object(std::initializer_list<std::pair<const char*, JSValue>> entries) {
    auto result = JSValue::object(); for (const auto& [key, value] : entries) result.set(key, value); return result;
}
void push(JSValue array, JSValue value) { array.set(std::to_string(array.length()), value); }
PropertyBinding* active = nullptr;
}
TEST_CASE("PropertyBinding parses ambiguous names and JS whitespace without losing Unicode", "[animation][property-binding]") {
    REQUIRE(PropertyBinding::parseTrackName("foo.bar.baz").get("nodeName").strictEquals("foo.bar"));
    const auto parsed = PropertyBinding::parseTrackName("parent:dir/急須.material[02].color[r]");
    REQUIRE(parsed.get("nodeName").strictEquals("急須"));
    REQUIRE(parsed.get("objectName").strictEquals("material"));
    REQUIRE(parsed.get("objectIndex").strictEquals("02"));
    REQUIRE(parsed.get("propertyIndex").strictEquals("r"));
    REQUIRE(PropertyBinding::parseTrackName(".value[a][b]").get("propertyIndex").strictEquals("a][b"));
    REQUIRE(PropertyBinding::parseTrackName(".value").hasOwn("nodeName"));
    REQUIRE(PropertyBinding::parseTrackName(".value").get("nodeName").isUndefined());
    REQUIRE(PropertyBinding::sanitizeNodeName("a\xC2\xA0" "b\xE2\x80\xA8" "c.[x]") == "a_b_cx");
    REQUIRE(PropertyBinding::sanitizeNodeName("a\xC2\x85" "b") == "a\xC2\x85" "b");
    REQUIRE_THROWS(PropertyBinding::parseTrackName(".value[]"));
    REQUIRE_THROWS(PropertyBinding::parseTrackName(".value[a\nb]"));
}
TEST_CASE("PropertyBinding retains old material and morph array identities until unbind", "[animation][property-binding]") {
    auto material = object({{"isMaterial", true}, {"opacity", 1}});
    auto influences = AnimationArray::array({.2,.3});
    auto root = object({{"isObject3D", true}, {"material", material}, {"morphTargetInfluences", nativeArray(influences)},
        {"geometry", object({{"morphAttributes", JSValue::object()}})}, {"morphTargetDictionary", object({{"smile", 1}})}});
    PropertyBinding opacity(root, ".material.opacity"), morph(root, ".morphTargetInfluences[smile]");
    opacity.bind(); morph.bind();
    auto replacement = object({{"isMaterial", true}, {"opacity", 1}}); root.set("material", replacement);
    root.set("morphTargetInfluences", nativeArray(AnimationArray::array({.8,.9})));
    opacity.setValue(AnimationArray::array({.25}), 0); morph.setValue(AnimationArray::array({.7}), 0);
    REQUIRE(material.get("opacity").strictEquals(.25));
    REQUIRE(material.get("needsUpdate").strictEquals(true));
    REQUIRE(replacement.get("opacity").strictEquals(1));
    REQUIRE(influences.get(1).strictEquals(.7));
    REQUIRE(root.get("morphTargetInfluences").get("1").strictEquals(.9));
    REQUIRE(root.get("matrixWorldNeedsUpdate").strictEquals(true));
    opacity.unbind(); opacity.setValue(AnimationArray::array({.5}), 0);
    REQUIRE(replacement.get("opacity").strictEquals(.5));
}
TEST_CASE("PropertyBinding unavailable state is sticky and null differs from missing", "[animation][property-binding]") {
    auto root = JSValue::object(); PropertyBinding binding(root, ".value");
    auto buffer = AnimationArray::array({99}); binding.getValue(buffer, 0);
    root.set("value", 4); binding.getValue(buffer, 0); REQUIRE(buffer.get(0).strictEquals(99));
    binding.unbind(); binding.getValue(buffer, 0); REQUIRE(buffer.get(0).strictEquals(4));
    root.set("value", nullptr); binding.unbind(); REQUIRE_THROWS_AS(binding.bind(), JSTypeError);
    REQUIRE_NOTHROW(binding.getValue(buffer, 0));
}
TEST_CASE("PropertyBinding fromArray calls preserve typed buffer identity and live version target", "[animation][property-binding]") {
    auto trace = JSValue::array(), vector = object({{"x", 0}}), later = object({{"isMaterial", true}});
    vector.set("toArray", JSValue::function([](const JSValue& receiver, const auto& args, const auto&) -> JSValue {
        auto array = args[0]; array.set(args[1].toString(), receiver.get("x")); return array;
    }));
    AnimationArray buffer = Float64Array({7,8});
    vector.set("fromArray", JSValue::function([](const JSValue& receiver, const auto& args, const auto& c) -> JSValue {
        REQUIRE(args[0].sameValue(c[0])); auto self = receiver; self.set("x", args[0].get(args[1].toString()));
        push(c[1], "fromArray"); active->targetObject = c[2]; return receiver;
    }, {nativeArray(buffer), trace, later}));
    auto root = object({{"isMaterial", true}, {"value", vector}});
    PropertyBinding binding(root, ".value"); active = &binding;
    binding.setValue(buffer, 1); active = nullptr;
    REQUIRE(vector.get("x").strictEquals(8)); REQUIRE(later.get("needsUpdate").strictEquals(true));
    REQUIRE(root.get("needsUpdate").isUndefined()); REQUIRE(trace.length() == 1);
}
TEST_CASE("PropertyBinding indexed quaternion style setters and inherited map remain generic", "[animation][property-binding]") {
    auto trace = JSValue::array(), quaternion = JSValue::object();
    JSPropertyDescriptor descriptor; descriptor.enumerable = true; descriptor.configurable = true;
    descriptor.getter = JSValue::function([](const JSValue&, const auto&, const auto&) -> JSValue { return 1; });
    descriptor.setter = JSValue::function([](const JSValue&, const auto& args, const auto& c) -> JSValue { push(c[0], args[0]); return {}; }, {trace});
    quaternion.defineProperty("w", descriptor);
    auto root = object({{"isObject3D", true}, {"quaternion", quaternion}});
    PropertyBinding binding(root, ".quaternion[w]"); binding.setValue(AnimationArray::array({.5}), 0);
    REQUIRE(trace.get("0").strictEquals(.5)); REQUIRE(root.get("matrixWorldNeedsUpdate").strictEquals(true));
    auto map = object({{"value", 5}}), prototype = object({{"map", map}}); auto inherited = JSValue::object(prototype);
    PropertyBinding mapped(inherited, ".map.value"); mapped.setValue(AnimationArray::array({8}), 0);
    REQUIRE(map.get("value").strictEquals(8)); REQUIRE(map.get("needsUpdate").isUndefined());
}
TEST_CASE("PropertyBinding dispatch tables share prototype arrays until explicitly replaced", "[animation][property-binding]") {
    auto root = object({{"value", 1}}); PropertyBinding a(root, ".value"), b(root, ".value");
    REQUIRE(a.GetterByBindingType.sameIdentity(b.GetterByBindingType));
    REQUIRE(a.SetterByBindingTypeAndVersioning[0].sameIdentity(b.SetterByBindingTypeAndVersioning[0]));
    const auto old = a.GetterByBindingType[0];
    a.GetterByBindingType[0] = [](PropertyBinding&, AnimationArray array, JSValue offset) { array.set(offset.toNumber(), 77); };
    AnimationArray buffer = AnimationArray::array({0}); b.getValue(buffer, 0);
    REQUIRE(buffer.get(0).strictEquals(77));
    a.GetterByBindingType = a.GetterByBindingType.slice(); a.GetterByBindingType[0] = old;
    a.getValue(buffer, 0); REQUIRE(buffer.get(0).strictEquals(1));
    b.GetterByBindingType[0] = old; // Restore the prototype table for subsequent tests.
}
