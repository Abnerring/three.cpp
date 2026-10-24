#include "nodes/core/NodeBaseFields.h"

#include "catch.hpp"

#include <string>

TEST_CASE("Node's name is the empty string rather than an absent one") {
    // The base class uses the empty string, while three of the classes below it use an absent
    // name, so a caller inspecting an arbitrary node's name cannot assume either one.
    REQUIRE(std::string(NodeBaseFields::kNameDefault) == "");
    REQUIRE(NodeBaseFields::kNameDefault != nullptr);
    REQUIRE(std::string(NodeBaseFields::kNameDefault).empty());
    REQUIRE(NodeBaseFields::isNode);
    REQUIRE(std::string(NodeBaseFields::kTypeName) == "Node");
}

TEST_CASE("Node's base class is quieter than the classes below it") {
    // Both flags are off here, and the classes that derive from it turn them on where needed: an
    // attribute is global, a variable is global and asks for its parents too.
    REQUIRE_FALSE(NodeBaseFields::kGlobalDefault);
    REQUIRE_FALSE(NodeBaseFields::kParentsDefault);
    // So the base class defaults are not the typical ones, which a reader comparing a derived
    // class against this one would need to know.
    REQUIRE_FALSE(NodeBaseFields::kGlobalDefault);
    REQUIRE(NodeBaseFields::kCacheableByDefault);
}

TEST_CASE("Node's update kinds are the word for none") {
    // A fresh node declares the same word in all three slots, so it carries three values that a
    // truth test would pass rather than three falsy ones.
    REQUIRE(std::string(NodeBaseFields::kUpdateTypeDefault) == "none");
    REQUIRE(std::string(NodeBaseFields::kUpdateTypeDefault) != "");
    REQUIRE_FALSE(std::string(NodeBaseFields::kUpdateTypeDefault).empty());
    REQUIRE(NodeBaseFields::kVersionDefault == 0);
}

TEST_CASE("Node's update flag raises the version only for an exact true") {
    // Exactly true raises it by one.
    REQUIRE(NodeBaseFields::versionAfterAssigning(0, true) == 1);
    REQUIRE(NodeBaseFields::versionAfterAssigning(7, true) == 8);
    // Anything else leaves it alone, so assigning false cannot lower a version.
    REQUIRE(NodeBaseFields::versionAfterAssigning(0, false) == 0);
    REQUIRE(NodeBaseFields::versionAfterAssigning(7, false) == 7);
    REQUIRE(NodeBaseFields::versionAfterAssigning(7, false) !=
            NodeBaseFields::versionAfterAssigning(7, true));
}

TEST_CASE("Node's identity comes from a counter and its trace from a class setting") {
    // The counter is raised for each node made, so two nodes never share an identifier and their
    // order tells which came first.
    REQUIRE(NodeBaseFields::identifierComesFromACounter());
    // The trace flag belongs to the class rather than to an instance, so turning it on records a
    // trace for every node rather than for the ones a caller names.
    REQUIRE(NodeBaseFields::traceIsAClassLevelSetting());
}