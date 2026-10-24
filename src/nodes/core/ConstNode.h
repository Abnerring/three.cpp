#pragma once
#include "core/JSValue.h"
#include <cmath>
#include <cstdint>
#include <string>

// Port of the parts of src/nodes/core/ConstNode.js at the pinned revision that decide what a
// constant's shader text looks like. A constant knows its own type and the type the output
// wants, and the rule it follows depends on whether both are numbers.
JSValue nativeConstNodeConstructor();
class ConstNode {
    JSValue object_;
  public:
    explicit ConstNode(const JSValue& value = {}, const JSValue& nodeType = nullptr);
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeConstNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    // Whether a type name is one of the numeric ones. The source tests the name against
    // /float|u?int/, so three names match and they match anywhere in the string rather than
    // only at its start.
    static bool isNumericType(const std::string& typeName);

    // Whether a constant can be written straight into the output without being converted. This
    // needs both the constant's own type and the output's to be numeric, so a numeric constant
    // going to a vector is converted rather than written directly.
    static bool writesDirectly(const std::string& typeName, const std::string& outputType);

    // The value to write for a numeric constant, after the declared type has had its say. An
    // integer loses its fraction by being cut towards zero. An unsigned integer does too, but a
    // negative one becomes zero first, so a negative value never reaches the cut at all.
    static double numericValue(const std::string& typeName, double value);

    // Whether the constant is weak, meaning an implicit number whose type may follow its
    // operands rather than being fixed. It is not weak unless a document says so, and the
    // document's word counts only when it is exactly true.
    static bool isWeak(bool hasValue, bool valueIsTrue);
};
