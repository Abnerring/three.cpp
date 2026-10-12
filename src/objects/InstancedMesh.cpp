#include "objects/InstancedMesh.h"
#include "core/Raycaster.h"

namespace {
thread_local Matrix4 instanceLocalMatrix, instanceWorldMatrix;
thread_local Box3 instanceBox;
thread_local Sphere instanceSphere;
struct RaycastScratch {
    Mesh mesh;
    std::vector<Intersection> intersects;
};
RaycastScratch& raycastScratch() { static thread_local RaycastScratch value; return value; }
double readArray(const BufferAttribute::Array& array, double index) {
    return std::visit([&](const auto& values) { return values.get(index).toNumber(); }, array);
}
void writeArray(BufferAttribute::Array& array, double index, double value) {
    std::visit([&](auto& values) { values.setIndex(index, value); }, array);
}
TextureImage::Ptr morphImage(const std::shared_ptr<DataTexture>& texture) {
    if (!texture) throw JSTypeError("Cannot read properties of null (reading 'source')");
    if (!texture->source) throw JSTypeError("Cannot read properties of null (reading 'data')");
    if (!texture->source->data) throw JSTypeError("Cannot read properties of null (reading 'data')");
    return texture->source->data;
}
double readImage(const TextureImage::Data& data, double index) {
    return std::visit([&](const auto& values) -> double {
        using T = std::decay_t<decltype(values)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) throw JSTypeError("Cannot read properties of null");
        else if constexpr (std::is_same_v<T, JSValue>) return values.get(JSValue(index).toString()).toNumber();
        else return values.get(index).toNumber();
    }, data);
}
void writeImage(TextureImage::Data& data, double index, double value) {
    std::visit([&](auto& values) {
        using T = std::decay_t<decltype(values)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) throw JSTypeError("Cannot set properties of null");
        else if constexpr (std::is_same_v<T, JSValue>) values.set(JSValue(index).toString(), value);
        else values.setIndex(index, value);
    }, data);
}
void setImage(TextureImage::Data& data, const std::vector<double>& values, double offset) {
    std::visit([&](auto& array) {
        using T = std::decay_t<decltype(array)>;
        if constexpr (std::is_same_v<T, std::nullptr_t>) throw JSTypeError("Cannot read properties of null (reading 'set')");
        else if constexpr (std::is_same_v<T, JSValue>) {
            auto input = JSValue::array();
            for (std::size_t i = 0; i < values.size(); ++i) input.set(std::to_string(i), values[i]);
            array.get("set").call(array, {input, offset});
        } else array.set(values, offset);
    }, data);
}
}
InstancedMesh::InstancedMesh(std::shared_ptr<BufferGeometry> geometry,
                             std::shared_ptr<Material> material, JSValue count)
    : Mesh(std::move(geometry), std::move(material), false) { initializeInstances(count); }
InstancedMesh::InstancedMesh(std::shared_ptr<BufferGeometry> geometry,
                             std::vector<std::shared_ptr<Material>> materials, JSValue count)
    : Mesh(std::move(geometry), std::move(materials), false) { initializeInstances(count); }
