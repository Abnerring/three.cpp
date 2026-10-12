#include "animation/tracks/VectorKeyframeTrack.h"

VectorKeyframeTrack::VectorKeyframeTrack(JSValue name, Array times, Array values, JSValue interpolation)
    : KeyframeTrack(std::move(name), std::move(times), std::move(values), std::move(interpolation),
                    {"vector", AnimationArrayType::Float32, InterpolateLinear, true, true}) {}
std::shared_ptr<KeyframeTrack> VectorKeyframeTrack::constructClone(JSValue name, Array times, Array values) const {
    return std::make_shared<VectorKeyframeTrack>(std::move(name), std::move(times), std::move(values));
}
