// Differential probe for the native src/math/Color.js port.
//
// Each stdin line is `op tokens...`. Tokens rather than numbers, because several operations take a CSS
// string or a colour space name. Results are the three components with signed zero and the non-finite
// values spelled out. The cases lean on what a hand port drifts on: hex parsing with and without an alpha
// channel, CSS functional notation, hue wrapping, and the two colour-space conversions.
#include "core/JsonValue.h"
#include "math/Color.h"
#include "math/Vector3.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
using Args = std::vector<std::string>;
JsonValue encode(double value) {
    if (value == 0 && std::signbit(value)) return JsonValue("-0");
    if (std::isnan(value)) return JsonValue("NaN");
    if (std::isinf(value)) return JsonValue(value > 0 ? "Infinity" : "-Infinity");
    return JsonValue(value);
}
JsonValue colorResult(const Color& color) {
    return JsonValue(JsonValue::Object{{"r", encode(color.r)}, {"g", encode(color.g)}, {"b", encode(color.b)}});
}
double number(const Args& args, std::size_t index) {
    return index < args.size() ? std::strtod(args[index].c_str(), nullptr) : 0.0;
}
const std::string& token(const Args& args, std::size_t index) {
    static const std::string empty;
    return index < args.size() ? args[index] : empty;
}
}
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream fields(line);
        std::string op;
        fields >> op;
        Args args;
        std::string tokenText;
        while (fields >> tokenText) args.push_back(tokenText);
        JsonValue result;
        if (op == "setRGB") result = colorResult(Color().setRGB(number(args, 0), number(args, 1), number(args, 2)));
        else if (op == "setHex") result = colorResult(Color().setHex(number(args, 0)));
        else if (op == "setStyle") result = colorResult(Color().setStyle(token(args, 0)));
        else if (op == "setHSL") result = colorResult(Color().setHSL(number(args, 0), number(args, 1), number(args, 2)));
        else if (op == "getHSL") {
            Vector3 hsl;
            Color(number(args, 0), number(args, 1), number(args, 2)).getHSL(hsl);
            result = JsonValue(JsonValue::Object{{"h", encode(hsl.x)}, {"s", encode(hsl.y)}, {"l", encode(hsl.z)}});
        } else if (op == "srgbToLinear") result = colorResult(Color(number(args, 0), number(args, 1), number(args, 2)).convertSRGBToLinear());
        else if (op == "linearToSrgb") result = colorResult(Color(number(args, 0), number(args, 1), number(args, 2)).convertLinearToSRGB());
        else if (op == "lerp") result = colorResult(Color(number(args, 0), number(args, 1), number(args, 2)).lerp(Color(number(args, 3), number(args, 4), number(args, 5)), number(args, 6)));
        else if (op == "getHex") result = JsonValue(JsonValue::Object{{"value", Color(number(args, 0), number(args, 1), number(args, 2)).getHex()}});
        else if (op == "getHexString") result = JsonValue(JsonValue::Object{{"value", Color(number(args, 0), number(args, 1), number(args, 2)).getHexString()}});
        else result = JsonValue("unknown");
        std::cout << result.dump() << '\n';
    }
    return 0;
}