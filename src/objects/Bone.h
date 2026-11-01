#pragma once
#include "core/Object3D.h"

class Bone : public Object3D {
  public:
    Bone();
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
};

// Complete managed class; typed renderer bridge remains separate.
#include "core/JSModuleObject.h"
JSValue nativeBoneConstructor();
JSValue nativeBoneModule();
JSValue nativeBoneBinding(const std::string& name);
inline constexpr JSModuleObject BoneModule{nativeBoneModule};
