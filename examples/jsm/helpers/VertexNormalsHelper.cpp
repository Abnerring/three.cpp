#include "VertexNormalsHelper.h"
#include "VertexHelperRuntime.h"
#include "objects/Points.h"
#include "objects/Sprite.h"

namespace {
thread_local Vector3 v1, v2;
thread_local Matrix3 normalMatrix;

void multiplyScalar(Vector3& vector, const JSValue& scalar) {
    // JavaScript converts an object scalar separately for each multiplication.
    // Read each left operand before conversion, which may reenter update().
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
    const auto normal = source->getAttribute("normal");
    if (!normal) throw JSTypeError("Cannot read properties of undefined (reading 'count')");
    const auto nNormals = normal->getCount();
    geometry->setAttribute("position", std::make_shared<Float32BufferAttribute>(nNormals * 2 * 3, 3));
    return geometry;
}
}

VertexNormalsHelper::VertexNormalsHelper(std::shared_ptr<Object3D> value, JSValue length,
                                       HelperColor color)
    : VertexNormalsHelper(value, std::move(length), std::move(color), createGeometry(value)) {}

VertexNormalsHelper::VertexNormalsHelper(std::shared_ptr<Object3D> value, JSValue length,
                                       HelperColor color, std::shared_ptr<BufferGeometry> prepared)
    : LineSegments(std::move(prepared), Helper_detail::lineMaterial(
          color.value.isUndefined() ? JSValue(0xff0000) : color.value)),
      object(std::move(value)), size(length.isUndefined() ? JSValue(1) : std::move(length)) {
    type = "VertexNormalsHelper";
    matrixAutoUpdate = false;
    update();
}

void VertexNormalsHelper::update() {
    {
        const auto current = object;
        if (!current) throw JSTypeError("Cannot read properties of undefined (reading 'updateMatrixWorld')");
        current->updateMatrixWorld(true);
    }
    const auto matrixOwner = object;
    if (!matrixOwner) throw JSTypeError("Cannot read properties of undefined (reading 'matrixWorld')");
    normalMatrix.getNormalMatrix(matrixOwner->matrixWorld);
    const auto& matrixWorld = matrixOwner->matrixWorld;
    if (!geometry) throw JSTypeError("Cannot read properties of undefined (reading 'attributes')");
    const auto position = geometry->getAttribute("position");
    const auto objGeometry = sourceGeometry(object);
    if (objGeometry) {
        const auto objPos = objGeometry->getAttribute("position");
        const auto objNorm = objGeometry->getAttribute("normal");
        if (!objPos) throw JSTypeError("Cannot read properties of undefined (reading 'count')");
        double idx = 0;
        for (double j = 0, jl = objPos->getCount(); j < jl; ++j) {
            v1.fromBufferAttribute(*objPos, j).applyMatrix4(matrixWorld);
            if (!objNorm) throw JSTypeError("Cannot read properties of undefined (reading 'getX')");
            v2.fromBufferAttribute(*objNorm, j);
            v2.applyMatrix3(normalMatrix).normalize();
            multiplyScalar(v2, JSValue(size));
            v2.add(v1);
            if (!position) throw JSTypeError("Cannot read properties of undefined (reading 'setXYZ')");
            position->setXYZ(idx, v1.x, v1.y, v1.z);
            idx = idx + 1;
            position->setXYZ(idx, v2.x, v2.y, v2.z);
            idx = idx + 1;
        }
    }
    if (!position) throw JSTypeError("Cannot set properties of undefined (setting 'needsUpdate')");
    position->setNeedsUpdate(true);
}

std::shared_ptr<Object3D> VertexNormalsHelper::clone(bool) const {
    // Object3D.clone invokes the dynamic constructor without its required source.
    return std::make_shared<VertexNormalsHelper>();
}

void VertexNormalsHelper::dispose() {
    Helper_detail::disposeGeometry(geometry);
    Helper_detail::disposeMaterial(material);
}

void VertexNormalsHelper::initializeNativeWrapper(JSValue wrapper) {
    VertexHelper_detail::initialize<VertexNormalsHelper>(wrapper);
    JSPropertyDescriptor d;d.enumerable=true;d.configurable=true;
    d.getter=JSValue::function([](const JSValue& self,const VertexHelper_detail::Args&,const VertexHelper_detail::Args&)->JSValue {
        return VertexHelper_detail::helper<VertexNormalsHelper>(self)->isVertexNormalsHelper;
    });
    d.setter=JSValue::function([](const JSValue& self,const VertexHelper_detail::Args& args,const VertexHelper_detail::Args&)->JSValue {
        // The native flag is a bool; size retains arbitrary JSValue types.
        VertexHelper_detail::helper<VertexNormalsHelper>(self)->isVertexNormalsHelper=(args.empty()?JSValue():args[0]).toBoolean();return {};
    });wrapper.defineProperty("isVertexNormalsHelper",d);
}
void VertexNormalsHelper::visitNativeObjectReferences(const ObjectVisitor& visitor) const { visitor(object); }
std::vector<const JSValue*> VertexNormalsHelper::nativeJSReferences() const { return {&size}; }
