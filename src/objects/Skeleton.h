#pragma once
#include "objects/Bone.h"
#include "textures/DataTexture.h"
#include <map>

class Skeleton : public std::enable_shared_from_this<Skeleton> {
  public:
    using Bones = SharedArray<std::shared_ptr<Bone>>;
    using Inverses = SharedArray<std::shared_ptr<Matrix4>>;
    std::string uuid;
    Bones bones;
    Inverses boneInverses;
    Float32Array boneMatrices;
    std::shared_ptr<DataTexture> boneTexture;

    explicit Skeleton(Bones bones = {}, Inverses boneInverses = {});
    void init();
    void calculateInverses();
    void pose();
    void update();
    std::shared_ptr<Skeleton> clone() const;
    Skeleton& computeBoneTexture();
    std::shared_ptr<Bone> getBoneByName(const std::string& name) const;
    void dispose();
    Skeleton& fromJSON(const JsonValue& json,
                       const std::map<std::string, std::shared_ptr<Bone>>& bones);
    JsonValue toJSON() const;
};
