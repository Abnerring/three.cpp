#include "loaders/BVHLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

namespace {
const char* kHierarchy =
    "HIERARCHY\n"
    "ROOT Hips\n"
    "{\n"
    "  OFFSET 0.00 0.00 0.00\n"
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

TEST_CASE("BVHLoader splits lines and strips carriage returns") {
    const auto lines = BVHLoader::splitLines("a\r\nb\nc");
    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0] == "a");
    REQUIRE(lines[1] == "b");
    REQUIRE(lines[2] == "c");
}

TEST_CASE("BVHLoader reads the joint tree") {
    const auto lines = BVHLoader::splitLines(kHierarchy);
    std::size_t cursor = 0;
    const auto hierarchy = BVHLoader::parseHierarchy(lines, cursor);
    REQUIRE(hierarchy.root != nullptr);
    REQUIRE(hierarchy.root->name == "Hips");
    REQUIRE(hierarchy.root->type == "ROOT");
    REQUIRE(hierarchy.root->channels.size() == 6);
    REQUIRE(hierarchy.root->channels[0] == "Xposition");
    REQUIRE(hierarchy.root->channels[5] == "Yrotation");
    REQUIRE(hierarchy.root->children.size() == 1);

    const auto joint = hierarchy.root->children[0];
    REQUIRE(joint->name == "LHipJoint");
    REQUIRE(joint->type == "JOINT");
    REQUIRE(joint->offset[0] == 5.0);
    REQUIRE(joint->channels.size() == 3);
    REQUIRE(joint->children.size() == 1);

    const auto site = joint->children[0];
    // An end site is named after its type and carries no channels.
    REQUIRE(site->type == "ENDSITE");
    REQUIRE(site->name == "ENDSITE");
    REQUIRE(site->channels.empty());
    REQUIRE(site->offset[0] == 10.0);
    REQUIRE(site->children.empty());
}

TEST_CASE("BVHLoader lists every node parents first") {
    const auto lines = BVHLoader::splitLines(kHierarchy);
    std::size_t cursor = 0;
    const auto hierarchy = BVHLoader::parseHierarchy(lines, cursor);
    REQUIRE(hierarchy.list.size() == 3);
    // Document order puts the root first, then its joint, then the end site.
    REQUIRE(hierarchy.list[0]->name == "Hips");
    REQUIRE(hierarchy.list[1]->name == "LHipJoint");
    REQUIRE(hierarchy.list[2]->type == "ENDSITE");
}

TEST_CASE("BVHLoader rejects a missing hierarchy keyword") {
    const auto lines = BVHLoader::splitLines("ROOT Hips\n{\n}\n");
    std::size_t cursor = 0;
    REQUIRE_THROWS_WITH(BVHLoader::parseHierarchy(lines, cursor), "THREE.BVHLoader: HIERARCHY expected.");
}

TEST_CASE("BVHLoader rejects a malformed offset") {
    const auto lines = BVHLoader::splitLines("HIERARCHY\nROOT Hips\n{\nOFFSET 0 0\n}\n");
    std::size_t cursor = 0;
    REQUIRE_THROWS_WITH(BVHLoader::parseHierarchy(lines, cursor),
                        "THREE.BVHLoader: Invalid number of values for OFFSET");
}
