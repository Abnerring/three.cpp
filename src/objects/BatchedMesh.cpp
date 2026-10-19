#include "objects/BatchedMesh.h"

#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "core/Intersection.h"
#include "core/Raycaster.h"
#include "materials/MeshBasicMaterial.h"
#include <algorithm>
#include <cmath>
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
void BatchedMesh::validateInstanceId(double instanceId) const {
    // Upstream rejects an id past the records and one that has been freed.
    if (instanceId < 0 || instanceId >= static_cast<double>(_instanceInfo.size()) ||
        !_instanceInfo[static_cast<std::size_t>(instanceId)].active)
        throw std::runtime_error("THREE.BatchedMesh: Invalid instanceId.");
}

void BatchedMesh::validateGeometryId(double geometryId) const {
    if (geometryId < 0 || geometryId >= _geometryCount || !_geometryInfo[static_cast<std::size_t>(geometryId)].active)
        throw std::runtime_error("THREE.BatchedMesh: Invalid geometryId.");
}

double BatchedMesh::addInstance(double geometryId) {
    validateGeometryId(geometryId);
    const bool atCapacity = static_cast<double>(_instanceInfo.size()) >= _maxInstanceCount;
    if (atCapacity && _availableInstanceIds.empty())
        throw std::runtime_error("THREE.BatchedMesh: Maximum item count reached.");

    InstanceInfo info;
    info.visible = true;
    info.active = true;
    info.geometryId = geometryId;

    double drawId = -1;
    if (!_availableInstanceIds.empty()) {
        // Freed ids are reused smallest first, which is what the upstream sort does.
        std::sort(_availableInstanceIds.begin(), _availableInstanceIds.end());
        drawId = _availableInstanceIds.front();
        _availableInstanceIds.erase(_availableInstanceIds.begin());
        _instanceInfo[static_cast<std::size_t>(drawId)] = info;
    } else {
        drawId = static_cast<double>(_instanceInfo.size());
        _instanceInfo.push_back(info);
    }

    // Upstream writes an identity matrix and a white colour for the new instance.
    const auto slot = static_cast<std::size_t>(drawId);
    if (_instanceMatrices.size() <= slot) _instanceMatrices.resize(slot + 1, Matrix4());
    if (_instanceColors.size() <= slot) _instanceColors.resize(slot + 1, Color(1.0, 1.0, 1.0));
    _instanceMatrices[slot].identity();
    _instanceColors[slot].setRGB(1.0, 1.0, 1.0);
    return drawId;
}

BatchedMesh& BatchedMesh::setVisibleAt(double instanceId, bool visible) {
    validateInstanceId(instanceId);
    auto& record = _instanceInfo[static_cast<std::size_t>(instanceId)];
    if (record.visible != visible) record.visible = visible;   // upstream returns early when unchanged
    return *this;
}

bool BatchedMesh::getVisibleAt(double instanceId) const {
    validateInstanceId(instanceId);
    return _instanceInfo[static_cast<std::size_t>(instanceId)].visible;
}

BatchedMesh& BatchedMesh::setGeometryIdAt(double instanceId, double geometryId) {
    validateInstanceId(instanceId);
    validateGeometryId(geometryId);
    _instanceInfo[static_cast<std::size_t>(instanceId)].geometryId = geometryId;
    return *this;
}

double BatchedMesh::getGeometryIdAt(double instanceId) const {
    validateInstanceId(instanceId);
    return _instanceInfo[static_cast<std::size_t>(instanceId)].geometryId;
}

BatchedMesh::GeometryInfo BatchedMesh::getGeometryRangeAt(double geometryId) const {
    validateGeometryId(geometryId);
    return _geometryInfo[static_cast<std::size_t>(geometryId)];
}
BatchedMesh& BatchedMesh::setMatrixAt(double instanceId, const Matrix4& matrix) {
    validateInstanceId(instanceId);
    _instanceMatrices[static_cast<std::size_t>(instanceId)] = matrix;
    return *this;
}

Matrix4 BatchedMesh::getMatrixAt(double instanceId) const {
    validateInstanceId(instanceId);
    return _instanceMatrices[static_cast<std::size_t>(instanceId)];
}

BatchedMesh& BatchedMesh::setColorAt(double instanceId, const Color& color) {
    validateInstanceId(instanceId);
    const auto slot = static_cast<std::size_t>(instanceId);
    if (_instanceColors.size() <= slot) _instanceColors.resize(slot + 1, Color(1.0, 1.0, 1.0));
    _instanceColors[slot] = color;
    return *this;
}

