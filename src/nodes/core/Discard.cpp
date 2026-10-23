#include "nodes/core/Discard.h"

Discard::Form Discard::form(bool hasCondition, bool conditionIsTruthy) {
    // A truth test, not a test for presence, so a condition that is there but falsy takes the
    // same branch as one that is missing.
    if (hasCondition && conditionIsTruthy) return Form::Conditional;
    return Form::Always;
}

std::string Discard::expression() {
    return "discard";
}

std::string Discard::alwaysExpression() {
    // The same statement word for word, since the difference between the forms is the condition
    // wrapped around it rather than the statement itself.
    return "discard";
}

std::string Discard::returnExpression() {
    return "return";
}
