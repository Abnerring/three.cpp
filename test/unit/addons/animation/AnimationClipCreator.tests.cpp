#include "catch.hpp"
#include "animation/AnimationClipCreator.h"
#include "animation/tracks/BooleanKeyframeTrack.h"
#include "animation/tracks/NumberKeyframeTrack.h"
#include "math/Color.h"
#include "math/Vector3.h"
#include <cmath>
#include <memory>

// The reference values come from the pinned AnimationClipCreator.js; the differential probe
// compares the same clips bit for bit.
TEST_CASE("AnimationClipCreator builds the pinned demonstration clips", "[animationclipcreator]") {
    auto rotation = AnimationClipCreator::CreateRotationAnimation(2, "y");
    REQUIRE(rotation->name.toString() == "");
    REQUIRE(rotation->duration == 2.);
    REQUIRE(rotation->tracks->size() == 1);
    REQUIRE((*rotation->tracks)[0]->name.toString() == ".rotation[y]");
    REQUIRE((*rotation->tracks)[0]->times.size() == 2);
    REQUIRE((*rotation->tracks)[0]->values.number(1) == 360.);
    auto scale = AnimationClipCreator::CreateScaleAxisAnimation(4);
    REQUIRE((*scale->tracks)[0]->name.toString() == ".scale[x]");
    REQUIRE((*scale->tracks)[0]->values.number(1) == 1.);
    auto visibility = AnimationClipCreator::CreateVisibilityAnimation(6);
    REQUIRE(visibility->duration == 6.);
    REQUIRE((*visibility->tracks)[0]->name.toString() == ".visible");
    REQUIRE((*visibility->tracks)[0]->times.size() == 3);
    REQUIRE((*visibility->tracks)[0]->times.number(1) == 3.);
    REQUIRE((*visibility->tracks)[0]->values.number(1) == 0.); // false
    auto constant = AnimationClipCreator::CreateMaterialColorAnimation(5, {Color(1, 0, 0)});
    REQUIRE((*constant->tracks)[0]->name.toString() == ".material.color");
    REQUIRE((*constant->tracks)[0]->times.number(0) == 0.); // a single colour keeps timeStep 0
    std::vector<Color> colors{Color(1, 0, 0), Color(0, 1, 0), Color(0, 0, 1)};
    auto material = AnimationClipCreator::CreateMaterialColorAnimation(4, colors);
    REQUIRE((*material->tracks)[0]->times.size() == 3);
    REQUIRE((*material->tracks)[0]->times.number(1) == 2.);
    REQUIRE((*material->tracks)[0]->values.size() == 9);
    REQUIRE((*material->tracks)[0]->values.number(4) == 1.);
    // The two random factories consume ten samples per time unit and use the injected source.
    std::size_t calls = 0;
    AnimationClipCreator::setRandomGenerator([&calls]() { calls += 1; return 0.5; });
    auto shake = AnimationClipCreator::CreateShakeAnimation(1.5, Vector3(2, 1, 0.5));
    REQUIRE(calls == 45);
    REQUIRE((*shake->tracks)[0]->name.toString() == ".position");
    REQUIRE((*shake->tracks)[0]->times.size() == 15);
    REQUIRE((*shake->tracks)[0]->times.number(14) == 1.4f); // the time buffer is Float32
    REQUIRE((*shake->tracks)[0]->values.number(0) == 0.); // ( 0.5 * 2 - 1 ) * 2
    auto pulse = AnimationClipCreator::CreatePulsationAnimation(1, 4);
    REQUIRE((*pulse->tracks)[0]->name.toString() == ".scale");
    REQUIRE((*pulse->tracks)[0]->values.number(0) == 2.); // 0.5 * 4
    AnimationClipCreator::setRandomGenerator(nullptr);
}