Color BatchedMesh::getColorAt(double instanceId) const {
    validateInstanceId(instanceId);
    const auto slot = static_cast<std::size_t>(instanceId);
    // Upstream answers white until the colour texture exists, and every instance starts white here.
    if (slot >= _instanceColors.size()) return Color(1.0, 1.0, 1.0);
    return _instanceColors[slot];
}

BatchedMesh& BatchedMesh::deleteInstance(double instanceId) {
    validateInstanceId(instanceId);
    _instanceInfo[static_cast<std::size_t>(instanceId)].active = false;
    _availableInstanceIds.push_back(instanceId);
    return *this;
}

BatchedMesh& BatchedMesh::deleteGeometry(double geometryId) {
    // Upstream returns quietly for an id that is out of range or already freed, unlike deleteInstance.
    if (geometryId < 0 || geometryId >= _geometryCount) return *this;
    auto& info = _geometryInfo[static_cast<std::size_t>(geometryId)];
    if (!info.active) return *this;
    // Every instance pointing at this geometry goes with it.
    for (std::size_t index = 0; index < _instanceInfo.size(); index += 1) {
        const auto& record = _instanceInfo[index];
        if (record.active && record.geometryId == geometryId) deleteInstance(static_cast<double>(index));
    }
    info.active = false;
    _availableGeometryIds.push_back(geometryId);
    return *this;
}
std::optional<Box3> BatchedMesh::getBoundingBoxAt(double geometryId) {
    if (geometryId < 0 || geometryId >= _geometryCount) return std::nullopt;
    auto& info = _geometryInfo[static_cast<std::size_t>(geometryId)];
    if (!info.boundingBox) {
        // The bounds come from the packed vertices this geometry occupies, following the batch index when
        // there is one, which is exactly what upstream walks.
        Box3 box;
        box.makeEmpty();
        const auto position = std::dynamic_pointer_cast<BufferAttribute>(geometry->getAttribute("position"));
        const auto index = geometry->getIndex();
        for (double i = info.start; i < info.start + info.count; i += 1) {
            const double vertex = index ? index->getX(i) : i;
            box.expandByPoint(Vector3(position->getX(vertex), position->getY(vertex), position->getZ(vertex)));
        }
        info.boundingBox = box;
    }
    return info.boundingBox;
}

std::optional<Sphere> BatchedMesh::getBoundingSphereAt(double geometryId) {
    if (geometryId < 0 || geometryId >= _geometryCount) return std::nullopt;
    auto& info = _geometryInfo[static_cast<std::size_t>(geometryId)];
    if (!info.boundingSphere) {
        // Upstream centres the sphere on the box centre and takes the largest distance to a vertex.
        Sphere sphere;
        const auto box = getBoundingBoxAt(geometryId);
        if (!box) return std::nullopt;
        box->getCenter(sphere.center);
        const auto position = std::dynamic_pointer_cast<BufferAttribute>(geometry->getAttribute("position"));
        const auto index = geometry->getIndex();
        double maxRadiusSq = 0.0;
        for (double i = info.start; i < info.start + info.count; i += 1) {
            const double vertex = index ? index->getX(i) : i;
            const Vector3 point(position->getX(vertex), position->getY(vertex), position->getZ(vertex));
            maxRadiusSq = std::max(maxRadiusSq, sphere.center.distanceToSquared(point));
        }
        sphere.radius = std::sqrt(maxRadiusSq);
        info.boundingSphere = sphere;
    }
    return info.boundingSphere;
}

void BatchedMesh::computeBoundingBox() {
    if (!boundingBox) boundingBox = Box3();
    boundingBox->makeEmpty();
    // Every active instance contributes its geometry bounds transformed by its own matrix.
    for (std::size_t i = 0; i < _instanceInfo.size(); i += 1) {
        if (!_instanceInfo[i].active) continue;
        const auto box = getBoundingBoxAt(_instanceInfo[i].geometryId);
        if (!box) continue;
        Box3 transformed = *box;
        transformed.applyMatrix4(_instanceMatrices[i]);
        boundingBox->unionWith(transformed);
    }
}

