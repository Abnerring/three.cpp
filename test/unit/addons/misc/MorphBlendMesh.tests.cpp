#include "misc/MorphBlendMesh.h"

#include "geometries/BoxGeometry.h"
#include "materials/MeshBasicMaterial.h"

#include "catch.hpp"

#include <cmath>
#include <memory>

TEST_CASE("MorphBlendMesh creates the default animation from the morph dictionary") {
    auto mesh = std::make_shared<MorphBlendMesh>(std::make_shared<BoxGeometry>(),
                                                 std::make_shared<MeshBasicMaterial>());
    REQUIRE(mesh->animationsMap.size() == 1);
    REQUIRE(mesh->animationsList.size() == 1);

    const auto& animation = mesh->animationsMap.at("__default");
    // With no morph targets the default range is start zero, end minus one, length zero and fps zero.
    REQUIRE(animation.start == Approx(0));
    REQUIRE(animation.end == Approx(-1));
    REQUIRE(animation.length == Approx(0));
    REQUIRE(animation.fps == Approx(0));
    REQUIRE(std::isinf(animation.duration));
    REQUIRE(animation.duration < 0);
    REQUIRE(animation.weight == Approx(1));
    REQUIRE(animation.active == false);
}

TEST_CASE("MorphBlendMesh createAnimation and the setters follow the pinned arithmetic") {
    auto mesh = std::make_shared<MorphBlendMesh>(std::make_shared<BoxGeometry>(),
                                                 std::make_shared<MeshBasicMaterial>());
    mesh->createAnimation("walk", 0, 2, 2);
    {
        const auto& animation = mesh->animationsMap.at("walk");
        REQUIRE(animation.length == Approx(3));
        REQUIRE(animation.duration == Approx(1));
    }
    mesh->setAnimationFPS("walk", 4);
    REQUIRE(mesh->getAnimationDuration("walk") == Approx(0.5));
    mesh->setAnimationDuration("walk", 1);
    REQUIRE(mesh->animationsMap.at("walk").fps == Approx(2));
    mesh->setAnimationWeight("walk", 0.5);
    REQUIRE(mesh->animationsMap.at("walk").weight == Approx(0.5));
    mesh->setAnimationTime("walk", 0.25);
    REQUIRE(mesh->getAnimationTime("walk") == Approx(0.25));
    mesh->setAnimationDirectionBackward("walk");
    REQUIRE(mesh->animationsMap.at("walk").direction == -1);
    REQUIRE(mesh->animationsMap.at("walk").directionBackwards);
    mesh->setAnimationDirectionForward("walk");
    REQUIRE(mesh->animationsMap.at("walk").direction == 1);
    REQUIRE_FALSE(mesh->animationsMap.at("walk").directionBackwards);

    // Unknown names are ignored by the setters and answer the pinned defaults from the getters.
    REQUIRE(mesh->getAnimationTime("missing") == Approx(0));
    REQUIRE(mesh->getAnimationDuration("missing") == Approx(-1));
    REQUIRE_NOTHROW(mesh->playAnimation("missing"));
}

TEST_CASE("MorphBlendMesh autoCreateAnimations derives frame ranges from the dictionary keys") {
    auto mesh = std::make_shared<MorphBlendMesh>(std::make_shared<BoxGeometry>(),
                                                 std::make_shared<MeshBasicMaterial>());
    // The dictionary is a sorted map here, so keys inserted in alphabetical order agree with the pinned
    // insertion order enumeration.
    // The pinned Mesh always owns an empty dictionary; this runtime only creates one from morph attributes.
    if (!mesh->morphTargetDictionary) {
        mesh->morphTargetDictionary = std::make_shared<std::map<std::string, std::size_t>>();
    }
    mesh->morphTargetDictionary->clear();
    (*mesh->morphTargetDictionary)["run_01"] = 0;
    (*mesh->morphTargetDictionary)["run_02"] = 1;
    (*mesh->morphTargetDictionary)["walk_01"] = 2;
    (*mesh->morphTargetDictionary)["walk_02"] = 3;

    mesh->autoCreateAnimations(30);
    REQUIRE(mesh->animationsMap.at("run").start == Approx(0));
    REQUIRE(mesh->animationsMap.at("run").end == Approx(1));
    REQUIRE(mesh->animationsMap.at("run").fps == Approx(30));
    REQUIRE(mesh->animationsMap.at("walk").start == Approx(2));
    REQUIRE(mesh->animationsMap.at("walk").end == Approx(3));
    REQUIRE(mesh->firstAnimation == "run");
}

TEST_CASE("MorphBlendMesh update advances the time and rewrites the influences") {
    auto mesh = std::make_shared<MorphBlendMesh>(std::make_shared<BoxGeometry>(),
                                                 std::make_shared<MeshBasicMaterial>());
    mesh->morphTargetInfluences = std::vector<double>(3, 0);
    mesh->createAnimation("walk", 0, 2, 2);
    mesh->setAnimationWeight("walk", 1);
    mesh->playAnimation("walk");
    REQUIRE(mesh->animationsMap.at("walk").active);
    REQUIRE(mesh->getAnimationTime("walk") == Approx(0));

    // The first step stays inside the first keyframe, so the current frame takes the full weight.
    mesh->update(0.1);
    REQUIRE(mesh->animationsMap.at("walk").time == Approx(0.1));
    auto& influences = mesh->morphTargetInfluences->storage()->values;
    REQUIRE(influences[0] == Approx(1));

    // A larger step crosses into the second keyframe and blends the two.
    mesh->update(0.5);
    REQUIRE(mesh->animationsMap.at("walk").currentFrame == Approx(1));
    REQUIRE(influences[0] == Approx(0.19999999999999996).margin(1e-12));
    REQUIRE(influences[1] == Approx(0.8000000000000002).margin(1e-12));
}

TEST_CASE("MorphBlendMesh update mirrors the time when the loop is mirrored") {
    auto mesh = std::make_shared<MorphBlendMesh>(std::make_shared<BoxGeometry>(),
                                                 std::make_shared<MeshBasicMaterial>());
    mesh->morphTargetInfluences = std::vector<double>(3, 0);
    mesh->createAnimation("walk", 0, 2, 2);
    mesh->animationsMap.at("walk").mirroredLoop = true;
    mesh->playAnimation("walk");
    mesh->setAnimationTime("walk", 0.9);
    mesh->update(0.2);
    const auto& animation = mesh->animationsMap.at("walk");
    REQUIRE(animation.direction == -1);
    REQUIRE(animation.time == Approx(1));
    REQUIRE(animation.directionBackwards);
    REQUIRE(animation.currentFrame == Approx(2));
}
