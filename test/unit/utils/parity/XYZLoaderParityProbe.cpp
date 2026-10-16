// Differential probe for the native examples/jsm/loaders/XYZLoader.js port.
// Each stdin line is one XYZ document, with "|" standing in for a newline. Output is the geometry
// shape plus the raw float32 bit patterns of both arrays: comparing bits rather than decimal
// literals is the only way to avoid the single precision rounding trap.
#include "core/JSValue.h"
#include "core/JsonValue.h"
#include "loaders/XYZLoader.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
namespace {
std::string decode(const std::string& line) {
    std::string text;
    for (char ch : line) text.push_back(ch == '|' ? '\n' : ch);
    return text;
}
JsonValue patterns(const JSValue& geometry, const char* name) {
    const auto attribute = geometry.get("attributes").get(name);
    if (attribute.isUndefined() || attribute.isNull()) return "absent";
    const auto array = attribute.get("array");
    const double length = array.get("length").toNumber();
    std::string buffer;
    for (double index = 0; index < length; index += 1) {
        const float single = static_cast<float>(array.get(index).toNumber());
        std::uint32_t bits = 0;
        std::memcpy(&bits, &single, sizeof(bits));
        char text[16];
        std::snprintf(text, sizeof(text), "%08x ", bits);
        buffer += text;
    }
    while (!buffer.empty() && buffer.back() == ' ') buffer.pop_back();
    return buffer;
}
}
int main() {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            XYZLoader loader;
            const auto geometry = loader.parse(JSValue(decode(line)));
            const auto attributes = geometry.get("attributes");
            std::cout << JsonValue(JsonValue::Object{
                             {"positionCount", attributes.get("position").get("count").toNumber()},
                             {"hasColor", !attributes.get("color").isUndefined() && !attributes.get("color").isNull()},
                             {"position", patterns(geometry, "position")},
                             {"color", patterns(geometry, "color")}})
                             .dump()
                      << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}