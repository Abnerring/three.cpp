// Differential probe for the pinned examples/jsm/utils/SortUtils.js radixSort port.
//
// Protocol: one scenario per stdin line, tokens separated by spaces.
//
//     <count> <getMode> <reversed> <auxMode> <key0> ... <keyN-1> [<aux0> ... <auxN-1>]
//
// Numeric tokens use the encoding produced by the JavaScript side: "NaN", "Infinity",
// "-Infinity" and "-0" are written literally. The probe prints one JSON object per line
// with the array contents after sorting, the caller-provided aux contents (or null when
// the module allocated its own scratch array) and the resulting key order used by the
// comparator, so the comparison covers placement, not only the sort result.
#include "core/JsonValue.h"
#include "utils/SortUtils.h"
#include <charconv>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

namespace {
double parse(const std::string& token) {
    if (token == "NaN") return std::numeric_limits<double>::quiet_NaN();
    if (token == "Infinity") return std::numeric_limits<double>::infinity();
    if (token == "-Infinity") return -std::numeric_limits<double>::infinity();
    if (token == "-0") return -0.;
    double value = 0;
    std::from_chars(token.data(), token.data() + token.size(), value);
    return value;
}
JsonValue encode(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value < 0 ? "-Infinity" : "Infinity";
    if (value == 0 && std::signbit(value)) return "-0";
    return value;
}
JsonValue encode(const std::vector<double>& values) {
    JsonValue::Array result;
    for (const auto value : values) result.push_back(encode(value));
    return result;
}
// The getter modes mirror the arrow functions used by compare-sort-utils.mjs.
double get(int mode, double value) {
    switch (mode) {
        case 0: return value;
        case 1: return value * 3 + 7;
        case 2: return std::fmod(std::floor(value), 3.);
        case 3: return value / 7;
        case 4: return value * 4294967296.;
        case 5: {
            // ~value: ToInt32 first, then the bitwise complement.
            double wrapped = 0;
            if (std::isfinite(value) && value != 0) {
                constexpr double modulus = 4294967296.;
                wrapped = std::fmod(std::trunc(value), modulus);
                if (wrapped < 0) wrapped += modulus;
                if (wrapped >= modulus / 2) wrapped -= modulus;
            }
            const auto complement = ~std::int32_t(wrapped);
            return double(complement);
        }
        default: return value * 1e20;
    }
}
struct Getter {
    int mode;
    double operator()(double value) const { return get(mode, value); }
};
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream stream(line);
        std::size_t count = 0;
        int getMode = 0, reversed = 0, auxMode = 0;
        stream >> count >> getMode >> reversed >> auxMode;
        std::vector<double> values(count), auxValues(count);
        std::string token;
        for (std::size_t i = 0; i < count; ++i) {
            stream >> token;
            values[i] = parse(token);
        }
        for (std::size_t i = 0; i < count && auxMode == 1; ++i) {
            stream >> token;
            auxValues[i] = parse(token);
        }
        SortUtils::RadixSortOptions<double> options;
        options.reversed = reversed != 0;
        if (auxMode == 1) options.aux = &auxValues;
        SortUtils::radixSort(values, options, Getter{getMode});
        JsonValue output = JsonValue::Object{{"arr", encode(values)},
            {"aux", auxMode == 1 ? encode(auxValues) : JsonValue{}},
            {"length", double(values.size())}};
        std::cout << output.dump() << '\n';
    }
    return 0;
}