void BatchedMesh::computeBoundingSphere() {
    if (!boundingSphere) boundingSphere = Sphere();
    boundingSphere->makeEmpty();
    for (std::size_t i = 0; i < _instanceInfo.size(); i += 1) {
        if (!_instanceInfo[i].active) continue;
        const auto sphere = getBoundingSphereAt(_instanceInfo[i].geometryId);
        if (!sphere) continue;
        Sphere transformed = *sphere;
        transformed.applyMatrix4(_instanceMatrices[i]);
        boundingSphere->unionWith(transformed);
    }
}
BatchedMesh& BatchedMesh::setInstanceCount(double maxInstanceCount) {
    // Trailing free ids are dropped first, so a shrink succeeds whenever the used ids fit.
    std::sort(_availableInstanceIds.begin(), _availableInstanceIds.end());
    while (!_availableInstanceIds.empty() &&
           _availableInstanceIds.back() == static_cast<double>(_instanceInfo.size()) - 1.0) {
        _instanceInfo.pop_back();
        _availableInstanceIds.pop_back();
    }
    if (maxInstanceCount < static_cast<double>(_instanceInfo.size()))
        throw std::runtime_error("THREE.BatchedMesh: Instance ids outside the range " +
                                 std::to_string(static_cast<int>(maxInstanceCount)) +
                                 " are being used. Cannot shrink instance count.");
    // Upstream reallocates its multi draw arrays and two instance textures here; the vectors that stand in for
    // the textures are resized instead, and the records beyond the new count are dropped.
    _maxInstanceCount = maxInstanceCount;
    const auto keep = static_cast<std::size_t>(maxInstanceCount);
    if (_instanceMatrices.size() > keep) _instanceMatrices.resize(keep);
    if (_instanceColors.size() > keep) _instanceColors.resize(keep);
    return *this;
}

