#pragma once
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
