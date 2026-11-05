#include "nodes/core/LightingModel.h"
#include "../../../utils/parity/LightingModelScenarios.h"

#include "catch.hpp"

#include <string>

using Hook = LightingModel::Hook;
using Caller = LightingModel::Caller;

TEST_CASE("LightingModel has six hooks") {
    const auto hooks = LightingModel::hooks();
    REQUIRE(hooks.size() == 6);
    REQUIRE(hooks[0] == Hook::Start);
    REQUIRE(hooks[1] == Hook::Finish);
    REQUIRE(hooks[5] == Hook::AmbientOcclusion);
    REQUIRE(std::string(LightingModel::kTypeName) == "LightingModel");
}

TEST_CASE("LightingModel gives a body to only its first hook") {
    // The other five are empty, which is what makes the class abstract in the sense the source
    // means rather than in the language's.
    REQUIRE(LightingModel::hasBody(Hook::Start));
    REQUIRE_FALSE(LightingModel::hasBody(Hook::Finish));
    REQUIRE_FALSE(LightingModel::hasBody(Hook::Direct));
    REQUIRE_FALSE(LightingModel::hasBody(Hook::DirectRectArea));
    REQUIRE_FALSE(LightingModel::hasBody(Hook::Indirect));
    REQUIRE_FALSE(LightingModel::hasBody(Hook::AmbientOcclusion));
    // Counting shows exactly one has a body.
    int withBody = 0;
    for (const auto hook : LightingModel::hooks()) {
        if (LightingModel::hasBody(hook)) withBody += 1;
    }
    REQUIRE(withBody == 1);
}

TEST_CASE("LightingModel's hooks are called by three different parties") {
    // Four by the framework directly.
    REQUIRE(LightingModel::callerOf(Hook::Start) == Caller::Framework);
    REQUIRE(LightingModel::callerOf(Hook::Finish) == Caller::Framework);
    REQUIRE(LightingModel::callerOf(Hook::Direct) == Caller::Framework);
    REQUIRE(LightingModel::callerOf(Hook::DirectRectArea) == Caller::Framework);
    // The indirect one through the first hook, not directly.
    REQUIRE(LightingModel::callerOf(Hook::Indirect) == Caller::TheFirstHook);
    REQUIRE(LightingModel::callerOf(Hook::Indirect) != Caller::Framework);
    // And the ambient occlusion one by nobody at all.
    REQUIRE(LightingModel::callerOf(Hook::AmbientOcclusion) == Caller::Nobody);
    REQUIRE(LightingModel::callerOf(Hook::AmbientOcclusion) != Caller::Framework);
}

TEST_CASE("LightingModel's first hook does two things") {
    // It sets the direct lights up and then calls the indirect hook, so an override that does
    // not call through skips both rather than only one.
    const auto steps = LightingModel::startSteps();
    REQUIRE(steps.size() == 2);
    REQUIRE(steps[0] == "setupLights");
    REQUIRE(steps[1] == "indirect");
    // The second step is a hook rather than a plain call, which is why the two are tied.
    REQUIRE(steps[1] == "indirect");
    REQUIRE(steps[0] != steps[1]);
}

TEST_CASE("LightingModel says its ambient occlusion is called by hand") {
    // The note is the clearest statement that the hook is not in the automatic path.
    const std::string note = LightingModel::ambientOcclusionNote();
    REQUIRE(note.find("must be called manually") != std::string::npos);
    REQUIRE(note.find("indirect term") != std::string::npos);
    // It says the opposite of automatic, which is what the caller table records.
    REQUIRE(note.find("automatically") == std::string::npos);
}

TEST_CASE("LightingModel constructs fresh objects and is the shared Basic and Phong base") {
    const auto result = LightingModelScenarios::constructionAndHooks();
    REQUIRE(result.get("instanceOwnNames").length() == 0);
    for (const auto* key : {"distinct", "notConstructor", "notCallable", "prototype", "constructor", "instanceof",
                            "objectPrototype", "functionPrototype", "newTargetPrototype", "newTargetConstructor",
                            "primitivePrototypeFallback", "basicBase", "basicPrototypeBase", "phongBase",
                            "phongCanonicalStart", "basicInstancePrototype", "phongInstancePrototype"})
        REQUIRE(result.get(key).strictEquals(true));
    REQUIRE(result.get("bareCall").strictEquals("TypeError"));
    const auto hooks = result.get("emptyHooks");
    REQUIRE(hooks.length() == 5);
    for (unsigned i = 0; i < hooks.length(); ++i) {
        REQUIRE(hooks.get(JSValue(i)).get("undefined").strictEquals(true));
        REQUIRE(hooks.get(JSValue(i)).get("new").strictEquals("TypeError"));
    }
}

TEST_CASE("LightingModel start reads lightsNode twice and preserves getter call order and receivers") {
    const auto result = LightingModelScenarios::startCase("baseline");
    REQUIRE(result.get("result").strictEquals("undefined"));
    REQUIRE(result.get("reads").strictEquals(2));
    REQUIRE(result.get("events").toJSON()->dump() ==
        "[\"get lightsNode:1\",\"get setupLights\",\"get lightsNode:2\",\"get getLightNodes\","
        "\"call getLightNodes\",\"call setupLights\",\"get indirect\",\"call indirect\"]");
    const auto checks = result.get("checks");
    REQUIRE(checks.length() == 8);
    for (unsigned i = 0; i < checks.length(); ++i) REQUIRE(checks.get(JSValue(i)).strictEquals(true));
    const auto replacement = LightingModelScenarios::startCase("replaceSetup");
    REQUIRE(replacement.get("events").toJSON()->dump() == result.get("events").toJSON()->dump());
}

TEST_CASE("LightingModel start short circuits exceptions and supports reentry from every callback") {
    for (const auto& mode : LightingModelScenarios::modes()) {
        const auto result = LightingModelScenarios::startCase(mode);
        const bool succeeds = mode == "baseline" || mode == "replaceSetup" || mode.rfind("reenter:", 0) == 0;
        REQUIRE(result.get("result").strictEquals(succeeds ? "undefined" : "TypeError"));
        const auto checks = result.get("checks");
        for (unsigned i = 0; i < checks.length(); ++i) REQUIRE(checks.get(JSValue(i)).strictEquals(true));
        if (mode.rfind("throw:", 0) == 0) {
            const auto events = result.get("events");
            REQUIRE(events.get(JSValue(events.length() - 1)).strictEquals(mode.substr(6)));
        }
        if (mode.rfind("reenter:", 0) == 0) {
            REQUIRE(result.get("reads").strictEquals(4));
            REQUIRE(result.get("events").length() == 16);
        }
    }
}
