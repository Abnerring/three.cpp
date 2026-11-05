#include "misc/MorphAnimMesh.h"

#include "animation/AnimationClip.h"
#include "animation/tracks/NumberKeyframeTrack.h"
#include "geometries/BoxGeometry.h"
#include "materials/MeshBasicMaterial.h"

#include "catch.hpp"

#include <memory>
#include <stdexcept>

namespace {
std::shared_ptr<AnimationClip> makeClip(const std::string& name, double duration, int trackCount) {
    auto tracks = std::make_shared<AnimationClip::Tracks>();
    for (int i = 0; i < trackCount; ++i) {
        tracks->push_back(std::make_shared<NumberKeyframeTrack>(
            ".position[x]", std::vector<double>{0, duration}, std::vector<double>{0, 1}));
    }
    return std::make_shared<AnimationClip>(name, duration, tracks);
}
}  // namespace

TEST_CASE("MorphAnimMesh owns a mixer and flips its direction") {
    auto mesh = MorphAnimMesh::create(std::make_shared<BoxGeometry>(),
                                      std::make_shared<MeshBasicMaterial>());
    REQUIRE(mesh->type == "MorphAnimMesh");
    REQUIRE(mesh->activeAction.isNull());
    REQUIRE(mesh->mixer->getRoot().toBoolean());

    mesh->setDirectionBackward();
    REQUIRE(mesh->mixer->timeScale.toNumber() == Approx(-1.0));
    mesh->setDirectionForward();
    REQUIRE(mesh->mixer->timeScale.toNumber() == Approx(1.0));
}

TEST_CASE("MorphAnimMesh plays a clip by label and scales its time") {
    auto mesh = MorphAnimMesh::create(std::make_shared<BoxGeometry>(),
                                      std::make_shared<MeshBasicMaterial>());
    // Clip with two tracks over two seconds: the action time scale becomes (2 * 30) / 2 = 30.
    mesh->animations.push_back(makeClip("walk", 2.0, 2));

    mesh->playAnimation("walk", 30.0);
    REQUIRE(mesh->activeAction.toBoolean());
    auto action = AnimationAction::fromObject(mesh->activeAction);
    REQUIRE(action.timeScale.toNumber() == Approx(30.0));
    REQUIRE(action.isRunning().toBoolean());

    REQUIRE_NOTHROW(mesh->updateAnimation(0.1));
    REQUIRE_NOTHROW(mesh->playAnimation("walk", 10.0));
    auto actionAfterReplay = AnimationAction::fromObject(mesh->activeAction);
    REQUIRE(actionAfterReplay.timeScale.toNumber() == Approx(10.0));
}

TEST_CASE("MorphAnimMesh rejects an unknown clip label") {
    auto mesh = MorphAnimMesh::create(std::make_shared<BoxGeometry>(),
                                      std::make_shared<MeshBasicMaterial>());
    REQUIRE_THROWS_AS(mesh->playAnimation("missing", 30.0), std::runtime_error);
}
