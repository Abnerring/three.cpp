#include "nodes/functions/VolumetricLightingModel.h"

#include "core/JSValue.h"

#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue countingAssign(const JSValue& holder) {
    return JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
            auto target = captures[0];
            target.set("count", JSValue(target.get("count").toNumber() + 1));
            return {};
        },
        {holder}, "assign", 1);
}
}  // namespace

int main() {
    try {
        const auto ctor = nativeVolumetricLightingModelConstructor();
        const auto model = ctor.construct();

        // direct bails when the light node is not analytic.
        auto plainLightNode = JSValue::object(nullptr);
        plainLightNode.set("isAnalyticLightNode", false);
        auto plainInputs = JSValue::object(nullptr);
        plainInputs.set("lightNode", plainLightNode);
        plainInputs.set("lightColor", JSValue::object(nullptr));
        const auto plainResult = model.get("direct").call(model, {plainInputs, JSValue::object(nullptr)});

        // direct bails as well when the node is analytic but the light has no distance.
        auto analyticNode = JSValue::object(nullptr);
        analyticNode.set("isAnalyticLightNode", true);
        analyticNode.set("light", JSValue::object(nullptr));
        auto analyticInputs = JSValue::object(nullptr);
        analyticInputs.set("lightNode", analyticNode);
        analyticInputs.set("lightColor", JSValue::object(nullptr));
        const auto analyticResult =
            model.get("direct").call(model, {analyticInputs, JSValue::object(nullptr)});

        // finish assigns the outgoing ray light once.
        auto outgoing = JSValue::object(nullptr);
        outgoing.set("count", 0);
        outgoing.set("assign", countingAssign(outgoing));
        auto context = JSValue::object(nullptr);
        context.set("outgoingLight", outgoing);
        auto builder = JSValue::object(nullptr);
        builder.set("context", context);
        builder.set("material", JSValue::object(nullptr));
        model.get("finish").call(model, {builder});

        // start reports that the sampling loop is not ported.
        // The sampling loop is not ported, so start has to fail. The failure surfaces as the runtime's own
        // error type rather than std::runtime_error, so any exception counts.
        bool startThrew = false;
        try {
            model.get("start").call(model, {builder});
        } catch (...) {
            startThrew = true;
        }

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("ctorLength", ctor.get("length"));
        output.set("startLength", model.get("start").get("length"));
        output.set("scatteringLightLength", model.get("scatteringLight").get("length"));
        output.set("directLength", model.get("direct").get("length"));
        output.set("directRectAreaLength", model.get("directRectArea").get("length"));
        output.set("finishLength", model.get("finish").get("length"));
        output.set("indirectLength", model.get("indirect").get("length"));
        output.set("directPlainIsUndefined", plainResult.isUndefined());
        output.set("directAnalyticIsUndefined", analyticResult.isUndefined());
        output.set("finishAssigns", outgoing.get("count"));
        output.set("startThrows", startThrew);
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
