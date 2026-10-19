#include "objects/BatchedMesh.h"

#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include <stdexcept>
#include <string>

BatchedMesh::BatchedMesh(double maxInstanceCount, double maxVertexCount, double maxIndexCount,
                         std::shared_ptr<Material> material)
    : Mesh(std::make_shared<BufferGeometry>(), material ? material : std::make_shared<MeshBasicMaterial>()),
      _maxInstanceCount(maxInstanceCount), _maxVertexCount(maxVertexCount),
      _maxIndexCount(maxIndexCount < 0 ? maxVertexCount * 2 : maxIndexCount) {}

void BatchedMesh::initializeGeometry(const std::shared_ptr<BufferGeometry>& reference) {
    // Upstream sizes every batch attribute from the reference one, keeping its element type, item size and
    // normalized flag, so the batch and the geometry it later accepts agree on all three.
    if (_geometryInitialized) return;
    for (const auto& [name, source] : reference->attributes) {
        const auto attribute = std::dynamic_pointer_cast<BufferAttribute>(source);
        if (!attribute) throw std::runtime_error("BatchedMesh: only buffer attributes can be batched");
        const auto length = static_cast<double>(_maxVertexCount) * attribute->itemSize;
        const auto values = std::vector<double>(static_cast<std::size_t>(length), 0.0);
        auto created = std::make_shared<Float32BufferAttribute>(std::vector<float>(values.begin(), values.end()),
                                                               attribute->itemSize);
        created->normalized = attribute->normalized;
        geometry->setAttribute(name, created);
    }
    if (reference->getIndex() != nullptr) {
        // Upstream reserves the last sixteen bit index for primitive restart, so past that point the batch
        // index has to be thirty two bits wide.
        const auto count = static_cast<std::size_t>(_maxIndexCount);
        if (_maxVertexCount > 65535)
            geometry->setIndex(std::make_shared<Uint32BufferAttribute>(std::vector<std::uint32_t>(count, 0), 1));
        else
            geometry->setIndex(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>(count, 0), 1));
    }
    _geometryInitialized = true;
}

void BatchedMesh::validateGeometry(const std::shared_ptr<BufferGeometry>& candidate) const {
    // Every geometry in a batch has to agree with the batch on whether it is indexed, which attributes it
    // carries, and their item size and normalized flag. Upstream reports each disagreement separately and so
    // does this, with the same wording.
    if ((candidate->getIndex() != nullptr) != (geometry->getIndex() != nullptr))
        throw std::runtime_error("THREE.BatchedMesh: All geometries must consistently have \"index\".");
    for (const auto& [name, source] : geometry->attributes) {
        if (!candidate->hasAttribute(name))
            throw std::runtime_error("THREE.BatchedMesh: Added geometry missing \"" + name +
                                     "\". All geometries must have consistent attributes.");
        const auto batch = std::dynamic_pointer_cast<BufferAttribute>(source);
        const auto other = std::dynamic_pointer_cast<BufferAttribute>(candidate->getAttribute(name));
        if (!batch || !other) throw std::runtime_error("BatchedMesh: only buffer attributes can be batched");
        if (other->itemSize != batch->itemSize || other->normalized != batch->normalized)
            throw std::runtime_error("THREE.BatchedMesh: Added geometry attribute \"" + name +
                                     "\" has an inconsistent itemSize or normalized value.");
    }
}