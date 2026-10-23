#include "nodes/core/LightingModel.h"

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