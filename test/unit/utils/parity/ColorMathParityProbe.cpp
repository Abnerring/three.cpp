// Differential probe for the native Color gamma conversions and the MathUtils helpers that
// the pinned modules implement with Math.pow/exp/log/log2.
//
// Protocol (one JSON object per line out):
//   G <kind> <r> <g> <b>           Color sRGB conversion; kind 0 copySRGBToLinear,
//                                  1 copyLinearToSRGB, 2 convertSRGBToLinear,
//                                  3 convertLinearToSRGB
//   D <x> <y> <lambda> <dt>        MathUtils.damp
//   P <kind> <value>               MathUtils 0 isPowerOfTwo, 1 floorPowerOfTwo, 2 ceilPowerOfTwo
#include "core/JsonValue.h"
#include "math/Color.h"
#include "math/MathUtils.h"
#include <charconv>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace {
JsonValue number(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value < 0 ? "-Infinity" : "Infinity";
    if (value == 0 && std::signbit(value)) return "-0";
    return value;
}
double parse(const std::string& token) {
    if (token == "NaN") return std::numeric_limits<double>::quiet_NaN();
    if (token == "Infinity") return std::numeric_limits<double>::infinity();
    if (token == "-Infinity") return -std::numeric_limits<double>::infinity();
    if (token == "-0") return -0.;
    double value = 0;
    std::from_chars(token.data(), token.data() + token.size(), value);
    return value;
}
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream stream(line);
        std::string kind, token;
        stream >> kind;
        try {
            JsonValue output = JsonValue::Object{};
            if (kind == "G") {
                double which = 0;
                stream >> which;
                double r = 0, g = 0, b = 0;
                stream >> token;
                r = parse(token);
                stream >> token;
                g = parse(token);
                stream >> token;
                b = parse(token);
                const Color source(r, g, b);
                Color target;
                if (which == 0) target.copySRGBToLinear(source);
                else if (which == 1) target.copyLinearToSRGB(source);
                else if (which == 2) target.copy(source).convertSRGBToLinear();
                else target.copy(source).convertLinearToSRGB();
                output["value"] = JsonValue::Array{number(target.r), number(target.g), number(target.b)};
            } else if (kind == "D") {
                // Tokens go through parse() so "NaN", "Infinity" and "-0" survive; the MSVC
                // stream extractor does not accept those spellings and would leave zeros.
                double values[4] = {0, 0, 0, 0};
                for (auto& value : values) {
                    stream >> token;
                    value = parse(token);
                }
                output["value"] = number(MathUtils::damp(values[0], values[1], values[2], values[3]));
            } else if (kind == "P") {
                double which = 0, value = 0;
                stream >> which >> token;
                value = parse(token);
                if (which == 0) output["value"] = MathUtils::isPowerOfTwo(value);
                else if (which == 1) output["value"] = number(MathUtils::floorPowerOfTwo(value));
                else output["value"] = number(MathUtils::ceilPowerOfTwo(value));
            } else {
                throw std::runtime_error("unknown scenario kind: " + kind);
            }
            std::cout << output.dump() << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}
