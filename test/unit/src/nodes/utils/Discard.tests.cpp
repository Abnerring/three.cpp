#include "nodes/utils/Discard.h"

#include "catch.hpp"

#include <string>

using Form = Discard::Form;

TEST_CASE("Discard takes the conditional form only for a truthy condition") {
    // A condition that is there and true is the conditional form.
    REQUIRE(Discard::form(true, true) == Form::Conditional);
    // One that is there but falsy is not, which is the part that does not match intuition.
    REQUIRE(Discard::form(true, false) == Form::Always);
    // And one that is missing is the same as a falsy one.
    REQUIRE(Discard::form(false, false) == Form::Always);
    // The last two agree, which is what a truth test rather than a presence test means.
    REQUIRE(Discard::form(true, false) == Discard::form(false, false));
    REQUIRE(Discard::form(false, true) == Discard::form(false, false));
}

TEST_CASE("Discard's unconditional form always discards") {
    // A discard with no condition runs whatever the fragment is, so an absent condition does not
    // mean nothing happens; that is the mistake the truth test could lead a reader to make.
    REQUIRE(Discard::form(false, false) == Form::Always);
    REQUIRE(Discard::form(false, false) != Form::Conditional);
    // The word is the same in both forms, since the condition is wrapped around it.
    REQUIRE(Discard::expression() == "discard");
    REQUIRE(Discard::alwaysExpression() == "discard");
    REQUIRE(Discard::alwaysExpression() == Discard::expression());
}

TEST_CASE("Discard's return takes no condition") {
    // The return statement is always the same, and there is no conditional form of it.
    REQUIRE(Discard::returnExpression() == "return");
    REQUIRE(Discard::returnExpression() != Discard::expression());
    // The leading character differs, which is what makes it a different statement rather than
    // another spelling of the same one.
    REQUIRE(Discard::returnExpression()[0] == 'r');
    REQUIRE(Discard::expression()[0] == 'd');
}
