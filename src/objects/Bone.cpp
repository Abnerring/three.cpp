#include "objects/Bone.h"

Bone::Bone() { isBone = true; type = "Bone"; }
std::shared_ptr<Object3D> Bone::clone(bool recursive) const {
    auto result = std::make_shared<Bone>();
    result->copy(*this, recursive);
    return result;
}
