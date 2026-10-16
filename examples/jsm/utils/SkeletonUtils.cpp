#include "utils/SkeletonUtils.h"
#include "animation/AnimationMixer.h"
#include "animation/AnimationRuntime.h"
#include "animation/tracks/QuaternionKeyframeTrack.h"
#include "animation/tracks/VectorKeyframeTrack.h"
#include "core/NativeObject.h"
#include "core/Object3D.h"
#include "core/TypedArray.h"
#include "objects/Bone.h"
#include "objects/Skeleton.h"
#include "objects/SkinnedMesh.h"
#include <cmath>
#include <limits>

namespace {
// A missing array entry behaves like the upstream `undefined`, which throws as soon as the
// algorithm reads a bone property from it.
std::shared_ptr<Bone> boneAt(const Skeleton::Bones& values, std::size_t index) {
    const auto storage = values.storage();
    if (index >= storage->values.size() || storage->holes.contains(index)) return nullptr;
    return storage->values[index];
}
std::shared_ptr<Bone> requirement(const std::shared_ptr<Bone>& bone, const char* property) {
    if (!bone) throw JSTypeError(std::string("Cannot read properties of undefined (reading '") + property + "')");
    return bone;
}
std::optional<std::string> boneName(const std::shared_ptr<Bone>& bone, const RetargetOptions& options) {
    if (options.getBoneName) return options.getBoneName(bone);
    const auto entry = options.names.find(bone->name);
    if (entry == options.names.end()) return std::nullopt;
    return entry->second;
}
std::shared_ptr<Bone> boneByName(const std::optional<std::string>& name, const Skeleton::Bones& bones) {
    if (!name) return nullptr;
    for (std::size_t i = 0, length = bones.size(); i < length; ++i) {
        const auto bone = boneAt(bones, i);
        if (bone && *name == bone->name) return bone;
    }
    return nullptr;
}
// The shared body of both retarget overloads. `targetObject` is null for the Skeleton
// form, which is exactly the upstream `target.isObject3D` branch.
void retargetSkeleton(Object3D* targetObject, Skeleton& targetSkeleton, Skeleton& sourceSkeleton,
                      RetargetOptions& options) {
    Vector3 scale;
    Quaternion quat;
    Matrix4 relativeMatrix, globalMatrix;

    if (!options.preserveBoneMatrix) options.preserveBoneMatrix = true;
    if (!options.preserveBonePositions) options.preserveBonePositions = true;
    if (!options.useTargetMatrix) options.useTargetMatrix = false;
    if (!options.hip) options.hip = "hip";
    if (!options.hipInfluence) options.hipInfluence = Vector3(1, 1, 1);
    if (!options.scale) options.scale = 1.;

    const Skeleton::Bones& sourceBones = sourceSkeleton.bones;
    const Skeleton::Bones& bones = targetSkeleton.bones;

    // reset bones
    if (targetObject) {
        targetSkeleton.pose();
    } else {
        options.useTargetMatrix = true;
        options.preserveBoneMatrix = false;
    }

    std::vector<Vector3> bonesPosition;
    if (*options.preserveBonePositions) {
        bonesPosition.reserve(bones.size());
        for (std::size_t i = 0, length = bones.size(); i < length; ++i)
            bonesPosition.push_back(requirement(boneAt(bones, i), "position")->position);
    }

    if (*options.preserveBoneMatrix) {
        // reset matrix
        targetObject->updateMatrixWorld();
        targetObject->matrixWorld.identity();
        // reset children matrix
        for (std::size_t i = 0, length = targetObject->children.size(); i < length; ++i) {
            const auto child = targetObject->children[i];
            if (!child) throw JSTypeError("Cannot read properties of undefined (reading 'updateMatrixWorld')");
            child->updateMatrixWorld(true);
        }
    }

    for (std::size_t i = 0, length = bones.size(); i < length; ++i) {
        const auto bone = requirement(boneAt(bones, i), "matrixWorld");
        const auto name = boneName(bone, options);
        const auto boneTo = boneByName(name, sourceBones);

        globalMatrix.copy(bone->matrixWorld);

        if (boneTo) {
            boneTo->updateMatrixWorld();

            if (*options.useTargetMatrix) {
                relativeMatrix.copy(boneTo->matrixWorld);
            } else {
                relativeMatrix.copy(targetObject->matrixWorld).invert();
                relativeMatrix.multiply(boneTo->matrixWorld);
            }

            // ignore scale to extract rotation
            scale.setFromMatrixScale(relativeMatrix);
            relativeMatrix.scale(scale.set(1 / scale.x, 1 / scale.y, 1 / scale.z));

            // apply to global matrix
            globalMatrix.makeRotationFromQuaternion(quat.setFromRotationMatrix(relativeMatrix));

            if (targetObject) {
                if (!options.localOffsets.empty() && name) {
                    const auto offset = options.localOffsets.find(bone->name);
                    if (offset != options.localOffsets.end()) globalMatrix.multiply(offset->second);
                }
            }

            globalMatrix.copyPosition(relativeMatrix);
        }

        if (name && *name == *options.hip) {
            auto elements = globalMatrix.elements();
            elements[12] *= *options.scale * options.hipInfluence->x;
            elements[13] *= *options.scale * options.hipInfluence->y;
            elements[14] *= *options.scale * options.hipInfluence->z;

            if (options.hipPosition) {
                elements[12] += options.hipPosition->x * *options.scale;
                elements[13] += options.hipPosition->y * *options.scale;
                elements[14] += options.hipPosition->z * *options.scale;
            }
        }

        if (bone->parent) {
            bone->matrix.copy(bone->parent->matrixWorld).invert();
            bone->matrix.multiply(globalMatrix);
        } else {
            bone->matrix.copy(globalMatrix);
        }

        bone->matrix.decompose(bone->position, bone->quaternion, bone->scale);
        bone->updateMatrixWorld();
    }

    if (*options.preserveBonePositions) {
        for (std::size_t i = 0, length = bones.size(); i < length; ++i) {
            const auto bone = requirement(boneAt(bones, i), "position");
            const auto name = boneName(bone, options);
            // `getBoneName( bone, options ) || bone.name` keeps the bone name for a falsy result.
            const std::string resolved = name && !name->empty() ? *name : bone->name;
            if (resolved != *options.hip) bone->position.copy(bonesPosition[i]);
        }
    }

    if (*options.preserveBoneMatrix) targetObject->updateMatrixWorld(true);
}

// The shared body of retargetClip(). `mixerRoot` is the animated root: the source mesh, or
// the wrapper that carries a bare skeleton for the Skeleton form.
std::shared_ptr<AnimationClip> retargetClipImpl(SkinnedMesh& target, const std::shared_ptr<Object3D>& mixerRoot,
                                                const std::shared_ptr<AnimationClip>& clip, RetargetOptions& options) {
    if (!options.useFirstFramePosition) options.useFirstFramePosition = false;

    // Calculate the fps from the source clip based on the track with the most frames,
    // unless fps is already provided.
    if (!options.fps) {
        double mostFrames = -std::numeric_limits<double>::infinity();
        for (const auto& track : *clip->tracks) mostFrames = std::max(mostFrames, double(track->times.size()));
        options.fps = mostFrames / clip->duration;
    }

    const double numFrames = std::round(clip->duration * (*options.fps / 1000) * 1000);
    const double delta = clip->duration / (numFrames - 1);
    AnimationClip::Tracks convertedTracks;
    AnimationMixer mixer(mixerRoot);
    const auto sourceSkeleton = dynamic_cast<SkinnedMesh*>(mixerRoot.get())->skeleton;
    if (!target.skeleton) throw JSTypeError("Cannot read properties of undefined (reading 'bones')");
    const Skeleton::Bones& bones = target.skeleton->bones;
    struct BoneData {
        std::shared_ptr<Bone> bone;
        bool hasPosition = false;
        std::optional<Float32Array> positionTimes, positionValues;
        std::optional<Float32Array> quaternionTimes, quaternionValues;
    };
    std::vector<std::optional<BoneData>> boneDatas(bones.size());
    std::optional<Vector3> positionOffset;

    auto action = mixer.clipAction(clip);
    action.play();

    // trim
    double start = 0, end = numFrames;
    if (options.trim) {
        start = std::round(options.trim->first * *options.fps);
        end = std::min(std::round(options.trim->second * *options.fps), numFrames) - start;
        mixer.update(JSValue(options.trim->first));
    } else {
        mixer.update(JSValue(0.));
    }

    mixerRoot->updateMatrixWorld();

    for (double frame = 0; frame < end; ++frame) {
        const double time = frame * delta;
        SkeletonUtils::retarget(target, *dynamic_cast<SkinnedMesh*>(mixerRoot.get()), options);

        for (std::size_t j = 0, length = bones.size(); j < length; ++j) {
            const auto bone = requirement(boneAt(bones, j), "name");
            const auto name = boneName(bone, options);
            // `getBoneName( bone, options ) || bone.name`
            const std::string resolved = name && !name->empty() ? *name : bone->name;
            const auto boneTo = boneByName(std::optional<std::string>(resolved), sourceSkeleton->bones);
            if (!boneTo) continue;

            auto& data = boneDatas[j];
            if (!data) {
                data = BoneData{};
                data->bone = bone;
            }

            if (*options.hip == resolved) {
                if (!data->hasPosition) {
                    data->hasPosition = true;
                    data->positionTimes = Float32Array(end);
                    data->positionValues = Float32Array(end * 3);
                }
                if (*options.useFirstFramePosition) {
                    if (frame == 0) positionOffset = bone->position;
                    bone->position.sub(*positionOffset);
                }
                (*data->positionTimes)[std::size_t(frame)] = time;
                std::vector<double> components;
                bone->position.toArray(components);
                for (std::size_t c = 0; c < 3; ++c)
                    (*data->positionValues)[std::size_t(frame) * 3 + c] = components[c];
            }

            if (!data->quaternionValues) {
                data->quaternionTimes = Float32Array(end);
                data->quaternionValues = Float32Array(end * 4);
            }
            (*data->quaternionTimes)[std::size_t(frame)] = time;
            (*data->quaternionValues)[std::size_t(frame) * 4 + 0] = bone->quaternion.x;
            (*data->quaternionValues)[std::size_t(frame) * 4 + 1] = bone->quaternion.y;
            (*data->quaternionValues)[std::size_t(frame) * 4 + 2] = bone->quaternion.z;
            (*data->quaternionValues)[std::size_t(frame) * 4 + 3] = bone->quaternion.w;
        }

        if (frame == end - 2) {
            // last mixer update before final loop iteration
            // make sure we do not go over or equal to clip duration
            mixer.update(JSValue(delta - 0.0000001));
        } else {
            mixer.update(JSValue(delta));
        }

        mixerRoot->updateMatrixWorld();
    }

    for (std::size_t i = 0, length = boneDatas.size(); i < length; ++i) {
        if (!boneDatas[i]) continue;
        auto& data = *boneDatas[i];
        if (data.hasPosition)
            convertedTracks.push_back(std::make_shared<VectorKeyframeTrack>(
                JSValue(".bones[" + data.bone->name + "].position"), AnimationArray(*data.positionTimes),
                AnimationArray(*data.positionValues)));
        convertedTracks.push_back(std::make_shared<QuaternionKeyframeTrack>(
            JSValue(".bones[" + data.bone->name + "].quaternion"), AnimationArray(*data.quaternionTimes),
            AnimationArray(*data.quaternionValues)));
    }

    mixer.uncacheAction(nativeAnimationClip(clip));

    return std::make_shared<AnimationClip>(clip->name, -1,
                                          std::make_shared<AnimationClip::Tracks>(std::move(convertedTracks)));
}
}

