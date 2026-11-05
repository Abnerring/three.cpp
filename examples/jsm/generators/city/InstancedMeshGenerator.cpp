#include "generators/city/InstancedMeshGenerator.h"

#include "core/JSValue.h"
#include "math/MathUtils.h"

#include <algorithm>
#include <utility>

InstancedMeshGenerator::InstancedMeshGenerator(
    JsonValue parameters,
    std::function<std::shared_ptr<BufferGeometry>(const JsonValue&)> createGeometry,
    std::function<std::shared_ptr<Material>()> createMaterial, std::string name, bool receiveShadow,
    JsonValue defaults)
    : parameters(std::move(defaults)),
      createGeometry(std::move(createGeometry)),
      createMaterial(std::move(createMaterial)),
      name(std::move(name)),
      _receiveShadow(receiveShadow) {
    // Object.assign( {}, defaults, parameters ): the parameters overwrite the defaults.
    if (parameters.isObject()) {
        for (const auto& entry : parameters.get<JsonValue::Object>()) {
            this->parameters[entry.first] = entry.second;
        }
    }
}

std::shared_ptr<InstancedMesh> InstancedMeshGenerator::build(const std::vector<Matrix4>& placements) {
    // const parametersKey = JSON.stringify( this.parameters )
    const std::string parametersKey = parameters.dump();
    const bool outgrewCapacity =
        mesh && placements.size() > static_cast<std::size_t>(mesh->instanceMatrix->getCount());
    if (parametersKey != parametersKey_ || outgrewCapacity) {
        dispose();
        parametersKey_ = parametersKey;
    }
    if (geometry == nullptr && createGeometry) geometry = createGeometry(parameters);
    if (material == nullptr && createMaterial) material = createMaterial();
    if (mesh == nullptr) {
        mesh = createInstances(geometry, material, placements.size(), name);
        mesh->receiveShadow = _receiveShadow;
    }
    updateInstances(mesh, placements);
    return mesh;
}

void InstancedMeshGenerator::dispose() {
    if (geometry) geometry->dispose();
    if (mesh) mesh->dispose();
    if (material) material->dispose();
    geometry = nullptr;
    material = nullptr;
    mesh = nullptr;
}

std::shared_ptr<InstancedMesh> createInstances(const std::shared_ptr<BufferGeometry>& geometry,
                                               const std::shared_ptr<Material>& material,
                                               std::size_t count, const std::string& name) {
    const double capacity =
        MathUtils::ceilPowerOfTwo(static_cast<double>(std::max<std::size_t>(1, count)));
    auto mesh = std::make_shared<InstancedMesh>(geometry, material, JSValue(capacity));
    mesh->castShadow = true;
    mesh->receiveShadow = true;
    mesh->name = name;
    return mesh;
}

void updateInstances(const std::shared_ptr<InstancedMesh>& mesh,
                     const std::vector<Matrix4>& placements) {
    mesh->count = JSValue(static_cast<double>(placements.size()));
    mesh->visible = !placements.empty();
    for (std::size_t i = 0; i < placements.size(); ++i) {
        mesh->setMatrixAt(static_cast<double>(i), placements[i]);
    }
    mesh->instanceMatrix->needsUpdate = JSValue(true);
    mesh->boundingBox = nullptr;
    mesh->computeBoundingSphere();
}