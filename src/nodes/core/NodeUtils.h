#pragma once
#include "core/JSValue.h"
#include <cstdint>
#include <span>
#include <string>

// The hashing part of src/nodes/core/NodeUtils.js (cyrb53), ported bit exactly.
//
// Upstream relies on JavaScript's ToInt32 and Math.imul for wrap-around 32 bit arithmetic,
// which maps to unsigned 32 bit arithmetic plus a signed reinterpretation here.
std::int32_t nodeImul(std::int32_t a, std::int32_t b);

// Hashes an array of numbers. Upstream applies JavaScript's number to int32 conversion to
// each element before mixing it in.
double hashArray(std::span<const double> values);
double hashString(const std::string& value);
// Hashes the parameters in order, equivalent to hashArray over them.
double hash(std::span<const double> params);

// Realm-local native port of the upstream NodeUtils module. Exported functions
// retain their lexical collections and use live runtime property access.
JSValue nativeNodeUtils();

// An explicit boundary for canonical math constructors that are not available
// in the native runtime. Unsupported inputs are never silently coerced.
class NodeUtilsUnsupported : public std::logic_error {
  public:
    using std::logic_error::logic_error;
};
