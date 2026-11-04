#include "nodes/lighting/LightProbeNode.h"

#include "math/Vector3.h"
#include "math/Color.h"

#include "catch.hpp"

TEST_CASE("LightProbeNode exposes the analytic light probe shape") {
    const auto module = nativeLightProbeNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.isCallable());
    REQUIRE(ctor.get("type").strictEquals("LightProbeNode"));

    auto light = JSValue::object(nullptr);
    auto lightColor = nativeColorConstructor().construct();
    REQUIRE(lightColor.get("copy").isCallable());
    light.set("color", lightColor);
    light.set("uuid", "probe");
    auto node = ctor.construct({light});
    REQUIRE(node.get("type").strictEquals("LightProbeNode"));
    REQUIRE(node.get("isLightingNode").strictEquals(true));
    REQUIRE(node.get("isAnalyticLightNode").strictEquals(true));
    REQUIRE(node.get("light").sameValue(light));
    REQUIRE(node.get("lightProbe").get("type").strictEquals("UniformArrayNode"));
    REQUIRE(node.get("lightProbe").get("array").get("length").strictEquals(9));
}

TEST_CASE("LightProbeNode update scales spherical harmonic coefficients") {
    auto light = JSValue::object(nullptr);
    auto lightColor = nativeColorConstructor().construct();
    REQUIRE(lightColor.get("copy").isCallable());
    light.set("color", lightColor);
    light.set("intensity", 2);
    auto coefficients = JSValue::array();
    const auto vector = nativeVector3Constructor();
    for (int i = 0; i < 9; ++i) {
        auto coefficient = vector.construct();
        REQUIRE(coefficient.get("set").isCallable());
        coefficient.get("set").call(coefficient, {i + 1, i + 2, i + 3});
        coefficients.set(JSValue(i), coefficient);
    }
    auto sh = JSValue::object(nullptr);
    sh.set("coefficients", coefficients);
    light.set("sh", sh);
    auto node = nativeLightProbeNodeConstructor().construct({light});
    REQUIRE(node.get("update").isCallable());
    node.get("update").call(node, {JSValue::object(nullptr)});
    const auto first = node.get("lightProbe").get("array").get(JSValue(0));
    REQUIRE(first.get("x").toNumber() == Approx(2));
    REQUIRE(first.get("y").toNumber() == Approx(4));
    REQUIRE(first.get("z").toNumber() == Approx(6));
}