namespace SkeletonUtils {

std::shared_ptr<Object3D> clone(const std::shared_ptr<Object3D>& source) {
    if (!source) throw JSTypeError("Cannot read properties of undefined (reading 'clone')");
    std::unordered_map<const Object3D*, std::shared_ptr<Object3D>> sourceLookup, cloneLookup;
    auto clone = source->clone();

    const std::function<void(const std::shared_ptr<Object3D>&, const std::shared_ptr<Object3D>&)> parallelTraverse =
        [&](const std::shared_ptr<Object3D>& a, const std::shared_ptr<Object3D>& b) {
            if (!a || !b) throw JSTypeError("Cannot read properties of undefined (reading 'children')");
            sourceLookup[b.get()] = a;
            cloneLookup[a.get()] = b;
            for (std::size_t i = 0, length = a->children.size(); i < length; ++i) {
                const auto sourceChild = a->children[i];
                const auto clonedChild = i < b->children.size() ? b->children[i]
                                                               : std::shared_ptr<Object3D>();
                parallelTraverse(sourceChild, clonedChild);
            }
        };
    parallelTraverse(source, clone);

    clone->traverse([&](Object3D& node) {
        if (!node.isSkinnedMesh) return;
        const auto clonedMesh = dynamic_cast<SkinnedMesh*>(&node);
        const auto lookup = sourceLookup.find(&node);
        const auto sourceMesh = lookup == sourceLookup.end()
                                    ? std::shared_ptr<SkinnedMesh>()
                                    : std::dynamic_pointer_cast<SkinnedMesh>(lookup->second);
        if (!sourceMesh) throw JSTypeError("Cannot read properties of undefined (reading 'skeleton')");
        if (!sourceMesh->skeleton) throw JSTypeError("Cannot read properties of undefined (reading 'bones')");
        const auto sourceBones = sourceMesh->skeleton->bones;

        clonedMesh->skeleton = sourceMesh->skeleton->clone();
        clonedMesh->bindMatrix.copy(sourceMesh->bindMatrix);

        Skeleton::Bones bones;
        bones.reserve(sourceBones.size());
        for (std::size_t i = 0, length = sourceBones.size(); i < length; ++i) {
            const auto bone = boneAt(sourceBones, i);
            const auto entry = bone ? cloneLookup.find(bone.get()) : cloneLookup.end();
            bones.push_back(entry == cloneLookup.end() ? nullptr : std::dynamic_pointer_cast<Bone>(entry->second));
        }
        clonedMesh->skeleton->bones = bones;

        clonedMesh->bind(clonedMesh->skeleton, clonedMesh->bindMatrix);
    });

    return clone;
}

void retarget(SkinnedMesh& target, SkinnedMesh& source, RetargetOptions& options) {
    if (!target.skeleton) throw JSTypeError("Cannot read properties of undefined (reading 'bones')");
    if (!source.skeleton) throw JSTypeError("Cannot read properties of undefined (reading 'bones')");
    retargetSkeleton(&target, *target.skeleton, *source.skeleton, options);
}

void retarget(Skeleton& target, Skeleton& source, RetargetOptions& options) {
    retargetSkeleton(nullptr, target, source, options);
}

std::shared_ptr<AnimationClip> retargetClip(SkinnedMesh& target, SkinnedMesh& source,
                                            const std::shared_ptr<AnimationClip>& clip, RetargetOptions& options) {
    if (!source.skeleton) throw JSTypeError("Cannot read properties of undefined (reading 'skeleton')");
    return retargetClipImpl(target, source.shared_from_this(), clip, options);
}

std::shared_ptr<AnimationClip> retargetClip(SkinnedMesh& target, Skeleton& source,
                                            const std::shared_ptr<AnimationClip>& clip, RetargetOptions& options) {
    // The upstream form wraps a bare skeleton in a SkeletonHelper and assigns its
    // `skeleton` slot so `.bones[ name ]` bindings resolve. The native adaptation uses an
    // empty SkinnedMesh for the same purpose: it owns the skeleton reference and gives the
    // animation bindings the same `root.skeleton.bones` lookup.
    auto wrapper = std::make_shared<SkinnedMesh>();
    wrapper->skeleton = source.shared_from_this();
    return retargetClipImpl(target, wrapper, clip, options);
}

} // namespace SkeletonUtils
