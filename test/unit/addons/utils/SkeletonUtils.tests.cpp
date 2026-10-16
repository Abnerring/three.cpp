#include "catch.hpp"
#include "core/Object3D.h"
#include "core/TypedArray.h"
#include "animation/tracks/QuaternionKeyframeTrack.h"
#include "animation/tracks/VectorKeyframeTrack.h"
#include "math/Quaternion.h"
#include "objects/Bone.h"
#include "objects/Skeleton.h"
#include "objects/SkinnedMesh.h"
#include "utils/SkeletonUtils.h"
#include <cmath>
#include <memory>
#include <vector>

namespace {
bool close(double actual, double expected) { return std::abs(actual - expected) <= 1e-12; }
std::shared_ptr<Bone> bone(const std::string& name, double x, double y, double z) {
    auto value = std::make_shared<Bone>();
    value->name = name;
    value->position.set(x, y, z);
    return value;
}
struct Rig {
    std::shared_ptr<Object3D> root;
    std::shared_ptr<SkinnedMesh> mesh;
    std::vector<std::shared_ptr<Bone>> bones;
};
Rig rig() {
    Rig result;
    result.root = std::make_shared<Object3D>();
    result.bones = {bone("hip", 0, 1, 0), bone("spine", 0, 0.5, 0), bone("arm", -0.4, 0.1, 0)};
    result.root->add(result.bones[0]);
    result.bones[0]->add(result.bones[1]);
    result.bones[1]->add(result.bones[2]);
    result.root->updateMatrixWorld(true);
    result.mesh = std::make_shared<SkinnedMesh>();
    Skeleton::Bones bones;
    for (const auto& entry : result.bones) bones.push_back(entry);
    result.mesh->skeleton = std::make_shared<Skeleton>(bones);
    result.mesh->bindMatrix.makeTranslation(0.1, 0., 0.2);
    result.root->add(result.mesh);
    result.root->updateMatrixWorld(true);
    return result;
}
std::shared_ptr<AnimationClip> waveClip() {
    auto tracks = std::make_shared<AnimationClip::Tracks>();
    const Float32Array times(std::vector<double>{0., 0.5, 1.});
    tracks->push_back(std::make_shared<VectorKeyframeTrack>(JSValue(".bones[hip].position"), AnimationArray(times),
        AnimationArray(Float32Array(std::vector<double>{0., 1., 0., 0.1, 1.2, 0.2, 0.2, 1.4, 0.4}))));
    tracks->push_back(std::make_shared<QuaternionKeyframeTrack>(JSValue(".bones[spine].quaternion"),
        AnimationArray(times),
        AnimationArray(Float32Array(std::vector<double>{0., 0., 0., 1., 0.1, 0., 0., 0.99498743710662, 0.2, 0., 0.,
            0.97979589711327}))));
    return std::make_shared<AnimationClip>(JSValue("wave"), 1., tracks);
}
}

TEST_CASE("SkeletonUtils clone rebinds every skinned mesh to cloned bones", "[skeletonutils]") {
    Rig source = rig();
    auto cloned = SkeletonUtils::clone(source.root);
    REQUIRE(cloned != nullptr);
    REQUIRE(cloned->children.size() == 2);
    REQUIRE(cloned->children[0] != source.root->children[0]);
    REQUIRE(cloned->children[0]->name == "hip");
    auto clonedMesh = std::dynamic_pointer_cast<SkinnedMesh>(cloned->children[1]);
    REQUIRE(clonedMesh != nullptr);
    REQUIRE(clonedMesh->skeleton != source.mesh->skeleton);
    REQUIRE(clonedMesh->skeleton->bones.size() == 3);
    for (std::size_t i = 0; i < 3; ++i) {
        REQUIRE(clonedMesh->skeleton->bones[i] != source.bones[i]);
        REQUIRE(clonedMesh->skeleton->bones[i]->name == source.bones[i]->name);
    }
    // bind() recomputes the inverse when the matrix is omitted, so the copied matrix and
    // its inverse stay consistent with the clone.
    REQUIRE(clonedMesh->bindMatrix.elements()[12] == 0.1);
    REQUIRE(clonedMesh->bindMatrix.elements()[14] == 0.2);
    REQUIRE(clonedMesh->skeleton->bones[2]->parent == clonedMesh->skeleton->bones[1].get());
    // The source hierarchy keeps its own bones.
    REQUIRE(source.mesh->skeleton->bones[0] == source.bones[0]);
    REQUIRE(cloned->children[0]->parent == cloned.get());
}