void InstancedMesh::initializeInstances(const JSValue& value) {
    isInstancedMesh = true;
    instanceMatrix = std::make_shared<InstancedBufferAttribute>(Float32Array(value.toNumber() * 16), 16);
    count = value;
    const Matrix4 identity;
    for (double i = 0; i < value.toNumber(); ++i) setMatrixAt(i, identity);
}
void InstancedMesh::computeBoundingBox() {
    const auto source = geometry;
    const auto length = count.asValue();
    if (!boundingBox) boundingBox = std::make_shared<Box3>();
    if (!source) throw JSTypeError("Cannot read properties of null (reading 'boundingBox')");
    if (!source->boundingBox) source->computeBoundingBox();
    boundingBox->makeEmpty();
    for (double i = 0; i < length.toNumber(); ++i) {
        getMatrixAt(i, instanceLocalMatrix);
        instanceBox.copy(*source->boundingBox).applyMatrix4(instanceLocalMatrix);
        boundingBox->unionWith(instanceBox);
    }
}
void InstancedMesh::computeBoundingSphere() {
    const auto source = geometry;
    const auto length = count.asValue();
    if (!boundingSphere) boundingSphere = std::make_shared<Sphere>();
    if (!source) throw JSTypeError("Cannot read properties of null (reading 'boundingSphere')");
    if (!source->boundingSphere) source->computeBoundingSphere();
    boundingSphere->makeEmpty();
    for (double i = 0; i < length.toNumber(); ++i) {
        getMatrixAt(i, instanceLocalMatrix);
        instanceSphere.copy(*source->boundingSphere).applyMatrix4(instanceLocalMatrix);
        boundingSphere->unionWith(instanceSphere);
    }
}
Object3D& InstancedMesh::copy(const Object3D& source, bool recursive) {
    const auto& other = dynamic_cast<const InstancedMesh&>(source);
    Mesh::copy(source, recursive);
    if (!instanceMatrix || !other.instanceMatrix)
        throw JSTypeError("Cannot read properties of null (reading 'copy')");
    instanceMatrix->copy(*other.instanceMatrix);
    morphTexture = other.morphTexture ? std::dynamic_pointer_cast<DataTexture>(other.morphTexture->clone()) : nullptr;
    instanceColor = other.instanceColor ? std::dynamic_pointer_cast<InstancedBufferAttribute>(other.instanceColor->clone()) : nullptr;
    count = other.count;
    boundingBox = other.boundingBox ? std::make_shared<Box3>(other.boundingBox->clone()) : nullptr;
    boundingSphere = other.boundingSphere ? std::make_shared<Sphere>(other.boundingSphere->clone()) : nullptr;
    return *this;
}
std::shared_ptr<Object3D> InstancedMesh::clone(bool recursive) const {
    auto result = std::make_shared<InstancedMesh>(); result->copy(*this, recursive); return result;
}
Color& InstancedMesh::getColorAt(double index, Color& color) const {
    if (!instanceColor) return color.setRGB(1, 1, 1);
    const auto array = instanceColor->array;
    color.r = readArray(array, index * 3);
    color.g = readArray(array, index * 3 + 1);
    color.b = readArray(array, index * 3 + 2);
    return color;
}
Matrix4& InstancedMesh::getMatrixAt(double index, Matrix4& matrix) const {
    if (!instanceMatrix) throw JSTypeError("Cannot read properties of null (reading 'array')");
    const auto array = instanceMatrix->array;
    for (std::size_t i = 0; i < 16; ++i) matrix.elements()[i] = readArray(array, index * 16 + double(i));
    return matrix;
}
void InstancedMesh::getMorphAt(double index, Mesh& object) const {
    const auto influences = object.morphTargetInfluences;
    const auto image = morphImage(morphTexture);
    const auto data = image->data;
    if (!influences) throw JSTypeError("Cannot read properties of undefined (reading 'length')");
    const double length = double(influences->size()) + 1;
    const double dataIndex = index * length + 1;
    const auto storage = influences->storage();
    for (std::size_t i = 0; i < storage->values.size(); ++i) {
        if (storage->flags.contains(i) && !storage->flags.at(i).writable)
            throw JSTypeError("Cannot assign to read only array element");
        storage->values[i] = readImage(data, dataIndex + double(i));
        storage->holes.erase(i);
    }
}
bool InstancedMesh::raycast(const Raycaster& raycaster, std::vector<Intersection>& intersects) {
    const auto length = count.asValue();
    auto& scratch = raycastScratch();
    auto& mesh = scratch.mesh;
    mesh.geometry = geometry; mesh.material = material;
    mesh.materials = materials; mesh.materialIsArray = materialIsArray;
    if (!material && !(materialIsArray || !materials.empty())) return true;
    if (!boundingSphere) computeBoundingSphere();
    instanceSphere.copy(*boundingSphere).applyMatrix4(matrixWorld);
    if (!raycaster.ray.intersectsSphere(instanceSphere)) return true;
    auto& hits = scratch.intersects;
    for (double i = 0; i < length.toNumber(); ++i) {
        getMatrixAt(i, instanceLocalMatrix);
        instanceWorldMatrix.multiplyMatrices(matrixWorld, instanceLocalMatrix);
        mesh.matrixWorld.copy(instanceWorldMatrix);
        mesh.raycast(raycaster, hits);
        for (auto& hit : hits) {
            hit.instanceId = i; hit.object = this; hit.objectOwner = weak_from_this().lock();
            intersects.push_back(std::move(hit));
        }
        hits.clear();
    }
    return true;
}
InstancedMesh& InstancedMesh::setColorAt(double index, const Color& color) {
    if (!instanceColor) {
        if (!instanceMatrix) throw JSTypeError("Cannot read properties of null (reading 'count')");
        Float32Array values(instanceMatrix->count * 3);
        values.assign(values.size(), 1);
        instanceColor = std::make_shared<InstancedBufferAttribute>(values, 3);
    }
    writeArray(instanceColor->array, index * 3, color.r);
    writeArray(instanceColor->array, index * 3 + 1, color.g);
    writeArray(instanceColor->array, index * 3 + 2, color.b);
    return *this;
}
InstancedMesh& InstancedMesh::setMatrixAt(double index, const Matrix4& matrix) {
    if (!instanceMatrix) throw JSTypeError("Cannot read properties of null (reading 'array')");
    for (std::size_t i = 0; i < 16; ++i)
        writeArray(instanceMatrix->array, index * 16 + double(i), matrix.elements()[i]);
    return *this;
}
InstancedMesh& InstancedMesh::setMorphAt(double index, const Mesh& object) {
    const auto influences = object.morphTargetInfluences;
    if (!influences) throw JSTypeError("Cannot read properties of undefined (reading 'length')");
    const double length = double(influences->size()) + 1;
    if (!morphTexture)
        morphTexture = std::make_shared<DataTexture>(Float32Array(length * count), length, count, RedFormat, FloatType);
    const auto image = morphImage(morphTexture);
    auto data = image->data;
    const auto storage = influences->storage();
    std::vector<double> values;
    double sum = 0;
    for (std::size_t i = 0; i < storage->values.size(); ++i) {
        const double value = storage->holes.contains(i) ? std::numeric_limits<double>::quiet_NaN() : storage->values[i];
        sum += value; values.push_back(value);
    }
    if (!geometry) throw JSTypeError("Cannot read properties of null (reading 'morphTargetsRelative')");
    const double base = geometry->morphTargetsRelative ? 1 : 1 - sum;
    const double dataIndex = length * index;
    writeImage(data, dataIndex, base);
    setImage(data, values, dataIndex + 1);
    return *this;
}
void InstancedMesh::dispose() {
    Mesh::dispose();
    if (morphTexture) {
        const auto texture = morphTexture;
        texture->dispose();
        morphTexture.reset();
    }
}
void InstancedMesh::serializeBeforeGeometry(Object3DJSONContext& meta, JsonValue& object) const {
    object["type"] = "InstancedMesh";
    if (const auto value = count.asValue().toJSON()) object["count"] = *value;
    if (!instanceMatrix) throw JSTypeError("Cannot read properties of null (reading 'toJSON')");
    object["instanceMatrix"] = instanceMatrix->toJSON();
    if (instanceColor) object["instanceColor"] = instanceColor->toJSON();
}
