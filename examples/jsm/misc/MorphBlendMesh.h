#pragma once

#include "objects/Mesh.h"

#include <string>
#include <unordered_map>
#include <vector>

// Native port of examples/jsm/misc/MorphBlendMesh.js from the pinned three.js revision
// (see upstream.json).
//
// The old morph target animation helper: it keeps a table of named frame ranges and walks the morph
// target influences by hand. The port keeps that state in plain C++ structs instead of JavaScript
// objects, and mirrors every method: createAnimation builds a range, autoCreateAnimations derives
// ranges from the morph target dictionary keys with the pinned pattern, the setters write one field,
// play and stop flip the active flag, and update advances the time, wraps or mirrors it and rewrites
// the influences of the current and previous keyframe.
//
// One structural difference is recorded rather than hidden: this runtime's morph target dictionary is
// a sorted std::map while the pinned object keeps insertion order, so autoCreateAnimations enumerates
// keys in sorted order here. For dictionaries inserted in alphabetical order both agree.
class MorphBlendMesh : public Mesh {
  public:
    struct Animation {
        std::string name;
        double start = 0;
        double end = 0;
        double length = 0;
        double fps = 0;
        double duration = 0;
        double lastFrame = 0;
        double currentFrame = 0;
        bool active = false;
        double time = 0;
        int direction = 1;
        double weight = 1;
        bool directionBackwards = false;
        bool mirroredLoop = false;
    };

    std::unordered_map<std::string, Animation> animationsMap;
    std::string firstAnimation;
    // Pointers are stable across insertions in an unordered_map, like the pinned list of shared objects.
    std::vector<Animation*> animationsList;

    MorphBlendMesh(std::shared_ptr<BufferGeometry> geometry = nullptr,
                   std::shared_ptr<Material> material = nullptr);

    void createAnimation(const std::string& name, double start, double end, double fps);
    void autoCreateAnimations(double fps);
    void setAnimationDirectionForward(const std::string& name);
    void setAnimationDirectionBackward(const std::string& name);
    void setAnimationFPS(const std::string& name, double fps);
    void setAnimationDuration(const std::string& name, double duration);
    void setAnimationWeight(const std::string& name, double weight);
    void setAnimationTime(const std::string& name, double time);
    double getAnimationTime(const std::string& name) const;
    double getAnimationDuration(const std::string& name) const;
    void playAnimation(const std::string& name);
    void stopAnimation(const std::string& name);
    void update(double delta);

  private:
    Animation* find(const std::string& name);
    const Animation* find(const std::string& name) const;
};
