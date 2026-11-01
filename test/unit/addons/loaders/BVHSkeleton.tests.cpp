#include "loaders/BVHLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

namespace {
const char* kHierarchy =
    "HIERARCHY\n"
    "ROOT Hips\n"
    "{\n"
    "  OFFSET 0.00 1.00 2.00\n"
    "  CHANNELS 6 Xposition Yposition Zposition Zrotation Xrotation Yrotation\n"
    "  JOINT LHipJoint\n"
    "  {\n"
    "    OFFSET 5.00 0.00 0.00\n"
    "    CHANNELS 3 Zrotation Xrotation Yrotation\n"
    "    End Site\n"
    "    {\n"
    "      OFFSET 10.00 0.00 0.00\n"
    "    }\n"
    "  }\n"
    "}\n";
}

TEST_CASE("BVHLoader turns the hierarchy into a bone list") {
    const auto lines = BVHLoader::splitLines(kHierarchy);
    std::size_t cursor = 0;
    const auto hierarchy = BVHLoader::parseHierarchy(lines, cursor);
    const auto bones = BVHLoader::buildSkeleton(hierarchy);
    REQUIRE(bones.size() == 3);
    // Parents come before children, and the root has no parent.
    REQUIRE(bones[0].name == "Hips");
    REQUIRE(bones[0].parent == -1);
    REQUIRE(bones[0].position[0] == 0.0);
    REQUIRE(bones[0].position[1] == 1.0);
    REQUIRE(bones[0].position[2] == 2.0);
}

TEST_CASE("BVHLoader links each bone to its parent index") {
    const auto lines = BVHLoader::splitLines(kHierarchy);
    std::size_t cursor = 0;
    const auto hierarchy = BVHLoader::parseHierarchy(lines, cursor);
    const auto bones = BVHLoader::buildSkeleton(hierarchy);
    REQUIRE(bones[1].name == "LHipJoint");
    REQUIRE(bones[1].parent == 0);
    REQUIRE(bones[1].position[0] == 5.0);
    REQUIRE_FALSE(bones[1].isEndSite);

    // The end site is a bone too, parented to the joint.
    REQUIRE(bones[2].isEndSite);
    REQUIRE(bones[2].parent == 1);
    REQUIRE(bones[2].position[0] == 10.0);
    REQUIRE(bones[2].name == "ENDSITE");
}

TEST_CASE("BVHLoader handles an empty hierarchy") {
    BVHLoader::Hierarchy empty;
    REQUIRE(BVHLoader::buildSkeleton(empty).empty());
}

TEST_CASE("BVHLoader keeps several children in document order") {
    const char* text =
        "HIERARCHY\n"
        "ROOT Root\n"
        "{\n"
        "  OFFSET 0 0 0\n"
        "  CHANNELS 3 Xposition Yposition Zposition\n"
        "  JOINT A\n"
        "  {\n"
        "    OFFSET 1 0 0\n"
        "    CHANNELS 3 Xposition Yposition Zposition\n"
        "  }\n"
        "  JOINT B\n"
        "  {\n"
        "    OFFSET 2 0 0\n"
        "    CHANNELS 3 Xposition Yposition Zposition\n"
        "  }\n"
        "}\n";
    const auto lines = BVHLoader::splitLines(text);
    std::size_t cursor = 0;
    const auto hierarchy = BVHLoader::parseHierarchy(lines, cursor);
    const auto bones = BVHLoader::buildSkeleton(hierarchy);
    REQUIRE(bones.size() == 3);
    // Both children hang off the root, and A comes before B.
    REQUIRE(bones[1].name == "A");
    REQUIRE(bones[1].parent == 0);
    REQUIRE(bones[2].name == "B");
    REQUIRE(bones[2].parent == 0);
}
