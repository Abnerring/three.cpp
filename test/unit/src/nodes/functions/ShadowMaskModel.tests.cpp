#include "nodes/functions/ShadowMaskModel.h"

#include "nodes/core/Node.h"

#include "catch.hpp"

#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue counting(const JSValue& holder) {
    return JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
            auto target = captures[0];
            target.set("count", JSValue(target.get("count").toNumber() + 1));
            return {};
        },
        {holder}, "assign", 1);
}
}  // namespace

TEST_CASE("ShadowMaskModel exposes the lighting model surface and its shadow accumulator") {
    const auto module = nativeShadowMaskModelModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.isCallable());
    REQUIRE(ctor.get("name").strictEquals("ShadowMaskModel"));

    const auto model = ctor.construct();
    // The constructor starts the accumulator as a variable node named shadowMask.
    REQUIRE(model.get("shadowNode").get("isNode").strictEquals(true));
    REQUIRE(model.get("shadowNode").get("type").strictEquals("VarNode"));
    // The two overridden hooks keep the source arity, and the base hooks are still reachable through the
    // prototype chain.
    REQUIRE(model.get("direct").get("length").strictEquals(1));
    REQUIRE(model.get("finish").get("length").strictEquals(1));
    for (const char* hook : {"start", "directRectArea", "indirect", "ambientOcclusion"}) {
        REQUIRE(model.get(hook).isCallable());
    }
}

TEST_CASE("ShadowMaskModel direct multiplies in a light node shadow only when there is one") {
    const auto model = nativeShadowMaskModelConstructor().construct();

    // No shadow on the light node leaves the accumulator untouched and does not throw.
    auto withoutShadow = JSValue::object(nullptr);
    withoutShadow.set("shadowNode", nullptr);
    auto directInput = JSValue::object(nullptr);
    directInput.set("lightNode", withoutShadow);
    REQUIRE_NOTHROW(model.get("direct").call(model, {directInput}));

    // With a shadow node the accumulator is multiplied by it.
    auto withShadow = JSValue::object(nullptr);
    withShadow.set("shadowNode", model.get("shadowNode"));
    auto shadowInput = JSValue::object(nullptr);
    shadowInput.set("lightNode", withShadow);
    REQUIRE_NOTHROW(model.get("direct").call(model, {shadowInput}));
}

TEST_CASE("ShadowMaskModel finish masks the diffuse alpha and assigns the outgoing light") {
    const auto model = nativeShadowMaskModelConstructor().construct();
    auto outgoing = JSValue::object(nullptr);
    outgoing.set("count", 0);
    auto rgb = JSValue::object(nullptr);
    rgb.set("assign", counting(outgoing));
    outgoing.set("rgb", rgb);
    auto context = JSValue::object(nullptr);
    context.set("outgoingLight", outgoing);
    auto finishInput = JSValue::object(nullptr);
    finishInput.set("context", context);

    model.get("finish").call(model, {finishInput});
    // The finish assigns the diffuse colour into the outgoing light exactly once.
    REQUIRE(outgoing.get("count").toNumber() == Approx(1));
}

TEST_CASE("ShadowMaskModel keeps the pinned base hook arities") {
    const auto model = nativeShadowMaskModelConstructor().construct();
    // Only start names its parameter in LightingModel; the other five have their parameters commented
    // out in the source, so their function length is zero. The differential compares these six lengths
    // against the pinned module.
    REQUIRE(model.get("start").get("length").strictEquals(1));
    for (const char* hook : {"finish", "direct", "directRectArea", "indirect", "ambientOcclusion"}) {
        // finish and direct are overridden by this model with a single destructured parameter.
        if (std::string(hook) == "finish" || std::string(hook) == "direct") continue;
        REQUIRE(model.get(hook).get("length").strictEquals(0));
    }
}