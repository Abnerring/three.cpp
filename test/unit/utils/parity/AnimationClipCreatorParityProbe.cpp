// Differential probe for the pinned examples/jsm/animation/AnimationClipCreator.js port.
//
// Protocol, one scenario per stdin line:
//
//   C <kind> <first> <second> <extra...>
//
// kind 0 CreateRotationAnimation( period, axis )   - <second> selects x/y/z
// kind 1 CreateScaleAxisAnimation( period, axis )  - <second> selects x/y/z
// kind 2 CreateShakeAnimation( duration, shakeScale ) - the first two extras are shakeScale.y/z
// kind 3 CreatePulsationAnimation( duration, pulseScale )
// kind 4 CreateVisibilityAnimation( duration )
// kind 5 CreateMaterialColorAnimation( duration, count ) - colours come from the index palette
//
// Every remaining number is fed to the injected random source as an indexed pull with
// wraparound, exactly like the JavaScript side stubs Math.random.
#include "animation/AnimationClipCreator.h"
#include "animation/KeyframeTrack.h"
#include "core/JsonValue.h"
#include "math/Color.h"
#include "math/Vector3.h"
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
JsonValue arrayJSON(const InterpolantArray& array) {
    JsonValue::Array values;
    for (std::size_t i = 0, length = array.size(); i < length; ++i) values.push_back(number(array.number(double(i))));
    return values;
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
JsonValue clipJSON(const std::shared_ptr<AnimationClip>& clip) {
    JsonValue::Array tracks;
    for (const auto& track : *clip->tracks)
        tracks.push_back(JsonValue::Object{
            {"name", track->name.toString()}, {"times", arrayJSON(track->times)}, {"values", arrayJSON(track->values)}});
    return JsonValue::Object{
        {"name", clip->name.toString()}, {"duration", number(clip->duration)}, {"tracks", std::move(tracks)}};
}
std::vector<Color> palette(double count) {
    std::vector<Color> colors;
    for (double i = 0; i < count; i += 1)
        colors.emplace_back(std::fmod(i, 7.) / 7., std::fmod(i * 3., 5.) / 5., std::fmod(i * 5., 11.) / 11.);
    return colors;
}
const char* axes[] = {"x", "y", "z"};
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream stream(line);
        std::string kind, token;
        stream >> kind;
        try {
            if (kind != "C") throw std::runtime_error("unknown scenario kind: " + kind);
            double which = 0, first = 0, second = 0;
            stream >> which >> token;
            first = parse(token);
            stream >> token;
            second = parse(token);
            std::vector<double> extra;
            while (stream >> token) extra.push_back(parse(token));
            // kind 2 carries the shake vector as <second>, extra[ 0 ], extra[ 1 ].
            Vector3 shakeScale(second, 0, 0);
            if (which == 2) {
                shakeScale.set(second, extra.at(0), extra.at(1));
                extra.erase(extra.begin(), extra.begin() + 2);
            }
            if (which == 2 || which == 3) {
                std::size_t cursor = 0;
                AnimationClipCreator::setRandomGenerator([&extra, &cursor]() {
                    if (extra.empty()) throw std::runtime_error("no random values provided");
                    return extra[cursor++ % extra.size()];
                });
            }
            std::shared_ptr<AnimationClip> result;
            if (which == 0) result = AnimationClipCreator::CreateRotationAnimation(first, axes[std::size_t(second)]);
            else if (which == 1) result = AnimationClipCreator::CreateScaleAxisAnimation(first, axes[std::size_t(second)]);
            else if (which == 2) result = AnimationClipCreator::CreateShakeAnimation(first, shakeScale);
            else if (which == 3) result = AnimationClipCreator::CreatePulsationAnimation(first, second);
            else if (which == 4) result = AnimationClipCreator::CreateVisibilityAnimation(first);
            else result = AnimationClipCreator::CreateMaterialColorAnimation(first, palette(second));
            std::cout << clipJSON(result).dump() << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}
