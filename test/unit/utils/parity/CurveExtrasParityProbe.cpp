// Differential probe for the pinned examples/jsm/curves/CurveExtras.js port and for the
// JSNumberMath functions those formulas rely on.
//
// Protocol: one scenario per stdin line, tokens separated by spaces.
//
//   W <x> <y>                                        Math.pow(x, y)
//   N <x>                                            Math.sin(x)
//   O <x>                                            Math.cos(x)
//   C <curveIndex> <hasScale> <scale> <tCount> <t...>  curve samples
//
// Curve indices are the export order of the pinned module: 0 GrannyKnot, 1 HeartCurve,
// 2 VivianiCurve, 3 KnotCurve, 4 HelixCurve, 5 TrefoilKnot, 6 TorusKnot, 7 CinquefoilKnot,
// 8 TrefoilPolynomialKnot, 9 FigureEightPolynomialKnot, 10 DecoratedTorusKnot4a,
// 11 DecoratedTorusKnot4b, 12 DecoratedTorusKnot5a, 13 DecoratedTorusKnot5c.
// A `C` line returns the sampled points, the tangents at the same parameters, the arc
// length for 8 and for the default 200 divisions, and the curve type string.
#include "core/JsonValue.h"
#include "curves/CurveExtras.h"
#include "math/JSNumberMath.h"
#include <charconv>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {
JsonValue number(double value) {
    if (std::isnan(value)) return "NaN";
    if (std::isinf(value)) return value < 0 ? "-Infinity" : "Infinity";
    if (value == 0 && std::signbit(value)) return "-0";
    return value;
}
JsonValue vectorJSON(const Vector3& value) { return JsonValue::Array{number(value.x), number(value.y), number(value.z)}; }
double parse(const std::string& token) {
    if (token == "NaN") return std::numeric_limits<double>::quiet_NaN();
    if (token == "Infinity") return std::numeric_limits<double>::infinity();
    if (token == "-Infinity") return -std::numeric_limits<double>::infinity();
    if (token == "-0") return -0.;
    double value = 0;
    std::from_chars(token.data(), token.data() + token.size(), value);
    return value;
}
std::unique_ptr<Curve<Vector3>> makeCurve(int index, bool hasScale, double scale) {
    switch (index) {
        case 0: return std::make_unique<GrannyKnot>();
        case 1: return std::make_unique<HeartCurve>(hasScale ? scale : 5.);
        case 2: return std::make_unique<VivianiCurve>(hasScale ? scale : 70.);
        case 3: return std::make_unique<KnotCurve>();
        case 4: return std::make_unique<HelixCurve>();
        case 5: return std::make_unique<TrefoilKnot>(hasScale ? scale : 10.);
        case 6: return std::make_unique<TorusKnot>(hasScale ? scale : 10.);
        case 7: return std::make_unique<CinquefoilKnot>(hasScale ? scale : 10.);
        case 8: return std::make_unique<TrefoilPolynomialKnot>(hasScale ? scale : 10.);
        case 9: return std::make_unique<FigureEightPolynomialKnot>(hasScale ? scale : 1.);
        case 10: return std::make_unique<DecoratedTorusKnot4a>(hasScale ? scale : 40.);
        case 11: return std::make_unique<DecoratedTorusKnot4b>(hasScale ? scale : 40.);
        case 12: return std::make_unique<DecoratedTorusKnot5a>(hasScale ? scale : 40.);
        case 13: return std::make_unique<DecoratedTorusKnot5c>(hasScale ? scale : 40.);
        default: throw std::runtime_error("unknown curve index");
    }
}
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream stream(line);
        std::string kind, token;
        stream >> kind;
        try {
            if (kind == "W" || kind == "N" || kind == "O") {
                stream >> token;
                const double x = parse(token);
                double result = 0;
                if (kind == "W") {
                    stream >> token;
                    result = JSNumberMath::pow(x, parse(token));
                } else if (kind == "N") {
                    result = JSNumberMath::sin(x);
                } else {
                    result = JSNumberMath::cos(x);
                }
                std::cout << JsonValue(JsonValue::Object{{"value", number(result)}}).dump() << '\n';
                continue;
            }
            if (kind == "C") {
                int index = 0, hasScale = 0;
                double scale = 0;
                stream >> index >> hasScale >> scale;
                std::size_t count = 0;
                stream >> count;
                std::vector<double> times(count);
                for (std::size_t i = 0; i < count; ++i) {
                    stream >> token;
                    times[i] = parse(token);
                }
                auto curve = makeCurve(index, hasScale != 0, scale);
                JsonValue::Array points, tangents;
                Vector3 target;
                for (const auto t : times) {
                    curve->getPoint(t, target);
                    points.push_back(vectorJSON(target));
                    curve->getTangent(t, target);
                    tangents.push_back(vectorJSON(target));
                }
                curve->arcLengthDivisions = 8;
                const double length8 = curve->getLength();
                curve->arcLengthDivisions = 200;
                const double length200 = curve->getLength();
                std::cout << JsonValue(JsonValue::Object{{"points", std::move(points)},
                    {"tangents", std::move(tangents)}, {"length8", number(length8)},
                    {"length200", number(length200)}, {"type", curve->type}}).dump()
                          << '\n';
                continue;
            }
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
            continue;
        }
        std::cout << JsonValue(JsonValue::Object{{"probeError", "unknown scenario kind"}}).dump() << '\n';
    }
    return 0;
}
