#include "objects/Skeleton.h"
#include "math/MathUtils.h"
#include <cmath>
#include <iostream>

namespace {
std::shared_ptr<Bone> boneAt(const Skeleton::Bones& values, std::size_t i) {
    const auto storage = values.storage();
    return i < storage->values.size() && !storage->holes.contains(i) ? storage->values[i] : nullptr;
}
const Matrix4& inverseAt(const Skeleton::Inverses& values, std::size_t i) {
    if (i >= values.size() || values.storage()->holes.contains(i) || !values[i])
        throw JSTypeError("Cannot read properties of undefined (reading 'elements')");
    return *values[i];
}
JsonValue matrixJSON(const Matrix4& matrix) {
    JsonValue::Array result;
    for (const auto value : matrix.elements()) result.emplace_back(value);
    return result;
}
}
Skeleton::Skeleton(Bones inputBones, Inverses inverses)
    : uuid(MathUtils::generateUUID()), bones(inputBones.slice()), boneInverses(std::move(inverses)) {
    init();
}
void Skeleton::init() {
    boneMatrices = Float32Array(double(bones.size()) * 16);
    if (boneInverses.empty()) calculateInverses();
    else if (bones.size() != boneInverses.size()) {
        std::cerr << "THREE.Skeleton: Number of inverse bone matrices does not match amount of bones.\n";
        boneInverses = Inverses();
        for (std::size_t i = 0, length = bones.size(); i < length; ++i)
            boneInverses.push_back(std::make_shared<Matrix4>());
    }
}
void Skeleton::calculateInverses() {
    boneInverses.clear();
    for (std::size_t i = 0, length = bones.size(); i < length; ++i) {
        auto inverse = std::make_shared<Matrix4>();
        if (const auto bone = boneAt(bones, i)) inverse->copy(bone->matrixWorld).invert();
        boneInverses.push_back(std::move(inverse));
    }
}
void Skeleton::pose() {
    for (std::size_t i = 0, length = bones.size(); i < length; ++i)
        if (const auto bone = boneAt(bones, i)) bone->matrixWorld.copy(inverseAt(boneInverses, i)).invert();
    for (std::size_t i = 0, length = bones.size(); i < length; ++i) {
        const auto bone = boneAt(bones, i);
        if (!bone) continue;
        if (bone->parent && bone->parent->isBone)
            bone->matrix.copy(bone->parent->matrixWorld).invert().multiply(bone->matrixWorld);
        else bone->matrix.copy(bone->matrixWorld);
        bone->matrix.decompose(bone->position, bone->quaternion, bone->scale);
    }
}
void Skeleton::update() {
    const auto currentBones = bones;
    const auto inverses = boneInverses;
    auto matrices = boneMatrices;
    const auto texture = boneTexture;
    const Matrix4 identity;
    Matrix4 offset;
    for (std::size_t i = 0, length = currentBones.size(); i < length; ++i) {
        const auto bone = boneAt(currentBones, i);
        offset.multiplyMatrices(bone ? bone->matrixWorld : identity, inverseAt(inverses, i));
        const auto elements = offset.elements();
        for (std::size_t j = 0; j < 16; ++j) matrices.setIndex(double(i * 16 + j), elements[j]);
    }
    if (texture) texture->needsUpdate = true;
}
std::shared_ptr<Skeleton> Skeleton::clone() const { return std::make_shared<Skeleton>(bones, boneInverses); }
Skeleton& Skeleton::computeBoneTexture() {
    const double size = std::max(std::ceil(std::sqrt(double(bones.size()) * 4) / 4) * 4, 4.0);
    Float32Array matrices(size * size * 4);
    matrices.set(boneMatrices);
    auto texture = std::make_shared<DataTexture>(matrices, size, size, RGBAFormat, FloatType);
    texture->needsUpdate = true;
    boneMatrices = matrices;
    boneTexture = std::move(texture);
    return *this;
}
std::shared_ptr<Bone> Skeleton::getBoneByName(const std::string& name) const {
    for (std::size_t i = 0, length = bones.size(); i < length; ++i) {
        const auto bone = boneAt(bones, i);
        if (!bone) throw JSTypeError("Cannot read properties of null (reading 'name')");
        if (bone->name == name) return bone;
    }
    return nullptr;
}
void Skeleton::dispose() {
    if (boneTexture) {
        const auto texture = boneTexture;
        texture->dispose();
        boneTexture.reset();
    }
}
Skeleton& Skeleton::fromJSON(const JsonValue& json,
                            const std::map<std::string, std::shared_ptr<Bone>>& lookup) {
    uuid = json.at("uuid").get<std::string>();
    const auto& ids = json.at("bones").get<JsonValue::Array>();
    const auto& inverses = json.at("boneInverses").get<JsonValue::Array>();
    for (std::size_t i = 0, length = ids.size(); i < length; ++i) {
        const auto id = ids[i].get<std::string>();
        const auto found = lookup.find(id);
        std::shared_ptr<Bone> bone;
        if (found == lookup.end()) {
            std::cerr << "THREE.Skeleton: No bone found with UUID: " << id << '\n';
            bone = std::make_shared<Bone>();
        } else bone = found->second;
        bones.push_back(std::move(bone));
        if (i >= inverses.size()) throw JSTypeError("Cannot read properties of undefined (reading '0')");
        const auto& values = inverses[i].get<JsonValue::Array>();
        auto inverse = std::make_shared<Matrix4>();
        for (std::size_t j = 0; j < 16; ++j)
            inverse->elements()[j] = j < values.size() ? values[j].get<double>() : std::numeric_limits<double>::quiet_NaN();
        boneInverses.push_back(std::move(inverse));
    }
    init();
    return *this;
}
JsonValue Skeleton::toJSON() const {
    JsonValue result = JsonValue::Object{
        {"metadata", JsonValue::Object{{"version", 4.7}, {"type", "Skeleton"}, {"generator", "Skeleton.toJSON"}}},
        {"bones", JsonValue::Array{}}, {"boneInverses", JsonValue::Array{}}, {"uuid", uuid}};
    for (std::size_t i = 0, length = bones.size(); i < length; ++i) {
        const auto bone = boneAt(bones, i);
        if (!bone) throw JSTypeError("Cannot read properties of null (reading 'uuid')");
        result["bones"].get<JsonValue::Array>().emplace_back(bone->uuid);
        result["boneInverses"].get<JsonValue::Array>().push_back(matrixJSON(inverseAt(boneInverses, i)));
    }
    return result;
}
