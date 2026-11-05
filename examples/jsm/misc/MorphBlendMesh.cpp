#include "misc/MorphBlendMesh.h"

#include "core/BufferGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "utils.h"

#include <cmath>
#include <limits>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>

namespace {
double clamp(double value, double minimum, double maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}
}  // namespace

MorphBlendMesh::MorphBlendMesh(std::shared_ptr<BufferGeometry> geometry,
                               std::shared_ptr<Material> material)
    : Mesh(geometry ? std::move(geometry) : std::make_shared<BufferGeometry>(),
           material ? std::move(material) : std::make_shared<MeshBasicMaterial>()) {
    animationsMap = {};
    animationsList = {};
    const double numFrames =
        morphTargetDictionary ? static_cast<double>(morphTargetDictionary->size()) : 0;
    // const name = '__default'; const startFrame = 0; const endFrame = numFrames - 1; const fps = numFrames / 1
    createAnimation("__default", 0, numFrames - 1, numFrames);
    setAnimationWeight("__default", 1);
}

MorphBlendMesh::Animation* MorphBlendMesh::find(const std::string& name) {
    const auto found = animationsMap.find(name);
    return found == animationsMap.end() ? nullptr : &found->second;
}

const MorphBlendMesh::Animation* MorphBlendMesh::find(const std::string& name) const {
    const auto found = animationsMap.find(name);
    return found == animationsMap.end() ? nullptr : &found->second;
}

void MorphBlendMesh::createAnimation(const std::string& name, double start, double end, double fps) {
    Animation animation;
    animation.name = name;
    animation.start = start;
    animation.end = end;
    animation.length = end - start + 1;
    animation.fps = fps;
    animation.duration = (end - start) / fps;
    animation.lastFrame = 0;
    animation.currentFrame = 0;
    animation.active = false;
    animation.time = 0;
    animation.direction = 1;
    animation.weight = 1;
    animation.directionBackwards = false;
    animation.mirroredLoop = false;
    auto [inserted, added] = animationsMap.insert_or_assign(name, animation);
    (void) added;
    animationsList.push_back(&inserted->second);
}

void MorphBlendMesh::autoCreateAnimations(double fps) {
    // const pattern = /([a-z]+)_?(\d+)/i
    static const std::regex pattern("([a-z]+)_?([0-9]+)", std::regex::icase);
    struct Range {
        double start = std::numeric_limits<double>::infinity();
        double end = -std::numeric_limits<double>::infinity();
    };
    std::vector<std::string> names;
    std::unordered_map<std::string, Range> frameRanges;
    std::string firstAnimationName;
    double index = 0;
    if (morphTargetDictionary) {
        for (const auto& [key, frame] : *morphTargetDictionary) {
            (void) frame;
            std::smatch chunks;
            if (std::regex_search(key, chunks, pattern) && chunks.size() > 1) {
                const std::string name = chunks[1].str();
                if (frameRanges.find(name) == frameRanges.end()) {
                    frameRanges[name] = Range{};
                    names.push_back(name);
                }
                auto& range = frameRanges[name];
                if (index < range.start) range.start = index;
                if (index > range.end) range.end = index;
                if (firstAnimationName.empty()) firstAnimationName = name;
            }
            index += 1;
        }
    }
    for (const auto& name : names) {
        const auto& range = frameRanges[name];
        createAnimation(name, range.start, range.end, fps);
    }
    firstAnimation = firstAnimationName;
}

void MorphBlendMesh::setAnimationDirectionForward(const std::string& name) {
    if (auto* animation = find(name)) {
        animation->direction = 1;
        animation->directionBackwards = false;
    }
}

void MorphBlendMesh::setAnimationDirectionBackward(const std::string& name) {
    if (auto* animation = find(name)) {
        animation->direction = -1;
        animation->directionBackwards = true;
    }
}

void MorphBlendMesh::setAnimationFPS(const std::string& name, double fps) {
    if (auto* animation = find(name)) {
        animation->fps = fps;
        animation->duration = (animation->end - animation->start) / animation->fps;
    }
}

void MorphBlendMesh::setAnimationDuration(const std::string& name, double duration) {
    if (auto* animation = find(name)) {
        animation->duration = duration;
        animation->fps = (animation->end - animation->start) / animation->duration;
    }
}

void MorphBlendMesh::setAnimationWeight(const std::string& name, double weight) {
    if (auto* animation = find(name)) animation->weight = weight;
}

void MorphBlendMesh::setAnimationTime(const std::string& name, double time) {
    if (auto* animation = find(name)) animation->time = time;
}

double MorphBlendMesh::getAnimationTime(const std::string& name) const {
    if (const auto* animation = find(name)) return animation->time;
    return 0;
}

double MorphBlendMesh::getAnimationDuration(const std::string& name) const {
    if (const auto* animation = find(name)) return animation->duration;
    return -1;
}

void MorphBlendMesh::playAnimation(const std::string& name) {
    if (auto* animation = find(name)) {
        animation->time = 0;
        animation->active = true;
    } else {
        ThreeUtils::warn({JSValue("THREE.MorphBlendMesh: animation[" + name +
                                  "] undefined in .playAnimation()")});
    }
}

void MorphBlendMesh::stopAnimation(const std::string& name) {
    if (auto* animation = find(name)) animation->active = false;
}

void MorphBlendMesh::update(double delta) {
    std::vector<double>* influences = nullptr;
    if (morphTargetInfluences) influences = &morphTargetInfluences->storage()->values;
    for (auto* animation : animationsList) {
        if (!animation->active) continue;
        const double frameTime = animation->duration / animation->length;
        animation->time += animation->direction * delta;
        if (animation->mirroredLoop) {
            if (animation->time > animation->duration || animation->time < 0) {
                animation->direction *= -1;
                if (animation->time > animation->duration) {
                    animation->time = animation->duration;
                    animation->directionBackwards = true;
                }
                if (animation->time < 0) {
                    animation->time = 0;
                    animation->directionBackwards = false;
                }
            }
        } else {
            animation->time = std::fmod(animation->time, animation->duration);
            if (animation->time < 0) animation->time += animation->duration;
        }
        const double keyframe =
            animation->start +
            clamp(std::floor(animation->time / frameTime), 0, animation->length - 1);
        const double weight = animation->weight;
        // Without an influence array there is nowhere to write, which the pinned code would turn into a
        // write onto an undefined index; here the animation state still advances.
        if (influences == nullptr) continue;
        const auto write = [&](double index, double value) {
            if (index >= 0 && static_cast<std::size_t>(index) < influences->size()) {
                (*influences)[static_cast<std::size_t>(index)] = value;
            }
        };
        if (keyframe != animation->currentFrame) {
            write(animation->lastFrame, 0);
            write(animation->currentFrame, weight);
            write(keyframe, 0);
            animation->lastFrame = animation->currentFrame;
            animation->currentFrame = keyframe;
        }
        double mix = std::fmod(animation->time, frameTime) / frameTime;
        if (animation->directionBackwards) mix = 1 - mix;
        if (animation->currentFrame != animation->lastFrame) {
            write(animation->currentFrame, mix * weight);
            write(animation->lastFrame, (1 - mix) * weight);
        } else {
            write(animation->currentFrame, weight);
        }
    }
}
