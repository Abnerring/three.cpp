#include "animation/tracks/ColorKeyframeTrack.h"

ColorKeyframeTrack::ColorKeyframeTrack(JSValue name, Array times, Array values, JSValue interpolation)
    : KeyframeTrack(std::move(name), std::move(times), std::move(values), std::move(interpolation),
                    {"color", AnimationArrayType::Float32, InterpolateLinear, true, true}) {}
std::shared_ptr<KeyframeTrack> ColorKeyframeTrack::constructClone(JSValue name, Array times, Array values) const {
    return std::make_shared<ColorKeyframeTrack>(std::move(name), std::move(times), std::move(values));
}
