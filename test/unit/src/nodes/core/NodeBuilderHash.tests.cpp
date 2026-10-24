#include "nodes/core/NodeBuilderHash.h"

#include "catch.hpp"

#include <cstdint>
#include <optional>

TEST_CASE("NodeBuilder's hash record is a plain object") {
    // A plain object rather than a map, so the hash is a string key and the record carries
    // whatever such an object carries.
    REQUIRE(NodeBuilderHash::recordIsAPlainObject());
}

TEST_CASE("NodeBuilder lets a later node take a hash from an earlier one") {
    // There is no check for an entry already being there, so the later node replaces the earlier.
    const auto replaced = NodeBuilderHash::afterSetting(std::uint64_t(1), 2);
    REQUIRE(replaced.has_value());
    REQUIRE(replaced.value() == 2);
    REQUIRE(replaced.value() != 1);
    // Setting into an empty record gives the new node, as does setting over one.
    const auto first = NodeBuilderHash::afterSetting(std::nullopt, 3);
    REQUIRE(first.value() == 3);
    // So the record holds one node per hash and cannot show that another ever had it.
    REQUIRE(NodeBuilderHash::afterSetting(std::nullopt, 3).value() ==
            NodeBuilderHash::afterSetting(std::uint64_t(1), 3).value());
}

TEST_CASE("NodeBuilder's absent hash answers nothing") {
    // Nothing at all rather than a default node.
    REQUIRE(NodeBuilderHash::absentAnswersNothing(false));
    REQUIRE_FALSE(NodeBuilderHash::absentAnswersNothing(true));
    REQUIRE(NodeBuilderHash::absentAnswersNothing(false) !=
            NodeBuilderHash::absentAnswersNothing(true));
}

TEST_CASE("NodeBuilder works a node's hash out when the node is added") {
    // Once, at add time, so a node whose answer would change later keeps the hash it had then and
    // the lookup itself is a plain read.
    REQUIRE(NodeBuilderHash::hashIsComputedAtAddTime());
    // And adding the same node twice changes nothing, since the set refuses the second copy and
    // the hash is not recomputed.
    REQUIRE(NodeBuilderHash::addingTheSameNodeTwiceChangesNothing(true));
    REQUIRE_FALSE(NodeBuilderHash::addingTheSameNodeTwiceChangesNothing(false));
}