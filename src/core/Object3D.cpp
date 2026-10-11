#include "core/Object3D.h"
#include "math/MathUtils.h"
#include <algorithm>
#include <atomic>
#include <stdexcept>

namespace {
std::atomic<std::uint64_t> nextObjectId{0};
const Vector3 xAxis{1, 0, 0}, yAxis{0, 1, 0}, zAxis{0, 0, 1};
JsonValue vectorJSON(const Vector3& vector) {
    return JsonValue::Array{vector.x(), vector.y(), vector.z()};
}
} // namespace

Vector3 Object3D::DEFAULT_UP{0, 1, 0};
bool Object3D::DEFAULT_MATRIX_AUTO_UPDATE = true;
bool Object3D::DEFAULT_MATRIX_WORLD_AUTO_UPDATE = true;

Object3D::Object3D()
    : id(nextObjectId.fetch_add(1)), uuid(MathUtils::generateUUID()), up(DEFAULT_UP),
      matrixAutoUpdate(DEFAULT_MATRIX_AUTO_UPDATE),
      matrixWorldAutoUpdate(DEFAULT_MATRIX_WORLD_AUTO_UPDATE) {
    rotation._onChange([this] { quaternion.setFromEuler(rotation, false); });
    quaternion._onChange([this] { rotation.setFromQuaternion(quaternion, std::nullopt, false); });
}

Object3D::~Object3D() {
    for (const auto& child : children) {
        if (child && child->parent == this)
            child->parent = nullptr;
    }
}

void Object3D::applyMatrix4(const Matrix4& transform) {
    if (matrixAutoUpdate)
        updateMatrix();
    matrix.premultiply(transform);
    matrix.decompose(position, quaternion, scale);
    if (pivot) {
        position.copy(*pivot);
        position.applyMatrix4(matrix);
        position.sub(*pivot);
    }
}

Object3D& Object3D::applyQuaternion(const Quaternion& q) {
    quaternion.premultiply(q);
    return *this;
}
void Object3D::setRotationFromAxisAngle(const Vector3& axis, HIGH_PRECISION angle) {
    quaternion.setFromAxisAngle(axis, angle);
}
void Object3D::setRotationFromEuler(const Euler& euler) {
    quaternion.setFromEuler(euler, true);
}
void Object3D::setRotationFromMatrix(const Matrix4& m) {
    quaternion.setFromRotationMatrix(m);
}
void Object3D::setRotationFromQuaternion(const Quaternion& q) {
    quaternion.copy(q);
}
Object3D& Object3D::rotateOnAxis(const Vector3& axis, HIGH_PRECISION angle) {
    Quaternion q;
    q.setFromAxisAngle(axis, angle);
    quaternion.multiply(q);
    return *this;
}
Object3D& Object3D::rotateOnWorldAxis(const Vector3& axis, HIGH_PRECISION angle) {
    Quaternion q;
    q.setFromAxisAngle(axis, angle);
    quaternion.premultiply(q);
    return *this;
}
Object3D& Object3D::rotateX(HIGH_PRECISION angle) {
    return rotateOnAxis(xAxis, angle);
}
Object3D& Object3D::rotateY(HIGH_PRECISION angle) {
    return rotateOnAxis(yAxis, angle);
}
Object3D& Object3D::rotateZ(HIGH_PRECISION angle) {
    return rotateOnAxis(zAxis, angle);
}
Object3D& Object3D::translateOnAxis(const Vector3& axis, HIGH_PRECISION distance) {
    Vector3 direction(axis);
    direction.applyQuaternion(quaternion);
    direction.multiplyScalar(distance);
    position.add(direction);
    return *this;
}
Object3D& Object3D::translateX(HIGH_PRECISION distance) {
    return translateOnAxis(xAxis, distance);
}
Object3D& Object3D::translateY(HIGH_PRECISION distance) {
    return translateOnAxis(yAxis, distance);
}
Object3D& Object3D::translateZ(HIGH_PRECISION distance) {
    return translateOnAxis(zAxis, distance);
}
Vector3& Object3D::localToWorld(Vector3& vector) {
    updateWorldMatrix(true, false);
    vector.applyMatrix4(matrixWorld);
    return vector;
}
Vector3& Object3D::worldToLocal(Vector3& vector) {
    updateWorldMatrix(true, false);
    Matrix4 inverse;
    inverse.copy(matrixWorld);
    inverse.invert();
    vector.applyMatrix4(inverse);
    return vector;
}

