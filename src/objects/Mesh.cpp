#include "core/Raycaster.h"
#include "math/Ray.h"
#include "math/Triangle.h"
#include "math/Frustum.h"
#include "objects/Mesh.h"
#include "math/MathUtils.h"
#include "geometries/ShapeGeometry.h"
#include "geometries/ExtrudeGeometry.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

#include "objects/Mesh.h"
std::optional<JsonValue> Mesh::propertyValue(const std::string& property) const {
    if (property == "isMesh")
        return isMesh;
    if (property == "count")
        return count.asValue().toJSON();
    return Object3D::propertyValue(property);
}
Mesh::Mesh(std::shared_ptr<BufferGeometry> g, std::shared_ptr<Material> m)
    : Mesh(std::move(g), std::move(m), true) {}
Mesh::Mesh(std::shared_ptr<BufferGeometry> g, std::shared_ptr<Material> m, bool initializeMorphTargets)
    : geometry(std::move(g)), material(std::move(m)) {
    isMesh = true;
    type = "Mesh";
    if (initializeMorphTargets) updateMorphTargets();
}
Mesh::Mesh(std::shared_ptr<BufferGeometry> g, std::vector<std::shared_ptr<Material>> ms)
    : Mesh(std::move(g), std::move(ms), true) {}
Mesh::Mesh(std::shared_ptr<BufferGeometry> g, std::vector<std::shared_ptr<Material>> ms,
           bool initializeMorphTargets)
    : Mesh(std::move(g), nullptr, initializeMorphTargets) {
    materials = std::move(ms);
    materialIsArray = true;
}
void Mesh::updateMorphTargets() {
    if (!geometry || geometry->morphAttributes.empty())
        return;
    const auto& attributes = geometry->morphAttributes.begin()->second;
    morphTargetInfluences = std::vector<double>{};
    morphTargetDictionary = std::make_shared<std::map<std::string, std::size_t>>();
    for (std::size_t i = 0; i < attributes.size(); ++i) {
        morphTargetInfluences->push_back(0);
        const auto name=attributes[i]->getName();
        (*morphTargetDictionary)[name.toBoolean()?name.toString():std::to_string(i)] = i;
    }
}
bool Mesh::intersectsFrustum(const Frustum& frustum) {return frustum.intersectsObject(*this);}
Vector3& Mesh::getVertexPosition(std::size_t index, Vector3& target) const {
    const auto position = geometry->getAttribute("position");
    target.fromBufferAttribute(*position,double(index));
    if (auto it = geometry->morphAttributes.find("position");
        it != geometry->morphAttributes.end() && morphTargetInfluences) {
        Vector3 morph;
        const auto influences = morphTargetInfluences->storage();
        for (std::size_t i = 0; i < it->second.size(); ++i) {
            const auto influence = i < influences->values.size() && !influences->holes.contains(i)
                ? influences->values[i] : std::numeric_limits<double>::quiet_NaN();
            if (influence == 0)
                continue;
            const auto& a = it->second[i];
            Vector3 v;v.fromBufferAttribute(*a,double(index));
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
    materials = s.materials.slice();
    materialIsArray = s.materialIsArray;
    morphTargetDictionary = s.morphTargetDictionary
        ? std::make_shared<std::map<std::string, std::size_t>>(*s.morphTargetDictionary) : nullptr;
    morphTargetInfluences = s.morphTargetInfluences
        ? std::optional<SharedArray<double>>(s.morphTargetInfluences->slice()) : std::nullopt;
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
        const auto storage = morphTargetInfluences->storage();
        for (std::size_t i = 0; i < storage->values.size(); ++i)
            influences.emplace_back(storage->holes.contains(i) ? JsonValue() : JsonValue(storage->values[i]));
        object["morphTargetInfluences"] = std::move(influences);
    }
    serializeBeforeGeometry(meta, object);
    const auto serializedGeometry = geometry;
    meta.cacheResource("geometries", serializedGeometry->uuid,
                       [&] { return serializedGeometry->toJSON(); });
    object["geometry"] = serializedGeometry->uuid;
    std::optional<ShapeGeometry::Shapes> geometryShapes;
    if (const auto shaped = std::dynamic_pointer_cast<ShapeGeometry>(geometry))
        geometryShapes = shaped->parameters.shapes;
    else if (const auto extruded = std::dynamic_pointer_cast<ExtrudeGeometry>(geometry))
        geometryShapes = extruded->parameters.shapes;
    if (geometryShapes) {
        const auto shapes = *geometryShapes;
        const auto serialize = [&](const std::shared_ptr<Shape>& shape) {
            if (!shape) throw JSTypeError("Cannot read properties of null (reading 'uuid')");
            meta.cacheResource("shapes", shape->uuid, [&] { return shape->toJSON(); });
        };
        if (const auto single = std::get_if<std::shared_ptr<Shape>>(&shapes)) serialize(*single);
        else {
            const auto list = std::get<std::shared_ptr<ShapeGeometry::ShapeList>>(shapes);
            if (!list) throw JSTypeError("Cannot read properties of null (reading 'length')");
            const auto count = list->size();
            for (std::size_t i = 0; i < count; ++i) {
                if (i >= list->size()) throw JSTypeError("Cannot read properties of undefined (reading 'uuid')");
                const auto shape = (*list)[i]; serialize(shape);
            }
        }
    }
    serializeBeforeMaterial(meta, object);
    if (materialIsArray || !materials.empty()) {
        JsonValue::Array references;
        for (const auto& entry : materials) {
            if (!entry)
                throw std::logic_error("Mesh JSON serialization requires non-null materials");
            const auto serializedMaterial = entry;
            meta.cacheResource("materials", serializedMaterial->uuid,
                               [&] { return serializedMaterial->toJSON(&meta); });
            references.emplace_back(serializedMaterial->uuid);
        }
        object["material"] = std::move(references);
    } else {
        if (!material)
            throw std::logic_error("Mesh JSON serialization requires material");
        const auto serializedMaterial = material;
        meta.cacheResource("materials", serializedMaterial->uuid,
                           [&] { return serializedMaterial->toJSON(&meta); });
        object["material"] = serializedMaterial->uuid;
    }
}


namespace {
std::size_t rangeEnd(std::size_t start, std::size_t count) {
    const auto maximum = std::numeric_limits<std::size_t>::max();
    return count > maximum - start ? maximum : start + count;
}
std::size_t vertexIndex(const BufferAttribute& index, std::size_t offset) {
    const double value = index.getX(offset);
    if (!std::isfinite(value) || value < 0 || std::floor(value) != value ||
        value >= double(std::numeric_limits<std::size_t>::max()))
        throw std::invalid_argument("Mesh raycast requires nonnegative integer vertex indices");
    return static_cast<std::size_t>(value);
}
std::optional<Intersection> checkGeometryIntersection(
    Mesh& object, const Material& material, const Raycaster& raycaster, const Ray& ray,
    const std::shared_ptr<VertexAttribute>& uv, const std::shared_ptr<VertexAttribute>& uv1,
    const std::shared_ptr<VertexAttribute>& normal, std::size_t a, std::size_t b, std::size_t c) {
    Vector3 vA, vB, vC, point;
    object.getVertexPosition(a, vA);
    object.getVertexPosition(b, vB);
    object.getVertexPosition(c, vC);
    auto hit = material.side == BackSide
                   ? ray.intersectTriangle(vC, vB, vA, true, point)
                   : ray.intersectTriangle(vA, vB, vC, material.side == FrontSide, point);
    if (!hit)
        return std::nullopt;
    Vector3 worldPoint;
    worldPoint.copy(point).applyMatrix4(object.matrixWorld);
    const double distance = raycaster.ray.origin.distanceTo(worldPoint);
    if (distance < raycaster.near || distance > raycaster.far)
        return std::nullopt;
    Intersection intersection;
    intersection.distance = distance;
    intersection.point = worldPoint;
    intersection.object = &object;
    intersection.objectOwner = object.weak_from_this().lock();
    Vector3 barycoord;
    Triangle::getBarycoord(point, vA, vB, vC, barycoord);
    if (uv) {
        intersection.uv.emplace();
        Triangle::getInterpolatedAttribute(*uv, a, b, c, barycoord, *intersection.uv);
    }
    if (uv1) {
        intersection.uv1.emplace();
        Triangle::getInterpolatedAttribute(*uv1, a, b, c, barycoord, *intersection.uv1);
    }
    if (normal) {
        intersection.normal.emplace();
        Triangle::getInterpolatedAttribute(*normal, a, b, c, barycoord, *intersection.normal);
        if (intersection.normal->dot(ray.direction) > 0)
            intersection.normal->multiplyScalar(-1);
    }
    intersection.face.emplace();
    intersection.face->a = a;
    intersection.face->b = b;
    intersection.face->c = c;
    Triangle::getNormal(vA, vB, vC, intersection.face->normal);
    intersection.barycoord = barycoord;
    return intersection;
}
} // namespace

bool Mesh::raycast(const Raycaster& raycaster, std::vector<Intersection>& intersects) {
    if (!material && !(materialIsArray || !materials.empty()))
        return true;
    if (!geometry)
        throw std::logic_error("Mesh raycast requires geometry");
    if (!geometry->boundingSphere)
        geometry->computeBoundingSphere();
    if (!geometry->boundingSphere)
        throw std::logic_error("Mesh raycast requires boundingSphere");
    Sphere sphere;
    sphere.copy(*geometry->boundingSphere).applyMatrix4(matrixWorld);
    Ray ray;
    ray.copy(raycaster.ray).recast(raycaster.near);
    if (!sphere.containsPoint(ray.origin)) {
        Vector3 sphereHit;
        if (!ray.intersectSphere(sphere, sphereHit))
            return true;
        const double extent = raycaster.far - raycaster.near;
        if (ray.origin.distanceToSquared(sphereHit) > extent * extent)
            return true;
    }
    Matrix4 inverseMatrix;
    inverseMatrix.copy(matrixWorld).invert();
    ray.copy(raycaster.ray).applyMatrix4(inverseMatrix);
    if (geometry->boundingBox && !ray.intersectsBox(*geometry->boundingBox))
        return true;
    _computeIntersections(raycaster, intersects, ray);
    return true;
}

void Mesh::_computeIntersections(const Raycaster& raycaster, std::vector<Intersection>& intersects,
                                 const Ray& rayLocalSpace) {
    const auto queryGeometry = geometry;
    const auto queryMaterial = material;
    const auto queryMaterials = materials;
    const auto queryMaterialIsArray = materialIsArray || !queryMaterials.empty();
    const auto queryGroups = queryGeometry->groups;
    const auto index = queryGeometry->index;
    const auto position = queryGeometry->getAttribute("position");
    const auto uv = queryGeometry->getAttribute("uv"), uv1 = queryGeometry->getAttribute("uv1"),
               normal = queryGeometry->getAttribute("normal");
    if (!index && !position)
        return;
    const auto count = index ? index->count : position->getCount();
    const auto& drawRange = queryGeometry->drawRange;
    auto testRange = [&](double start, double end,
                         std::shared_ptr<Material> queryMaterial, std::size_t materialIndex) {
        if (!(start < end))
            return;
        if (!std::isfinite(start) || start < 0 || std::floor(start) != start ||
            start >= double(std::numeric_limits<std::size_t>::max()))
            throw std::invalid_argument("Mesh raycast requires nonnegative integer vertex indices");
        if (!queryMaterial)
            throw std::logic_error("Mesh raycast group requires material");
        for (std::size_t i = static_cast<std::size_t>(start); i < end;) {
            const auto a = index ? vertexIndex(*index, i) : i,
                       b = index ? vertexIndex(*index, rangeEnd(i, 1)) : rangeEnd(i, 1),
                       c = index ? vertexIndex(*index, rangeEnd(i, 2)) : rangeEnd(i, 2);
            auto intersection = checkGeometryIntersection(*this, *queryMaterial, raycaster,
                                                          rayLocalSpace, uv, uv1, normal, a, b, c);
            if (intersection) {
                intersection->faceIndex = i / 3;
                intersection->face->materialIndex = materialIndex;
                intersects.push_back(std::move(*intersection));
            }
            if (i > std::numeric_limits<std::size_t>::max() - 3)
                break;
            i += 3;
        }
    };
    if (queryMaterialIsArray) {
        const auto groupCount = queryGroups.size();
        for (std::size_t i = 0; i < groupCount; ++i) {
            const auto& group = queryGroups[i];
            const auto start = MathUtils::max(group.start, drawRange.start);
            const auto end = MathUtils::min(count, MathUtils::min(group.start + group.count,
                                                      drawRange.start + drawRange.count));
            if (!(start < end))
                continue;
            if (!(group.materialIndex >= 0 && group.materialIndex < double(queryMaterials.size())) ||
                std::floor(group.materialIndex) != group.materialIndex)
                throw std::logic_error("Mesh raycast materialIndex out of range");
            const auto materialIndex = static_cast<std::size_t>(group.materialIndex);
            testRange(start, end, queryMaterials[materialIndex], materialIndex);
        }
    } else
        testRange(drawRange.start, MathUtils::min(count, drawRange.start + drawRange.count),
                  queryMaterial, 0);
}
