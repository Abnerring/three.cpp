#include "loaders/VOXLoader.h"

#include "catch.hpp"

#include <map>
#include <string>
#include <vector>

TEST_CASE("VOXLoader reads a transform node") {
    const std::map<std::string, std::string> dict{
        {"node_id", "3"}, {"_name", "body"}, {"_t", "0 1 2"}, {"_r", "33"}};
    const auto transform = VOXLoader::parseTransform(dict);
    REQUIRE(transform.nodeId == 3);
    REQUIRE(transform.name == "body");
    REQUIRE(transform.hasTranslation);
    REQUIRE(transform.translation[0] == 0.0);
    REQUIRE(transform.translation[1] == 1.0);
    REQUIRE(transform.translation[2] == 2.0);
    REQUIRE(transform.hasRotation);
    REQUIRE(transform.rotation == 33);
}

TEST_CASE("VOXLoader leaves a transform without optional fields alone") {
    const std::map<std::string, std::string> dict{{"node_id", "0"}};
    const auto transform = VOXLoader::parseTransform(dict);
    REQUIRE(transform.nodeId == 0);
    REQUIRE(transform.name.empty());
    // Without the keys nothing is flagged as present, so no matrix is applied later.
    REQUIRE_FALSE(transform.hasTranslation);
    REQUIRE_FALSE(transform.hasRotation);
}

TEST_CASE("VOXLoader rejects a translation with fewer than three numbers") {
    const std::map<std::string, std::string> dict{{"_t", "1 2"}};
    const auto transform = VOXLoader::parseTransform(dict);
    // A short translation is treated as absent rather than as a partial offset.
    REQUIRE_FALSE(transform.hasTranslation);
}

TEST_CASE("VOXLoader reads a child list") {
    const std::map<std::string, std::string> dict{{"_c", "4 5 6"}};
    const auto children = VOXLoader::parseChildren(dict);
    REQUIRE(children.size() == 3);
    REQUIRE(children[0] == 4);
    REQUIRE(children[2] == 6);
}

TEST_CASE("VOXLoader handles a missing child list") {
    const std::map<std::string, std::string> dict{{"_name", "x"}};
    REQUIRE(VOXLoader::parseChildren(dict).empty());
}