#include "nodes/core/NodeSetup.h"

#include "catch.hpp"

using Source = NodeSetup::OutputSource;

TEST_CASE("Node's reference is itself until something replaces it") {
    // A node that refers to nothing is its own reference, so a caller always has something.
    REQUIRE(NodeSetup::defaultReferenceIsTheNode());
    // Installing a callback replaces the method on that instance rather than recording a
    // callback, so the class's method is shadowed there and the replacement runs in its place.
    REQUIRE(NodeSetup::replacesTheMethodOnTheInstance());
}

TEST_CASE("Node's global answer is its flag") {
    // By default the flag is the answer; a class may compute one instead.
    REQUIRE(NodeSetup::globalAnswerIsTheFlag());
}

TEST_CASE("Node marks itself set up before the work runs") {
    // The mark goes on first, so a call that comes back round during its own setup finds it and
    // does not set the node up twice. Setting it afterwards would let the second call in and the
    // work would run twice.
    REQUIRE(NodeSetup::marksBeforeTheWork());
}

TEST_CASE("Node keeps an output node that was already there") {
    // What setting up answered wins when it answered something.
    REQUIRE(NodeSetup::outputSource(true, false) == Source::FromSetup);
    REQUIRE(NodeSetup::outputSource(true, true) == Source::FromSetup);
    // When it answered nothing the earlier one is kept, so a class whose setup answers nothing
    // does not clear it.
    REQUIRE(NodeSetup::outputSource(false, true) == Source::FromBefore);
    REQUIRE(NodeSetup::outputSource(false, true) != Source::FromSetup);
    // And with neither there is none.
    REQUIRE(NodeSetup::outputSource(false, false) == Source::None);
    // The three are distinct, which is why the choice is not a pair of booleans.
    REQUIRE(NodeSetup::outputSource(true, false) != NodeSetup::outputSource(false, true));
    REQUIRE(NodeSetup::outputSource(false, false) != NodeSetup::outputSource(false, true));
}

TEST_CASE("Node registers a parent only for an exact true") {
    // The flag has to be exactly true rather than merely a value.
    REQUIRE(NodeSetup::registersAsParent(true));
    REQUIRE_FALSE(NodeSetup::registersAsParent(false));
    REQUIRE(NodeSetup::registersAsParent(true) != NodeSetup::registersAsParent(false));
}