void Object3D::lookAt(HIGH_PRECISION x, HIGH_PRECISION y, HIGH_PRECISION z) {
    lookAt(Vector3{x, y, z});
}
void Object3D::lookAt(const Vector3& target) {
    // Same upstream limitation: non-uniform parent scaling is not supported.
    const Vector3 targetCopy(target);
    Object3D* oldParent = parent;
    updateWorldMatrix(true, false);
    Vector3 worldPosition;
    worldPosition.setFromMatrixPosition(matrixWorld);
    Matrix4 m;
    if (isCamera || isLight)
        m.lookAt(worldPosition, targetCopy, up);
    else
        m.lookAt(targetCopy, worldPosition, up);
    quaternion.setFromRotationMatrix(m);
    if (oldParent) {
        m.extractRotation(oldParent->matrixWorld);
        Quaternion q;
        q.setFromRotationMatrix(m);
        q.invert();
        quaternion.premultiply(q);
    }
}

void Object3D::checkChild(const std::shared_ptr<Object3D>& object) const {
    if (!object)
        throw std::invalid_argument("Object3D child must not be null");
    for (const auto* ancestor = this; ancestor; ancestor = ancestor->parent) {
        if (ancestor == object.get())
            throw std::invalid_argument("Object3D cannot contain itself or an ancestor");
    }
}

Object3D& Object3D::add(std::shared_ptr<Object3D> object) {
    checkChild(object);
    object->removeFromParent();
    object->parent = this;
    children.push_back(object);
    object->dispatchEvent(Event{"added"});
    dispatchEvent(Event{"childadded", nullptr, object.get()});
    return *this;
}

Object3D& Object3D::remove(Object3D* object) {
    const auto found = std::find_if(children.begin(), children.end(),
                                    [object](const auto& child) { return child.get() == object; });
    if (found == children.end())
        return *this;
    auto retained = *found;
    retained->parent = nullptr;
    children.erase(found);
    retained->dispatchEvent(Event{"removed"});
    dispatchEvent(Event{"childremoved", nullptr, retained.get()});
    return *this;
}

Object3D& Object3D::removeFromParent() {
    // Retain ownership while removal events run, even when the parent owns the last reference.
    const auto retained = weak_from_this().lock();
    if (parent)
        parent->remove(this);
    return *this;
}

Object3D& Object3D::clear() {
    const auto oldChildren = children;
    for (const auto& child : oldChildren)
        remove(child);
    return *this;
}

Object3D& Object3D::attach(std::shared_ptr<Object3D> object) {
    checkChild(object);
    updateWorldMatrix(true, false);
    Matrix4 inverse;
    inverse.copy(matrixWorld);
    inverse.invert();
    if (object->parent) {
        object->parent->updateWorldMatrix(true, false);
        inverse.multiply(object->parent->matrixWorld);
    }
    object->applyMatrix4(inverse);
    object->removeFromParent();
    object->parent = this;
    children.push_back(object);
    object->updateWorldMatrix(false, true);
    object->dispatchEvent(Event{"added"});
    dispatchEvent(Event{"childadded", nullptr, object.get()});
    return *this;
}

Object3D* Object3D::getObjectById(std::uint64_t objectId) {
    if (id == objectId)
        return this;
    for (const auto& child : children)
        if (auto* found = child->getObjectById(objectId))
            return found;
    return nullptr;
}
Object3D* Object3D::getObjectByName(const std::string& objectName) {
    if (name == objectName)
        return this;
    for (const auto& child : children)
        if (auto* found = child->getObjectByName(objectName))
            return found;
    return nullptr;
}
std::optional<JsonValue> Object3D::propertyValue(const std::string& property) const {
    if (property == "id")
        return id;
    if (property == "uuid")
        return uuid;
    if (property == "name")
        return name;
    if (property == "type")
        return type;
    if (property == "visible")
        return visible;
    if (property == "castShadow")
        return castShadow;
    if (property == "receiveShadow")
        return receiveShadow;
    if (property == "frustumCulled")
        return frustumCulled;
    if (property == "renderOrder")
        return renderOrder;
    if (property == "static")
        return isStatic;
    if (property == "matrixAutoUpdate")
        return matrixAutoUpdate;
    if (property == "matrixWorldAutoUpdate")
        return matrixWorldAutoUpdate;
    if (property == "matrixWorldNeedsUpdate")
        return matrixWorldNeedsUpdate;
    if (property == "isObject3D")
        return isObject3D;
    return std::nullopt;
}
Object3D* Object3D::getObjectByProperty(const std::string& property, const JsonValue& value) {
    if (auto actual = propertyValue(property); actual && *actual == value)
        return this;
    for (const auto& child : children)
        if (auto* found = child->getObjectByProperty(property, value))
            return found;
    return nullptr;
}
std::vector<Object3D*>& Object3D::getObjectsByProperty(const std::string& property,
                                                       const JsonValue& value,
                                                       std::vector<Object3D*>& result) {
    if (auto actual = propertyValue(property); actual && *actual == value)
        result.push_back(this);
    for (const auto& child : children)
        child->getObjectsByProperty(property, value, result);
    return result;
}

