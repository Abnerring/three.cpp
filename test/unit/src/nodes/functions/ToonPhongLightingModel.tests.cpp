#include "nodes/functions/PhongLightingModel.h"
#include "nodes/functions/ToonLightingModel.h"

#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

#include <string>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue vec3Node(double x, double y, double z) {
    return nativeTSLBaseBinding("vec3").call({}, {JSValue(x), JSValue(y), JSValue(z)});
}

// A reflected light stand-in that counts the assignments the models make into it.
struct ReflectedLight {
    JSValue object, diffuse, specular, indirectDiffuse;
    ReflectedLight() {
        diffuse = counter("addAssign", "mulAssign");
        specular = counter("addAssign", "mulAssign");
        indirectDiffuse = counter("addAssign", "mulAssign");
        object = JSValue::object(nullptr);
        object.set("directDiffuse", diffuse);
        object.set("directSpecular", specular);
        object.set("indirectDiffuse", indirectDiffuse);
    }
    static JSValue counter(const char* first, const char* second) {
        auto value = JSValue::object(nullptr);
        value.set("addAssignCount", 0);
        value.set("mulAssignCount", 0);
        for (const char* name : {first, second}) {
            const std::string key = name;
            value.set(name, JSValue::function(
                +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
                    auto holder = captures[0];
                    const std::string field = captures[1].string() + "Count";
                    holder.set(field.c_str(), JSValue(holder.get(field.c_str()).toNumber() + 1));
                    return {};
                },
                {value, JSValue(key)}, name, 1));
        }
        return value;
    }
};

JSValue directInputs(const ReflectedLight& reflected) {
    auto inputs = JSValue::object(nullptr);
    inputs.set("lightDirection", vec3Node(0, 0, 1));
    inputs.set("lightColor", vec3Node(1, 1, 1));
    inputs.set("reflectedLight", reflected.object);
    return inputs;
}

JSValue builderWithContext(const ReflectedLight& reflected) {
    auto context = JSValue::object(nullptr);
    context.set("reflectedLight", reflected.object);
    context.set("ambientOcclusion", nativeTSLBaseBinding("float").call({}, {JSValue(1.0)}));
    context.set("irradiance", vec3Node(1, 1, 1));
    auto builder = JSValue::object(nullptr);
    builder.set("context", context);
    auto material = JSValue::object(nullptr);
    builder.set("material", material);
    return builder;
}
}  // namespace

TEST_CASE("ToonLightingModel keeps the pinned hook arities") {
    const auto module = nativeToonLightingModelModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("ToonLightingModel"));
    const auto model = ctor.construct();
    REQUIRE(model.get("direct").get("length").strictEquals(2));
    REQUIRE(model.get("indirect").get("length").strictEquals(1));
    // The inherited hooks keep their base arities.
    REQUIRE(model.get("start").get("length").strictEquals(1));
    REQUIRE(model.get("finish").get("length").strictEquals(0));
}

TEST_CASE("ToonLightingModel direct and indirect assign into the reflected light") {
    const auto model = nativeToonLightingModelConstructor().construct();
    ReflectedLight reflected;
    model.get("direct").call(model, {directInputs(reflected), builderWithContext(reflected)});
    REQUIRE(reflected.diffuse.get("addAssignCount").toNumber() == Approx(1));

    model.get("indirect").call(model, {builderWithContext(reflected)});
    // The indirect term is added and then multiplied by the ambient occlusion.
    REQUIRE(reflected.indirectDiffuse.get("addAssignCount").toNumber() == Approx(1));
    REQUIRE(reflected.indirectDiffuse.get("mulAssignCount").toNumber() == Approx(1));
}

TEST_CASE("PhongLightingModel defaults its specular flag and honours it") {
    const auto module = nativePhongLightingModelModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("PhongLightingModel"));
    // constructor( specular = true ) has a default, so Function.length is 0.
    REQUIRE(ctor.get("length").strictEquals(0));

    const auto withSpecular = ctor.construct();
    REQUIRE(withSpecular.get("specular").strictEquals(true));
    ReflectedLight reflected;
    withSpecular.get("direct").call(withSpecular, {directInputs(reflected), builderWithContext(reflected)});
    REQUIRE(reflected.diffuse.get("addAssignCount").toNumber() == Approx(1));
    REQUIRE(reflected.specular.get("addAssignCount").toNumber() == Approx(1));

    // Turning the flag off skips the specular term entirely.
    const auto withoutSpecular = ctor.construct({JSValue(false)});
    REQUIRE(withoutSpecular.get("specular").strictEquals(false));
    ReflectedLight plain;
    withoutSpecular.get("direct").call(withoutSpecular, {directInputs(plain), builderWithContext(plain)});
    REQUIRE(plain.diffuse.get("addAssignCount").toNumber() == Approx(1));
    REQUIRE(plain.specular.get("addAssignCount").toNumber() == Approx(0));
}

TEST_CASE("PhongLightingModel indirect assigns and masks like the toon model") {
    const auto model = nativePhongLightingModelConstructor().construct();
    ReflectedLight reflected;
    model.get("indirect").call(model, {builderWithContext(reflected)});
    REQUIRE(reflected.indirectDiffuse.get("addAssignCount").toNumber() == Approx(1));
    REQUIRE(reflected.indirectDiffuse.get("mulAssignCount").toNumber() == Approx(1));
    // The phong direct takes the destructured inputs only, so its length is one; indirect keeps the
    // plain builder parameter.
    REQUIRE(model.get("direct").get("length").strictEquals(1));
    REQUIRE(model.get("indirect").get("length").strictEquals(1));
}
