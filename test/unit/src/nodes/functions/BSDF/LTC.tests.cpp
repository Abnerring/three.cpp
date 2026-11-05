#include "nodes/functions/BSDF/LTC.h"

#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"
#include "../../../../utils/parity/LTCFixtures.h"

#include "catch.hpp"

namespace {
JSValue vec3Node(double x, double y, double z) {
    return nativeTSLBaseBinding("vec3").call({}, {JSValue(x), JSValue(y), JSValue(z)});
}
JSValue floatNode(double value) { return nativeTSLBaseBinding("float").call({}, {JSValue(value)}); }
}  // namespace

TEST_CASE("LTC exports the three pinned functions") {
    const auto module = nativeLTCModule();
    for (const char* name : {"LTC_Uv", "LTC_Evaluate", "LTC_Evaluate_Volume"}) {
        REQUIRE(module.get(name).isCallable());
    }
    REQUIRE(module.get("notAFunction").isUndefined());
}

TEST_CASE("LTC_Uv builds the lookup coordinate from the roughness and the view dot normal") {
    auto inputs = JSValue::object(nullptr);
    inputs.set("N", vec3Node(0, 0, 1));
    inputs.set("V", vec3Node(0, 0, 1));
    inputs.set("roughness", floatNode(0.5));
    const auto uv = nativeLTCBinding("LTC_Uv").call({}, {inputs});
    REQUIRE(uv.get("isNode").strictEquals(true));
    // The source assigns the scaled coordinate back into the uv before returning it, and that answers a
    // var typed node. The differential runs the same call against the pinned module and compares this
    // type, so the expectation here is not a guess.
    REQUIRE(uv.get("type").strictEquals("VarNode"));
}

TEST_CASE("LTC exports preserve the pinned typed inputs and return types") {
    const auto shapes = LTCFixtures::snapshot().get("shapes");
    const auto check = [&](const char* name, const char* type,
                           std::initializer_list<std::pair<const char*, const char*>> inputs) {
        const auto shape = shapes.get(name);
        REQUIRE(shape.get("callbackLength").strictEquals(1));
        REQUIRE(shape.get("nodeType").strictEquals(type));
        REQUIRE(shape.get("layoutType").strictEquals(type));
        REQUIRE(shape.get("inputs").length() == inputs.size());
        std::size_t i = 0;
        for (const auto& [input, inputType] : inputs) {
            const auto actual = shape.get("inputs").get(std::to_string(i++));
            REQUIRE(actual.get("name").strictEquals(input));
            REQUIRE(actual.get("type").strictEquals(inputType));
        }
    };
    check("LTC_Uv", "vec2", {{"N", "vec3"}, {"V", "vec3"}, {"roughness", "float"}});
    check("LTC_Evaluate", "vec3", {{"N", "vec3"}, {"V", "vec3"}, {"P", "vec3"}, {"mInv", "mat3"},
                                    {"p0", "vec3"}, {"p1", "vec3"}, {"p2", "vec3"}, {"p3", "vec3"}});
    check("LTC_Evaluate_Volume", "vec3", {{"P", "vec3"}, {"p0", "vec3"}, {"p1", "vec3"}, {"p2", "vec3"}, {"p3", "vec3"}});
}

TEST_CASE("LTC bodies consume named inputs and retain typed internal helper calls") {
    const auto uv = LTCFixtures::uvBody();
    REQUIRE(uv.get("isNode").strictEquals(true));
    REQUIRE(uv.get("assignments").toNumber() > 0);
    const auto internal = LTCFixtures::internalShapes();
    REQUIRE(internal.keys().size() == 2);
    const auto clipped = internal.get("LTC_ClippedSphereFormFactor");
    REQUIRE(clipped.get("callbackLength").strictEquals(1));
    REQUIRE(clipped.get("nodeType").strictEquals("float"));
    REQUIRE(clipped.get("layoutType").strictEquals("float"));
    REQUIRE(clipped.get("inputs").length() == 1);
    REQUIRE(clipped.get("inputs").get("0").get("name").strictEquals("f"));
    REQUIRE(clipped.get("inputs").get("0").get("type").strictEquals("vec3"));
    const auto edge = internal.get("LTC_EdgeVectorFormFactor");
    REQUIRE(edge.get("callbackLength").strictEquals(1));
    REQUIRE(edge.get("nodeType").strictEquals("vec3"));
    REQUIRE(edge.get("layoutType").strictEquals("vec3"));
    REQUIRE(edge.get("inputs").length() == 2);
    REQUIRE(edge.get("inputs").get("0").get("name").strictEquals("v1"));
    REQUIRE(edge.get("inputs").get("1").get("name").strictEquals("v2"));
    REQUIRE(edge.get("inputs").get("0").get("type").strictEquals("vec3"));
    REQUIRE(edge.get("inputs").get("1").get("type").strictEquals("vec3"));
}
