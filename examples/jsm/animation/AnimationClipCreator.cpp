#include "animation/AnimationClipCreator.h"
#include "animation/tracks/BooleanKeyframeTrack.h"
#include "animation/tracks/ColorKeyframeTrack.h"
#include "animation/tracks/NumberKeyframeTrack.h"
#include "animation/tracks/VectorKeyframeTrack.h"
#include "math/MathUtils.h"
#include "math/Vector3.h"
#include <memory>

namespace {
// The `Math.random` source of the pinned module, replaceable like the other injected sources.
AnimationClipCreator::RandomSource& source() {
    static thread_local AnimationClipCreator::RandomSource randomFunction;
    return randomFunction;
}
double random() {
    if (source()) return source()();
    return MathUtils::seededRandom();
}
std::shared_ptr<AnimationClip> clip(double duration, std::shared_ptr<KeyframeTrack> track) {
    auto tracks = std::make_shared<AnimationClip::Tracks>();
    tracks->push_back(std::move(track));
    return std::make_shared<AnimationClip>("", duration, tracks);
}
}

void AnimationClipCreator::setRandomGenerator(RandomSource randomFunction) {
    source() = std::move(randomFunction);
}

std::shared_ptr<AnimationClip> AnimationClipCreator::CreateRotationAnimation(double period, const std::string& axis) {
    const std::vector<double> times{0, period}, values{0, 360};
    const std::string trackName = ".rotation[" + axis + "]";
    return clip(period, std::make_shared<NumberKeyframeTrack>(trackName, times, values));
}

std::shared_ptr<AnimationClip> AnimationClipCreator::CreateScaleAxisAnimation(double period, const std::string& axis) {
    const std::vector<double> times{0, period}, values{0, 1};
    const std::string trackName = ".scale[" + axis + "]";
    return clip(period, std::make_shared<NumberKeyframeTrack>(trackName, times, values));
}

std::shared_ptr<AnimationClip> AnimationClipCreator::CreateShakeAnimation(double duration, const Vector3& shakeScale) {
    std::vector<double> times, values;
    Vector3 temporary;
    for (double i = 0; i < duration * 10; i += 1) {
        times.push_back(i / 10);
        temporary.set(random() * 2.0 - 1.0, random() * 2.0 - 1.0, random() * 2.0 - 1.0)
            .multiply(shakeScale)
            .toArray(values, values.size());
    }
    return clip(duration, std::make_shared<VectorKeyframeTrack>(".position", times, values));
}

std::shared_ptr<AnimationClip> AnimationClipCreator::CreatePulsationAnimation(double duration, double pulseScale) {
    std::vector<double> times, values;
    Vector3 temporary;
    for (double i = 0; i < duration * 10; i += 1) {
        times.push_back(i / 10);
        const double scaleFactor = random() * pulseScale;
        temporary.set(scaleFactor, scaleFactor, scaleFactor).toArray(values, values.size());
    }
    return clip(duration, std::make_shared<VectorKeyframeTrack>(".scale", times, values));
}

std::shared_ptr<AnimationClip> AnimationClipCreator::CreateVisibilityAnimation(double duration) {
    const std::vector<double> times{0, duration / 2, duration};
    const std::vector<bool> values{true, false, true};
    return clip(duration, std::make_shared<BooleanKeyframeTrack>(".visible", times, values));
}

std::shared_ptr<AnimationClip> AnimationClipCreator::CreateMaterialColorAnimation(
    double duration, const std::vector<Color>& colors) {
    std::vector<double> times, values;
    const double timeStep = (colors.size() > 1) ? duration / (double(colors.size()) - 1) : 0;
    for (std::size_t i = 0; i < colors.size(); ++i) {
        times.push_back(double(i) * timeStep);
        const Color& color = colors[i];
        values.push_back(color.r);
        values.push_back(color.g);
        values.push_back(color.b);
    }
    return clip(duration, std::make_shared<ColorKeyframeTrack>(".material.color", times, values));
}