Vector3& Object3D::getWorldPosition(Vector3& target) {
    updateWorldMatrix(true, false);
    target.setFromMatrixPosition(matrixWorld);
    return target;
}
Quaternion& Object3D::getWorldQuaternion(Quaternion& target) {
    updateWorldMatrix(true, false);
    Vector3 p, s;
    matrixWorld.decompose(p, target, s);
    return target;
}
Vector3& Object3D::getWorldScale(Vector3& target) {
    updateWorldMatrix(true, false);
    Vector3 p;
    Quaternion q;
    matrixWorld.decompose(p, q, target);
    return target;
}
Vector3& Object3D::getWorldDirection(Vector3& target) {
    updateWorldMatrix(true, false);
    const auto e = matrixWorld.elements();
    target.set(e[8], e[9], e[10]);
    target.normalize();
    return target;
}
void Object3D::traverse(const std::function<void(Object3D&)>& callback) {
    callback(*this);
    const auto descendants = children;
    for (const auto& child : descendants)
        child->traverse(callback);
}
void Object3D::traverseVisible(const std::function<void(Object3D&)>& callback) {
    if (!visible)
        return;
    callback(*this);
    const auto descendants = children;
    for (const auto& child : descendants)
        child->traverseVisible(callback);
}
void Object3D::traverseAncestors(const std::function<void(Object3D&)>& callback) {
    if (parent) {
        callback(*parent);
        parent->traverseAncestors(callback);
    }
}

void Object3D::updateMatrix() {
    matrix.compose(position, quaternion, scale);
    if (pivot) {
        const auto e = matrix.elements();
        const auto px = pivot->x(), py = pivot->y(), pz = pivot->z();
        matrix.setPosition(e[12] + px - e[0] * px - e[4] * py - e[8] * pz,
                           e[13] + py - e[1] * px - e[5] * py - e[9] * pz,
                           e[14] + pz - e[2] * px - e[6] * py - e[10] * pz);
    }
    matrixWorldNeedsUpdate = true;
}
void Object3D::updateMatrixWorld(bool force) {
    if (matrixAutoUpdate)
        updateMatrix();
    if (matrixWorldNeedsUpdate || force) {
        if (matrixWorldAutoUpdate) {
            if (!parent)
                matrixWorld.copy(matrix);
            else
                matrixWorld.multiplyMatrices(parent->matrixWorld, matrix);
        }
        matrixWorldNeedsUpdate = false;
        force = true;
    }
    for (const auto& child : children)
        child->updateMatrixWorld(force);
}
void Object3D::updateWorldMatrix(bool updateParents, bool updateChildren, bool force) {
    if (updateParents && parent)
        parent->updateWorldMatrix(true, false);
    if (matrixAutoUpdate)
        updateMatrix();
    if (matrixWorldNeedsUpdate || force) {
        if (matrixWorldAutoUpdate) {
            if (!parent)
                matrixWorld.copy(matrix);
            else
                matrixWorld.multiplyMatrices(parent->matrixWorld, matrix);
        }
        matrixWorldNeedsUpdate = false;
        force = true;
    }
    if (updateChildren)
        for (const auto& child : children)
            child->updateWorldMatrix(false, true, force);
}

