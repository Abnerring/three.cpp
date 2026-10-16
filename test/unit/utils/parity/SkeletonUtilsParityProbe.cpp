// Differential probe for the pinned examples/jsm/utils/SkeletonUtils.js port.
//
// The probe builds the same rig as compare-skeleton-utils.mjs (identical literal bone
// tables), runs the requested operation and prints one JSON object. Command line:
//
//     SkeletonUtilsParityProbe clone|retarget|retarget-clip
//
// `clone` reports the cloned hierarchy, the rebinding of every SkinnedMesh and whether the
// cloned bones are distinct objects; `retarget` reports the target bone transforms after
// the call plus the option fields retarget() fills in; `retarget-clip` additionally samples
// the produced clip. A mismatch in the shared literals fails the comparison instead of
// passing silently.
#include "core/JsonValue.h"
#include "core/Object3D.h"
#include "core/TypedArray.h"
#include "animation/tracks/QuaternionKeyframeTrack.h"
#include "animation/tracks/VectorKeyframeTrack.h"
#include "math/Quaternion.h"
#include "math/Vector3.h"
#include "objects/Bone.h"
#include "objects/Skeleton.h"
#include "objects/SkinnedMesh.h"
#include "utils/SkeletonUtils.h"
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {
JsonValue number(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value < 0 ? "-Infinity" : "Infinity";
    if (value == 0 && std::signbit(value)) return "-0";
    return value;
}
JsonValue vectorJSON(const Vector3& value) { return JsonValue::Array{number(value.x), number(value.y), number(value.z)}; }
JsonValue quaternionJSON(const Quaternion& value) {
    return JsonValue::Array{number(value.x), number(value.y), number(value.z), number(value.w)};
}
double degrees(double value) { return value * 3.141592653589793 / 180; }

struct BoneSpec {
    const char* name;
    double x, y, z;
    double axisX, axisY, axisZ, degrees;
    int parent;
};
// Shared literal rig: 0 hip, 1 spine (child of hip), 2 arm (child of spine).
const BoneSpec sourceSpec[] = {
    {"hip", 0., 1., 0., 0., 0., 1., 0., -1},
    {"spine", 0., 0.5, 0., 0., 0., 1., 30., 0},
    {"arm", -0.4, 0.1, 0., 1., 0., 0., 15., 1}};
const BoneSpec targetSpec[] = {
    {"hip", 0.25, 0.9, 0.1, 0., 1., 0., 10., -1},
    {"spine", 0., 0.4, 0., 0., 1., 0., 20., 0},
    {"arm", -0.3, 0.2, 0.05, 1., 0., 0., 25., 1}};

struct Rig {
    std::shared_ptr<Object3D> root;
    std::shared_ptr<SkinnedMesh> mesh;
    Skeleton::Bones bones;
    std::vector<std::shared_ptr<Bone>> list;
};

Rig buildRig(const BoneSpec* spec, std::size_t count, const char* meshName) {
    Rig rig;
    rig.root = std::make_shared<Object3D>();
    rig.root->name = meshName;
    for (std::size_t i = 0; i < count; ++i) {
        auto bone = std::make_shared<Bone>();
        bone->name = spec[i].name;
        bone->position.set(spec[i].x, spec[i].y, spec[i].z);
        bone->quaternion.setFromAxisAngle(Vector3(spec[i].axisX, spec[i].axisY, spec[i].axisZ), degrees(spec[i].degrees));
        if (spec[i].parent >= 0) rig.list[std::size_t(spec[i].parent)]->add(bone);
        else rig.root->add(bone);
        rig.list.push_back(bone);
        rig.bones.push_back(bone);
    }
    rig.root->updateMatrixWorld(true);
    rig.mesh = std::make_shared<SkinnedMesh>();
    rig.mesh->name = std::string(meshName) + "Mesh";
    rig.mesh->skeleton = std::make_shared<Skeleton>(rig.bones);
    rig.mesh->bindMatrix.makeTranslation(0.1, 0., 0.2);
    rig.mesh->bindMatrixInverse.copy(rig.mesh->bindMatrix).invert();
    rig.root->add(rig.mesh);
    rig.root->updateMatrixWorld(true);
    return rig;
}

std::shared_ptr<AnimationClip> buildClip() {
    auto tracks = std::make_shared<AnimationClip::Tracks>();
    const Float32Array times(std::vector<double>{0., 0.5, 1.});
    tracks->push_back(std::make_shared<VectorKeyframeTrack>(JSValue(".bones[hip].position"), AnimationArray(times),
        AnimationArray(Float32Array(std::vector<double>{0., 1., 0., 0.1, 1.2, 0.2, 0.2, 1.4, 0.4}))));
    tracks->push_back(std::make_shared<QuaternionKeyframeTrack>(JSValue(".bones[spine].quaternion"), AnimationArray(times),
        AnimationArray(Float32Array(std::vector<double>{0., 0., 0., 1., 0.1, 0., 0., 0.99498743710662,
            0.2, 0., 0., 0.97979589711327}))));
    return std::make_shared<AnimationClip>(JSValue("wave"), 1., tracks);
}

JsonValue boneJSON(const std::vector<std::shared_ptr<Bone>>& bones) {
    JsonValue::Array result;
    for (const auto& bone : bones)
        result.push_back(JsonValue::Object{{"name", bone->name}, {"position", vectorJSON(bone->position)},
            {"quaternion", quaternionJSON(bone->quaternion)}, {"scale", vectorJSON(bone->scale)}});
    return result;
}

JsonValue runClone() {
    Rig source = buildRig(sourceSpec, 3, "armature");
    auto cloned = SkeletonUtils::clone(source.root);
    JsonValue::Array hierarchy;
    cloned->traverse([&](Object3D& node) {
        hierarchy.push_back(JsonValue::Object{{"name", node.name}, {"type", node.type},
            {"children", double(node.children.size())}, {"chained", node.parent != nullptr}});
    });
    auto clonedMesh = std::dynamic_pointer_cast<SkinnedMesh>(cloned->children[1]);
    JsonValue::Array rebinding;
    bool distinctSkeleton = clonedMesh && clonedMesh->skeleton != source.mesh->skeleton;
    bool distinctBones = true;
    if (clonedMesh && clonedMesh->skeleton) {
        for (std::size_t i = 0; i < clonedMesh->skeleton->bones.size() && i < source.bones.size(); ++i) {
            const auto bone = clonedMesh->skeleton->bones[i];
            rebinding.push_back(JsonValue::Object{{"name", bone ? bone->name : std::string("undefined")},
                {"distinct", bone && bone != source.bones[i]}});
            if (bone && bone == source.bones[i]) distinctBones = false;
        }
    }
    return JsonValue::Object{{"hierarchy", std::move(hierarchy)}, {"rebinding", std::move(rebinding)},
        {"distinctSkeleton", distinctSkeleton}, {"distinctBones", distinctBones},
        {"bindMatrix", JsonValue::Array{[&] {
             JsonValue::Array values;
             for (const auto value : clonedMesh->bindMatrix.elements()) values.push_back(number(value));
             return values;
         }()}},
        {"sourceSkeletonUntouched", source.mesh->skeleton->bones[0] == source.bones[0]}};
}

JsonValue runRetarget(bool withClip) {
    Rig source = buildRig(sourceSpec, 3, "source");
    Rig target = buildRig(targetSpec, 3, "target");
    RetargetOptions options;
    JsonValue output = JsonValue::Object{};
    if (!withClip) {
        SkeletonUtils::retarget(*target.mesh, *source.mesh, options);
        output["bones"] = boneJSON(target.list);
        output["filled"] = JsonValue::Object{{"preserveBoneMatrix", *options.preserveBoneMatrix},
            {"preserveBonePositions", *options.preserveBonePositions}, {"useTargetMatrix", *options.useTargetMatrix},
            {"hip", *options.hip}, {"hipInfluence", vectorJSON(*options.hipInfluence)}, {"scale", *options.scale}};
        // A second run with an explicit hip and scale exercises the hip adjustments.
        Rig scaled = buildRig(targetSpec, 3, "scaled");
        RetargetOptions adjusted;
        adjusted.hip = "spine";
        adjusted.scale = 2.;
        adjusted.hipInfluence = Vector3(1., 0.5, 0.25);
        adjusted.hipPosition = Vector3(0.1, 0., -0.1);
        adjusted.preserveBonePositions = false;
        SkeletonUtils::retarget(*scaled.mesh, *source.mesh, adjusted);
        output["adjustedBones"] = boneJSON(scaled.list);
        output["adjustedFilled"] = JsonValue::Object{{"preserveBoneMatrix", *adjusted.preserveBoneMatrix},
            {"preserveBonePositions", *adjusted.preserveBonePositions}, {"useTargetMatrix", *adjusted.useTargetMatrix}};
        return output;
    }
    auto clip = buildClip();
    RetargetOptions clipOptions;
    auto converted = SkeletonUtils::retargetClip(*target.mesh, *source.mesh, clip, clipOptions);
    JsonValue::Array tracks;
    for (const auto& track : *converted->tracks) {
        JsonValue::Array times, values;
        for (std::size_t i = 0, length = track->times.size(); i < length; ++i)
            times.push_back(number(track->times.number(double(i))));
        for (std::size_t i = 0, length = track->values.size(); i < length; ++i)
            values.push_back(number(track->values.number(double(i))));
        tracks.push_back(JsonValue::Object{{"name", track->name.toString()}, {"times", std::move(times)},
            {"values", std::move(values)}});
    }
    output["clip"] = JsonValue::Object{{"name", converted->name.toString()}, {"duration", number(converted->duration)},
        {"tracks", std::move(tracks)}};
    output["fps"] = number(*clipOptions.fps);
    output["bones"] = boneJSON(target.list);
    // The Skeleton form wraps the same bones and must produce the same clip.
    Rig skeletonTarget = buildRig(targetSpec, 3, "skeletonTarget");
    auto skeletonClip = SkeletonUtils::retargetClip(*skeletonTarget.mesh, *source.mesh->skeleton, buildClip());
    JsonValue::Array skeletonNames;
    for (const auto& track : *skeletonClip->tracks) skeletonNames.push_back(track->name.toString());
    output["skeletonFormTracks"] = std::move(skeletonNames);
    output["skeletonFormDuration"] = number(skeletonClip->duration);
    return output;
}
}

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "clone";
    try {
        if (mode == "clone") std::cout << runClone().dump() << '\n';
        else if (mode == "retarget") std::cout << runRetarget(false).dump() << '\n';
        else if (mode == "retarget-clip") std::cout << runRetarget(true).dump() << '\n';
        else throw std::runtime_error("unknown mode: " + mode);
    } catch (const std::exception& error) {
        std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        return 1;
    }
    return 0;
}
