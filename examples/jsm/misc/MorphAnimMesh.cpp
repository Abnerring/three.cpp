#include "misc/MorphAnimMesh.h"

#include "animation/AnimationClip.h"
#include "core/BufferGeometry.h"
#include "materials/MeshBasicMaterial.h"

#include <stdexcept>

std::shared_ptr<MorphAnimMesh> MorphAnimMesh::create(const std::shared_ptr<BufferGeometry>& geometry,
                                                     const std::shared_ptr<Material>& material) {
    auto mesh = std::shared_ptr<MorphAnimMesh>(new MorphAnimMesh(
        geometry ? geometry : std::make_shared<BufferGeometry>(),
        material ? material : std::make_shared<MeshBasicMaterial>()));
    // new AnimationMixer( this ): the shared handle exists from here on, so the mixer can point back.
    mesh->mixer = std::make_shared<AnimationMixer>(std::static_pointer_cast<Object3D>(mesh));
    return mesh;
}

MorphAnimMesh::MorphAnimMesh(const std::shared_ptr<BufferGeometry>& geometry,
                             const std::shared_ptr<Material>& material)
    : Mesh(geometry, material) {
    type = "MorphAnimMesh";
    activeAction = JSValue(nullptr);
}

void MorphAnimMesh::setDirectionForward() {
    mixer->timeScale = JSValue(1.0);
}

void MorphAnimMesh::setDirectionBackward() {
    mixer->timeScale = JSValue(-1.0);
}

void MorphAnimMesh::playAnimation(const std::string& label, double fps) {
    if (activeAction.toBoolean()) {
        AnimationAction::fromObject(activeAction).stop();
        activeAction = JSValue(nullptr);
    }
    const auto clip = AnimationClip::findByName(*this, JSValue(label));
    if (!clip) {
        throw std::runtime_error("THREE.MorphAnimMesh: animations[" + label +
                                 "] undefined in .playAnimation()");
    }
    auto action = mixer->clipAction(clip);
    action.timeScale =
        JSValue((static_cast<double>(clip->tracks->size()) * fps) / clip->duration);
    activeAction = action.play();
}

void MorphAnimMesh::updateAnimation(double delta) {
    mixer->update(JSValue(delta));
}

Object3D& MorphAnimMesh::copy(const Object3D& source, bool recursive) {
    Mesh::copy(source, recursive);
    // this.mixer = new AnimationMixer( this )
    mixer = std::make_shared<AnimationMixer>(std::static_pointer_cast<Object3D>(shared_from_this()));
    return *this;
}
