#pragma once
#include "animation/AnimationClip.h"
#include "core/JSValue.h"
#include "math/Matrix4.h"
#include "math/Vector3.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Bone;
class Object3D;
class Skeleton;
class SkinnedMesh;

// Native port of the pinned examples/jsm/utils/SkeletonUtils.js.
//
// The ordered `options` object of the source is a value type here: an absent optional is
// the upstream `undefined` and selects the documented default. Like the source, retarget()
// writes the resolved defaults back into the options it was handed, so a caller sees the
// same filled-in fields after the call. `options.getBoneName` and `options.names` are the
// two name lookups of the source; `getBoneName` returns a plain name, and a missing
// `names` entry stays absent instead of becoming an empty string.
struct RetargetOptions {
    std::optional<bool> preserveBoneMatrix;
    std::optional<bool> preserveBonePositions;
    std::optional<bool> useTargetMatrix;
    std::optional<std::string> hip;
    std::optional<Vector3> hipInfluence;
    std::optional<double> scale;
    std::unordered_map<std::string, std::string> names;
    std::function<std::string(const std::shared_ptr<Bone>&)> getBoneName;
    std::unordered_map<std::string, Matrix4> localOffsets;
    std::optional<Vector3> hipPosition;
    // retargetClip() only.
    std::optional<std::pair<double, double>> trim;
    std::optional<bool> useFirstFramePosition;
    std::optional<double> fps;
};

namespace SkeletonUtils {

// Clones the object and rebinds every SkinnedMesh to the cloned bones.
std::shared_ptr<Object3D> clone(const std::shared_ptr<Object3D>& source);

// Retargets the skeleton of `target` from `source`.
void retarget(SkinnedMesh& target, SkinnedMesh& source, RetargetOptions& options);
void retarget(Skeleton& target, Skeleton& source, RetargetOptions& options);
inline void retarget(SkinnedMesh& target, SkinnedMesh& source) {
    RetargetOptions options;
    retarget(target, source, options);
}
inline void retarget(Skeleton& target, Skeleton& source) {
    RetargetOptions options;
    retarget(target, source, options);
}

// Retargets the animation clip of `source` onto `target`.
std::shared_ptr<AnimationClip> retargetClip(SkinnedMesh& target, SkinnedMesh& source,
                                            const std::shared_ptr<AnimationClip>& clip, RetargetOptions& options);
std::shared_ptr<AnimationClip> retargetClip(SkinnedMesh& target, Skeleton& source,
                                            const std::shared_ptr<AnimationClip>& clip, RetargetOptions& options);
inline std::shared_ptr<AnimationClip> retargetClip(SkinnedMesh& target, SkinnedMesh& source,
                                                   const std::shared_ptr<AnimationClip>& clip) {
    RetargetOptions options;
    return retargetClip(target, source, clip, options);
}
inline std::shared_ptr<AnimationClip> retargetClip(SkinnedMesh& target, Skeleton& source,
                                                   const std::shared_ptr<AnimationClip>& clip) {
    RetargetOptions options;
    return retargetClip(target, source, clip, options);
}

} // namespace SkeletonUtils
