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
// Complete native managed source class; original typed renderer adapter remains above.
#include "core/JSGlobal.h"
#include "core/JSModuleObject.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/JSError.h"
#include "core/NativeObject.h"
#include "core/InstancedBufferAttribute.h"
#include "core/BufferGeometry.h"
#include "math/JSNumberMath.h"
#include "math/Matrix4.h"
#include "math/Box3.h"
#include "math/Sphere.h"
#include "math/Frustum.h"
#include "math/FrustumArray.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Color.h"
#include "math/ColorManagement.h"
#include "math/Ray.h"
#include "objects/Mesh.h"
#include "textures/DataTexture.h"
#include "constants.h"
#include "utils.h"
#include <string_view>
#include <unordered_set>
#include "core/JSPromise.h"
#include "objects/BatchedMeshManagedSource.inc"
namespace BatchedMeshManaged {

using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
void own(JSValue object,const JSValue& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
void setReference(JSValue base,const JSValue& key,const JSValue& value){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot assign property of null/undefined");const auto property=key.toPropertyKey();if(base.isObject()){base.set(property,value);return;}auto object=base.toObject();if(!object.reflectSet(property,value,base))throw JSTypeError("Cannot assign primitive property");}
JSValue deleteReference(JSValue base,const JSValue& key){if(base.isNull()||base.isUndefined())throw JSTypeError("Cannot delete property of null/undefined");const auto property=key.toPropertyKey();auto object=base.toObject();if(!object.erase(property))throw JSTypeError("Cannot delete nonconfigurable property");return true;}
JSValue argumentObject(const Args& args){auto object=JSValue::object();JSPropertyDescriptor length;length.value=args.size();length.writable=length.configurable=true;object.defineProperty("length",length);for(unsigned i=0;i<args.size();++i)own(object,JSValue(i),args[i]);return object;}
JSValue typeOf(const JSValue& value){if(value.isUndefined())return "undefined";if(value.isCallable())return "function";if(value.isSymbol())return "symbol";switch(value.type()){case JSValue::Type::Boolean:return "boolean";case JSValue::Type::Number:return "number";case JSValue::Type::String:return "string";default:return "object";}}
JSValue binary(std::string_view op,const JSValue& left,const JSValue& right){
 if(op=="in")return right.has(left);if(op=="===")return left.strictEquals(right);if(op=="!==")return !left.strictEquals(right);
 if(op=="+"){const auto a=left.toPrimitive();const auto b=right.toPrimitive();if(a.type()==JSValue::Type::String||b.type()==JSValue::Type::String){const auto x=a.toString();const auto y=b.toString();return x+y;}const auto x=a.toNumber();const auto y=b.toNumber();return x+y;}
 if(op=="<"||op==">"||op=="<="||op==">="){const auto a=left.toPrimitive(JSValue::PreferredType::Number);const auto b=right.toPrimitive(JSValue::PreferredType::Number);if(a.type()==JSValue::Type::String&&b.type()==JSValue::Type::String){const auto x=jsUTF16(a.string()),y=jsUTF16(b.string());if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}const auto x=a.toNumber();const auto y=b.toNumber();if(op=="<")return x<y;if(op==">")return x>y;if(op=="<=")return x<=y;return x>=y;}
 const auto a=left.toNumber();const auto b=right.toNumber();if(op=="-")return a-b;if(op=="*")return a*b;if(op=="/")return a/b;if(op=="**")return JSNumberMath::pow(a,b);throw std::logic_error("Untranslated arithmetic operator");
}
void appendSpread(Args& values,const JSValue& iterable){auto iterator=getJSIterator(iterable);while(const auto value=stepJSIterator(iterator))values.push_back(*value);}
void copyDataProperties(JSValue target,JSValue source){if(source.isNull()||source.isUndefined())return;if(!source.isObject())source=source.toObject();for(const auto& key:source.ownPropertyKeys()){const auto d=source.getOwnPropertyDescriptor(key);if(d&&d->enumerable.value_or(false)){const auto value=source.get(key);own(target,key,value);}}}

class ForInCursor{
 JSValue receiver;std::vector<std::string> keys;std::size_t index=0;
 public:explicit ForInCursor(JSValue source):receiver(source.isNull()||source.isUndefined()||source.isSymbol()?JSValue(nullptr):source.toObject()){
 std::unordered_set<std::string> visited;auto level=receiver;
 while(!level.isNull()){const bool proxy=level.isProxy();for(const auto&key:level.ownPropertyNames()){if(visited.contains(key))continue;if(proxy){visited.insert(key);keys.push_back(key);}else{const auto d=level.getOwnPropertyDescriptor(key);if(!d)continue;visited.insert(key);if(d->enumerable.value_or(false))keys.push_back(key);}}level=level.getPrototype();}}
 std::optional<JSValue> next(){while(index<keys.size()){const auto key=keys[index++];auto owner=receiver;while(!owner.isNull()){const auto d=owner.getOwnPropertyDescriptor(key);if(d){if(!owner.isProxy()||d->enumerable.value_or(false))return JSValue(key);break;}if(owner.isHostObject("three.AnimationArray")&&(key=="-0"||JSValue(JSValue(key).toNumber()).toString()==key))break;owner=owner.getPrototype();}}return std::nullopt;}
};

JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
#include "objects/BatchedMeshManagedStatements.inc"
}
JSValue nativeBatchedMeshModule(){using namespace BatchedMeshManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;static thread_local bool initializing=false;JSModuleInitializationScope initialization(initializing,"BatchedMesh module");auto module=JSValue::object(nullptr);
{auto fn=JSValue::constructor(helper_ascIdSort,{module},"ascIdSort",2,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText(BatchedMeshManagedSource::helper_ascIdSort);own(module,"ascIdSort",fn);}
{auto fn=JSValue::constructor(helper_sortOpaque,{module},"sortOpaque",2,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText(BatchedMeshManagedSource::helper_sortOpaque);own(module,"sortOpaque",fn);}
{auto fn=JSValue::constructor(helper_sortTransparent,{module},"sortTransparent",2,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText(BatchedMeshManagedSource::helper_sortTransparent);own(module,"sortTransparent",fn);}
{auto fn=JSValue::constructor(helper_copyAttributeData,{module},"copyAttributeData",2,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText(BatchedMeshManagedSource::helper_copyAttributeData);own(module,"copyAttributeData",fn);}
{auto fn=JSValue::constructor(helper_copyArrayContents,{module},"copyArrayContents",2,JSValue::ConstructorKind::Callable);fn.setFunctionSourceText(BatchedMeshManagedSource::helper_copyArrayContents);own(module,"copyArrayContents",fn);}
own(module,"BufferAttribute",nativeBufferAttributeConstructor());
own(module,"BufferGeometry",nativeBufferGeometryConstructor());
own(module,"DataTexture",nativeDataTextureConstructor());
own(module,"FloatType",nativeConstantsBinding("FloatType"));
own(module,"RedIntegerFormat",nativeConstantsBinding("RedIntegerFormat"));
own(module,"UnsignedIntType",nativeConstantsBinding("UnsignedIntType"));
own(module,"RGBAFormat",nativeConstantsBinding("RGBAFormat"));
own(module,"Matrix4",nativeMatrix4Constructor());
own(module,"Mesh",nativeMeshConstructor());
own(module,"ColorManagement",nativeColorManagement());
own(module,"Box3",nativeBox3Constructor());
own(module,"Sphere",nativeSphereConstructor());
own(module,"Frustum",nativeFrustumConstructor());
own(module,"Vector3",nativeVector3Constructor());
own(module,"Color",nativeColorConstructor());
own(module,"FrustumArray",nativeFrustumArrayConstructor());
{auto ctor=JSValue::constructor(MultiDrawRenderListMethod0,{module,JSValue{}},"MultiDrawRenderList",0);ctor.setFunctionSourceText(BatchedMeshManagedSource::MultiDrawRenderListClass);auto prototype=ctor.get("prototype");own(module,"MultiDrawRenderList",ctor);
{auto fn=JSValue::function(MultiDrawRenderListMethod1,{module,prototype},"push",4);fn.setFunctionSourceText(BatchedMeshManagedSource::MultiDrawRenderListMethod1);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("push",d);}
{auto fn=JSValue::function(MultiDrawRenderListMethod2,{module,prototype},"reset",0);fn.setFunctionSourceText(BatchedMeshManagedSource::MultiDrawRenderListMethod2);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("reset",d);}
}
own(module,"_matrix",([&]()->JSValue{const auto nativeStatementTemp3534=module.get("Matrix4");Args nativeStatementTemp3535;return nativeStatementTemp3534.construct(nativeStatementTemp3535);}()));
own(module,"_whiteColor",([&]()->JSValue{const auto nativeStatementTemp3536=module.get("Color");Args nativeStatementTemp3537;nativeStatementTemp3537.push_back(JSValue(1));nativeStatementTemp3537.push_back(JSValue(1));nativeStatementTemp3537.push_back(JSValue(1));return nativeStatementTemp3536.construct(nativeStatementTemp3537);}()));
own(module,"_frustum",([&]()->JSValue{const auto nativeStatementTemp3538=module.get("Frustum");Args nativeStatementTemp3539;return nativeStatementTemp3538.construct(nativeStatementTemp3539);}()));
own(module,"_frustumArray",([&]()->JSValue{const auto nativeStatementTemp3540=module.get("FrustumArray");Args nativeStatementTemp3541;return nativeStatementTemp3540.construct(nativeStatementTemp3541);}()));
own(module,"_box",([&]()->JSValue{const auto nativeStatementTemp3542=module.get("Box3");Args nativeStatementTemp3543;return nativeStatementTemp3542.construct(nativeStatementTemp3543);}()));
own(module,"_sphere",([&]()->JSValue{const auto nativeStatementTemp3544=module.get("Sphere");Args nativeStatementTemp3545;return nativeStatementTemp3544.construct(nativeStatementTemp3545);}()));
own(module,"_vector",([&]()->JSValue{const auto nativeStatementTemp3546=module.get("Vector3");Args nativeStatementTemp3547;return nativeStatementTemp3546.construct(nativeStatementTemp3547);}()));
own(module,"_cameraPosition",([&]()->JSValue{const auto nativeStatementTemp3548=module.get("Vector3");Args nativeStatementTemp3549;return nativeStatementTemp3548.construct(nativeStatementTemp3549);}()));
own(module,"_forward",([&]()->JSValue{const auto nativeStatementTemp3550=module.get("Vector3");Args nativeStatementTemp3551;return nativeStatementTemp3550.construct(nativeStatementTemp3551);}()));
own(module,"_temp",([&]()->JSValue{const auto nativeStatementTemp3552=module.get("Vector3");Args nativeStatementTemp3553;return nativeStatementTemp3552.construct(nativeStatementTemp3553);}()));
own(module,"_renderList",([&]()->JSValue{const auto nativeStatementTemp3554=module.get("MultiDrawRenderList");Args nativeStatementTemp3555;return nativeStatementTemp3554.construct(nativeStatementTemp3555);}()));
own(module,"_mesh",([&]()->JSValue{const auto nativeStatementTemp3556=module.get("Mesh");Args nativeStatementTemp3557;return nativeStatementTemp3556.construct(nativeStatementTemp3557);}()));
own(module,"_batchIntersects",([&]()->JSValue{auto nativeStatementTemp3558=JSValue::array();return nativeStatementTemp3558;}()));
{auto ctor=JSValue::builtinConstructor(BatchedMeshMethod0,{module,JSValue{}},"BatchedMesh",2);ctor.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshClass);auto prototype=ctor.get("prototype");own(module,"BatchedMesh",ctor);
ctor.setPrototype(module.get("Mesh"));prototype.setPrototype(module.get("Mesh").get("prototype"));
{auto fn=JSValue::function(BatchedMeshMethod1,{module,prototype},"get maxInstanceCount",0);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod1);JSPropertyDescriptor d;d.configurable=true;d.getter=fn;prototype.defineProperty("maxInstanceCount",d);}
{auto fn=JSValue::function(BatchedMeshMethod2,{module,prototype},"get instanceCount",0);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod2);JSPropertyDescriptor d;d.configurable=true;d.getter=fn;prototype.defineProperty("instanceCount",d);}
{auto fn=JSValue::function(BatchedMeshMethod3,{module,prototype},"get unusedVertexCount",0);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod3);JSPropertyDescriptor d;d.configurable=true;d.getter=fn;prototype.defineProperty("unusedVertexCount",d);}
{auto fn=JSValue::function(BatchedMeshMethod4,{module,prototype},"get unusedIndexCount",0);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod4);JSPropertyDescriptor d;d.configurable=true;d.getter=fn;prototype.defineProperty("unusedIndexCount",d);}
{auto fn=JSValue::function(BatchedMeshMethod5,{module,prototype},"_initMatricesTexture",0);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod5);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("_initMatricesTexture",d);}
{auto fn=JSValue::function(BatchedMeshMethod6,{module,prototype},"_initIndirectTexture",0);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod6);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("_initIndirectTexture",d);}
{auto fn=JSValue::function(BatchedMeshMethod7,{module,prototype},"_initColorsTexture",0);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod7);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("_initColorsTexture",d);}
{auto fn=JSValue::function(BatchedMeshMethod8,{module,prototype},"_initializeGeometry",1);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod8);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("_initializeGeometry",d);}
{auto fn=JSValue::function(BatchedMeshMethod9,{module,prototype},"_validateGeometry",1);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod9);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("_validateGeometry",d);}
{auto fn=JSValue::function(BatchedMeshMethod10,{module,prototype},"validateInstanceId",1);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod10);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("validateInstanceId",d);}
{auto fn=JSValue::function(BatchedMeshMethod11,{module,prototype},"validateGeometryId",1);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod11);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("validateGeometryId",d);}
{auto fn=JSValue::function(BatchedMeshMethod12,{module,prototype},"setCustomSort",1);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod12);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("setCustomSort",d);}
{auto fn=JSValue::function(BatchedMeshMethod13,{module,prototype},"computeBoundingBox",0);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod13);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("computeBoundingBox",d);}
{auto fn=JSValue::function(BatchedMeshMethod14,{module,prototype},"computeBoundingSphere",0);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod14);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("computeBoundingSphere",d);}
{auto fn=JSValue::function(BatchedMeshMethod15,{module,prototype},"addInstance",1);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod15);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("addInstance",d);}
{auto fn=JSValue::function(BatchedMeshMethod16,{module,prototype},"addGeometry",1);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod16);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("addGeometry",d);}
{auto fn=JSValue::function(BatchedMeshMethod17,{module,prototype},"setGeometryAt",2);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod17);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("setGeometryAt",d);}
{auto fn=JSValue::function(BatchedMeshMethod18,{module,prototype},"deleteGeometry",1);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod18);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("deleteGeometry",d);}
{auto fn=JSValue::function(BatchedMeshMethod19,{module,prototype},"deleteInstance",1);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod19);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("deleteInstance",d);}
{auto fn=JSValue::function(BatchedMeshMethod20,{module,prototype},"optimize",0);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod20);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("optimize",d);}
{auto fn=JSValue::function(BatchedMeshMethod21,{module,prototype},"getBoundingBoxAt",2);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod21);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getBoundingBoxAt",d);}
{auto fn=JSValue::function(BatchedMeshMethod22,{module,prototype},"getBoundingSphereAt",2);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod22);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getBoundingSphereAt",d);}
{auto fn=JSValue::function(BatchedMeshMethod23,{module,prototype},"setMatrixAt",2);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod23);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("setMatrixAt",d);}
{auto fn=JSValue::function(BatchedMeshMethod24,{module,prototype},"getMatrixAt",2);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod24);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getMatrixAt",d);}
{auto fn=JSValue::function(BatchedMeshMethod25,{module,prototype},"setColorAt",2);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod25);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("setColorAt",d);}
{auto fn=JSValue::function(BatchedMeshMethod26,{module,prototype},"getColorAt",2);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod26);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getColorAt",d);}
{auto fn=JSValue::function(BatchedMeshMethod27,{module,prototype},"setVisibleAt",2);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod27);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("setVisibleAt",d);}
{auto fn=JSValue::function(BatchedMeshMethod28,{module,prototype},"getVisibleAt",1);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod28);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getVisibleAt",d);}
{auto fn=JSValue::function(BatchedMeshMethod29,{module,prototype},"setGeometryIdAt",2);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod29);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("setGeometryIdAt",d);}
{auto fn=JSValue::function(BatchedMeshMethod30,{module,prototype},"getGeometryIdAt",1);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod30);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getGeometryIdAt",d);}
{auto fn=JSValue::function(BatchedMeshMethod31,{module,prototype},"getGeometryRangeAt",1);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod31);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("getGeometryRangeAt",d);}
{auto fn=JSValue::function(BatchedMeshMethod32,{module,prototype},"setInstanceCount",1);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod32);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("setInstanceCount",d);}
{auto fn=JSValue::function(BatchedMeshMethod33,{module,prototype},"setGeometrySize",2);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod33);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("setGeometrySize",d);}
{auto fn=JSValue::function(BatchedMeshMethod34,{module,prototype},"raycast",2);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod34);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("raycast",d);}
{auto fn=JSValue::function(BatchedMeshMethod35,{module,prototype},"copy",1);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod35);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("copy",d);}
{auto fn=JSValue::function(BatchedMeshMethod36,{module,prototype},"dispose",0);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod36);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("dispose",d);}
{auto fn=JSValue::function(BatchedMeshMethod37,{module,prototype},"_updateDrawParameters",3);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod37);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("_updateDrawParameters",d);}
{auto fn=JSValue::function(BatchedMeshMethod38,{module,prototype},"onBeforeRender",5);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod38);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("onBeforeRender",d);}
{auto fn=JSValue::function(BatchedMeshMethod39,{module,prototype},"onBeforeShadow",6);fn.setFunctionSourceText(BatchedMeshManagedSource::BatchedMeshMethod39);JSPropertyDescriptor d;d.configurable=true;d.value=fn;d.writable=true;prototype.defineProperty("onBeforeShadow",d);}
}
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();own(facade,"BatchedMesh",module.get("BatchedMesh"));facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeBatchedMeshBinding(const std::string& key){const auto keeper=nativeBatchedMeshModule();return BatchedMeshManaged::lexical().lock()->get(key);}
JSValue nativeBatchedMeshConstructor(){return nativeBatchedMeshBinding("BatchedMesh");}