TEST_CASE("SkeletonUtils retarget writes the resolved defaults into options", "[skeletonutils]") {
    Rig source = rig();
    Rig target = rig();
    target.bones[0]->position.set(0.25, 0.9, 0.1);
    target.root->updateMatrixWorld(true);
    RetargetOptions options;
    REQUIRE_FALSE(options.preserveBoneMatrix.has_value());
    SkeletonUtils::retarget(*target.mesh, *source.mesh, options);
    REQUIRE(options.preserveBoneMatrix.value_or(false));
    REQUIRE(options.preserveBonePositions.value_or(false));
    REQUIRE_FALSE(options.useTargetMatrix.value_or(true));
    REQUIRE(options.hip.value() == "hip");
    REQUIRE(options.hipInfluence->x == 1.);
    REQUIRE(options.scale.value() == 1.);
    // preserveBonePositions keeps the rest positions of every bone except the hip, which
    // keeps the retargeted transform (the source hip sits at ( 0, 1, 0 )).
    REQUIRE(close(target.bones[0]->position.x, 0.));
    REQUIRE(close(target.bones[0]->position.y, 1.));
    REQUIRE(close(target.bones[1]->position.x, 0.));
    REQUIRE(close(target.bones[1]->position.y, 0.5));
    REQUIRE(close(target.bones[2]->position.x, -0.4));
}

TEST_CASE("SkeletonUtils retarget applies hip scale, influence and position", "[skeletonutils]") {
    Rig source = rig();
    Rig target = rig();
    RetargetOptions options;
    options.hip = "spine";
    options.scale = 2.;
    options.hipInfluence = Vector3(1., 0.5, 0.25);
    options.hipPosition = Vector3(0.1, 0., -0.1);
    options.preserveBonePositions = false;
    SkeletonUtils::retarget(*target.mesh, *source.mesh, options);
    // The hip bone keeps the rotated global matrix with the scaled position column.
    REQUIRE(std::isfinite(target.bones[1]->position.x));
    REQUIRE(std::isfinite(target.bones[1]->quaternion.w));
    // A skeleton target switches the options the way the upstream code does.
    Skeleton& targetSkeleton = *target.mesh->skeleton;
    Skeleton& sourceSkeleton = *source.mesh->skeleton;
    RetargetOptions skeletonOptions;
    SkeletonUtils::retarget(targetSkeleton, sourceSkeleton, skeletonOptions);
    REQUIRE(skeletonOptions.useTargetMatrix.value_or(false));
    REQUIRE_FALSE(skeletonOptions.preserveBoneMatrix.value_or(true));
}

TEST_CASE("SkeletonUtils retargetClip converts the sampled bones into tracks", "[skeletonutils]") {
    Rig source = rig();
    Rig target = rig();
    auto clip = waveClip();
    RetargetOptions options;
    auto converted = SkeletonUtils::retargetClip(*target.mesh, *source.mesh, clip, options);
    REQUIRE(converted != nullptr);
    REQUIRE(converted->name.string() == "wave");
    REQUIRE(options.fps.value() == 3.);
    REQUIRE(options.useFirstFramePosition.has_value());
    REQUIRE_FALSE(options.useFirstFramePosition.value());
    // One position track for the hip plus one quaternion track per bound bone.
    REQUIRE(converted->tracks->size() == 4);
    REQUIRE((*converted->tracks)[0]->name.string() == ".bones[hip].position");
    REQUIRE((*converted->tracks)[0]->times.size() == 3);
    REQUIRE((*converted->tracks)[0]->values.size() == 9);
    REQUIRE((*converted->tracks)[1]->name.string() == ".bones[hip].quaternion");
    REQUIRE((*converted->tracks)[1]->values.size() == 12);
    REQUIRE((*converted->tracks)[2]->name.string() == ".bones[spine].quaternion");
    REQUIRE((*converted->tracks)[3]->name.string() == ".bones[arm].quaternion");
    REQUIRE(converted->duration == 1.);
    // The Skeleton form wraps the same bones and produces the same track names.
    Rig skeletonTarget = rig();
    auto skeletonClip = SkeletonUtils::retargetClip(*skeletonTarget.mesh, *source.mesh->skeleton, waveClip());
    REQUIRE(skeletonClip->tracks->size() == converted->tracks->size());
    for (std::size_t i = 0; i < skeletonClip->tracks->size(); ++i)
        REQUIRE((*skeletonClip->tracks)[i]->name.string() == (*converted->tracks)[i]->name.string());
}
