#include "nodes/core/FunctionOverloadingNode.h"

#include "catch.hpp"

#include <string>
#include <vector>

using Candidate = FunctionOverloadingNode::Candidate;

TEST_CASE("FunctionOverloadingNode ignores a candidate of the wrong length") {
    // A candidate whose parameter count differs is not scored at all, so a definition of the
    // right shape but the wrong length is passed over rather than ranked low.
    const std::vector<Candidate> candidates{{{"float", "float", "float"}}, {{"float"}}};
    REQUIRE(FunctionOverloadingNode::isConsidered(candidates[0], 1) == false);
    REQUIRE(FunctionOverloadingNode::isConsidered(candidates[1], 1));
    // With one argument, only the single parameter candidate can be chosen.
    const int chosen = FunctionOverloadingNode::choose(candidates, {"int"});
    REQUIRE(chosen == 1);
    REQUIRE(chosen != 0);
}

TEST_CASE("FunctionOverloadingNode chooses the best match") {
    // Two candidates of the same length: the one matching more types wins.
    const std::vector<Candidate> candidates{{{"float", "vec3"}}, {{"float", "float"}}};
    REQUIRE(FunctionOverloadingNode::choose(candidates, {"float", "vec3"}) == 0);
    REQUIRE(FunctionOverloadingNode::choose(candidates, {"float", "float"}) == 1);
    // The score is the number of matching types, which a test checks directly.
    REQUIRE(FunctionOverloadingNode::score(candidates[0], {"float", "vec3"}) == 2);
    REQUIRE(FunctionOverloadingNode::score(candidates[0], {"float", "float"}) == 1);
    REQUIRE(FunctionOverloadingNode::score(candidates[0], {"int", "int"}) == 0);
}

TEST_CASE("FunctionOverloadingNode chooses a candidate that matches nothing") {
    // The search starts at minus one, so a candidate scoring nought still beats the start and
    // is chosen; starting at nought would have left nothing chosen and the setup would fail.
    REQUIRE(FunctionOverloadingNode::kStartingScore == -1);
    const std::vector<Candidate> candidates{{{"vec3"}}};
    REQUIRE(FunctionOverloadingNode::choose(candidates, {"float"}) == 0);
    // And the score really is nought, which is what makes the choice worth noting.
    REQUIRE(FunctionOverloadingNode::score(candidates[0], {"float"}) == 0);
}

TEST_CASE("FunctionOverloadingNode keeps the earlier of two equal matches") {
    // The comparison is strictly greater, so a later candidate with the same score does not
    // displace the earlier one. The convert node keeps the last of its matches, so the two
    // modules answer the same question in opposite ways.
    const std::vector<Candidate> candidates{{{"float"}}, {{"float"}}};
    REQUIRE(FunctionOverloadingNode::choose(candidates, {"float"}) == 0);
    REQUIRE(FunctionOverloadingNode::choose(candidates, {"float"}) != 1);
    // Two candidates each matching nothing also keep the first.
    const std::vector<Candidate> none{{{"vec3"}}, {{"mat4"}}};
    REQUIRE(FunctionOverloadingNode::choose(none, {"float"}) == 0);
    REQUIRE(std::string(FunctionOverloadingNode::kTypeName) == "FunctionOverloadingNode");
}

TEST_CASE("FunctionOverloadingNode answers with nothing when no candidate fits") {
    // With no candidate of the right length, nothing is chosen at all.
    const std::vector<Candidate> candidates{{{"float", "float"}}};
    REQUIRE(FunctionOverloadingNode::choose(candidates, {"float"}) == -1);
    REQUIRE(FunctionOverloadingNode::choose({}, {"float"}) == -1);
    // And the choice is remembered, so the search is not run again.
    REQUIRE(FunctionOverloadingNode::remembersChoice());
}