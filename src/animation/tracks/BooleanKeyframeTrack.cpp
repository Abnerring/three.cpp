#include "animation/tracks/BooleanKeyframeTrack.h"

BooleanKeyframeTrack::BooleanKeyframeTrack(JSValue name, Array times, Array values, JSValue )
    : KeyframeTrack(std::move(name), std::move(times), std::move(values), JSValue{},
                    {"bool", AnimationArrayType::Array, InterpolateDiscrete, false, false}) {}
std::shared_ptr<KeyframeTrack> BooleanKeyframeTrack::constructClone(JSValue name, Array times, Array values) const {
    return std::make_shared<BooleanKeyframeTrack>(std::move(name), std::move(times), std::move(values));
}
