// Differential probe for the native src/loaders/AnimationLoader.js port.
// Each stdin line is one serialized clip list, passed through the same parser the probes use.
// Output is the clip count and, per clip, its name, track count and duration as raw float64 bits:
// bits rather than the decimal value, so a duration that is not exactly representable cannot be
// papered over.
#include "animation/AnimationUtils.h"
#include "core/JsonValue.h"
#include "core/JSValue.h"
#include "loaders/AnimationLoader.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

int main() {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            AnimationLoader loader;
            const auto clips = loader.parse(Animation_detail::parseJSON(line));
            const double count = clips.get("length").toNumber();
            JsonValue::Array encoded;
            for (double index = 0; index < count; index += 1) {
                const auto clip = clips.get(std::to_string(static_cast<long long>(index)));
                const double duration = clip.get("duration").toNumber();
                std::uint64_t bits = 0;
                std::memcpy(&bits, &duration, sizeof(bits));
                char hex[32];
                std::snprintf(hex, sizeof(hex), "%016llx", static_cast<unsigned long long>(bits));
                encoded.emplace_back(JsonValue::Object{
                    {"name", clip.get("name").toString()},
                    {"tracks", clip.get("tracks").get("length").toNumber()},
                    {"durationBits", std::string(hex)}});
            }
            std::cout << JsonValue(JsonValue::Object{{"count", count}, {"clips", JsonValue(encoded)}}).dump() << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}