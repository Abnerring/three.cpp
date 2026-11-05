#pragma once

#include "animation/AnimationAction.h"
#include "animation/AnimationMixer.h"
#include "core/JSValue.h"
#include "objects/Mesh.h"

#include <memory>
#include <string>

// Native port of examples/jsm/misc/MorphAnimMesh.js from the pinned three.js revision
// (see upstream.json).
//
// A Mesh that owns an AnimationMixer pointing back at itself and plays one clip at a time by
// label. The pinned constructor builds that mixer with `new AnimationMixer( this )`, which needs a
// shared handle to the object being constructed. A C++ constructor cannot hand out that handle yet,
// so construction goes through create(): it makes the object first and then wires the mixer, which
// keeps every observable behaviour (mixer present right after construction, root pointing at the
// mesh) identical while moving the construction step into a factory.
class MorphAnimMesh : public Mesh {
  public:
    std::shared_ptr<AnimationMixer> mixer;
    JSValue activeAction;

    static std::shared_ptr<MorphAnimMesh> create(
        const std::shared_ptr<BufferGeometry>& geometry = nullptr,
        const std::shared_ptr<Material>& material = nullptr);

    void setDirectionForward();
    void setDirectionBackward();
    void playAnimation(const std::string& label, double fps);
    void updateAnimation(double delta);
    Object3D& copy(const Object3D& source, bool recursive = true) override;

  private:
    MorphAnimMesh(const std::shared_ptr<BufferGeometry>& geometry,
                  const std::shared_ptr<Material>& material);
};
