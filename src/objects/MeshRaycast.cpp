#include "core/Raycaster.h"
#include "math/Ray.h"
#include "math/Triangle.h"
#include "objects/Mesh.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

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
    const std::shared_ptr<BufferAttribute>& uv, const std::shared_ptr<BufferAttribute>& uv1,
    const std::shared_ptr<BufferAttribute>& normal, std::size_t a, std::size_t b, std::size_t c) {
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
    const auto count = index ? index->count : position->count;
    const auto& drawRange = queryGeometry->drawRange;
    auto testRange = [&](std::size_t start, std::size_t end,
                         std::shared_ptr<Material> queryMaterial, std::size_t materialIndex) {
        if (start >= end)
            return;
        if (!queryMaterial)
            throw std::logic_error("Mesh raycast group requires material");
        for (std::size_t i = start; i < end;) {
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
            const auto start = std::max(group.start, drawRange.start);
            const auto end = std::min(count, std::min(rangeEnd(group.start, group.count),
                                                      rangeEnd(drawRange.start, drawRange.count)));
            if (start >= end)
                continue;
            if (group.materialIndex >= queryMaterials.size())
                throw std::logic_error("Mesh raycast materialIndex out of range");
            testRange(start, end, queryMaterials[group.materialIndex], group.materialIndex);
        }
    } else
        testRange(drawRange.start, std::min(count, rangeEnd(drawRange.start, drawRange.count)),
                  queryMaterial, 0);
}