BatchedMesh& BatchedMesh::setGeometrySize(double maxVertexCount, double maxIndexCount) {
    // The active ranges decide how far the buffers may shrink.
    double requiredVertexLength = 0.0, requiredIndexLength = 0.0;
    for (const auto& info : _geometryInfo) {
        if (!info.active) continue;
        requiredVertexLength = std::max(requiredVertexLength, info.vertexStart + info.reservedVertexCount);
        requiredIndexLength = std::max(requiredIndexLength, info.indexStart + info.reservedIndexCount);
    }
    if (requiredVertexLength > maxVertexCount)
        // The upstream message names the index count here; it reads odd, and it is copied as it stands.
        throw std::runtime_error("THREE.BatchedMesh: Geometry vertex values are being used outside the range " +
                                 std::to_string(static_cast<int>(maxIndexCount)) + ". Cannot shrink further.");
    if (geometry->getIndex() != nullptr && requiredIndexLength > maxIndexCount)
        throw std::runtime_error("THREE.BatchedMesh: Geometry index values are being used outside the range " +
                                 std::to_string(static_cast<int>(maxIndexCount)) + ". Cannot shrink further.");

    const auto previous = geometry;
    _maxVertexCount = maxVertexCount;
    _maxIndexCount = maxIndexCount;
    if (_geometryInitialized) {
        _geometryInitialized = false;
        geometry = std::make_shared<BufferGeometry>();
        initializeGeometry(previous);
    }
    // Copy what fits from the old buffers into the new ones, attribute by attribute.
    for (const auto& [name, target] : geometry->attributes) {
        const auto destination = std::dynamic_pointer_cast<BufferAttribute>(target);
        const auto origin = std::dynamic_pointer_cast<BufferAttribute>(previous->getAttribute(name));
        if (!destination || !origin) continue;
        const double vertices = std::min(origin->count, destination->count);
        for (double i = 0; i < vertices; i += 1)
            for (double c = 0; c < destination->itemSize && c < origin->itemSize; c += 1)
                destination->setComponent(i, c, origin->getComponent(i, c));
    }
    if (previous->getIndex() != nullptr && geometry->getIndex() != nullptr) {
        const auto destination = geometry->getIndex();
        const auto origin = previous->getIndex();
        const double count = std::min(origin->count, destination->count);
        for (double i = 0; i < count; i += 1) destination->setX(i, origin->getX(i));
    }
    return *this;
}
BatchedMesh& BatchedMesh::copy(const Object3D& source, bool recursive) {
    Mesh::copy(source, recursive);
    const auto& value = dynamic_cast<const BatchedMesh&>(source);
    geometry = value.geometry ? value.geometry->clone() : std::make_shared<BufferGeometry>();
    perObjectFrustumCulled = value.perObjectFrustumCulled;
    sortObjects = value.sortObjects;
    boundingBox = value.boundingBox;
    boundingSphere = value.boundingSphere;
    _geometryInfo = value._geometryInfo;
    _instanceInfo = value._instanceInfo;
    _availableInstanceIds = value._availableInstanceIds;
    _availableGeometryIds = value._availableGeometryIds;
    _nextIndexStart = value._nextIndexStart;
    _nextVertexStart = value._nextVertexStart;
    _geometryCount = value._geometryCount;
    _maxInstanceCount = value._maxInstanceCount;
    _maxVertexCount = value._maxVertexCount;
    _maxIndexCount = value._maxIndexCount;
    _geometryInitialized = value._geometryInitialized;
    _instanceMatrices = value._instanceMatrices;
    _instanceColors = value._instanceColors;
    return *this;
}
bool BatchedMesh::raycast(const Raycaster& raycaster, std::vector<Intersection>& intersects) {
    const auto before = intersects.size();
    Mesh scratch;
    scratch.material = material;
    scratch.geometry = std::make_shared<BufferGeometry>();
    scratch.geometry->attributes = geometry->attributes;
    if (geometry->getIndex() != nullptr) scratch.geometry->setIndex(geometry->getIndex());
    for (std::size_t i = 0; i < _instanceInfo.size(); i += 1) {
        const auto& record = _instanceInfo[i];
        if (!record.visible || !record.active) continue;
        const auto range = _geometryInfo[static_cast<std::size_t>(record.geometryId)];
        scratch.geometry->setDrawRange(range.start, range.count);
        scratch.matrixWorld = matrixWorld;
        scratch.matrixWorld.multiply(_instanceMatrices[i]);
        if (const auto box = getBoundingBoxAt(record.geometryId)) scratch.geometry->boundingBox = box;
        if (const auto sphere = getBoundingSphereAt(record.geometryId)) scratch.geometry->boundingSphere = sphere;
        std::vector<Intersection> hits;
        scratch.raycast(raycaster, hits);
        for (auto& hit : hits) {
            hit.object = this;
            hit.batchId = static_cast<double>(i);
            intersects.push_back(std::move(hit));
        }
    }
    return intersects.size() > before;
}
BatchedMesh& BatchedMesh::optimize() {
    // The ranges are visited in vertex order, because a range can be reused and so need not be sorted.
    std::vector<std::size_t> order(_geometryInfo.size());
    for (std::size_t i = 0; i < order.size(); i += 1) order[i] = i;
    std::sort(order.begin(), order.end(), [this](std::size_t a, std::size_t b) {
        return _geometryInfo[a].vertexStart < _geometryInfo[b].vertexStart;
    });

    const auto index = geometry->getIndex();
    double nextVertexStart = 0.0, nextIndexStart = 0.0;
    for (const auto id : order) {
        auto& info = _geometryInfo[id];
        if (!info.active) continue;
        if (index) {
            if (info.indexStart != nextIndexStart) {
                // The index values are rebased on the vertex move before the block itself is copied, so the
                // original start is still available when the delta is taken.
                const double delta = nextVertexStart - info.vertexStart;
                const auto count = static_cast<std::size_t>(info.reservedIndexCount);
                std::vector<double> shifted(count);
                for (std::size_t i = 0; i < count; i += 1) shifted[i] = index->getX(info.indexStart + static_cast<double>(i)) + delta;
                for (std::size_t i = 0; i < count; i += 1) index->setX(nextIndexStart + static_cast<double>(i), shifted[i]);
                info.indexStart = nextIndexStart;
            }
            nextIndexStart += info.reservedIndexCount;
        }
        if (info.vertexStart != nextVertexStart) {
            for (const auto& [name, target] : geometry->attributes) {
                const auto attribute = std::dynamic_pointer_cast<BufferAttribute>(target);
                if (!attribute) continue;
                const auto vertices = static_cast<std::size_t>(info.reservedVertexCount);
                const auto itemSize = static_cast<std::size_t>(attribute->itemSize);
                // A temporary keeps the move safe when the destination overlaps the source.
                std::vector<double> moved(vertices * itemSize);
                for (std::size_t v = 0; v < vertices; v += 1)
                    for (std::size_t c = 0; c < itemSize; c += 1)
                        moved[v * itemSize + c] = attribute->getComponent(info.vertexStart + static_cast<double>(v), static_cast<double>(c));
                for (std::size_t v = 0; v < vertices; v += 1)
                    for (std::size_t c = 0; c < itemSize; c += 1)
                        attribute->setComponent(nextVertexStart + static_cast<double>(v), static_cast<double>(c), moved[v * itemSize + c]);
            }
            info.vertexStart = nextVertexStart;
        }
        nextVertexStart += info.reservedVertexCount;
        info.start = index ? info.indexStart : info.vertexStart;
    }
    _nextIndexStart = nextIndexStart;
    _nextVertexStart = nextVertexStart;
    return *this;
}