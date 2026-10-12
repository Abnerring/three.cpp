#include "VertexTangentsHelper.h"
#include "VertexHelperRuntime.h"
#include "objects/Points.h"
#include "objects/Sprite.h"

namespace {
thread_local Vector3 v1, v2;

void multiplyScalar(Vector3& vector, const JSValue& scalar) {
    const double x = vector.x;
    vector.x = x * scalar.toNumber();
    const double y = vector.y;
    vector.y = y * scalar.toNumber();
    const double z = vector.z;
    vector.z = z * scalar.toNumber();
}

std::shared_ptr<BufferGeometry> sourceGeometry(const std::shared_ptr<Object3D>& object) {
    if (!object) throw JSTypeError("Cannot read properties of undefined (reading 'geometry')");
    if (const auto mesh = std::dynamic_pointer_cast<Mesh>(object)) return mesh->geometry;
    if (const auto line = std::dynamic_pointer_cast<Line>(object)) return line->geometry;
    if (const auto points = std::dynamic_pointer_cast<Points>(object)) return points->geometry;
    if (const auto sprite = std::dynamic_pointer_cast<Sprite>(object)) return sprite->geometry;
    return nullptr;
}
std::shared_ptr<BufferGeometry> createGeometry(const std::shared_ptr<Object3D>& object) {
    auto geometry = std::make_shared<BufferGeometry>();
    const auto source = sourceGeometry(object);
    if (!source) throw JSTypeError("Cannot read properties of undefined (reading 'attributes')");
    const auto tangent = source->getAttribute("tangent");
    if (!tangent) throw JSTypeError("Cannot read properties of undefined (reading 'count')");
    const auto nTangents = tangent->getCount();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(nTangents * 2 * 3, 3));
    return geometry;
}
}

VertexTangentsHelper::VertexTangentsHelper(std::shared_ptr<Object3D> value, JSValue length,
                                         HelperColor color)
    : VertexTangentsHelper(value, std::move(length), std::move(color), createGeometry(value)) {}

VertexTangentsHelper::VertexTangentsHelper(std::shared_ptr<Object3D> value, JSValue length,
                                         HelperColor color, std::shared_ptr<BufferGeometry> prepared)
    : LineSegments(std::move(prepared), Helper_detail::lineMaterial(
          color.value.isUndefined() ? JSValue(0x00ffff) : color.value)),
      object(std::move(value)), size(length.isUndefined() ? JSValue(1) : std::move(length)) {
    type = "VertexTangentsHelper";
    matrixAutoUpdate = false;
    update();
}

void VertexTangentsHelper::update() {
    {
        const auto current = object;
        if (!current) throw JSTypeError("Cannot read properties of undefined (reading 'updateMatrixWorld')");
        current->updateMatrixWorld(true);
    }
    const auto matrixOwner = object;
    if (!matrixOwner) throw JSTypeError("Cannot read properties of undefined (reading 'matrixWorld')");
    const auto& matrixWorld = matrixOwner->matrixWorld;
    if (!geometry) throw JSTypeError("Cannot read properties of undefined (reading 'attributes')");
    const auto position = geometry->getAttribute("position");
    const auto objGeometry = sourceGeometry(object);
    if (!objGeometry) throw JSTypeError("Cannot read properties of undefined (reading 'attributes')");
    const auto objPos = objGeometry->getAttribute("position");
    const auto objTan = objGeometry->getAttribute("tangent");
    if (!objPos) throw JSTypeError("Cannot read properties of undefined (reading 'count')");
    double idx = 0;
    for (double j = 0, jl = objPos->getCount(); j < jl; ++j) {
        v1.fromBufferAttribute(*objPos, j).applyMatrix4(matrixWorld);
        if (!objTan) throw JSTypeError("Cannot read properties of undefined (reading 'getX')");
        v2.fromBufferAttribute(*objTan, j);
        v2.transformDirection(matrixWorld);
        multiplyScalar(v2, JSValue(size));
        v2.add(v1);
        if (!position) throw JSTypeError("Cannot read properties of undefined (reading 'setXYZ')");
        position->setXYZ(idx, v1.x, v1.y, v1.z);
        idx = idx + 1;
        position->setXYZ(idx, v2.x, v2.y, v2.z);
        idx = idx + 1;
    }
    if (!position) throw JSTypeError("Cannot set properties of undefined (setting 'needsUpdate')");
    position->setNeedsUpdate(true);
}

std::shared_ptr<Object3D> VertexTangentsHelper::clone(bool) const {
    return std::make_shared<VertexTangentsHelper>();
}

void VertexTangentsHelper::dispose() {
    Helper_detail::disposeGeometry(geometry);
    Helper_detail::disposeMaterial(material);
}

void VertexTangentsHelper::initializeNativeWrapper(JSValue wrapper) { VertexHelper_detail::initialize<VertexTangentsHelper>(wrapper); }
void VertexTangentsHelper::visitNativeObjectReferences(const ObjectVisitor& visitor) const { visitor(object); }
std::vector<const JSValue*> VertexTangentsHelper::nativeJSReferences() const { return {&size}; }
