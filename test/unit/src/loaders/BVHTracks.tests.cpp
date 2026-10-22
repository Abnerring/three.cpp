#include "loaders/BVHLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

namespace {
BVHLoader::BoneInfo makeBone(const std::string& name, double x, bool endSite = false) {
    BVHLoader::BoneInfo bone;
    bone.name = name;
    bone.position[0] = x;
    bone.isEndSite = endSite;
    return bone;
}

BVHLoader::Keyframe makeKeyframe(double time, double x, double y, double z) {
    BVHLoader::Keyframe frame;
    frame.time = time;
    frame.position[0] = x;
    frame.position[1] = y;
    frame.position[2] = z;
    frame.rotation[0] = 0;
    frame.rotation[1] = 0;
    frame.rotation[2] = 0;
    frame.rotation[3] = 1;
    return frame;
}
}  // namespace

TEST_CASE("BVHLoader builds a position and quaternion track per bone") {
    const std::vector<BVHLoader::BoneInfo> bones{makeBone("Hips", 0)};
    const std::vector<std::vector<BVHLoader::Keyframe>> frames{
        {makeKeyframe(0, 1, 2, 3), makeKeyframe(0.5, 4, 5, 6)}};

    const auto clip = BVHLoader::buildAnimation(bones, frames);
    // Upstream names the clip animation and leaves the duration to be derived.
    REQUIRE(clip.name == "animation");
    REQUIRE(clip.duration == -1);
    REQUIRE(clip.tracks.size() == 2);
    REQUIRE(clip.tracks[0].name == "Hips.position");
    REQUIRE_FALSE(clip.tracks[0].isQuaternion);
    REQUIRE(clip.tracks[1].name == "Hips.quaternion");
    REQUIRE(clip.tracks[1].isQuaternion);
    // Two keys, three values each for the position, four for the quaternion.
    REQUIRE(clip.tracks[0].times.size() == 2);
    REQUIRE(clip.tracks[0].values.size() == 6);
    REQUIRE(clip.tracks[1].values.size() == 8);
}

TEST_CASE("BVHLoader adds the bone offset to every position value") {
    // The bone rests five units along X, and the animation drives position directly.
    const std::vector<BVHLoader::BoneInfo> bones{makeBone("Hips", 5)};
    const std::vector<std::vector<BVHLoader::Keyframe>> frames{
        {makeKeyframe(0, 1, 2, 3), makeKeyframe(0.5, 4, 5, 6)}};

    const auto clip = BVHLoader::buildAnimation(bones, frames);
    const auto& position = clip.tracks[0];
    REQUIRE(position.values[0] == 6.0);
    REQUIRE(position.values[1] == 2.0);
    REQUIRE(position.values[2] == 3.0);
    REQUIRE(position.values[3] == 9.0);
    REQUIRE(position.values[4] == 5.0);
    REQUIRE(position.values[5] == 6.0);
}

TEST_CASE("BVHLoader skips end sites and bones with no frames") {
    const std::vector<BVHLoader::BoneInfo> bones{makeBone("Root", 0), makeBone("ENDSITE", 1, true),
                                                 makeBone("Child", 2)};
    std::vector<std::vector<BVHLoader::Keyframe>> frames(3);
    frames[0] = {makeKeyframe(0, 0, 0, 0)};
    // The end site has no frames at all, and neither does the child.
    const auto clip = BVHLoader::buildAnimation(bones, frames);
    // Only the root contributes its two tracks.
    REQUIRE(clip.tracks.size() == 2);
    REQUIRE(clip.tracks[0].name == "Root.position");
}

TEST_CASE("BVHLoader honours the animate flags") {
    const std::vector<BVHLoader::BoneInfo> bones{makeBone("Hips", 0)};
    const std::vector<std::vector<BVHLoader::Keyframe>> frames{{makeKeyframe(0, 0, 0, 0)}};
    // With positions off only the quaternion track appears.
    const auto rotationsOnly = BVHLoader::buildAnimation(bones, frames, false, true);
    REQUIRE(rotationsOnly.tracks.size() == 1);
    REQUIRE(rotationsOnly.tracks[0].isQuaternion);

    const auto positionsOnly = BVHLoader::buildAnimation(bones, frames, true, false);
    REQUIRE(positionsOnly.tracks.size() == 1);
    REQUIRE_FALSE(positionsOnly.tracks[0].isQuaternion);
}
