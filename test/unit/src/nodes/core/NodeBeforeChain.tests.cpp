#include "nodes/core/NodeBeforeChain.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

using StageAnswer = NodeBeforeChain::StageAnswer;

TEST_CASE("Node's before list starts absent and grows by pushing") {
    // The list is absent until something is added, so a node with nothing before it carries
    // nothing rather than an empty list.
    REQUIRE(NodeBeforeChain::listStartsAbsent());
    // Adding pushes, so the order is the order of the calls.
    const auto one = NodeBeforeChain::afterAdding({}, 7);
    REQUIRE(one.size() == 1);
    REQUIRE(one[0] == 7);
    const auto two = NodeBeforeChain::afterAdding(one, 9);
    REQUIRE(two.size() == 2);
    REQUIRE(two[0] == 7);
    REQUIRE(two[1] == 9);
    REQUIRE(two[1] != two[0]);
}

TEST_CASE("Node's before list does not de-duplicate") {
    // The same node added twice appears twice, so a caller that added one twice would run it
    // twice; nothing compares the entries.
    auto chain = NodeBeforeChain::afterAdding({}, 7);
    chain = NodeBeforeChain::afterAdding(chain, 7);
    REQUIRE(chain.size() == 2);
    REQUIRE(chain[0] == chain[1]);
    // And adding answers the node itself, so calls can be chained.
    REQUIRE(NodeBeforeChain::returnsTheNode());
}

TEST_CASE("Node's three build stages answer three different things") {
    // The first prepares and answers the node or a variant of it.
    REQUIRE(NodeBeforeChain::answerFor("setup") == StageAnswer::Node);
    // The second only analyses and answers nothing.
    REQUIRE(NodeBeforeChain::answerFor("analyze") == StageAnswer::Nothing);
    // The third produces text.
    REQUIRE(NodeBeforeChain::answerFor("generate") == StageAnswer::Text);
    // The three differ, so one stage's answer cannot be used as another's.
    REQUIRE(NodeBeforeChain::answerFor("setup") != NodeBeforeChain::answerFor("analyze"));
    REQUIRE(NodeBeforeChain::answerFor("analyze") != NodeBeforeChain::answerFor("generate"));
    REQUIRE(NodeBeforeChain::answerFor("setup") != NodeBeforeChain::answerFor("generate"));
}

TEST_CASE("Node's middle stage answers nothing rather than an empty text") {
    // The two are different things: nothing is the absence of an answer, while an empty text is a
    // text of no length.
    REQUIRE(NodeBeforeChain::middleStageAnswersNothing());
    // The stages run in the order they are listed, so a node needing the analysis before
    // generating gets it.
    REQUIRE(NodeBeforeChain::stagesRunInOrder());
}