#include "objects/Mesh.h"
std::optional<JsonValue> Mesh::propertyValue(const std::string& property) const {
    if (property == "isMesh")
        return isMesh;
    if (property == "count")
        return count;
    return Object3D::propertyValue(property);
}
Mesh::Mesh(std::shared_ptr<BufferGeometry> g, std::shared_ptr<Material> m)
    : geometry(std::move(g)), material(std::move(m)) {
    isMesh = true;
    type = "Mesh";
    updateMorphTargets();
}
Mesh::Mesh(std::shared_ptr<BufferGeometry> g, std::vector<std::shared_ptr<Material>> ms)
    : Mesh(std::move(g), nullptr) {
    materials = std::move(ms);
    materialIsArray = true;
}
void Mesh::updateMorphTargets() {
    if (!geometry || geometry->morphAttributes.empty())
        return;
    const auto& attributes = geometry->morphAttributes.begin()->second;
    morphTargetInfluences = std::vector<double>{};
    morphTargetDictionary = std::map<std::string, std::size_t>{};
    for (std::size_t i = 0; i < attributes.size(); ++i) {
        morphTargetInfluences->push_back(0);
        (*morphTargetDictionary)[attributes[i]->name.empty() ? std::to_string(i)
                                                             : attributes[i]->name] = i;
    }
}
Vector3& Mesh::getVertexPosition(std::size_t index, Vector3& target) const {
    const auto position = geometry->getAttribute("position");
    target.set(position->getX(index), position->getY(index), position->getZ(index));
    if (auto it = geometry->morphAttributes.find("position");
        it != geometry->morphAttributes.end() && morphTargetInfluences) {
        Vector3 morph;
        for (std::size_t i = 0; i < it->second.size(); ++i) {
            const auto influence = morphTargetInfluences->at(i);
            if (influence == 0)
                continue;
            const auto& a = it->second[i];
            Vector3 v(a->getX(index), a->getY(index), a->getZ(index));
            if (!geometry->morphTargetsRelative)
                v.sub(target);
            morph.addScaledVector(v, influence);
        }
        target.add(morph);
    }
    return target;
}
Object3D& Mesh::copy(const Object3D& source, bool recursive) {
    const auto& s = dynamic_cast<const Mesh&>(source);
    Object3D::copy(source, recursive);
    geometry = s.geometry;
    material = s.material;
    materials = s.materials;
    materialIsArray = s.materialIsArray;
    morphTargetDictionary = s.morphTargetDictionary;
    morphTargetInfluences = s.morphTargetInfluences;
    return *this;
}
std::shared_ptr<Object3D> Mesh::clone(bool recursive) const {
    auto result = std::make_shared<Mesh>();
    result->copy(*this, recursive);
    return result;
}
JsonValue Mesh::toJSON(Object3DJSONContext* meta) const {
    return Object3D::toJSON(meta);
}
void Mesh::serializeObjectResources(Object3DJSONContext& meta, JsonValue& object) const {
    if (!geometry)
        throw std::logic_error("Mesh JSON serialization requires geometry");
    if (morphTargetDictionary) {
        JsonValue dictionary = JsonValue::Object{};
        for (const auto& [name, index] : *morphTargetDictionary)
            dictionary[name] = index;
        object["morphTargetDictionary"] = std::move(dictionary);
    }
    if (morphTargetInfluences) {
        JsonValue::Array influences;
        for (const auto value : *morphTargetInfluences)
            influences.emplace_back(value);
        object["morphTargetInfluences"] = std::move(influences);
    }
    const auto serializedGeometry = geometry;
    meta.cacheResource("geometries", serializedGeometry->uuid,
                       [&] { return serializedGeometry->toJSON(); });
    object["geometry"] = serializedGeometry->uuid;
    if (materialIsArray || !materials.empty()) {
        JsonValue::Array references;
        for (const auto& entry : materials) {
            if (!entry)
                throw std::logic_error("Mesh JSON serialization requires non-null materials");
            const auto serializedMaterial = entry;
            meta.cacheResource("materials", serializedMaterial->uuid,
                               [&] { return serializedMaterial->toJSON(); });
            references.emplace_back(serializedMaterial->uuid);
        }
        object["material"] = std::move(references);
    } else {
        if (!material)
            throw std::logic_error("Mesh JSON serialization requires material");
        const auto serializedMaterial = material;
        meta.cacheResource("materials", serializedMaterial->uuid,
                           [&] { return serializedMaterial->toJSON(); });
        object["material"] = serializedMaterial->uuid;
    }
}
