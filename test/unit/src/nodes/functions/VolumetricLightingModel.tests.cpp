#include "nodes/functions/VolumetricLightingModel.h"

#include "nodes/core/Node.h"

#include "catch.hpp"

#include <stdexcept>
#include <string>

TEST_CASE("VolumetricLightingModel exposes the pinned hook surface") {
    const auto module = nativeVolumetricLightingModelModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.isCallable());
    REQUIRE(ctor.get("name").strictEquals("VolumetricLightingModel"));

    const auto model = ctor.construct();
    // The two hooks that the source declares with a destructured inputs parameter and a builder.
    REQUIRE(model.get("direct").get("length").strictEquals(2));
    REQUIRE(model.get("directRectArea").get("length").strictEquals(2));
    // scatteringLight takes the colour and the builder, and finish takes the builder alone.
    REQUIRE(model.get("scatteringLight").get("length").strictEquals(2));
    REQUIRE(model.get("finish").get("length").strictEquals(1));
    // The inherited hooks keep the base arities.
    REQUIRE(model.get("start").get("length").strictEquals(1));
    REQUIRE(model.get("indirect").get("length").strictEquals(0));
}

TEST_CASE("VolumetricLightingModel direct returns early for a light that is not analytic") {
    const auto model = nativeVolumetricLightingModelConstructor().construct();
    // The source bails when the light node is not analytic or the light has no distance.
    auto lightNode = JSValue::object(nullptr);
    lightNode.set("isAnalyticLightNode", false);
    auto inputs = JSValue::object(nullptr);
    inputs.set("lightNode", lightNode);
    inputs.set("lightColor", JSValue::object(nullptr));
    REQUIRE(model.get("direct").call(model, {inputs, JSValue::object(nullptr)}).isUndefined());

    // And it bails as well when the node is analytic but the light has no distance.
    auto analytic = JSValue::object(nullptr);
    analytic.set("isAnalyticLightNode", true);
    auto plainLight = JSValue::object(nullptr);
    analytic.set("light", plainLight);
    auto analyticInputs = JSValue::object(nullptr);
    analyticInputs.set("lightNode", analytic);
    analyticInputs.set("lightColor", JSValue::object(nullptr));
    REQUIRE(model.get("direct").call(model, {analyticInputs, JSValue::object(nullptr)}).isUndefined());
}

TEST_CASE("VolumetricLightingModel finish assigns the outgoing ray light") {
    const auto model = nativeVolumetricLightingModelConstructor().construct();
    auto outgoing = JSValue::object(nullptr);
    outgoing.set("count", 0);
    outgoing.set("assign", JSValue::function(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>& captures) -> JSValue {
            auto holder = captures[0];
            holder.set("count", JSValue(holder.get("count").toNumber() + 1));
            return {};
        },
        {outgoing}, "assign", 1));
    auto context = JSValue::object(nullptr);
    context.set("outgoingLight", outgoing);
    auto builder = JSValue::object(nullptr);
    builder.set("context", context);
    model.get("finish").call(model, {builder});
    REQUIRE(outgoing.get("count").toNumber() == Approx(1));
}

TEST_CASE("VolumetricLightingModel start reports that its sampling loop is not ported") {
    const auto model = nativeVolumetricLightingModelConstructor().construct();
    auto builder = JSValue::object(nullptr);
    auto material = JSValue::object(nullptr);
    builder.set("material", material);
    // The ray marching loop is not ported, so the method has to fail rather than silently ignore the
    // volume. The failure surfaces as the runtime's own error type, which is why any exception counts.
    REQUIRE_THROWS(model.get("start").call(model, {builder}));
}
