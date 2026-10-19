#include "objects/BatchedMesh.h"

#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include <algorithm>
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
double BatchedMesh::addGeometry(const std::shared_ptr<BufferGeometry>& geometry, double reservedVertexCount,
                                double reservedIndexCount) {
    initializeGeometry(geometry);
    validateGeometry(geometry);

    GeometryInfo info;
    info.vertexStart = _nextVertexStart;
    const auto position = std::dynamic_pointer_cast<BufferAttribute>(geometry->getAttribute("position"));
    info.reservedVertexCount = reservedVertexCount == -1 ? position->count : reservedVertexCount;

    const auto index = geometry->getIndex();
    const bool hasIndex = index != nullptr;
    if (hasIndex) {
        info.indexStart = _nextIndexStart;
        info.reservedIndexCount = reservedIndexCount == -1 ? index->count : reservedIndexCount;
    }

    // The reserved space has to fit in the buffers the batch allocated when it was built.
    if ((info.indexStart != -1 && info.indexStart + info.reservedIndexCount > _maxIndexCount) ||
        info.vertexStart + info.reservedVertexCount > _maxVertexCount)
        throw std::runtime_error("THREE.BatchedMesh: Reserved space request exceeds the maximum buffer size.");

    // A freed id is reused, and the smallest one goes first, which is what the upstream sort does.
    double geometryId = -1;
    if (!_availableGeometryIds.empty()) {
        std::sort(_availableGeometryIds.begin(), _availableGeometryIds.end());
        geometryId = _availableGeometryIds.front();
        _availableGeometryIds.erase(_availableGeometryIds.begin());
        _geometryInfo[static_cast<std::size_t>(geometryId)] = info;
    } else {
        geometryId = _geometryCount;
        _geometryCount += 1;
        _geometryInfo.push_back(info);
    }

    setGeometryAt(geometryId, geometry);
    _nextIndexStart = info.indexStart + info.reservedIndexCount;
    _nextVertexStart = info.vertexStart + info.reservedVertexCount;
    return geometryId;
}

double BatchedMesh::setGeometryAt(double geometryId, const std::shared_ptr<BufferGeometry>& source) {
    if (geometryId >= _geometryCount)
        throw std::runtime_error("THREE.BatchedMesh: Maximum geometry count reached.");
    validateGeometry(source);

    const bool hasIndex = geometry->getIndex() != nullptr;
    const auto destinationIndex = geometry->getIndex();
    const auto sourceIndex = source->getIndex();
    auto& info = _geometryInfo[static_cast<std::size_t>(geometryId)];
    if ((hasIndex && sourceIndex->count > info.reservedIndexCount) ||
        std::dynamic_pointer_cast<BufferAttribute>(source->getAttribute("position"))->count > info.reservedVertexCount)
        throw std::runtime_error("THREE.BatchedMesh: Reserved space not large enough for provided geometry.");

    const double vertexStart = info.vertexStart;
    const double reservedVertexCount = info.reservedVertexCount;
    const auto sourcePosition = std::dynamic_pointer_cast<BufferAttribute>(source->getAttribute("position"));
    info.vertexCount = sourcePosition->count;

    for (const auto& [name, target] : geometry->attributes) {
        const auto destination = std::dynamic_pointer_cast<BufferAttribute>(target);
        const auto origin = std::dynamic_pointer_cast<BufferAttribute>(source->getAttribute(name));
        // Copy the source components, then fill the reserved tail with zeroes, exactly as upstream does.
        for (double i = 0; i < origin->count; i += 1)
            for (double c = 0; c < origin->itemSize; c += 1)
                destination->setComponent(vertexStart + i, c, origin->getComponent(i, c));
        for (double i = origin->count; i < reservedVertexCount; i += 1)
            for (double c = 0; c < origin->itemSize; c += 1)
                destination->setComponent(vertexStart + i, c, 0.0);
    }

    if (hasIndex) {
        const double indexStart = info.indexStart;
        const double reservedIndexCount = info.reservedIndexCount;
        info.indexCount = sourceIndex->count;
        // The batch index is rebased on the vertex range this geometry occupies.
        for (double i = 0; i < sourceIndex->count; i += 1)
            destinationIndex->setX(indexStart + i, vertexStart + sourceIndex->getX(i));
        for (double i = sourceIndex->count; i < reservedIndexCount; i += 1)
            destinationIndex->setX(indexStart + i, vertexStart);
    }

    info.start = hasIndex ? info.indexStart : info.vertexStart;
    info.count = hasIndex ? info.indexCount : info.vertexCount;
    info.boundingBox = source->boundingBox;
    info.boundingSphere = source->boundingSphere;
    return geometryId;
}