std::shared_ptr<Object3D> Object3D::clone(bool recursive) const {
    auto result = std::make_shared<Object3D>();
    result->copy(*this, recursive);
    return result;
}
Object3D& Object3D::copy(const Object3D& source, bool recursive) {
    if (&source == this)
        return *this;
    name = source.name;
    up.copy(source.up);
    position.copy(source.position);
    rotation.setOrder(source.rotation.order());
    quaternion.copy(source.quaternion);
    scale.copy(source.scale);
    pivot = source.pivot;
    matrix.copy(source.matrix);
    matrixWorld.copy(source.matrixWorld);
    matrixAutoUpdate = source.matrixAutoUpdate;
    matrixWorldAutoUpdate = source.matrixWorldAutoUpdate;
    matrixWorldNeedsUpdate = source.matrixWorldNeedsUpdate;
    layers.mask = source.layers.mask;
    visible = source.visible;
    castShadow = source.castShadow;
    receiveShadow = source.receiveShadow;
    frustumCulled = source.frustumCulled;
    renderOrder = source.renderOrder;
    isStatic = source.isStatic;
    animations = source.animations;
    userData = source.userData.copyJSON();
    if (recursive)
        for (const auto& child : source.children)
            add(child->clone());
    return *this;
}

OrderedMap<JsonValue>& Object3DJSONContext::library(const std::string& name) {
    if (name == "geometries")
        return geometries;
    if (name == "materials")
        return materials;
    if (name == "textures")
        return textures;
    if (name == "images")
        return images;
    if (name == "shapes")
        return shapes;
    if (name == "skeletons")
        return skeletons;
    if (name == "animations")
        return animations;
    if (name == "nodes")
        return nodes;
    throw std::invalid_argument("Unknown Object3D JSON resource library: " + name);
}

void Object3DJSONContext::cacheResource(const std::string& name, std::string uuid,
                                        const std::function<JsonValue()>& serialize) {
    auto& cache = library(name);
    if (!cache.contains(uuid)) {
        // Insert only after the virtual serializer returns, matching upstream assignment order.
        auto resource = serialize();
        cache[uuid] = std::move(resource);
    }
}

void Object3DJSONContext::extractCaches(JsonValue& output) {
    for (const auto* name : {"geometries", "materials", "textures", "images", "shapes", "skeletons",
                             "animations", "nodes"}) {
        auto& cache = library(name);
        if (cache.empty())
            continue;
        JsonValue::Array values;
        for (auto& [uuid, resource] : cache) {
            resource.get<JsonValue::Object>().erase("metadata");
            values.push_back(resource);
        }
        output[name] = std::move(values);
    }
}

JsonValue Object3D::toJSON(bool root) const {
    if (root)
        return toJSON(static_cast<Object3DJSONContext*>(nullptr));
    Object3DJSONContext meta;
    auto output = toJSON(&meta);
    meta.extractCaches(output);
    return output;
}

void Object3D::serializeObjectResources(Object3DJSONContext&, JsonValue&) const {}

JsonValue Object3D::toJSON(Object3DJSONContext* meta) const {
    const bool root = meta == nullptr;
    Object3DJSONContext localMeta;
    if (root)
        meta = &localMeta;
    JsonValue output = JsonValue::Object{};
    if (root)
        output["metadata"] = JsonValue::Object{
            {"version", 4.7}, {"type", "Object"}, {"generator", "Object3D.toJSON"}};
    JsonValue object = JsonValue::Object{{"uuid", uuid},
                                         {"type", type},
                                         {"name", name},
                                         {"castShadow", castShadow},
                                         {"receiveShadow", receiveShadow},
                                         {"visible", visible},
                                         {"frustumCulled", frustumCulled},
                                         {"renderOrder", renderOrder},
                                         {"static", isStatic},
                                         {"matrixAutoUpdate", matrixAutoUpdate},
                                         {"layers", layers.mask}};
    if (userData.size() > 0)
        object["userData"] = userData;
    JsonValue::Array matrixArray;
    for (const auto element : matrix.elements())
        matrixArray.emplace_back(element);
    object["matrix"] = std::move(matrixArray);
    object["up"] = vectorJSON(up);
    if (pivot)
        object["pivot"] = vectorJSON(*pivot);
    serializeObjectResources(*meta, object);
    if (!children.empty()) {
        JsonValue::Array childArray;
        for (const auto& child : children) {
            const auto childOutput = child->toJSON(meta);
            childArray.push_back(childOutput.at("object"));
        }
        object["children"] = std::move(childArray);
    }
    if (!animations.empty())
        throw std::logic_error("AnimationClip JSON serialization is not implemented");
    if (root)
        meta->extractCaches(output);
    output["object"] = std::move(object);
    return output;
}
void Object3D::dispose() {
    dispatchEvent(Event{"dispose"});
}
