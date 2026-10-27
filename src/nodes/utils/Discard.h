#pragma once
#include <string>
#include "core/JSValue.h"
JSValue nativeDiscardModule();
JSValue nativeDiscardBinding(const std::string& name);

// Port of the two expressions in src/nodes/utils/Discard.js at the pinned revision. Both are
// shader statements rather than values, and one of them takes an optional condition whose
// absence does not mean what it might.
class Discard {
  public:
    // Which form a call produces. The condition decides it with a truth test, so a condition
    // that is present but falsy is treated exactly like one that is missing, and both give the
    // unconditional form: a discard with no condition always discards. A reader might expect the
    // opposite, that no condition means nothing is discarded.
    enum class Form { Conditional, Always };

    static Form form(bool hasCondition, bool conditionIsTruthy);

    // The text each form carries, which the conditional one wraps its condition around.
    static std::string expression();

    // The text of the always form, which is the same statement without a condition.
    static std::string alwaysExpression();

    // The return statement, which takes no condition at all.
    static std::string returnExpression();
};
