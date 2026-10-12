#include "animation/tracks/QuaternionKeyframeTrack.h"
#include "math/interpolants/QuaternionLinearInterpolant.h"

QuaternionKeyframeTrack::QuaternionKeyframeTrack(JSValue name, Array times, Array values, JSValue interpolation)
    : KeyframeTrack(std::move(name), std::move(times), std::move(values), std::move(interpolation),
                    {"quaternion", AnimationArrayType::Float32, InterpolateLinear, true, false}) {}
std::shared_ptr<KeyframeTrack> QuaternionKeyframeTrack::constructClone(JSValue name, Array times, Array values) const {
    return std::make_shared<QuaternionKeyframeTrack>(std::move(name), std::move(times), std::move(values));
}
std::shared_ptr<Interpolant> QuaternionKeyframeTrack::InterpolantFactoryMethodLinear(Array result) {
    const auto t = times, v = values; const auto size = getValueSize();
    return std::make_shared<QuaternionLinearInterpolant>(t, v, size, std::move(result));
}
