#include "animation/tracks/StringKeyframeTrack.h"

StringKeyframeTrack::StringKeyframeTrack(JSValue name, Array times, Array values, JSValue )
    : KeyframeTrack(std::move(name), std::move(times), std::move(values), JSValue{},
                    {"string", AnimationArrayType::Array, InterpolateDiscrete, false, false}) {}
std::shared_ptr<KeyframeTrack> StringKeyframeTrack::constructClone(JSValue name, Array times, Array values) const {
    return std::make_shared<StringKeyframeTrack>(std::move(name), std::move(times), std::move(values));
}
