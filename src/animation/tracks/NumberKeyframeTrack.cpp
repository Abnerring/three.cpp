#include "animation/tracks/NumberKeyframeTrack.h"

NumberKeyframeTrack::NumberKeyframeTrack(JSValue name, Array times, Array values, JSValue interpolation)
    : KeyframeTrack(std::move(name), std::move(times), std::move(values), std::move(interpolation),
                    {"number", AnimationArrayType::Float32, InterpolateLinear, true, true}) {}
std::shared_ptr<KeyframeTrack> NumberKeyframeTrack::constructClone(JSValue name, Array times, Array values) const {
    return std::make_shared<NumberKeyframeTrack>(std::move(name), std::move(times), std::move(values));
}
