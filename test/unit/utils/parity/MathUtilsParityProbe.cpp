// Differential probe for the native src/math/MathUtils.js port.
//
// Each stdin line is `name arg...`, one call per line, so the comparison covers the pure helpers with
// their awkward inputs: a reversed clamp range, negative euclidean modulo, a zero divisor, a zero-length
// map range, and the power-of-two edge cases. Results that are not finite are spelled out rather than
// dumped, because JSON has no spelling for them.
#include "core/JsonValue.h"
#include "math/MathUtils.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
JsonValue encode(double value) {
    if (std::isnan(value)) return JsonValue("NaN");
    if (std::isinf(value)) return JsonValue(value > 0 ? "Infinity" : "-Infinity");
    return JsonValue(value);
}
}
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream fields(line);
        std::string name;
        fields >> name;
        std::vector<double> args;
        double value = 0;
        while (fields >> value) args.push_back(value);
        const auto at = [&](std::size_t index) { return index < args.size() ? args[index] : 0.0; };
        JsonValue result;
        if (name == "clamp") result = encode(MathUtils::clamp(at(0), at(1), at(2)));
        else if (name == "euclideanModulo") result = encode(MathUtils::euclideanModulo(at(0), at(1)));
        else if (name == "mapLinear") result = encode(MathUtils::mapLinear(at(0), at(1), at(2), at(3), at(4)));
        else if (name == "inverseLerp") result = encode(MathUtils::inverseLerp(at(0), at(1), at(2)));
        else if (name == "lerp") result = encode(MathUtils::lerp(at(0), at(1), at(2)));
        else if (name == "damp") result = encode(MathUtils::damp(at(0), at(1), at(2), at(3)));
        else if (name == "pingpong") result = encode(MathUtils::pingpong(at(0), at(1)));
        else if (name == "smoothstep") result = encode(MathUtils::smoothstep(at(0), at(1), at(2)));
        else if (name == "smootherstep") result = encode(MathUtils::smootherstep(at(0), at(1), at(2)));
        else if (name == "degToRad") result = encode(MathUtils::degToRad(at(0)));
        else if (name == "radToDeg") result = encode(MathUtils::radToDeg(at(0)));
        else if (name == "isPowerOfTwo") result = JsonValue(MathUtils::isPowerOfTwo(at(0)));
        else if (name == "ceilPowerOfTwo") result = encode(MathUtils::ceilPowerOfTwo(at(0)));
        else if (name == "floorPowerOfTwo") result = encode(MathUtils::floorPowerOfTwo(at(0)));
        else result = JsonValue("unknown");
        std::cout << JsonValue(JsonValue::Object{{"result", result}}).dump() << '\n';
    }
    return 0;
}