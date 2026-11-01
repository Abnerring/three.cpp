#include "core/Object3D.h"
#include "animation/AnimationClip.h"
#include "math/MathUtils.h"
#include <algorithm>
#include <atomic>
#include <stdexcept>

namespace {
std::atomic<std::uint64_t> nextObjectId{0};
const Vector3 xAxis{1, 0, 0}, yAxis{0, 1, 0}, zAxis{0, 0, 1};
JsonValue vectorJSON(const Vector3& vector) {
    return JsonValue::Array{vector.x, vector.y, vector.z};
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
    const auto oldChildren = children.slice();
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
    for (std::size_t i = 0, count = descendants.size(); i < count; ++i) {
        if (i >= descendants.size() || !descendants[i]) throw JSTypeError("Object3D child is undefined");
        const auto child = descendants[i]; child->traverse(callback);
    }
}
void Object3D::traverseVisible(const std::function<void(Object3D&)>& callback) {
    if (!visible)
        return;
    callback(*this);
    const auto descendants = children;
    for (std::size_t i = 0, count = descendants.size(); i < count; ++i) {
        if (i >= descendants.size() || !descendants[i]) throw JSTypeError("Object3D child is undefined");
        const auto child = descendants[i]; child->traverseVisible(callback);
    }
}
void Object3D::traverseAncestors(const std::function<void(Object3D&)>& callback) {
    const auto ancestor = parent;
    const auto keepAlive = ancestor ? ancestor->weak_from_this().lock() : nullptr;
    if (ancestor) {
        callback(*ancestor);
        ancestor->traverseAncestors(callback);
    }
}

void Object3D::updateMatrix() {
    matrix.compose(position, quaternion, scale);
    if (pivot) {
        const auto e = matrix.elements();
        const auto px = pivot->x, py = pivot->y, pz = pivot->z;
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
    const auto descendants = children;
    for (std::size_t i = 0, count = descendants.size(); i < count; ++i) {
        if (i >= descendants.size() || !descendants[i]) throw JSTypeError("Object3D child is undefined");
        const auto child = descendants[i]; child->updateMatrixWorld(force);
    }
}
void Object3D::updateWorldMatrix(bool updateParents, bool updateChildren, bool force) {
    const auto ancestor = parent;
    const auto keepAlive = ancestor ? ancestor->weak_from_this().lock() : nullptr;
    if (updateParents && ancestor)
        ancestor->updateWorldMatrix(true, false);
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
    if (updateChildren) {
        const auto descendants = children;
        for (std::size_t i = 0, count = descendants.size(); i < count; ++i) {
            if (i >= descendants.size() || !descendants[i]) throw JSTypeError("Object3D child is undefined");
            const auto child = descendants[i]; child->updateWorldMatrix(false, true, force);
        }
    }
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
    rotation.setOrder(source.rotation.order);
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
        for (std::size_t i = 0; i < source.children.size(); ++i) {
            const auto child = source.children[i];
            if (!child) throw JSTypeError("Object3D child is undefined");
            add(child->clone());
        }
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
                                         {"matrixAutoUpdate", matrixAutoUpdate}};
    if (userData.size() > 0)
        object["userData"] = userData.sharedCopy();
    if (const auto value = layers.mask.value().toJSON()) object["layers"] = *value;
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
    if (!animations.empty()) {
        JsonValue::Array references;
        for (std::size_t i = 0; i < animations.size(); ++i) {
            const auto animation = animations[i];
            if (!animation) throw JSTypeError("Cannot read properties of null (reading 'uuid')");
            meta->cacheResource("animations", animation->uuid.toString(), [&] { return animation->toJSONValue(); });
            references.push_back(animation->uuid.toJSON().value_or(JsonValue{}));
        }
        object["animations"] = std::move(references);
    }
    if (root)
        meta->extractCaches(output);
    output["object"] = std::move(object);
    return output;
}
void Object3D::dispose() {
    dispatchEvent(Event{"dispose"});
}

// Complete native statement port of pinned Object3D.js; see THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "math/JSNumberMath.h"
#include "core/JSStringEncoding.h"
#include "core/Object3DRuntimeSource.inc"
#include "utils.h"
#include <string_view>
#include <unordered_set>
namespace Object3DManaged {

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
class ForInCursor{JSValue object;std::vector<JSValue> keys;std::unordered_set<std::string> visited;std::size_t index=0;bool loaded=false;public:explicit ForInCursor(JSValue source):object(source.isNull()||source.isUndefined()?JSValue(nullptr):source.toObject()){}std::optional<JSValue> next(){while(!object.isNull()){if(!loaded){keys=object.ownPropertyKeys();index=0;loaded=true;}while(index<keys.size()){const auto key=keys[index++];if(key.isSymbol())continue;const auto name=key.string();if(visited.contains(name))continue;const auto d=object.getOwnPropertyDescriptor(key);if(!d)continue;visited.insert(name);if(d->enumerable.value_or(false))return key;}object=object.getPrototype();loaded=false;}return std::nullopt;}};

JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
JSValue nested0(const JSValue&,const Args&,const Args&);
JSValue nested1(const JSValue&,const Args&,const Args&);
JSValue nested2(const JSValue&,const Args&,const Args&);
JSValue nested3(const JSValue&,const Args&,const Args&);
JSValue nested4(const JSValue&,const Args&,const Args&);
JSValue nested5(const JSValue&,const Args&,const Args&);
JSValue nested0(const JSValue& nativeStatementTemp74,const Args& nativeStatementTemp75,const Args& nativeStatementTemp76){
auto nativeStatementTemp77=nativeStatementTemp76[0];auto nativeStatementTemp78=nativeStatementTemp74;
([&]()->JSValue{auto nativeStatementTemp83=nativeStatementTemp76[1];const auto nativeStatementTemp84=JSValue("setFromEuler");const auto nativeStatementTemp80=nativeStatementTemp83.get(nativeStatementTemp84);const auto nativeStatementTemp81=nativeStatementTemp83;Args nativeStatementTemp82;nativeStatementTemp82.push_back(nativeStatementTemp76[2]);nativeStatementTemp82.push_back(JSValue(false));return nativeStatementTemp80.call(nativeStatementTemp81,nativeStatementTemp82);}());
return JSValue{};
}

JSValue nested1(const JSValue& nativeStatementTemp86,const Args& nativeStatementTemp87,const Args& nativeStatementTemp88){
auto nativeStatementTemp89=nativeStatementTemp88[0];auto nativeStatementTemp90=nativeStatementTemp86;
([&]()->JSValue{auto nativeStatementTemp95=nativeStatementTemp88[1];const auto nativeStatementTemp96=JSValue("setFromQuaternion");const auto nativeStatementTemp92=nativeStatementTemp95.get(nativeStatementTemp96);const auto nativeStatementTemp93=nativeStatementTemp95;Args nativeStatementTemp94;nativeStatementTemp94.push_back(nativeStatementTemp88[2]);nativeStatementTemp94.push_back(JSValue{});nativeStatementTemp94.push_back(JSValue(false));return nativeStatementTemp92.call(nativeStatementTemp93,nativeStatementTemp94);}());
return JSValue{};
}

JSValue nested2(const JSValue& nativeStatementTemp1440,const Args& nativeStatementTemp1441,const Args& nativeStatementTemp1442){
auto nativeStatementTemp1443=nativeStatementTemp1442[0];auto nativeStatementTemp1444=nativeStatementTemp1440;
JSValue cache=arg(nativeStatementTemp1441,0);
JSValue values=([&]()->JSValue{auto nativeStatementTemp1446=JSValue::array();return nativeStatementTemp1446;}());
{ForInCursor nativeStatementTemp1447(cache);while(const auto nativeStatementTemp1448=nativeStatementTemp1447.next()){const auto key=*nativeStatementTemp1448;{
JSValue data=([&]()->JSValue{const auto nativeStatementTemp1449=cache;const auto nativeStatementTemp1450=key;return nativeStatementTemp1449.get(nativeStatementTemp1450);}());
([&]()->JSValue{auto nativeStatementTemp1451=data;const auto nativeStatementTemp1452=JSValue("metadata");return deleteReference(nativeStatementTemp1451,nativeStatementTemp1452);}());
([&]()->JSValue{auto nativeStatementTemp1456=values;const auto nativeStatementTemp1457=JSValue("push");const auto nativeStatementTemp1453=nativeStatementTemp1456.get(nativeStatementTemp1457);const auto nativeStatementTemp1454=nativeStatementTemp1456;Args nativeStatementTemp1455;nativeStatementTemp1455.push_back(data);return nativeStatementTemp1453.call(nativeStatementTemp1454,nativeStatementTemp1455);}());

}
}}
return values;
return JSValue{};
}

JSValue nested3(const JSValue& nativeStatementTemp1766,const Args& nativeStatementTemp1767,const Args& nativeStatementTemp1768){
auto nativeStatementTemp1769=nativeStatementTemp1768[0];auto nativeStatementTemp1770=nativeStatementTemp1766;
JSValue info=arg(nativeStatementTemp1767,0);
return ([&]()->JSValue{auto nativeStatementTemp1772=JSValue::object();copyDataProperties(nativeStatementTemp1772,info);{const auto nativeStatementTemp1773=JSValue("boundingBox");const auto nativeStatementTemp1774=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp1775=info;const auto nativeStatementTemp1776=JSValue("boundingBox");return nativeStatementTemp1775.get(nativeStatementTemp1776);}()).toBoolean())return ([&]()->JSValue{auto nativeStatementTemp1780=([&]()->JSValue{const auto nativeStatementTemp1782=info;const auto nativeStatementTemp1783=JSValue("boundingBox");return nativeStatementTemp1782.get(nativeStatementTemp1783);}());const auto nativeStatementTemp1781=JSValue("toJSON");const auto nativeStatementTemp1777=nativeStatementTemp1780.get(nativeStatementTemp1781);const auto nativeStatementTemp1778=nativeStatementTemp1780;Args nativeStatementTemp1779;return nativeStatementTemp1777.call(nativeStatementTemp1778,nativeStatementTemp1779);}());return JSValue{};}());own(nativeStatementTemp1772,nativeStatementTemp1773,nativeStatementTemp1774);}{const auto nativeStatementTemp1784=JSValue("boundingSphere");const auto nativeStatementTemp1785=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp1786=info;const auto nativeStatementTemp1787=JSValue("boundingSphere");return nativeStatementTemp1786.get(nativeStatementTemp1787);}()).toBoolean())return ([&]()->JSValue{auto nativeStatementTemp1791=([&]()->JSValue{const auto nativeStatementTemp1793=info;const auto nativeStatementTemp1794=JSValue("boundingSphere");return nativeStatementTemp1793.get(nativeStatementTemp1794);}());const auto nativeStatementTemp1792=JSValue("toJSON");const auto nativeStatementTemp1788=nativeStatementTemp1791.get(nativeStatementTemp1792);const auto nativeStatementTemp1789=nativeStatementTemp1791;Args nativeStatementTemp1790;return nativeStatementTemp1788.call(nativeStatementTemp1789,nativeStatementTemp1790);}());return JSValue{};}());own(nativeStatementTemp1772,nativeStatementTemp1784,nativeStatementTemp1785);}return nativeStatementTemp1772;}());
}

JSValue nested4(const JSValue& nativeStatementTemp1808,const Args& nativeStatementTemp1809,const Args& nativeStatementTemp1810){
auto nativeStatementTemp1811=nativeStatementTemp1810[0];auto nativeStatementTemp1812=nativeStatementTemp1808;
JSValue info=arg(nativeStatementTemp1809,0);
return ([&]()->JSValue{auto nativeStatementTemp1814=JSValue::object();copyDataProperties(nativeStatementTemp1814,info);return nativeStatementTemp1814;}());
}

JSValue nested5(const JSValue& nativeStatementTemp1961,const Args& nativeStatementTemp1962,const Args& nativeStatementTemp1963){
auto nativeStatementTemp1964=nativeStatementTemp1963[0];auto nativeStatementTemp1965=nativeStatementTemp1961;
JSValue library=arg(nativeStatementTemp1962,0);
JSValue element=arg(nativeStatementTemp1962,1);
if(([&]()->JSValue{const auto nativeStatementTemp1967=([&]()->JSValue{const auto nativeStatementTemp1969=library;const auto nativeStatementTemp1970=([&]()->JSValue{const auto nativeStatementTemp1971=element;const auto nativeStatementTemp1972=JSValue("uuid");return nativeStatementTemp1971.get(nativeStatementTemp1972);}());return nativeStatementTemp1969.get(nativeStatementTemp1970);}());const auto nativeStatementTemp1968=JSValue{};return binary("===",nativeStatementTemp1967,nativeStatementTemp1968);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp1973=library;const auto nativeStatementTemp1974=([&]()->JSValue{const auto nativeStatementTemp1975=element;const auto nativeStatementTemp1976=JSValue("uuid");return nativeStatementTemp1975.get(nativeStatementTemp1976);}());const auto nativeStatementTemp1977=([&]()->JSValue{auto nativeStatementTemp1983=element;const auto nativeStatementTemp1984=JSValue("toJSON");const auto nativeStatementTemp1980=nativeStatementTemp1983.get(nativeStatementTemp1984);const auto nativeStatementTemp1981=nativeStatementTemp1983;Args nativeStatementTemp1982;nativeStatementTemp1982.push_back(nativeStatementTemp1963[1]);return nativeStatementTemp1980.call(nativeStatementTemp1981,nativeStatementTemp1982);}());setReference(nativeStatementTemp1973,nativeStatementTemp1974,nativeStatementTemp1977);return nativeStatementTemp1977;}());

}
return ([&]()->JSValue{const auto nativeStatementTemp1985=element;const auto nativeStatementTemp1986=JSValue("uuid");return nativeStatementTemp1985.get(nativeStatementTemp1986);}());
return JSValue{};
}
JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
const auto nativeStatementTemp5=nativeStatementTemp0;nativeStatementTemp4={};
([&]()->JSValue{const auto nativeStatementTemp9=nativeStatementTemp3.get("Object3D").getPrototype();Args nativeStatementTemp8;nativeStatementTemp4=nativeStatementTemp9.construct(nativeStatementTemp8,nativeStatementTemp5);return nativeStatementTemp4;}());
([&]()->JSValue{auto nativeStatementTemp10=nativeStatementTemp4;const auto nativeStatementTemp11=JSValue("isObject3D");const auto nativeStatementTemp12=JSValue(true);setReference(nativeStatementTemp10,nativeStatementTemp11,nativeStatementTemp12);return nativeStatementTemp12;}());
([&]()->JSValue{auto nativeStatementTemp18=nativeGlobalBinding("Object");const auto nativeStatementTemp19=JSValue("defineProperty");const auto nativeStatementTemp15=nativeStatementTemp18.get(nativeStatementTemp19);const auto nativeStatementTemp16=nativeStatementTemp18;Args nativeStatementTemp17;nativeStatementTemp17.push_back(nativeStatementTemp4);nativeStatementTemp17.push_back(JSValue("id"));nativeStatementTemp17.push_back(([&]()->JSValue{auto nativeStatementTemp20=JSValue::object();{const auto nativeStatementTemp21=JSValue("value");const auto nativeStatementTemp22=([&]()->JSValue{const auto nativeStatementTemp23=nativeStatementTemp3.get("_object3DId").toNumber();const auto nativeStatementTemp24=nativeStatementTemp23+1;nativeStatementTemp3.set("_object3DId",nativeStatementTemp24);return nativeStatementTemp23;}());own(nativeStatementTemp20,nativeStatementTemp21,nativeStatementTemp22);}return nativeStatementTemp20;}()));return nativeStatementTemp15.call(nativeStatementTemp16,nativeStatementTemp17);}());
([&]()->JSValue{auto nativeStatementTemp25=nativeStatementTemp4;const auto nativeStatementTemp26=JSValue("uuid");const auto nativeStatementTemp27=([&]()->JSValue{const auto nativeStatementTemp30=nativeStatementTemp3.get("generateUUID");const JSValue nativeStatementTemp31;Args nativeStatementTemp32;return nativeStatementTemp30.call(nativeStatementTemp31,nativeStatementTemp32);}());setReference(nativeStatementTemp25,nativeStatementTemp26,nativeStatementTemp27);return nativeStatementTemp27;}());
([&]()->JSValue{auto nativeStatementTemp33=nativeStatementTemp4;const auto nativeStatementTemp34=JSValue("name");const auto nativeStatementTemp35=JSValue("");setReference(nativeStatementTemp33,nativeStatementTemp34,nativeStatementTemp35);return nativeStatementTemp35;}());
([&]()->JSValue{auto nativeStatementTemp38=nativeStatementTemp4;const auto nativeStatementTemp39=JSValue("type");const auto nativeStatementTemp40=JSValue("Object3D");setReference(nativeStatementTemp38,nativeStatementTemp39,nativeStatementTemp40);return nativeStatementTemp40;}());
([&]()->JSValue{auto nativeStatementTemp43=nativeStatementTemp4;const auto nativeStatementTemp44=JSValue("parent");const auto nativeStatementTemp45=JSValue(nullptr);setReference(nativeStatementTemp43,nativeStatementTemp44,nativeStatementTemp45);return nativeStatementTemp45;}());
([&]()->JSValue{auto nativeStatementTemp48=nativeStatementTemp4;const auto nativeStatementTemp49=JSValue("children");const auto nativeStatementTemp50=([&]()->JSValue{auto nativeStatementTemp53=JSValue::array();return nativeStatementTemp53;}());setReference(nativeStatementTemp48,nativeStatementTemp49,nativeStatementTemp50);return nativeStatementTemp50;}());
([&]()->JSValue{auto nativeStatementTemp54=nativeStatementTemp4;const auto nativeStatementTemp55=JSValue("up");const auto nativeStatementTemp56=([&]()->JSValue{auto nativeStatementTemp62=([&]()->JSValue{const auto nativeStatementTemp64=nativeStatementTemp3.get("Object3D");const auto nativeStatementTemp65=JSValue("DEFAULT_UP");return nativeStatementTemp64.get(nativeStatementTemp65);}());const auto nativeStatementTemp63=JSValue("clone");const auto nativeStatementTemp59=nativeStatementTemp62.get(nativeStatementTemp63);const auto nativeStatementTemp60=nativeStatementTemp62;Args nativeStatementTemp61;return nativeStatementTemp59.call(nativeStatementTemp60,nativeStatementTemp61);}());setReference(nativeStatementTemp54,nativeStatementTemp55,nativeStatementTemp56);return nativeStatementTemp56;}());
JSValue position=([&]()->JSValue{const auto nativeStatementTemp66=nativeStatementTemp3.get("Vector3");Args nativeStatementTemp67;return nativeStatementTemp66.construct(nativeStatementTemp67);}());
JSValue rotation=([&]()->JSValue{const auto nativeStatementTemp68=nativeStatementTemp3.get("Euler");Args nativeStatementTemp69;return nativeStatementTemp68.construct(nativeStatementTemp69);}());
JSValue quaternion=([&]()->JSValue{const auto nativeStatementTemp70=nativeStatementTemp3.get("Quaternion");Args nativeStatementTemp71;return nativeStatementTemp70.construct(nativeStatementTemp71);}());
JSValue scale=([&]()->JSValue{const auto nativeStatementTemp72=nativeStatementTemp3.get("Vector3");Args nativeStatementTemp73;nativeStatementTemp73.push_back(JSValue(1));nativeStatementTemp73.push_back(JSValue(1));nativeStatementTemp73.push_back(JSValue(1));return nativeStatementTemp72.construct(nativeStatementTemp73);}());
JSValue onRotationChange=([&]()->JSValue{auto nativeStatementTemp85=JSValue::constructor(nested0,{nativeStatementTemp3,quaternion,rotation},"onRotationChange",0,JSValue::ConstructorKind::Callable);nativeStatementTemp85.setFunctionSourceText("function onRotationChange() {\r\n\r\n\t\t\tquaternion.setFromEuler( rotation, false );\r\n\r\n\t\t}");return nativeStatementTemp85;}());
JSValue onQuaternionChange=([&]()->JSValue{auto nativeStatementTemp97=JSValue::constructor(nested1,{nativeStatementTemp3,rotation,quaternion},"onQuaternionChange",0,JSValue::ConstructorKind::Callable);nativeStatementTemp97.setFunctionSourceText("function onQuaternionChange() {\r\n\r\n\t\t\trotation.setFromQuaternion( quaternion, undefined, false );\r\n\r\n\t\t}");return nativeStatementTemp97;}());
([&]()->JSValue{auto nativeStatementTemp101=rotation;const auto nativeStatementTemp102=JSValue("_onChange");const auto nativeStatementTemp98=nativeStatementTemp101.get(nativeStatementTemp102);const auto nativeStatementTemp99=nativeStatementTemp101;Args nativeStatementTemp100;nativeStatementTemp100.push_back(onRotationChange);return nativeStatementTemp98.call(nativeStatementTemp99,nativeStatementTemp100);}());
([&]()->JSValue{auto nativeStatementTemp106=quaternion;const auto nativeStatementTemp107=JSValue("_onChange");const auto nativeStatementTemp103=nativeStatementTemp106.get(nativeStatementTemp107);const auto nativeStatementTemp104=nativeStatementTemp106;Args nativeStatementTemp105;nativeStatementTemp105.push_back(onQuaternionChange);return nativeStatementTemp103.call(nativeStatementTemp104,nativeStatementTemp105);}());
([&]()->JSValue{auto nativeStatementTemp111=nativeGlobalBinding("Object");const auto nativeStatementTemp112=JSValue("defineProperties");const auto nativeStatementTemp108=nativeStatementTemp111.get(nativeStatementTemp112);const auto nativeStatementTemp109=nativeStatementTemp111;Args nativeStatementTemp110;nativeStatementTemp110.push_back(nativeStatementTemp4);nativeStatementTemp110.push_back(([&]()->JSValue{auto nativeStatementTemp113=JSValue::object();{const auto nativeStatementTemp114=JSValue("position");const auto nativeStatementTemp115=([&]()->JSValue{auto nativeStatementTemp116=JSValue::object();{const auto nativeStatementTemp117=JSValue("configurable");const auto nativeStatementTemp118=JSValue(true);own(nativeStatementTemp116,nativeStatementTemp117,nativeStatementTemp118);}{const auto nativeStatementTemp119=JSValue("enumerable");const auto nativeStatementTemp120=JSValue(true);own(nativeStatementTemp116,nativeStatementTemp119,nativeStatementTemp120);}{const auto nativeStatementTemp121=JSValue("value");const auto nativeStatementTemp122=position;own(nativeStatementTemp116,nativeStatementTemp121,nativeStatementTemp122);}return nativeStatementTemp116;}());own(nativeStatementTemp113,nativeStatementTemp114,nativeStatementTemp115);}{const auto nativeStatementTemp123=JSValue("rotation");const auto nativeStatementTemp124=([&]()->JSValue{auto nativeStatementTemp125=JSValue::object();{const auto nativeStatementTemp126=JSValue("configurable");const auto nativeStatementTemp127=JSValue(true);own(nativeStatementTemp125,nativeStatementTemp126,nativeStatementTemp127);}{const auto nativeStatementTemp128=JSValue("enumerable");const auto nativeStatementTemp129=JSValue(true);own(nativeStatementTemp125,nativeStatementTemp128,nativeStatementTemp129);}{const auto nativeStatementTemp130=JSValue("value");const auto nativeStatementTemp131=rotation;own(nativeStatementTemp125,nativeStatementTemp130,nativeStatementTemp131);}return nativeStatementTemp125;}());own(nativeStatementTemp113,nativeStatementTemp123,nativeStatementTemp124);}{const auto nativeStatementTemp132=JSValue("quaternion");const auto nativeStatementTemp133=([&]()->JSValue{auto nativeStatementTemp134=JSValue::object();{const auto nativeStatementTemp135=JSValue("configurable");const auto nativeStatementTemp136=JSValue(true);own(nativeStatementTemp134,nativeStatementTemp135,nativeStatementTemp136);}{const auto nativeStatementTemp137=JSValue("enumerable");const auto nativeStatementTemp138=JSValue(true);own(nativeStatementTemp134,nativeStatementTemp137,nativeStatementTemp138);}{const auto nativeStatementTemp139=JSValue("value");const auto nativeStatementTemp140=quaternion;own(nativeStatementTemp134,nativeStatementTemp139,nativeStatementTemp140);}return nativeStatementTemp134;}());own(nativeStatementTemp113,nativeStatementTemp132,nativeStatementTemp133);}{const auto nativeStatementTemp141=JSValue("scale");const auto nativeStatementTemp142=([&]()->JSValue{auto nativeStatementTemp143=JSValue::object();{const auto nativeStatementTemp144=JSValue("configurable");const auto nativeStatementTemp145=JSValue(true);own(nativeStatementTemp143,nativeStatementTemp144,nativeStatementTemp145);}{const auto nativeStatementTemp146=JSValue("enumerable");const auto nativeStatementTemp147=JSValue(true);own(nativeStatementTemp143,nativeStatementTemp146,nativeStatementTemp147);}{const auto nativeStatementTemp148=JSValue("value");const auto nativeStatementTemp149=scale;own(nativeStatementTemp143,nativeStatementTemp148,nativeStatementTemp149);}return nativeStatementTemp143;}());own(nativeStatementTemp113,nativeStatementTemp141,nativeStatementTemp142);}{const auto nativeStatementTemp150=JSValue("modelViewMatrix");const auto nativeStatementTemp151=([&]()->JSValue{auto nativeStatementTemp152=JSValue::object();{const auto nativeStatementTemp153=JSValue("value");const auto nativeStatementTemp154=([&]()->JSValue{const auto nativeStatementTemp155=nativeStatementTemp3.get("Matrix4");Args nativeStatementTemp156;return nativeStatementTemp155.construct(nativeStatementTemp156);}());own(nativeStatementTemp152,nativeStatementTemp153,nativeStatementTemp154);}return nativeStatementTemp152;}());own(nativeStatementTemp113,nativeStatementTemp150,nativeStatementTemp151);}{const auto nativeStatementTemp157=JSValue("normalMatrix");const auto nativeStatementTemp158=([&]()->JSValue{auto nativeStatementTemp159=JSValue::object();{const auto nativeStatementTemp160=JSValue("value");const auto nativeStatementTemp161=([&]()->JSValue{const auto nativeStatementTemp162=nativeStatementTemp3.get("Matrix3");Args nativeStatementTemp163;return nativeStatementTemp162.construct(nativeStatementTemp163);}());own(nativeStatementTemp159,nativeStatementTemp160,nativeStatementTemp161);}return nativeStatementTemp159;}());own(nativeStatementTemp113,nativeStatementTemp157,nativeStatementTemp158);}return nativeStatementTemp113;}()));return nativeStatementTemp108.call(nativeStatementTemp109,nativeStatementTemp110);}());
([&]()->JSValue{auto nativeStatementTemp164=nativeStatementTemp4;const auto nativeStatementTemp165=JSValue("matrix");const auto nativeStatementTemp166=([&]()->JSValue{const auto nativeStatementTemp169=nativeStatementTemp3.get("Matrix4");Args nativeStatementTemp170;return nativeStatementTemp169.construct(nativeStatementTemp170);}());setReference(nativeStatementTemp164,nativeStatementTemp165,nativeStatementTemp166);return nativeStatementTemp166;}());
([&]()->JSValue{auto nativeStatementTemp171=nativeStatementTemp4;const auto nativeStatementTemp172=JSValue("matrixWorld");const auto nativeStatementTemp173=([&]()->JSValue{const auto nativeStatementTemp176=nativeStatementTemp3.get("Matrix4");Args nativeStatementTemp177;return nativeStatementTemp176.construct(nativeStatementTemp177);}());setReference(nativeStatementTemp171,nativeStatementTemp172,nativeStatementTemp173);return nativeStatementTemp173;}());
([&]()->JSValue{auto nativeStatementTemp178=nativeStatementTemp4;const auto nativeStatementTemp179=JSValue("matrixAutoUpdate");const auto nativeStatementTemp180=([&]()->JSValue{const auto nativeStatementTemp183=nativeStatementTemp3.get("Object3D");const auto nativeStatementTemp184=JSValue("DEFAULT_MATRIX_AUTO_UPDATE");return nativeStatementTemp183.get(nativeStatementTemp184);}());setReference(nativeStatementTemp178,nativeStatementTemp179,nativeStatementTemp180);return nativeStatementTemp180;}());
([&]()->JSValue{auto nativeStatementTemp185=nativeStatementTemp4;const auto nativeStatementTemp186=JSValue("matrixWorldAutoUpdate");const auto nativeStatementTemp187=([&]()->JSValue{const auto nativeStatementTemp190=nativeStatementTemp3.get("Object3D");const auto nativeStatementTemp191=JSValue("DEFAULT_MATRIX_WORLD_AUTO_UPDATE");return nativeStatementTemp190.get(nativeStatementTemp191);}());setReference(nativeStatementTemp185,nativeStatementTemp186,nativeStatementTemp187);return nativeStatementTemp187;}());
([&]()->JSValue{auto nativeStatementTemp192=nativeStatementTemp4;const auto nativeStatementTemp193=JSValue("matrixWorldNeedsUpdate");const auto nativeStatementTemp194=JSValue(false);setReference(nativeStatementTemp192,nativeStatementTemp193,nativeStatementTemp194);return nativeStatementTemp194;}());
([&]()->JSValue{auto nativeStatementTemp197=nativeStatementTemp4;const auto nativeStatementTemp198=JSValue("layers");const auto nativeStatementTemp199=([&]()->JSValue{const auto nativeStatementTemp202=nativeStatementTemp3.get("Layers");Args nativeStatementTemp203;return nativeStatementTemp202.construct(nativeStatementTemp203);}());setReference(nativeStatementTemp197,nativeStatementTemp198,nativeStatementTemp199);return nativeStatementTemp199;}());
([&]()->JSValue{auto nativeStatementTemp204=nativeStatementTemp4;const auto nativeStatementTemp205=JSValue("visible");const auto nativeStatementTemp206=JSValue(true);setReference(nativeStatementTemp204,nativeStatementTemp205,nativeStatementTemp206);return nativeStatementTemp206;}());
([&]()->JSValue{auto nativeStatementTemp209=nativeStatementTemp4;const auto nativeStatementTemp210=JSValue("castShadow");const auto nativeStatementTemp211=JSValue(false);setReference(nativeStatementTemp209,nativeStatementTemp210,nativeStatementTemp211);return nativeStatementTemp211;}());
([&]()->JSValue{auto nativeStatementTemp214=nativeStatementTemp4;const auto nativeStatementTemp215=JSValue("receiveShadow");const auto nativeStatementTemp216=JSValue(false);setReference(nativeStatementTemp214,nativeStatementTemp215,nativeStatementTemp216);return nativeStatementTemp216;}());
([&]()->JSValue{auto nativeStatementTemp219=nativeStatementTemp4;const auto nativeStatementTemp220=JSValue("frustumCulled");const auto nativeStatementTemp221=JSValue(true);setReference(nativeStatementTemp219,nativeStatementTemp220,nativeStatementTemp221);return nativeStatementTemp221;}());
([&]()->JSValue{auto nativeStatementTemp224=nativeStatementTemp4;const auto nativeStatementTemp225=JSValue("renderOrder");const auto nativeStatementTemp226=JSValue(0);setReference(nativeStatementTemp224,nativeStatementTemp225,nativeStatementTemp226);return nativeStatementTemp226;}());
([&]()->JSValue{auto nativeStatementTemp229=nativeStatementTemp4;const auto nativeStatementTemp230=JSValue("animations");const auto nativeStatementTemp231=([&]()->JSValue{auto nativeStatementTemp234=JSValue::array();return nativeStatementTemp234;}());setReference(nativeStatementTemp229,nativeStatementTemp230,nativeStatementTemp231);return nativeStatementTemp231;}());
([&]()->JSValue{auto nativeStatementTemp235=nativeStatementTemp4;const auto nativeStatementTemp236=JSValue("customDepthMaterial");const auto nativeStatementTemp237=JSValue{};setReference(nativeStatementTemp235,nativeStatementTemp236,nativeStatementTemp237);return nativeStatementTemp237;}());
([&]()->JSValue{auto nativeStatementTemp240=nativeStatementTemp4;const auto nativeStatementTemp241=JSValue("customDistanceMaterial");const auto nativeStatementTemp242=JSValue{};setReference(nativeStatementTemp240,nativeStatementTemp241,nativeStatementTemp242);return nativeStatementTemp242;}());
([&]()->JSValue{auto nativeStatementTemp245=nativeStatementTemp4;const auto nativeStatementTemp246=JSValue("static");const auto nativeStatementTemp247=JSValue(false);setReference(nativeStatementTemp245,nativeStatementTemp246,nativeStatementTemp247);return nativeStatementTemp247;}());
([&]()->JSValue{auto nativeStatementTemp250=nativeStatementTemp4;const auto nativeStatementTemp251=JSValue("userData");const auto nativeStatementTemp252=([&]()->JSValue{auto nativeStatementTemp255=JSValue::object();return nativeStatementTemp255;}());setReference(nativeStatementTemp250,nativeStatementTemp251,nativeStatementTemp252);return nativeStatementTemp252;}());
([&]()->JSValue{auto nativeStatementTemp256=nativeStatementTemp4;const auto nativeStatementTemp257=JSValue("pivot");const auto nativeStatementTemp258=JSValue(nullptr);setReference(nativeStatementTemp256,nativeStatementTemp257,nativeStatementTemp258);return nativeStatementTemp258;}());
return nativeStatementTemp4;
}

JSValue method1(const JSValue& nativeStatementTemp261,const Args& nativeStatementTemp262,const Args& nativeStatementTemp263){
auto nativeStatementTemp264=nativeStatementTemp263[0];auto nativeStatementTemp265=nativeStatementTemp261;
return JSValue{};
}

JSValue method2(const JSValue& nativeStatementTemp267,const Args& nativeStatementTemp268,const Args& nativeStatementTemp269){
auto nativeStatementTemp270=nativeStatementTemp269[0];auto nativeStatementTemp271=nativeStatementTemp267;
return JSValue{};
}

JSValue method3(const JSValue& nativeStatementTemp273,const Args& nativeStatementTemp274,const Args& nativeStatementTemp275){
auto nativeStatementTemp276=nativeStatementTemp275[0];auto nativeStatementTemp277=nativeStatementTemp273;
return JSValue{};
}

JSValue method4(const JSValue& nativeStatementTemp279,const Args& nativeStatementTemp280,const Args& nativeStatementTemp281){
auto nativeStatementTemp282=nativeStatementTemp281[0];auto nativeStatementTemp283=nativeStatementTemp279;
return JSValue{};
}

JSValue method5(const JSValue& nativeStatementTemp285,const Args& nativeStatementTemp286,const Args& nativeStatementTemp287){
auto nativeStatementTemp288=nativeStatementTemp287[0];auto nativeStatementTemp289=nativeStatementTemp285;
JSValue matrix=arg(nativeStatementTemp286,0);
if(([&]()->JSValue{const auto nativeStatementTemp291=nativeStatementTemp289;const auto nativeStatementTemp292=JSValue("matrixAutoUpdate");return nativeStatementTemp291.get(nativeStatementTemp292);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp296=nativeStatementTemp289;const auto nativeStatementTemp297=JSValue("updateMatrix");const auto nativeStatementTemp293=nativeStatementTemp296.get(nativeStatementTemp297);const auto nativeStatementTemp294=nativeStatementTemp296;Args nativeStatementTemp295;return nativeStatementTemp293.call(nativeStatementTemp294,nativeStatementTemp295);}());
([&]()->JSValue{auto nativeStatementTemp301=([&]()->JSValue{const auto nativeStatementTemp303=nativeStatementTemp289;const auto nativeStatementTemp304=JSValue("matrix");return nativeStatementTemp303.get(nativeStatementTemp304);}());const auto nativeStatementTemp302=JSValue("premultiply");const auto nativeStatementTemp298=nativeStatementTemp301.get(nativeStatementTemp302);const auto nativeStatementTemp299=nativeStatementTemp301;Args nativeStatementTemp300;nativeStatementTemp300.push_back(matrix);return nativeStatementTemp298.call(nativeStatementTemp299,nativeStatementTemp300);}());
([&]()->JSValue{auto nativeStatementTemp308=([&]()->JSValue{const auto nativeStatementTemp310=nativeStatementTemp289;const auto nativeStatementTemp311=JSValue("matrix");return nativeStatementTemp310.get(nativeStatementTemp311);}());const auto nativeStatementTemp309=JSValue("decompose");const auto nativeStatementTemp305=nativeStatementTemp308.get(nativeStatementTemp309);const auto nativeStatementTemp306=nativeStatementTemp308;Args nativeStatementTemp307;nativeStatementTemp307.push_back(([&]()->JSValue{const auto nativeStatementTemp312=nativeStatementTemp289;const auto nativeStatementTemp313=JSValue("position");return nativeStatementTemp312.get(nativeStatementTemp313);}()));nativeStatementTemp307.push_back(([&]()->JSValue{const auto nativeStatementTemp314=nativeStatementTemp289;const auto nativeStatementTemp315=JSValue("quaternion");return nativeStatementTemp314.get(nativeStatementTemp315);}()));nativeStatementTemp307.push_back(([&]()->JSValue{const auto nativeStatementTemp316=nativeStatementTemp289;const auto nativeStatementTemp317=JSValue("scale");return nativeStatementTemp316.get(nativeStatementTemp317);}()));return nativeStatementTemp305.call(nativeStatementTemp306,nativeStatementTemp307);}());
if(([&]()->JSValue{const auto nativeStatementTemp318=([&]()->JSValue{const auto nativeStatementTemp320=nativeStatementTemp289;const auto nativeStatementTemp321=JSValue("pivot");return nativeStatementTemp320.get(nativeStatementTemp321);}());const auto nativeStatementTemp319=JSValue(nullptr);return binary("!==",nativeStatementTemp318,nativeStatementTemp319);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp325=([&]()->JSValue{auto nativeStatementTemp330=([&]()->JSValue{auto nativeStatementTemp335=([&]()->JSValue{const auto nativeStatementTemp337=nativeStatementTemp289;const auto nativeStatementTemp338=JSValue("position");return nativeStatementTemp337.get(nativeStatementTemp338);}());const auto nativeStatementTemp336=JSValue("copy");const auto nativeStatementTemp332=nativeStatementTemp335.get(nativeStatementTemp336);const auto nativeStatementTemp333=nativeStatementTemp335;Args nativeStatementTemp334;nativeStatementTemp334.push_back(([&]()->JSValue{const auto nativeStatementTemp339=nativeStatementTemp289;const auto nativeStatementTemp340=JSValue("pivot");return nativeStatementTemp339.get(nativeStatementTemp340);}()));return nativeStatementTemp332.call(nativeStatementTemp333,nativeStatementTemp334);}());const auto nativeStatementTemp331=JSValue("applyMatrix4");const auto nativeStatementTemp327=nativeStatementTemp330.get(nativeStatementTemp331);const auto nativeStatementTemp328=nativeStatementTemp330;Args nativeStatementTemp329;nativeStatementTemp329.push_back(([&]()->JSValue{const auto nativeStatementTemp341=nativeStatementTemp289;const auto nativeStatementTemp342=JSValue("matrix");return nativeStatementTemp341.get(nativeStatementTemp342);}()));return nativeStatementTemp327.call(nativeStatementTemp328,nativeStatementTemp329);}());const auto nativeStatementTemp326=JSValue("sub");const auto nativeStatementTemp322=nativeStatementTemp325.get(nativeStatementTemp326);const auto nativeStatementTemp323=nativeStatementTemp325;Args nativeStatementTemp324;nativeStatementTemp324.push_back(([&]()->JSValue{const auto nativeStatementTemp343=nativeStatementTemp289;const auto nativeStatementTemp344=JSValue("pivot");return nativeStatementTemp343.get(nativeStatementTemp344);}()));return nativeStatementTemp322.call(nativeStatementTemp323,nativeStatementTemp324);}());

}
return JSValue{};
}

JSValue method6(const JSValue& nativeStatementTemp345,const Args& nativeStatementTemp346,const Args& nativeStatementTemp347){
auto nativeStatementTemp348=nativeStatementTemp347[0];auto nativeStatementTemp349=nativeStatementTemp345;
JSValue q=arg(nativeStatementTemp346,0);
([&]()->JSValue{auto nativeStatementTemp354=([&]()->JSValue{const auto nativeStatementTemp356=nativeStatementTemp349;const auto nativeStatementTemp357=JSValue("quaternion");return nativeStatementTemp356.get(nativeStatementTemp357);}());const auto nativeStatementTemp355=JSValue("premultiply");const auto nativeStatementTemp351=nativeStatementTemp354.get(nativeStatementTemp355);const auto nativeStatementTemp352=nativeStatementTemp354;Args nativeStatementTemp353;nativeStatementTemp353.push_back(q);return nativeStatementTemp351.call(nativeStatementTemp352,nativeStatementTemp353);}());
return nativeStatementTemp349;
return JSValue{};
}

JSValue method7(const JSValue& nativeStatementTemp358,const Args& nativeStatementTemp359,const Args& nativeStatementTemp360){
auto nativeStatementTemp361=nativeStatementTemp360[0];auto nativeStatementTemp362=nativeStatementTemp358;
JSValue axis=arg(nativeStatementTemp359,0);
JSValue angle=arg(nativeStatementTemp359,1);
([&]()->JSValue{auto nativeStatementTemp367=([&]()->JSValue{const auto nativeStatementTemp369=nativeStatementTemp362;const auto nativeStatementTemp370=JSValue("quaternion");return nativeStatementTemp369.get(nativeStatementTemp370);}());const auto nativeStatementTemp368=JSValue("setFromAxisAngle");const auto nativeStatementTemp364=nativeStatementTemp367.get(nativeStatementTemp368);const auto nativeStatementTemp365=nativeStatementTemp367;Args nativeStatementTemp366;nativeStatementTemp366.push_back(axis);nativeStatementTemp366.push_back(angle);return nativeStatementTemp364.call(nativeStatementTemp365,nativeStatementTemp366);}());
return JSValue{};
}

JSValue method8(const JSValue& nativeStatementTemp371,const Args& nativeStatementTemp372,const Args& nativeStatementTemp373){
auto nativeStatementTemp374=nativeStatementTemp373[0];auto nativeStatementTemp375=nativeStatementTemp371;
JSValue euler=arg(nativeStatementTemp372,0);
([&]()->JSValue{auto nativeStatementTemp380=([&]()->JSValue{const auto nativeStatementTemp382=nativeStatementTemp375;const auto nativeStatementTemp383=JSValue("quaternion");return nativeStatementTemp382.get(nativeStatementTemp383);}());const auto nativeStatementTemp381=JSValue("setFromEuler");const auto nativeStatementTemp377=nativeStatementTemp380.get(nativeStatementTemp381);const auto nativeStatementTemp378=nativeStatementTemp380;Args nativeStatementTemp379;nativeStatementTemp379.push_back(euler);nativeStatementTemp379.push_back(JSValue(true));return nativeStatementTemp377.call(nativeStatementTemp378,nativeStatementTemp379);}());
return JSValue{};
}

JSValue method9(const JSValue& nativeStatementTemp384,const Args& nativeStatementTemp385,const Args& nativeStatementTemp386){
auto nativeStatementTemp387=nativeStatementTemp386[0];auto nativeStatementTemp388=nativeStatementTemp384;
JSValue m=arg(nativeStatementTemp385,0);
([&]()->JSValue{auto nativeStatementTemp393=([&]()->JSValue{const auto nativeStatementTemp395=nativeStatementTemp388;const auto nativeStatementTemp396=JSValue("quaternion");return nativeStatementTemp395.get(nativeStatementTemp396);}());const auto nativeStatementTemp394=JSValue("setFromRotationMatrix");const auto nativeStatementTemp390=nativeStatementTemp393.get(nativeStatementTemp394);const auto nativeStatementTemp391=nativeStatementTemp393;Args nativeStatementTemp392;nativeStatementTemp392.push_back(m);return nativeStatementTemp390.call(nativeStatementTemp391,nativeStatementTemp392);}());
return JSValue{};
}

JSValue method10(const JSValue& nativeStatementTemp397,const Args& nativeStatementTemp398,const Args& nativeStatementTemp399){
auto nativeStatementTemp400=nativeStatementTemp399[0];auto nativeStatementTemp401=nativeStatementTemp397;
JSValue q=arg(nativeStatementTemp398,0);
([&]()->JSValue{auto nativeStatementTemp406=([&]()->JSValue{const auto nativeStatementTemp408=nativeStatementTemp401;const auto nativeStatementTemp409=JSValue("quaternion");return nativeStatementTemp408.get(nativeStatementTemp409);}());const auto nativeStatementTemp407=JSValue("copy");const auto nativeStatementTemp403=nativeStatementTemp406.get(nativeStatementTemp407);const auto nativeStatementTemp404=nativeStatementTemp406;Args nativeStatementTemp405;nativeStatementTemp405.push_back(q);return nativeStatementTemp403.call(nativeStatementTemp404,nativeStatementTemp405);}());
return JSValue{};
}

JSValue method11(const JSValue& nativeStatementTemp410,const Args& nativeStatementTemp411,const Args& nativeStatementTemp412){
auto nativeStatementTemp413=nativeStatementTemp412[0];auto nativeStatementTemp414=nativeStatementTemp410;
JSValue axis=arg(nativeStatementTemp411,0);
JSValue angle=arg(nativeStatementTemp411,1);
([&]()->JSValue{auto nativeStatementTemp419=nativeStatementTemp413.get("_q1");const auto nativeStatementTemp420=JSValue("setFromAxisAngle");const auto nativeStatementTemp416=nativeStatementTemp419.get(nativeStatementTemp420);const auto nativeStatementTemp417=nativeStatementTemp419;Args nativeStatementTemp418;nativeStatementTemp418.push_back(axis);nativeStatementTemp418.push_back(angle);return nativeStatementTemp416.call(nativeStatementTemp417,nativeStatementTemp418);}());
([&]()->JSValue{auto nativeStatementTemp424=([&]()->JSValue{const auto nativeStatementTemp426=nativeStatementTemp414;const auto nativeStatementTemp427=JSValue("quaternion");return nativeStatementTemp426.get(nativeStatementTemp427);}());const auto nativeStatementTemp425=JSValue("multiply");const auto nativeStatementTemp421=nativeStatementTemp424.get(nativeStatementTemp425);const auto nativeStatementTemp422=nativeStatementTemp424;Args nativeStatementTemp423;nativeStatementTemp423.push_back(nativeStatementTemp413.get("_q1"));return nativeStatementTemp421.call(nativeStatementTemp422,nativeStatementTemp423);}());
return nativeStatementTemp414;
return JSValue{};
}

JSValue method12(const JSValue& nativeStatementTemp428,const Args& nativeStatementTemp429,const Args& nativeStatementTemp430){
auto nativeStatementTemp431=nativeStatementTemp430[0];auto nativeStatementTemp432=nativeStatementTemp428;
JSValue axis=arg(nativeStatementTemp429,0);
JSValue angle=arg(nativeStatementTemp429,1);
([&]()->JSValue{auto nativeStatementTemp437=nativeStatementTemp431.get("_q1");const auto nativeStatementTemp438=JSValue("setFromAxisAngle");const auto nativeStatementTemp434=nativeStatementTemp437.get(nativeStatementTemp438);const auto nativeStatementTemp435=nativeStatementTemp437;Args nativeStatementTemp436;nativeStatementTemp436.push_back(axis);nativeStatementTemp436.push_back(angle);return nativeStatementTemp434.call(nativeStatementTemp435,nativeStatementTemp436);}());
([&]()->JSValue{auto nativeStatementTemp442=([&]()->JSValue{const auto nativeStatementTemp444=nativeStatementTemp432;const auto nativeStatementTemp445=JSValue("quaternion");return nativeStatementTemp444.get(nativeStatementTemp445);}());const auto nativeStatementTemp443=JSValue("premultiply");const auto nativeStatementTemp439=nativeStatementTemp442.get(nativeStatementTemp443);const auto nativeStatementTemp440=nativeStatementTemp442;Args nativeStatementTemp441;nativeStatementTemp441.push_back(nativeStatementTemp431.get("_q1"));return nativeStatementTemp439.call(nativeStatementTemp440,nativeStatementTemp441);}());
return nativeStatementTemp432;
return JSValue{};
}

JSValue method13(const JSValue& nativeStatementTemp446,const Args& nativeStatementTemp447,const Args& nativeStatementTemp448){
auto nativeStatementTemp449=nativeStatementTemp448[0];auto nativeStatementTemp450=nativeStatementTemp446;
JSValue angle=arg(nativeStatementTemp447,0);
return ([&]()->JSValue{auto nativeStatementTemp455=nativeStatementTemp450;const auto nativeStatementTemp456=JSValue("rotateOnAxis");const auto nativeStatementTemp452=nativeStatementTemp455.get(nativeStatementTemp456);const auto nativeStatementTemp453=nativeStatementTemp455;Args nativeStatementTemp454;nativeStatementTemp454.push_back(nativeStatementTemp449.get("_xAxis"));nativeStatementTemp454.push_back(angle);return nativeStatementTemp452.call(nativeStatementTemp453,nativeStatementTemp454);}());
return JSValue{};
}

JSValue method14(const JSValue& nativeStatementTemp457,const Args& nativeStatementTemp458,const Args& nativeStatementTemp459){
auto nativeStatementTemp460=nativeStatementTemp459[0];auto nativeStatementTemp461=nativeStatementTemp457;
JSValue angle=arg(nativeStatementTemp458,0);
return ([&]()->JSValue{auto nativeStatementTemp466=nativeStatementTemp461;const auto nativeStatementTemp467=JSValue("rotateOnAxis");const auto nativeStatementTemp463=nativeStatementTemp466.get(nativeStatementTemp467);const auto nativeStatementTemp464=nativeStatementTemp466;Args nativeStatementTemp465;nativeStatementTemp465.push_back(nativeStatementTemp460.get("_yAxis"));nativeStatementTemp465.push_back(angle);return nativeStatementTemp463.call(nativeStatementTemp464,nativeStatementTemp465);}());
return JSValue{};
}

JSValue method15(const JSValue& nativeStatementTemp468,const Args& nativeStatementTemp469,const Args& nativeStatementTemp470){
auto nativeStatementTemp471=nativeStatementTemp470[0];auto nativeStatementTemp472=nativeStatementTemp468;
JSValue angle=arg(nativeStatementTemp469,0);
return ([&]()->JSValue{auto nativeStatementTemp477=nativeStatementTemp472;const auto nativeStatementTemp478=JSValue("rotateOnAxis");const auto nativeStatementTemp474=nativeStatementTemp477.get(nativeStatementTemp478);const auto nativeStatementTemp475=nativeStatementTemp477;Args nativeStatementTemp476;nativeStatementTemp476.push_back(nativeStatementTemp471.get("_zAxis"));nativeStatementTemp476.push_back(angle);return nativeStatementTemp474.call(nativeStatementTemp475,nativeStatementTemp476);}());
return JSValue{};
}

JSValue method16(const JSValue& nativeStatementTemp479,const Args& nativeStatementTemp480,const Args& nativeStatementTemp481){
auto nativeStatementTemp482=nativeStatementTemp481[0];auto nativeStatementTemp483=nativeStatementTemp479;
JSValue axis=arg(nativeStatementTemp480,0);
JSValue distance=arg(nativeStatementTemp480,1);
([&]()->JSValue{auto nativeStatementTemp488=([&]()->JSValue{auto nativeStatementTemp493=nativeStatementTemp482.get("_v1");const auto nativeStatementTemp494=JSValue("copy");const auto nativeStatementTemp490=nativeStatementTemp493.get(nativeStatementTemp494);const auto nativeStatementTemp491=nativeStatementTemp493;Args nativeStatementTemp492;nativeStatementTemp492.push_back(axis);return nativeStatementTemp490.call(nativeStatementTemp491,nativeStatementTemp492);}());const auto nativeStatementTemp489=JSValue("applyQuaternion");const auto nativeStatementTemp485=nativeStatementTemp488.get(nativeStatementTemp489);const auto nativeStatementTemp486=nativeStatementTemp488;Args nativeStatementTemp487;nativeStatementTemp487.push_back(([&]()->JSValue{const auto nativeStatementTemp495=nativeStatementTemp483;const auto nativeStatementTemp496=JSValue("quaternion");return nativeStatementTemp495.get(nativeStatementTemp496);}()));return nativeStatementTemp485.call(nativeStatementTemp486,nativeStatementTemp487);}());
([&]()->JSValue{auto nativeStatementTemp500=([&]()->JSValue{const auto nativeStatementTemp502=nativeStatementTemp483;const auto nativeStatementTemp503=JSValue("position");return nativeStatementTemp502.get(nativeStatementTemp503);}());const auto nativeStatementTemp501=JSValue("add");const auto nativeStatementTemp497=nativeStatementTemp500.get(nativeStatementTemp501);const auto nativeStatementTemp498=nativeStatementTemp500;Args nativeStatementTemp499;nativeStatementTemp499.push_back(([&]()->JSValue{auto nativeStatementTemp507=nativeStatementTemp482.get("_v1");const auto nativeStatementTemp508=JSValue("multiplyScalar");const auto nativeStatementTemp504=nativeStatementTemp507.get(nativeStatementTemp508);const auto nativeStatementTemp505=nativeStatementTemp507;Args nativeStatementTemp506;nativeStatementTemp506.push_back(distance);return nativeStatementTemp504.call(nativeStatementTemp505,nativeStatementTemp506);}()));return nativeStatementTemp497.call(nativeStatementTemp498,nativeStatementTemp499);}());
return nativeStatementTemp483;
return JSValue{};
}

JSValue method17(const JSValue& nativeStatementTemp509,const Args& nativeStatementTemp510,const Args& nativeStatementTemp511){
auto nativeStatementTemp512=nativeStatementTemp511[0];auto nativeStatementTemp513=nativeStatementTemp509;
JSValue distance=arg(nativeStatementTemp510,0);
return ([&]()->JSValue{auto nativeStatementTemp518=nativeStatementTemp513;const auto nativeStatementTemp519=JSValue("translateOnAxis");const auto nativeStatementTemp515=nativeStatementTemp518.get(nativeStatementTemp519);const auto nativeStatementTemp516=nativeStatementTemp518;Args nativeStatementTemp517;nativeStatementTemp517.push_back(nativeStatementTemp512.get("_xAxis"));nativeStatementTemp517.push_back(distance);return nativeStatementTemp515.call(nativeStatementTemp516,nativeStatementTemp517);}());
return JSValue{};
}

JSValue method18(const JSValue& nativeStatementTemp520,const Args& nativeStatementTemp521,const Args& nativeStatementTemp522){
auto nativeStatementTemp523=nativeStatementTemp522[0];auto nativeStatementTemp524=nativeStatementTemp520;
JSValue distance=arg(nativeStatementTemp521,0);
return ([&]()->JSValue{auto nativeStatementTemp529=nativeStatementTemp524;const auto nativeStatementTemp530=JSValue("translateOnAxis");const auto nativeStatementTemp526=nativeStatementTemp529.get(nativeStatementTemp530);const auto nativeStatementTemp527=nativeStatementTemp529;Args nativeStatementTemp528;nativeStatementTemp528.push_back(nativeStatementTemp523.get("_yAxis"));nativeStatementTemp528.push_back(distance);return nativeStatementTemp526.call(nativeStatementTemp527,nativeStatementTemp528);}());
return JSValue{};
}

JSValue method19(const JSValue& nativeStatementTemp531,const Args& nativeStatementTemp532,const Args& nativeStatementTemp533){
auto nativeStatementTemp534=nativeStatementTemp533[0];auto nativeStatementTemp535=nativeStatementTemp531;
JSValue distance=arg(nativeStatementTemp532,0);
return ([&]()->JSValue{auto nativeStatementTemp540=nativeStatementTemp535;const auto nativeStatementTemp541=JSValue("translateOnAxis");const auto nativeStatementTemp537=nativeStatementTemp540.get(nativeStatementTemp541);const auto nativeStatementTemp538=nativeStatementTemp540;Args nativeStatementTemp539;nativeStatementTemp539.push_back(nativeStatementTemp534.get("_zAxis"));nativeStatementTemp539.push_back(distance);return nativeStatementTemp537.call(nativeStatementTemp538,nativeStatementTemp539);}());
return JSValue{};
}

JSValue method20(const JSValue& nativeStatementTemp542,const Args& nativeStatementTemp543,const Args& nativeStatementTemp544){
auto nativeStatementTemp545=nativeStatementTemp544[0];auto nativeStatementTemp546=nativeStatementTemp542;
JSValue vector=arg(nativeStatementTemp543,0);
([&]()->JSValue{auto nativeStatementTemp551=nativeStatementTemp546;const auto nativeStatementTemp552=JSValue("updateWorldMatrix");const auto nativeStatementTemp548=nativeStatementTemp551.get(nativeStatementTemp552);const auto nativeStatementTemp549=nativeStatementTemp551;Args nativeStatementTemp550;nativeStatementTemp550.push_back(JSValue(true));nativeStatementTemp550.push_back(JSValue(false));return nativeStatementTemp548.call(nativeStatementTemp549,nativeStatementTemp550);}());
return ([&]()->JSValue{auto nativeStatementTemp556=vector;const auto nativeStatementTemp557=JSValue("applyMatrix4");const auto nativeStatementTemp553=nativeStatementTemp556.get(nativeStatementTemp557);const auto nativeStatementTemp554=nativeStatementTemp556;Args nativeStatementTemp555;nativeStatementTemp555.push_back(([&]()->JSValue{const auto nativeStatementTemp558=nativeStatementTemp546;const auto nativeStatementTemp559=JSValue("matrixWorld");return nativeStatementTemp558.get(nativeStatementTemp559);}()));return nativeStatementTemp553.call(nativeStatementTemp554,nativeStatementTemp555);}());
return JSValue{};
}

JSValue method21(const JSValue& nativeStatementTemp560,const Args& nativeStatementTemp561,const Args& nativeStatementTemp562){
auto nativeStatementTemp563=nativeStatementTemp562[0];auto nativeStatementTemp564=nativeStatementTemp560;
JSValue vector=arg(nativeStatementTemp561,0);
([&]()->JSValue{auto nativeStatementTemp569=nativeStatementTemp564;const auto nativeStatementTemp570=JSValue("updateWorldMatrix");const auto nativeStatementTemp566=nativeStatementTemp569.get(nativeStatementTemp570);const auto nativeStatementTemp567=nativeStatementTemp569;Args nativeStatementTemp568;nativeStatementTemp568.push_back(JSValue(true));nativeStatementTemp568.push_back(JSValue(false));return nativeStatementTemp566.call(nativeStatementTemp567,nativeStatementTemp568);}());
return ([&]()->JSValue{auto nativeStatementTemp574=vector;const auto nativeStatementTemp575=JSValue("applyMatrix4");const auto nativeStatementTemp571=nativeStatementTemp574.get(nativeStatementTemp575);const auto nativeStatementTemp572=nativeStatementTemp574;Args nativeStatementTemp573;nativeStatementTemp573.push_back(([&]()->JSValue{auto nativeStatementTemp579=([&]()->JSValue{auto nativeStatementTemp584=nativeStatementTemp563.get("_m1");const auto nativeStatementTemp585=JSValue("copy");const auto nativeStatementTemp581=nativeStatementTemp584.get(nativeStatementTemp585);const auto nativeStatementTemp582=nativeStatementTemp584;Args nativeStatementTemp583;nativeStatementTemp583.push_back(([&]()->JSValue{const auto nativeStatementTemp586=nativeStatementTemp564;const auto nativeStatementTemp587=JSValue("matrixWorld");return nativeStatementTemp586.get(nativeStatementTemp587);}()));return nativeStatementTemp581.call(nativeStatementTemp582,nativeStatementTemp583);}());const auto nativeStatementTemp580=JSValue("invert");const auto nativeStatementTemp576=nativeStatementTemp579.get(nativeStatementTemp580);const auto nativeStatementTemp577=nativeStatementTemp579;Args nativeStatementTemp578;return nativeStatementTemp576.call(nativeStatementTemp577,nativeStatementTemp578);}()));return nativeStatementTemp571.call(nativeStatementTemp572,nativeStatementTemp573);}());
return JSValue{};
}

JSValue method22(const JSValue& nativeStatementTemp588,const Args& nativeStatementTemp589,const Args& nativeStatementTemp590){
auto nativeStatementTemp591=nativeStatementTemp590[0];auto nativeStatementTemp592=nativeStatementTemp588;
JSValue x=arg(nativeStatementTemp589,0);
JSValue y=arg(nativeStatementTemp589,1);
JSValue z=arg(nativeStatementTemp589,2);
if(([&]()->JSValue{const auto nativeStatementTemp594=x;const auto nativeStatementTemp595=JSValue("isVector3");return nativeStatementTemp594.get(nativeStatementTemp595);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp599=nativeStatementTemp591.get("_target");const auto nativeStatementTemp600=JSValue("copy");const auto nativeStatementTemp596=nativeStatementTemp599.get(nativeStatementTemp600);const auto nativeStatementTemp597=nativeStatementTemp599;Args nativeStatementTemp598;nativeStatementTemp598.push_back(x);return nativeStatementTemp596.call(nativeStatementTemp597,nativeStatementTemp598);}());

}
else {
([&]()->JSValue{auto nativeStatementTemp604=nativeStatementTemp591.get("_target");const auto nativeStatementTemp605=JSValue("set");const auto nativeStatementTemp601=nativeStatementTemp604.get(nativeStatementTemp605);const auto nativeStatementTemp602=nativeStatementTemp604;Args nativeStatementTemp603;nativeStatementTemp603.push_back(x);nativeStatementTemp603.push_back(y);nativeStatementTemp603.push_back(z);return nativeStatementTemp601.call(nativeStatementTemp602,nativeStatementTemp603);}());

}
JSValue parent=([&]()->JSValue{const auto nativeStatementTemp606=nativeStatementTemp592;const auto nativeStatementTemp607=JSValue("parent");return nativeStatementTemp606.get(nativeStatementTemp607);}());
([&]()->JSValue{auto nativeStatementTemp611=nativeStatementTemp592;const auto nativeStatementTemp612=JSValue("updateWorldMatrix");const auto nativeStatementTemp608=nativeStatementTemp611.get(nativeStatementTemp612);const auto nativeStatementTemp609=nativeStatementTemp611;Args nativeStatementTemp610;nativeStatementTemp610.push_back(JSValue(true));nativeStatementTemp610.push_back(JSValue(false));return nativeStatementTemp608.call(nativeStatementTemp609,nativeStatementTemp610);}());
([&]()->JSValue{auto nativeStatementTemp616=nativeStatementTemp591.get("_position");const auto nativeStatementTemp617=JSValue("setFromMatrixPosition");const auto nativeStatementTemp613=nativeStatementTemp616.get(nativeStatementTemp617);const auto nativeStatementTemp614=nativeStatementTemp616;Args nativeStatementTemp615;nativeStatementTemp615.push_back(([&]()->JSValue{const auto nativeStatementTemp618=nativeStatementTemp592;const auto nativeStatementTemp619=JSValue("matrixWorld");return nativeStatementTemp618.get(nativeStatementTemp619);}()));return nativeStatementTemp613.call(nativeStatementTemp614,nativeStatementTemp615);}());
if(([&]()->JSValue{const auto nativeStatementTemp620=([&]()->JSValue{const auto nativeStatementTemp621=nativeStatementTemp592;const auto nativeStatementTemp622=JSValue("isCamera");return nativeStatementTemp621.get(nativeStatementTemp622);}());if(nativeStatementTemp620.toBoolean())return nativeStatementTemp620;return ([&]()->JSValue{const auto nativeStatementTemp623=nativeStatementTemp592;const auto nativeStatementTemp624=JSValue("isLight");return nativeStatementTemp623.get(nativeStatementTemp624);}());}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp628=nativeStatementTemp591.get("_m1");const auto nativeStatementTemp629=JSValue("lookAt");const auto nativeStatementTemp625=nativeStatementTemp628.get(nativeStatementTemp629);const auto nativeStatementTemp626=nativeStatementTemp628;Args nativeStatementTemp627;nativeStatementTemp627.push_back(nativeStatementTemp591.get("_position"));nativeStatementTemp627.push_back(nativeStatementTemp591.get("_target"));nativeStatementTemp627.push_back(([&]()->JSValue{const auto nativeStatementTemp630=nativeStatementTemp592;const auto nativeStatementTemp631=JSValue("up");return nativeStatementTemp630.get(nativeStatementTemp631);}()));return nativeStatementTemp625.call(nativeStatementTemp626,nativeStatementTemp627);}());

}
else {
([&]()->JSValue{auto nativeStatementTemp635=nativeStatementTemp591.get("_m1");const auto nativeStatementTemp636=JSValue("lookAt");const auto nativeStatementTemp632=nativeStatementTemp635.get(nativeStatementTemp636);const auto nativeStatementTemp633=nativeStatementTemp635;Args nativeStatementTemp634;nativeStatementTemp634.push_back(nativeStatementTemp591.get("_target"));nativeStatementTemp634.push_back(nativeStatementTemp591.get("_position"));nativeStatementTemp634.push_back(([&]()->JSValue{const auto nativeStatementTemp637=nativeStatementTemp592;const auto nativeStatementTemp638=JSValue("up");return nativeStatementTemp637.get(nativeStatementTemp638);}()));return nativeStatementTemp632.call(nativeStatementTemp633,nativeStatementTemp634);}());

}
([&]()->JSValue{auto nativeStatementTemp642=([&]()->JSValue{const auto nativeStatementTemp644=nativeStatementTemp592;const auto nativeStatementTemp645=JSValue("quaternion");return nativeStatementTemp644.get(nativeStatementTemp645);}());const auto nativeStatementTemp643=JSValue("setFromRotationMatrix");const auto nativeStatementTemp639=nativeStatementTemp642.get(nativeStatementTemp643);const auto nativeStatementTemp640=nativeStatementTemp642;Args nativeStatementTemp641;nativeStatementTemp641.push_back(nativeStatementTemp591.get("_m1"));return nativeStatementTemp639.call(nativeStatementTemp640,nativeStatementTemp641);}());
if(parent.toBoolean()){
([&]()->JSValue{auto nativeStatementTemp649=nativeStatementTemp591.get("_m1");const auto nativeStatementTemp650=JSValue("extractRotation");const auto nativeStatementTemp646=nativeStatementTemp649.get(nativeStatementTemp650);const auto nativeStatementTemp647=nativeStatementTemp649;Args nativeStatementTemp648;nativeStatementTemp648.push_back(([&]()->JSValue{const auto nativeStatementTemp651=parent;const auto nativeStatementTemp652=JSValue("matrixWorld");return nativeStatementTemp651.get(nativeStatementTemp652);}()));return nativeStatementTemp646.call(nativeStatementTemp647,nativeStatementTemp648);}());
([&]()->JSValue{auto nativeStatementTemp656=nativeStatementTemp591.get("_q1");const auto nativeStatementTemp657=JSValue("setFromRotationMatrix");const auto nativeStatementTemp653=nativeStatementTemp656.get(nativeStatementTemp657);const auto nativeStatementTemp654=nativeStatementTemp656;Args nativeStatementTemp655;nativeStatementTemp655.push_back(nativeStatementTemp591.get("_m1"));return nativeStatementTemp653.call(nativeStatementTemp654,nativeStatementTemp655);}());
([&]()->JSValue{auto nativeStatementTemp661=([&]()->JSValue{const auto nativeStatementTemp663=nativeStatementTemp592;const auto nativeStatementTemp664=JSValue("quaternion");return nativeStatementTemp663.get(nativeStatementTemp664);}());const auto nativeStatementTemp662=JSValue("premultiply");const auto nativeStatementTemp658=nativeStatementTemp661.get(nativeStatementTemp662);const auto nativeStatementTemp659=nativeStatementTemp661;Args nativeStatementTemp660;nativeStatementTemp660.push_back(([&]()->JSValue{auto nativeStatementTemp668=nativeStatementTemp591.get("_q1");const auto nativeStatementTemp669=JSValue("invert");const auto nativeStatementTemp665=nativeStatementTemp668.get(nativeStatementTemp669);const auto nativeStatementTemp666=nativeStatementTemp668;Args nativeStatementTemp667;return nativeStatementTemp665.call(nativeStatementTemp666,nativeStatementTemp667);}()));return nativeStatementTemp658.call(nativeStatementTemp659,nativeStatementTemp660);}());

}
return JSValue{};
}

JSValue method23(const JSValue& nativeStatementTemp670,const Args& nativeStatementTemp671,const Args& nativeStatementTemp672){
auto nativeStatementTemp673=nativeStatementTemp672[0];auto nativeStatementTemp674=nativeStatementTemp670;
auto nativeStatementTemp676=argumentObject(nativeStatementTemp671);
JSValue object=arg(nativeStatementTemp671,0);
if(([&]()->JSValue{const auto nativeStatementTemp677=([&]()->JSValue{const auto nativeStatementTemp679=nativeStatementTemp676;const auto nativeStatementTemp680=JSValue("length");return nativeStatementTemp679.get(nativeStatementTemp680);}());const auto nativeStatementTemp678=JSValue(1);return binary(">",nativeStatementTemp677,nativeStatementTemp678);}()).toBoolean()){
{JSValue i=JSValue(0);
for(;([&]()->JSValue{const auto nativeStatementTemp681=i;const auto nativeStatementTemp682=([&]()->JSValue{const auto nativeStatementTemp683=nativeStatementTemp676;const auto nativeStatementTemp684=JSValue("length");return nativeStatementTemp683.get(nativeStatementTemp684);}());return binary("<",nativeStatementTemp681,nativeStatementTemp682);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp685=i.toNumber();const auto nativeStatementTemp686=nativeStatementTemp685+1;i=nativeStatementTemp686;return nativeStatementTemp685;}())){
([&]()->JSValue{auto nativeStatementTemp690=nativeStatementTemp674;const auto nativeStatementTemp691=JSValue("add");const auto nativeStatementTemp687=nativeStatementTemp690.get(nativeStatementTemp691);const auto nativeStatementTemp688=nativeStatementTemp690;Args nativeStatementTemp689;nativeStatementTemp689.push_back(([&]()->JSValue{const auto nativeStatementTemp692=nativeStatementTemp676;const auto nativeStatementTemp693=i;return nativeStatementTemp692.get(nativeStatementTemp693);}()));return nativeStatementTemp687.call(nativeStatementTemp688,nativeStatementTemp689);}());

}
}
return nativeStatementTemp674;

}
if(([&]()->JSValue{const auto nativeStatementTemp694=object;const auto nativeStatementTemp695=nativeStatementTemp674;return binary("===",nativeStatementTemp694,nativeStatementTemp695);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp696=nativeStatementTemp673.get("error");const JSValue nativeStatementTemp697;Args nativeStatementTemp698;nativeStatementTemp698.push_back(JSValue("Object3D.add: object can't be added as a child of itself."));nativeStatementTemp698.push_back(object);return nativeStatementTemp696.call(nativeStatementTemp697,nativeStatementTemp698);}());
return nativeStatementTemp674;

}
if(([&]()->JSValue{const auto nativeStatementTemp699=object;if(!nativeStatementTemp699.toBoolean())return nativeStatementTemp699;return ([&]()->JSValue{const auto nativeStatementTemp700=object;const auto nativeStatementTemp701=JSValue("isObject3D");return nativeStatementTemp700.get(nativeStatementTemp701);}());}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp705=object;const auto nativeStatementTemp706=JSValue("removeFromParent");const auto nativeStatementTemp702=nativeStatementTemp705.get(nativeStatementTemp706);const auto nativeStatementTemp703=nativeStatementTemp705;Args nativeStatementTemp704;return nativeStatementTemp702.call(nativeStatementTemp703,nativeStatementTemp704);}());
([&]()->JSValue{auto nativeStatementTemp707=object;const auto nativeStatementTemp708=JSValue("parent");const auto nativeStatementTemp709=nativeStatementTemp674;setReference(nativeStatementTemp707,nativeStatementTemp708,nativeStatementTemp709);return nativeStatementTemp709;}());
([&]()->JSValue{auto nativeStatementTemp715=([&]()->JSValue{const auto nativeStatementTemp717=nativeStatementTemp674;const auto nativeStatementTemp718=JSValue("children");return nativeStatementTemp717.get(nativeStatementTemp718);}());const auto nativeStatementTemp716=JSValue("push");const auto nativeStatementTemp712=nativeStatementTemp715.get(nativeStatementTemp716);const auto nativeStatementTemp713=nativeStatementTemp715;Args nativeStatementTemp714;nativeStatementTemp714.push_back(object);return nativeStatementTemp712.call(nativeStatementTemp713,nativeStatementTemp714);}());
([&]()->JSValue{auto nativeStatementTemp722=object;const auto nativeStatementTemp723=JSValue("dispatchEvent");const auto nativeStatementTemp719=nativeStatementTemp722.get(nativeStatementTemp723);const auto nativeStatementTemp720=nativeStatementTemp722;Args nativeStatementTemp721;nativeStatementTemp721.push_back(nativeStatementTemp673.get("_addedEvent"));return nativeStatementTemp719.call(nativeStatementTemp720,nativeStatementTemp721);}());
([&]()->JSValue{auto nativeStatementTemp724=nativeStatementTemp673.get("_childaddedEvent");const auto nativeStatementTemp725=JSValue("child");const auto nativeStatementTemp726=object;setReference(nativeStatementTemp724,nativeStatementTemp725,nativeStatementTemp726);return nativeStatementTemp726;}());
([&]()->JSValue{auto nativeStatementTemp732=nativeStatementTemp674;const auto nativeStatementTemp733=JSValue("dispatchEvent");const auto nativeStatementTemp729=nativeStatementTemp732.get(nativeStatementTemp733);const auto nativeStatementTemp730=nativeStatementTemp732;Args nativeStatementTemp731;nativeStatementTemp731.push_back(nativeStatementTemp673.get("_childaddedEvent"));return nativeStatementTemp729.call(nativeStatementTemp730,nativeStatementTemp731);}());
([&]()->JSValue{auto nativeStatementTemp734=nativeStatementTemp673.get("_childaddedEvent");const auto nativeStatementTemp735=JSValue("child");const auto nativeStatementTemp736=JSValue(nullptr);setReference(nativeStatementTemp734,nativeStatementTemp735,nativeStatementTemp736);return nativeStatementTemp736;}());

}
else {
([&]()->JSValue{const auto nativeStatementTemp739=nativeStatementTemp673.get("error");const JSValue nativeStatementTemp740;Args nativeStatementTemp741;nativeStatementTemp741.push_back(JSValue("Object3D.add: object not an instance of THREE.Object3D."));nativeStatementTemp741.push_back(object);return nativeStatementTemp739.call(nativeStatementTemp740,nativeStatementTemp741);}());

}
return nativeStatementTemp674;
return JSValue{};
}

JSValue method24(const JSValue& nativeStatementTemp742,const Args& nativeStatementTemp743,const Args& nativeStatementTemp744){
auto nativeStatementTemp745=nativeStatementTemp744[0];auto nativeStatementTemp746=nativeStatementTemp742;
auto nativeStatementTemp748=argumentObject(nativeStatementTemp743);
JSValue object=arg(nativeStatementTemp743,0);
if(([&]()->JSValue{const auto nativeStatementTemp749=([&]()->JSValue{const auto nativeStatementTemp751=nativeStatementTemp748;const auto nativeStatementTemp752=JSValue("length");return nativeStatementTemp751.get(nativeStatementTemp752);}());const auto nativeStatementTemp750=JSValue(1);return binary(">",nativeStatementTemp749,nativeStatementTemp750);}()).toBoolean()){
{JSValue i=JSValue(0);
for(;([&]()->JSValue{const auto nativeStatementTemp753=i;const auto nativeStatementTemp754=([&]()->JSValue{const auto nativeStatementTemp755=nativeStatementTemp748;const auto nativeStatementTemp756=JSValue("length");return nativeStatementTemp755.get(nativeStatementTemp756);}());return binary("<",nativeStatementTemp753,nativeStatementTemp754);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp757=i.toNumber();const auto nativeStatementTemp758=nativeStatementTemp757+1;i=nativeStatementTemp758;return nativeStatementTemp757;}())){
([&]()->JSValue{auto nativeStatementTemp762=nativeStatementTemp746;const auto nativeStatementTemp763=JSValue("remove");const auto nativeStatementTemp759=nativeStatementTemp762.get(nativeStatementTemp763);const auto nativeStatementTemp760=nativeStatementTemp762;Args nativeStatementTemp761;nativeStatementTemp761.push_back(([&]()->JSValue{const auto nativeStatementTemp764=nativeStatementTemp748;const auto nativeStatementTemp765=i;return nativeStatementTemp764.get(nativeStatementTemp765);}()));return nativeStatementTemp759.call(nativeStatementTemp760,nativeStatementTemp761);}());

}
}
return nativeStatementTemp746;

}
JSValue index=([&]()->JSValue{auto nativeStatementTemp769=([&]()->JSValue{const auto nativeStatementTemp771=nativeStatementTemp746;const auto nativeStatementTemp772=JSValue("children");return nativeStatementTemp771.get(nativeStatementTemp772);}());const auto nativeStatementTemp770=JSValue("indexOf");const auto nativeStatementTemp766=nativeStatementTemp769.get(nativeStatementTemp770);const auto nativeStatementTemp767=nativeStatementTemp769;Args nativeStatementTemp768;nativeStatementTemp768.push_back(object);return nativeStatementTemp766.call(nativeStatementTemp767,nativeStatementTemp768);}());
if(([&]()->JSValue{const auto nativeStatementTemp773=index;const auto nativeStatementTemp774=([&]()->JSValue{const auto nativeStatementTemp775=JSValue(1);return -nativeStatementTemp775.toNumber();}());return binary("!==",nativeStatementTemp773,nativeStatementTemp774);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp776=object;const auto nativeStatementTemp777=JSValue("parent");const auto nativeStatementTemp778=JSValue(nullptr);setReference(nativeStatementTemp776,nativeStatementTemp777,nativeStatementTemp778);return nativeStatementTemp778;}());
([&]()->JSValue{auto nativeStatementTemp784=([&]()->JSValue{const auto nativeStatementTemp786=nativeStatementTemp746;const auto nativeStatementTemp787=JSValue("children");return nativeStatementTemp786.get(nativeStatementTemp787);}());const auto nativeStatementTemp785=JSValue("splice");const auto nativeStatementTemp781=nativeStatementTemp784.get(nativeStatementTemp785);const auto nativeStatementTemp782=nativeStatementTemp784;Args nativeStatementTemp783;nativeStatementTemp783.push_back(index);nativeStatementTemp783.push_back(JSValue(1));return nativeStatementTemp781.call(nativeStatementTemp782,nativeStatementTemp783);}());
([&]()->JSValue{auto nativeStatementTemp791=object;const auto nativeStatementTemp792=JSValue("dispatchEvent");const auto nativeStatementTemp788=nativeStatementTemp791.get(nativeStatementTemp792);const auto nativeStatementTemp789=nativeStatementTemp791;Args nativeStatementTemp790;nativeStatementTemp790.push_back(nativeStatementTemp745.get("_removedEvent"));return nativeStatementTemp788.call(nativeStatementTemp789,nativeStatementTemp790);}());
([&]()->JSValue{auto nativeStatementTemp793=nativeStatementTemp745.get("_childremovedEvent");const auto nativeStatementTemp794=JSValue("child");const auto nativeStatementTemp795=object;setReference(nativeStatementTemp793,nativeStatementTemp794,nativeStatementTemp795);return nativeStatementTemp795;}());
([&]()->JSValue{auto nativeStatementTemp801=nativeStatementTemp746;const auto nativeStatementTemp802=JSValue("dispatchEvent");const auto nativeStatementTemp798=nativeStatementTemp801.get(nativeStatementTemp802);const auto nativeStatementTemp799=nativeStatementTemp801;Args nativeStatementTemp800;nativeStatementTemp800.push_back(nativeStatementTemp745.get("_childremovedEvent"));return nativeStatementTemp798.call(nativeStatementTemp799,nativeStatementTemp800);}());
([&]()->JSValue{auto nativeStatementTemp803=nativeStatementTemp745.get("_childremovedEvent");const auto nativeStatementTemp804=JSValue("child");const auto nativeStatementTemp805=JSValue(nullptr);setReference(nativeStatementTemp803,nativeStatementTemp804,nativeStatementTemp805);return nativeStatementTemp805;}());

}
return nativeStatementTemp746;
return JSValue{};
}

JSValue method25(const JSValue& nativeStatementTemp808,const Args& nativeStatementTemp809,const Args& nativeStatementTemp810){
auto nativeStatementTemp811=nativeStatementTemp810[0];auto nativeStatementTemp812=nativeStatementTemp808;
JSValue parent=([&]()->JSValue{const auto nativeStatementTemp814=nativeStatementTemp812;const auto nativeStatementTemp815=JSValue("parent");return nativeStatementTemp814.get(nativeStatementTemp815);}());
if(([&]()->JSValue{const auto nativeStatementTemp816=parent;const auto nativeStatementTemp817=JSValue(nullptr);return binary("!==",nativeStatementTemp816,nativeStatementTemp817);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp821=parent;const auto nativeStatementTemp822=JSValue("remove");const auto nativeStatementTemp818=nativeStatementTemp821.get(nativeStatementTemp822);const auto nativeStatementTemp819=nativeStatementTemp821;Args nativeStatementTemp820;nativeStatementTemp820.push_back(nativeStatementTemp812);return nativeStatementTemp818.call(nativeStatementTemp819,nativeStatementTemp820);}());

}
return nativeStatementTemp812;
return JSValue{};
}

JSValue method26(const JSValue& nativeStatementTemp823,const Args& nativeStatementTemp824,const Args& nativeStatementTemp825){
auto nativeStatementTemp826=nativeStatementTemp825[0];auto nativeStatementTemp827=nativeStatementTemp823;
return ([&]()->JSValue{auto nativeStatementTemp832=nativeStatementTemp827;const auto nativeStatementTemp833=JSValue("remove");const auto nativeStatementTemp829=nativeStatementTemp832.get(nativeStatementTemp833);const auto nativeStatementTemp830=nativeStatementTemp832;Args nativeStatementTemp831;appendSpread(nativeStatementTemp831,([&]()->JSValue{const auto nativeStatementTemp834=nativeStatementTemp827;const auto nativeStatementTemp835=JSValue("children");return nativeStatementTemp834.get(nativeStatementTemp835);}()));return nativeStatementTemp829.call(nativeStatementTemp830,nativeStatementTemp831);}());
return JSValue{};
}

JSValue method27(const JSValue& nativeStatementTemp836,const Args& nativeStatementTemp837,const Args& nativeStatementTemp838){
auto nativeStatementTemp839=nativeStatementTemp838[0];auto nativeStatementTemp840=nativeStatementTemp836;
JSValue object=arg(nativeStatementTemp837,0);
([&]()->JSValue{auto nativeStatementTemp845=nativeStatementTemp840;const auto nativeStatementTemp846=JSValue("updateWorldMatrix");const auto nativeStatementTemp842=nativeStatementTemp845.get(nativeStatementTemp846);const auto nativeStatementTemp843=nativeStatementTemp845;Args nativeStatementTemp844;nativeStatementTemp844.push_back(JSValue(true));nativeStatementTemp844.push_back(JSValue(false));return nativeStatementTemp842.call(nativeStatementTemp843,nativeStatementTemp844);}());
([&]()->JSValue{auto nativeStatementTemp850=([&]()->JSValue{auto nativeStatementTemp855=nativeStatementTemp839.get("_m1");const auto nativeStatementTemp856=JSValue("copy");const auto nativeStatementTemp852=nativeStatementTemp855.get(nativeStatementTemp856);const auto nativeStatementTemp853=nativeStatementTemp855;Args nativeStatementTemp854;nativeStatementTemp854.push_back(([&]()->JSValue{const auto nativeStatementTemp857=nativeStatementTemp840;const auto nativeStatementTemp858=JSValue("matrixWorld");return nativeStatementTemp857.get(nativeStatementTemp858);}()));return nativeStatementTemp852.call(nativeStatementTemp853,nativeStatementTemp854);}());const auto nativeStatementTemp851=JSValue("invert");const auto nativeStatementTemp847=nativeStatementTemp850.get(nativeStatementTemp851);const auto nativeStatementTemp848=nativeStatementTemp850;Args nativeStatementTemp849;return nativeStatementTemp847.call(nativeStatementTemp848,nativeStatementTemp849);}());
if(([&]()->JSValue{const auto nativeStatementTemp859=([&]()->JSValue{const auto nativeStatementTemp861=object;const auto nativeStatementTemp862=JSValue("parent");return nativeStatementTemp861.get(nativeStatementTemp862);}());const auto nativeStatementTemp860=JSValue(nullptr);return binary("!==",nativeStatementTemp859,nativeStatementTemp860);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp866=([&]()->JSValue{const auto nativeStatementTemp868=object;const auto nativeStatementTemp869=JSValue("parent");return nativeStatementTemp868.get(nativeStatementTemp869);}());const auto nativeStatementTemp867=JSValue("updateWorldMatrix");const auto nativeStatementTemp863=nativeStatementTemp866.get(nativeStatementTemp867);const auto nativeStatementTemp864=nativeStatementTemp866;Args nativeStatementTemp865;nativeStatementTemp865.push_back(JSValue(true));nativeStatementTemp865.push_back(JSValue(false));return nativeStatementTemp863.call(nativeStatementTemp864,nativeStatementTemp865);}());
([&]()->JSValue{auto nativeStatementTemp873=nativeStatementTemp839.get("_m1");const auto nativeStatementTemp874=JSValue("multiply");const auto nativeStatementTemp870=nativeStatementTemp873.get(nativeStatementTemp874);const auto nativeStatementTemp871=nativeStatementTemp873;Args nativeStatementTemp872;nativeStatementTemp872.push_back(([&]()->JSValue{const auto nativeStatementTemp875=([&]()->JSValue{const auto nativeStatementTemp877=object;const auto nativeStatementTemp878=JSValue("parent");return nativeStatementTemp877.get(nativeStatementTemp878);}());const auto nativeStatementTemp876=JSValue("matrixWorld");return nativeStatementTemp875.get(nativeStatementTemp876);}()));return nativeStatementTemp870.call(nativeStatementTemp871,nativeStatementTemp872);}());

}
([&]()->JSValue{auto nativeStatementTemp882=object;const auto nativeStatementTemp883=JSValue("applyMatrix4");const auto nativeStatementTemp879=nativeStatementTemp882.get(nativeStatementTemp883);const auto nativeStatementTemp880=nativeStatementTemp882;Args nativeStatementTemp881;nativeStatementTemp881.push_back(nativeStatementTemp839.get("_m1"));return nativeStatementTemp879.call(nativeStatementTemp880,nativeStatementTemp881);}());
([&]()->JSValue{auto nativeStatementTemp887=object;const auto nativeStatementTemp888=JSValue("removeFromParent");const auto nativeStatementTemp884=nativeStatementTemp887.get(nativeStatementTemp888);const auto nativeStatementTemp885=nativeStatementTemp887;Args nativeStatementTemp886;return nativeStatementTemp884.call(nativeStatementTemp885,nativeStatementTemp886);}());
([&]()->JSValue{auto nativeStatementTemp889=object;const auto nativeStatementTemp890=JSValue("parent");const auto nativeStatementTemp891=nativeStatementTemp840;setReference(nativeStatementTemp889,nativeStatementTemp890,nativeStatementTemp891);return nativeStatementTemp891;}());
([&]()->JSValue{auto nativeStatementTemp897=([&]()->JSValue{const auto nativeStatementTemp899=nativeStatementTemp840;const auto nativeStatementTemp900=JSValue("children");return nativeStatementTemp899.get(nativeStatementTemp900);}());const auto nativeStatementTemp898=JSValue("push");const auto nativeStatementTemp894=nativeStatementTemp897.get(nativeStatementTemp898);const auto nativeStatementTemp895=nativeStatementTemp897;Args nativeStatementTemp896;nativeStatementTemp896.push_back(object);return nativeStatementTemp894.call(nativeStatementTemp895,nativeStatementTemp896);}());
([&]()->JSValue{auto nativeStatementTemp904=object;const auto nativeStatementTemp905=JSValue("updateWorldMatrix");const auto nativeStatementTemp901=nativeStatementTemp904.get(nativeStatementTemp905);const auto nativeStatementTemp902=nativeStatementTemp904;Args nativeStatementTemp903;nativeStatementTemp903.push_back(JSValue(false));nativeStatementTemp903.push_back(JSValue(true));return nativeStatementTemp901.call(nativeStatementTemp902,nativeStatementTemp903);}());
([&]()->JSValue{auto nativeStatementTemp909=object;const auto nativeStatementTemp910=JSValue("dispatchEvent");const auto nativeStatementTemp906=nativeStatementTemp909.get(nativeStatementTemp910);const auto nativeStatementTemp907=nativeStatementTemp909;Args nativeStatementTemp908;nativeStatementTemp908.push_back(nativeStatementTemp839.get("_addedEvent"));return nativeStatementTemp906.call(nativeStatementTemp907,nativeStatementTemp908);}());
([&]()->JSValue{auto nativeStatementTemp911=nativeStatementTemp839.get("_childaddedEvent");const auto nativeStatementTemp912=JSValue("child");const auto nativeStatementTemp913=object;setReference(nativeStatementTemp911,nativeStatementTemp912,nativeStatementTemp913);return nativeStatementTemp913;}());
([&]()->JSValue{auto nativeStatementTemp919=nativeStatementTemp840;const auto nativeStatementTemp920=JSValue("dispatchEvent");const auto nativeStatementTemp916=nativeStatementTemp919.get(nativeStatementTemp920);const auto nativeStatementTemp917=nativeStatementTemp919;Args nativeStatementTemp918;nativeStatementTemp918.push_back(nativeStatementTemp839.get("_childaddedEvent"));return nativeStatementTemp916.call(nativeStatementTemp917,nativeStatementTemp918);}());
([&]()->JSValue{auto nativeStatementTemp921=nativeStatementTemp839.get("_childaddedEvent");const auto nativeStatementTemp922=JSValue("child");const auto nativeStatementTemp923=JSValue(nullptr);setReference(nativeStatementTemp921,nativeStatementTemp922,nativeStatementTemp923);return nativeStatementTemp923;}());
return nativeStatementTemp840;
return JSValue{};
}

JSValue method28(const JSValue& nativeStatementTemp926,const Args& nativeStatementTemp927,const Args& nativeStatementTemp928){
auto nativeStatementTemp929=nativeStatementTemp928[0];auto nativeStatementTemp930=nativeStatementTemp926;
JSValue id=arg(nativeStatementTemp927,0);
return ([&]()->JSValue{auto nativeStatementTemp935=nativeStatementTemp930;const auto nativeStatementTemp936=JSValue("getObjectByProperty");const auto nativeStatementTemp932=nativeStatementTemp935.get(nativeStatementTemp936);const auto nativeStatementTemp933=nativeStatementTemp935;Args nativeStatementTemp934;nativeStatementTemp934.push_back(JSValue("id"));nativeStatementTemp934.push_back(id);return nativeStatementTemp932.call(nativeStatementTemp933,nativeStatementTemp934);}());
return JSValue{};
}

JSValue method29(const JSValue& nativeStatementTemp937,const Args& nativeStatementTemp938,const Args& nativeStatementTemp939){
auto nativeStatementTemp940=nativeStatementTemp939[0];auto nativeStatementTemp941=nativeStatementTemp937;
JSValue name=arg(nativeStatementTemp938,0);
return ([&]()->JSValue{auto nativeStatementTemp946=nativeStatementTemp941;const auto nativeStatementTemp947=JSValue("getObjectByProperty");const auto nativeStatementTemp943=nativeStatementTemp946.get(nativeStatementTemp947);const auto nativeStatementTemp944=nativeStatementTemp946;Args nativeStatementTemp945;nativeStatementTemp945.push_back(JSValue("name"));nativeStatementTemp945.push_back(name);return nativeStatementTemp943.call(nativeStatementTemp944,nativeStatementTemp945);}());
return JSValue{};
}

JSValue method30(const JSValue& nativeStatementTemp948,const Args& nativeStatementTemp949,const Args& nativeStatementTemp950){
auto nativeStatementTemp951=nativeStatementTemp950[0];auto nativeStatementTemp952=nativeStatementTemp948;
JSValue name=arg(nativeStatementTemp949,0);
JSValue value=arg(nativeStatementTemp949,1);
if(([&]()->JSValue{const auto nativeStatementTemp954=([&]()->JSValue{const auto nativeStatementTemp956=nativeStatementTemp952;const auto nativeStatementTemp957=name;return nativeStatementTemp956.get(nativeStatementTemp957);}());const auto nativeStatementTemp955=value;return binary("===",nativeStatementTemp954,nativeStatementTemp955);}()).toBoolean())return nativeStatementTemp952;
{JSValue i=JSValue(0);
JSValue l=([&]()->JSValue{const auto nativeStatementTemp958=([&]()->JSValue{const auto nativeStatementTemp960=nativeStatementTemp952;const auto nativeStatementTemp961=JSValue("children");return nativeStatementTemp960.get(nativeStatementTemp961);}());const auto nativeStatementTemp959=JSValue("length");return nativeStatementTemp958.get(nativeStatementTemp959);}());
for(;([&]()->JSValue{const auto nativeStatementTemp962=i;const auto nativeStatementTemp963=l;return binary("<",nativeStatementTemp962,nativeStatementTemp963);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp964=i.toNumber();const auto nativeStatementTemp965=nativeStatementTemp964+1;i=nativeStatementTemp965;return nativeStatementTemp964;}())){
JSValue child=([&]()->JSValue{const auto nativeStatementTemp966=([&]()->JSValue{const auto nativeStatementTemp968=nativeStatementTemp952;const auto nativeStatementTemp969=JSValue("children");return nativeStatementTemp968.get(nativeStatementTemp969);}());const auto nativeStatementTemp967=i;return nativeStatementTemp966.get(nativeStatementTemp967);}());
JSValue object=([&]()->JSValue{auto nativeStatementTemp973=child;const auto nativeStatementTemp974=JSValue("getObjectByProperty");const auto nativeStatementTemp970=nativeStatementTemp973.get(nativeStatementTemp974);const auto nativeStatementTemp971=nativeStatementTemp973;Args nativeStatementTemp972;nativeStatementTemp972.push_back(name);nativeStatementTemp972.push_back(value);return nativeStatementTemp970.call(nativeStatementTemp971,nativeStatementTemp972);}());
if(([&]()->JSValue{const auto nativeStatementTemp975=object;const auto nativeStatementTemp976=JSValue{};return binary("!==",nativeStatementTemp975,nativeStatementTemp976);}()).toBoolean()){
return object;

}

}
}
return JSValue{};
return JSValue{};
}

JSValue method31(const JSValue& nativeStatementTemp977,const Args& nativeStatementTemp978,const Args& nativeStatementTemp979){
auto nativeStatementTemp980=nativeStatementTemp979[0];auto nativeStatementTemp981=nativeStatementTemp977;
JSValue name=arg(nativeStatementTemp978,0);
JSValue value=arg(nativeStatementTemp978,1);
JSValue result=arg(nativeStatementTemp978,2);
if(result.isUndefined())result=([&]()->JSValue{auto nativeStatementTemp983=JSValue::array();return nativeStatementTemp983;}());
if(([&]()->JSValue{const auto nativeStatementTemp984=([&]()->JSValue{const auto nativeStatementTemp986=nativeStatementTemp981;const auto nativeStatementTemp987=name;return nativeStatementTemp986.get(nativeStatementTemp987);}());const auto nativeStatementTemp985=value;return binary("===",nativeStatementTemp984,nativeStatementTemp985);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp991=result;const auto nativeStatementTemp992=JSValue("push");const auto nativeStatementTemp988=nativeStatementTemp991.get(nativeStatementTemp992);const auto nativeStatementTemp989=nativeStatementTemp991;Args nativeStatementTemp990;nativeStatementTemp990.push_back(nativeStatementTemp981);return nativeStatementTemp988.call(nativeStatementTemp989,nativeStatementTemp990);}());
JSValue children=([&]()->JSValue{const auto nativeStatementTemp993=nativeStatementTemp981;const auto nativeStatementTemp994=JSValue("children");return nativeStatementTemp993.get(nativeStatementTemp994);}());
{JSValue i=JSValue(0);
JSValue l=([&]()->JSValue{const auto nativeStatementTemp995=children;const auto nativeStatementTemp996=JSValue("length");return nativeStatementTemp995.get(nativeStatementTemp996);}());
for(;([&]()->JSValue{const auto nativeStatementTemp997=i;const auto nativeStatementTemp998=l;return binary("<",nativeStatementTemp997,nativeStatementTemp998);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp999=i.toNumber();const auto nativeStatementTemp1000=nativeStatementTemp999+1;i=nativeStatementTemp1000;return nativeStatementTemp999;}())){
([&]()->JSValue{auto nativeStatementTemp1004=([&]()->JSValue{const auto nativeStatementTemp1006=children;const auto nativeStatementTemp1007=i;return nativeStatementTemp1006.get(nativeStatementTemp1007);}());const auto nativeStatementTemp1005=JSValue("getObjectsByProperty");const auto nativeStatementTemp1001=nativeStatementTemp1004.get(nativeStatementTemp1005);const auto nativeStatementTemp1002=nativeStatementTemp1004;Args nativeStatementTemp1003;nativeStatementTemp1003.push_back(name);nativeStatementTemp1003.push_back(value);nativeStatementTemp1003.push_back(result);return nativeStatementTemp1001.call(nativeStatementTemp1002,nativeStatementTemp1003);}());

}
}
return result;
return JSValue{};
}

JSValue method32(const JSValue& nativeStatementTemp1008,const Args& nativeStatementTemp1009,const Args& nativeStatementTemp1010){
auto nativeStatementTemp1011=nativeStatementTemp1010[0];auto nativeStatementTemp1012=nativeStatementTemp1008;
JSValue target=arg(nativeStatementTemp1009,0);
([&]()->JSValue{auto nativeStatementTemp1017=nativeStatementTemp1012;const auto nativeStatementTemp1018=JSValue("updateWorldMatrix");const auto nativeStatementTemp1014=nativeStatementTemp1017.get(nativeStatementTemp1018);const auto nativeStatementTemp1015=nativeStatementTemp1017;Args nativeStatementTemp1016;nativeStatementTemp1016.push_back(JSValue(true));nativeStatementTemp1016.push_back(JSValue(false));return nativeStatementTemp1014.call(nativeStatementTemp1015,nativeStatementTemp1016);}());
return ([&]()->JSValue{auto nativeStatementTemp1022=target;const auto nativeStatementTemp1023=JSValue("setFromMatrixPosition");const auto nativeStatementTemp1019=nativeStatementTemp1022.get(nativeStatementTemp1023);const auto nativeStatementTemp1020=nativeStatementTemp1022;Args nativeStatementTemp1021;nativeStatementTemp1021.push_back(([&]()->JSValue{const auto nativeStatementTemp1024=nativeStatementTemp1012;const auto nativeStatementTemp1025=JSValue("matrixWorld");return nativeStatementTemp1024.get(nativeStatementTemp1025);}()));return nativeStatementTemp1019.call(nativeStatementTemp1020,nativeStatementTemp1021);}());
return JSValue{};
}

JSValue method33(const JSValue& nativeStatementTemp1026,const Args& nativeStatementTemp1027,const Args& nativeStatementTemp1028){
auto nativeStatementTemp1029=nativeStatementTemp1028[0];auto nativeStatementTemp1030=nativeStatementTemp1026;
JSValue target=arg(nativeStatementTemp1027,0);
([&]()->JSValue{auto nativeStatementTemp1035=nativeStatementTemp1030;const auto nativeStatementTemp1036=JSValue("updateWorldMatrix");const auto nativeStatementTemp1032=nativeStatementTemp1035.get(nativeStatementTemp1036);const auto nativeStatementTemp1033=nativeStatementTemp1035;Args nativeStatementTemp1034;nativeStatementTemp1034.push_back(JSValue(true));nativeStatementTemp1034.push_back(JSValue(false));return nativeStatementTemp1032.call(nativeStatementTemp1033,nativeStatementTemp1034);}());
([&]()->JSValue{auto nativeStatementTemp1040=([&]()->JSValue{const auto nativeStatementTemp1042=nativeStatementTemp1030;const auto nativeStatementTemp1043=JSValue("matrixWorld");return nativeStatementTemp1042.get(nativeStatementTemp1043);}());const auto nativeStatementTemp1041=JSValue("decompose");const auto nativeStatementTemp1037=nativeStatementTemp1040.get(nativeStatementTemp1041);const auto nativeStatementTemp1038=nativeStatementTemp1040;Args nativeStatementTemp1039;nativeStatementTemp1039.push_back(nativeStatementTemp1029.get("_position"));nativeStatementTemp1039.push_back(target);nativeStatementTemp1039.push_back(nativeStatementTemp1029.get("_scale"));return nativeStatementTemp1037.call(nativeStatementTemp1038,nativeStatementTemp1039);}());
return target;
return JSValue{};
}

JSValue method34(const JSValue& nativeStatementTemp1044,const Args& nativeStatementTemp1045,const Args& nativeStatementTemp1046){
auto nativeStatementTemp1047=nativeStatementTemp1046[0];auto nativeStatementTemp1048=nativeStatementTemp1044;
JSValue target=arg(nativeStatementTemp1045,0);
([&]()->JSValue{auto nativeStatementTemp1053=nativeStatementTemp1048;const auto nativeStatementTemp1054=JSValue("updateWorldMatrix");const auto nativeStatementTemp1050=nativeStatementTemp1053.get(nativeStatementTemp1054);const auto nativeStatementTemp1051=nativeStatementTemp1053;Args nativeStatementTemp1052;nativeStatementTemp1052.push_back(JSValue(true));nativeStatementTemp1052.push_back(JSValue(false));return nativeStatementTemp1050.call(nativeStatementTemp1051,nativeStatementTemp1052);}());
([&]()->JSValue{auto nativeStatementTemp1058=([&]()->JSValue{const auto nativeStatementTemp1060=nativeStatementTemp1048;const auto nativeStatementTemp1061=JSValue("matrixWorld");return nativeStatementTemp1060.get(nativeStatementTemp1061);}());const auto nativeStatementTemp1059=JSValue("decompose");const auto nativeStatementTemp1055=nativeStatementTemp1058.get(nativeStatementTemp1059);const auto nativeStatementTemp1056=nativeStatementTemp1058;Args nativeStatementTemp1057;nativeStatementTemp1057.push_back(nativeStatementTemp1047.get("_position"));nativeStatementTemp1057.push_back(nativeStatementTemp1047.get("_quaternion"));nativeStatementTemp1057.push_back(target);return nativeStatementTemp1055.call(nativeStatementTemp1056,nativeStatementTemp1057);}());
return target;
return JSValue{};
}

JSValue method35(const JSValue& nativeStatementTemp1062,const Args& nativeStatementTemp1063,const Args& nativeStatementTemp1064){
auto nativeStatementTemp1065=nativeStatementTemp1064[0];auto nativeStatementTemp1066=nativeStatementTemp1062;
JSValue target=arg(nativeStatementTemp1063,0);
([&]()->JSValue{auto nativeStatementTemp1071=nativeStatementTemp1066;const auto nativeStatementTemp1072=JSValue("updateWorldMatrix");const auto nativeStatementTemp1068=nativeStatementTemp1071.get(nativeStatementTemp1072);const auto nativeStatementTemp1069=nativeStatementTemp1071;Args nativeStatementTemp1070;nativeStatementTemp1070.push_back(JSValue(true));nativeStatementTemp1070.push_back(JSValue(false));return nativeStatementTemp1068.call(nativeStatementTemp1069,nativeStatementTemp1070);}());
JSValue e=([&]()->JSValue{const auto nativeStatementTemp1073=([&]()->JSValue{const auto nativeStatementTemp1075=nativeStatementTemp1066;const auto nativeStatementTemp1076=JSValue("matrixWorld");return nativeStatementTemp1075.get(nativeStatementTemp1076);}());const auto nativeStatementTemp1074=JSValue("elements");return nativeStatementTemp1073.get(nativeStatementTemp1074);}());
return ([&]()->JSValue{auto nativeStatementTemp1080=([&]()->JSValue{auto nativeStatementTemp1085=target;const auto nativeStatementTemp1086=JSValue("set");const auto nativeStatementTemp1082=nativeStatementTemp1085.get(nativeStatementTemp1086);const auto nativeStatementTemp1083=nativeStatementTemp1085;Args nativeStatementTemp1084;nativeStatementTemp1084.push_back(([&]()->JSValue{const auto nativeStatementTemp1087=e;const auto nativeStatementTemp1088=JSValue(8);return nativeStatementTemp1087.get(nativeStatementTemp1088);}()));nativeStatementTemp1084.push_back(([&]()->JSValue{const auto nativeStatementTemp1089=e;const auto nativeStatementTemp1090=JSValue(9);return nativeStatementTemp1089.get(nativeStatementTemp1090);}()));nativeStatementTemp1084.push_back(([&]()->JSValue{const auto nativeStatementTemp1091=e;const auto nativeStatementTemp1092=JSValue(10);return nativeStatementTemp1091.get(nativeStatementTemp1092);}()));return nativeStatementTemp1082.call(nativeStatementTemp1083,nativeStatementTemp1084);}());const auto nativeStatementTemp1081=JSValue("normalize");const auto nativeStatementTemp1077=nativeStatementTemp1080.get(nativeStatementTemp1081);const auto nativeStatementTemp1078=nativeStatementTemp1080;Args nativeStatementTemp1079;return nativeStatementTemp1077.call(nativeStatementTemp1078,nativeStatementTemp1079);}());
return JSValue{};
}

JSValue method36(const JSValue& nativeStatementTemp1093,const Args& nativeStatementTemp1094,const Args& nativeStatementTemp1095){
auto nativeStatementTemp1096=nativeStatementTemp1095[0];auto nativeStatementTemp1097=nativeStatementTemp1093;
return JSValue{};
}

JSValue method37(const JSValue& nativeStatementTemp1099,const Args& nativeStatementTemp1100,const Args& nativeStatementTemp1101){
auto nativeStatementTemp1102=nativeStatementTemp1101[0];auto nativeStatementTemp1103=nativeStatementTemp1099;
return JSValue{};
}

JSValue method38(const JSValue& nativeStatementTemp1105,const Args& nativeStatementTemp1106,const Args& nativeStatementTemp1107){
auto nativeStatementTemp1108=nativeStatementTemp1107[0];auto nativeStatementTemp1109=nativeStatementTemp1105;
JSValue callback=arg(nativeStatementTemp1106,0);
([&]()->JSValue{const auto nativeStatementTemp1111=callback;const JSValue nativeStatementTemp1112;Args nativeStatementTemp1113;nativeStatementTemp1113.push_back(nativeStatementTemp1109);return nativeStatementTemp1111.call(nativeStatementTemp1112,nativeStatementTemp1113);}());
JSValue children=([&]()->JSValue{const auto nativeStatementTemp1114=nativeStatementTemp1109;const auto nativeStatementTemp1115=JSValue("children");return nativeStatementTemp1114.get(nativeStatementTemp1115);}());
{JSValue i=JSValue(0);
JSValue l=([&]()->JSValue{const auto nativeStatementTemp1116=children;const auto nativeStatementTemp1117=JSValue("length");return nativeStatementTemp1116.get(nativeStatementTemp1117);}());
for(;([&]()->JSValue{const auto nativeStatementTemp1118=i;const auto nativeStatementTemp1119=l;return binary("<",nativeStatementTemp1118,nativeStatementTemp1119);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1120=i.toNumber();const auto nativeStatementTemp1121=nativeStatementTemp1120+1;i=nativeStatementTemp1121;return nativeStatementTemp1120;}())){
([&]()->JSValue{auto nativeStatementTemp1125=([&]()->JSValue{const auto nativeStatementTemp1127=children;const auto nativeStatementTemp1128=i;return nativeStatementTemp1127.get(nativeStatementTemp1128);}());const auto nativeStatementTemp1126=JSValue("traverse");const auto nativeStatementTemp1122=nativeStatementTemp1125.get(nativeStatementTemp1126);const auto nativeStatementTemp1123=nativeStatementTemp1125;Args nativeStatementTemp1124;nativeStatementTemp1124.push_back(callback);return nativeStatementTemp1122.call(nativeStatementTemp1123,nativeStatementTemp1124);}());

}
}
return JSValue{};
}

JSValue method39(const JSValue& nativeStatementTemp1129,const Args& nativeStatementTemp1130,const Args& nativeStatementTemp1131){
auto nativeStatementTemp1132=nativeStatementTemp1131[0];auto nativeStatementTemp1133=nativeStatementTemp1129;
JSValue callback=arg(nativeStatementTemp1130,0);
if(([&]()->JSValue{const auto nativeStatementTemp1135=([&]()->JSValue{const auto nativeStatementTemp1137=nativeStatementTemp1133;const auto nativeStatementTemp1138=JSValue("visible");return nativeStatementTemp1137.get(nativeStatementTemp1138);}());const auto nativeStatementTemp1136=JSValue(false);return binary("===",nativeStatementTemp1135,nativeStatementTemp1136);}()).toBoolean())return JSValue{};
([&]()->JSValue{const auto nativeStatementTemp1139=callback;const JSValue nativeStatementTemp1140;Args nativeStatementTemp1141;nativeStatementTemp1141.push_back(nativeStatementTemp1133);return nativeStatementTemp1139.call(nativeStatementTemp1140,nativeStatementTemp1141);}());
JSValue children=([&]()->JSValue{const auto nativeStatementTemp1142=nativeStatementTemp1133;const auto nativeStatementTemp1143=JSValue("children");return nativeStatementTemp1142.get(nativeStatementTemp1143);}());
{JSValue i=JSValue(0);
JSValue l=([&]()->JSValue{const auto nativeStatementTemp1144=children;const auto nativeStatementTemp1145=JSValue("length");return nativeStatementTemp1144.get(nativeStatementTemp1145);}());
for(;([&]()->JSValue{const auto nativeStatementTemp1146=i;const auto nativeStatementTemp1147=l;return binary("<",nativeStatementTemp1146,nativeStatementTemp1147);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1148=i.toNumber();const auto nativeStatementTemp1149=nativeStatementTemp1148+1;i=nativeStatementTemp1149;return nativeStatementTemp1148;}())){
([&]()->JSValue{auto nativeStatementTemp1153=([&]()->JSValue{const auto nativeStatementTemp1155=children;const auto nativeStatementTemp1156=i;return nativeStatementTemp1155.get(nativeStatementTemp1156);}());const auto nativeStatementTemp1154=JSValue("traverseVisible");const auto nativeStatementTemp1150=nativeStatementTemp1153.get(nativeStatementTemp1154);const auto nativeStatementTemp1151=nativeStatementTemp1153;Args nativeStatementTemp1152;nativeStatementTemp1152.push_back(callback);return nativeStatementTemp1150.call(nativeStatementTemp1151,nativeStatementTemp1152);}());

}
}
return JSValue{};
}

JSValue method40(const JSValue& nativeStatementTemp1157,const Args& nativeStatementTemp1158,const Args& nativeStatementTemp1159){
auto nativeStatementTemp1160=nativeStatementTemp1159[0];auto nativeStatementTemp1161=nativeStatementTemp1157;
JSValue callback=arg(nativeStatementTemp1158,0);
JSValue parent=([&]()->JSValue{const auto nativeStatementTemp1163=nativeStatementTemp1161;const auto nativeStatementTemp1164=JSValue("parent");return nativeStatementTemp1163.get(nativeStatementTemp1164);}());
if(([&]()->JSValue{const auto nativeStatementTemp1165=parent;const auto nativeStatementTemp1166=JSValue(nullptr);return binary("!==",nativeStatementTemp1165,nativeStatementTemp1166);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp1167=callback;const JSValue nativeStatementTemp1168;Args nativeStatementTemp1169;nativeStatementTemp1169.push_back(parent);return nativeStatementTemp1167.call(nativeStatementTemp1168,nativeStatementTemp1169);}());
([&]()->JSValue{auto nativeStatementTemp1173=parent;const auto nativeStatementTemp1174=JSValue("traverseAncestors");const auto nativeStatementTemp1170=nativeStatementTemp1173.get(nativeStatementTemp1174);const auto nativeStatementTemp1171=nativeStatementTemp1173;Args nativeStatementTemp1172;nativeStatementTemp1172.push_back(callback);return nativeStatementTemp1170.call(nativeStatementTemp1171,nativeStatementTemp1172);}());

}
return JSValue{};
}

JSValue method41(const JSValue& nativeStatementTemp1175,const Args& nativeStatementTemp1176,const Args& nativeStatementTemp1177){
auto nativeStatementTemp1178=nativeStatementTemp1177[0];auto nativeStatementTemp1179=nativeStatementTemp1175;
([&]()->JSValue{auto nativeStatementTemp1184=([&]()->JSValue{const auto nativeStatementTemp1186=nativeStatementTemp1179;const auto nativeStatementTemp1187=JSValue("matrix");return nativeStatementTemp1186.get(nativeStatementTemp1187);}());const auto nativeStatementTemp1185=JSValue("compose");const auto nativeStatementTemp1181=nativeStatementTemp1184.get(nativeStatementTemp1185);const auto nativeStatementTemp1182=nativeStatementTemp1184;Args nativeStatementTemp1183;nativeStatementTemp1183.push_back(([&]()->JSValue{const auto nativeStatementTemp1188=nativeStatementTemp1179;const auto nativeStatementTemp1189=JSValue("position");return nativeStatementTemp1188.get(nativeStatementTemp1189);}()));nativeStatementTemp1183.push_back(([&]()->JSValue{const auto nativeStatementTemp1190=nativeStatementTemp1179;const auto nativeStatementTemp1191=JSValue("quaternion");return nativeStatementTemp1190.get(nativeStatementTemp1191);}()));nativeStatementTemp1183.push_back(([&]()->JSValue{const auto nativeStatementTemp1192=nativeStatementTemp1179;const auto nativeStatementTemp1193=JSValue("scale");return nativeStatementTemp1192.get(nativeStatementTemp1193);}()));return nativeStatementTemp1181.call(nativeStatementTemp1182,nativeStatementTemp1183);}());
JSValue pivot=([&]()->JSValue{const auto nativeStatementTemp1194=nativeStatementTemp1179;const auto nativeStatementTemp1195=JSValue("pivot");return nativeStatementTemp1194.get(nativeStatementTemp1195);}());
if(([&]()->JSValue{const auto nativeStatementTemp1196=pivot;const auto nativeStatementTemp1197=JSValue(nullptr);return binary("!==",nativeStatementTemp1196,nativeStatementTemp1197);}()).toBoolean()){
JSValue px=([&]()->JSValue{const auto nativeStatementTemp1198=pivot;const auto nativeStatementTemp1199=JSValue("x");return nativeStatementTemp1198.get(nativeStatementTemp1199);}());
JSValue py=([&]()->JSValue{const auto nativeStatementTemp1200=pivot;const auto nativeStatementTemp1201=JSValue("y");return nativeStatementTemp1200.get(nativeStatementTemp1201);}());
JSValue pz=([&]()->JSValue{const auto nativeStatementTemp1202=pivot;const auto nativeStatementTemp1203=JSValue("z");return nativeStatementTemp1202.get(nativeStatementTemp1203);}());
JSValue te=([&]()->JSValue{const auto nativeStatementTemp1204=([&]()->JSValue{const auto nativeStatementTemp1206=nativeStatementTemp1179;const auto nativeStatementTemp1207=JSValue("matrix");return nativeStatementTemp1206.get(nativeStatementTemp1207);}());const auto nativeStatementTemp1205=JSValue("elements");return nativeStatementTemp1204.get(nativeStatementTemp1205);}());
([&]()->JSValue{auto nativeStatementTemp1208=te;const auto nativeStatementTemp1209=JSValue(12);const auto nativeStatementTemp1211=nativeStatementTemp1208.get(nativeStatementTemp1209);const auto nativeStatementTemp1212=([&]()->JSValue{const auto nativeStatementTemp1213=([&]()->JSValue{const auto nativeStatementTemp1215=([&]()->JSValue{const auto nativeStatementTemp1217=px;const auto nativeStatementTemp1218=([&]()->JSValue{const auto nativeStatementTemp1219=([&]()->JSValue{const auto nativeStatementTemp1221=te;const auto nativeStatementTemp1222=JSValue(0);return nativeStatementTemp1221.get(nativeStatementTemp1222);}());const auto nativeStatementTemp1220=px;return binary("*",nativeStatementTemp1219,nativeStatementTemp1220);}());return binary("-",nativeStatementTemp1217,nativeStatementTemp1218);}());const auto nativeStatementTemp1216=([&]()->JSValue{const auto nativeStatementTemp1223=([&]()->JSValue{const auto nativeStatementTemp1225=te;const auto nativeStatementTemp1226=JSValue(4);return nativeStatementTemp1225.get(nativeStatementTemp1226);}());const auto nativeStatementTemp1224=py;return binary("*",nativeStatementTemp1223,nativeStatementTemp1224);}());return binary("-",nativeStatementTemp1215,nativeStatementTemp1216);}());const auto nativeStatementTemp1214=([&]()->JSValue{const auto nativeStatementTemp1227=([&]()->JSValue{const auto nativeStatementTemp1229=te;const auto nativeStatementTemp1230=JSValue(8);return nativeStatementTemp1229.get(nativeStatementTemp1230);}());const auto nativeStatementTemp1228=pz;return binary("*",nativeStatementTemp1227,nativeStatementTemp1228);}());return binary("-",nativeStatementTemp1213,nativeStatementTemp1214);}());const auto nativeStatementTemp1210=binary("+",nativeStatementTemp1211,nativeStatementTemp1212);setReference(nativeStatementTemp1208,nativeStatementTemp1209,nativeStatementTemp1210);return nativeStatementTemp1210;}());
([&]()->JSValue{auto nativeStatementTemp1231=te;const auto nativeStatementTemp1232=JSValue(13);const auto nativeStatementTemp1234=nativeStatementTemp1231.get(nativeStatementTemp1232);const auto nativeStatementTemp1235=([&]()->JSValue{const auto nativeStatementTemp1236=([&]()->JSValue{const auto nativeStatementTemp1238=([&]()->JSValue{const auto nativeStatementTemp1240=py;const auto nativeStatementTemp1241=([&]()->JSValue{const auto nativeStatementTemp1242=([&]()->JSValue{const auto nativeStatementTemp1244=te;const auto nativeStatementTemp1245=JSValue(1);return nativeStatementTemp1244.get(nativeStatementTemp1245);}());const auto nativeStatementTemp1243=px;return binary("*",nativeStatementTemp1242,nativeStatementTemp1243);}());return binary("-",nativeStatementTemp1240,nativeStatementTemp1241);}());const auto nativeStatementTemp1239=([&]()->JSValue{const auto nativeStatementTemp1246=([&]()->JSValue{const auto nativeStatementTemp1248=te;const auto nativeStatementTemp1249=JSValue(5);return nativeStatementTemp1248.get(nativeStatementTemp1249);}());const auto nativeStatementTemp1247=py;return binary("*",nativeStatementTemp1246,nativeStatementTemp1247);}());return binary("-",nativeStatementTemp1238,nativeStatementTemp1239);}());const auto nativeStatementTemp1237=([&]()->JSValue{const auto nativeStatementTemp1250=([&]()->JSValue{const auto nativeStatementTemp1252=te;const auto nativeStatementTemp1253=JSValue(9);return nativeStatementTemp1252.get(nativeStatementTemp1253);}());const auto nativeStatementTemp1251=pz;return binary("*",nativeStatementTemp1250,nativeStatementTemp1251);}());return binary("-",nativeStatementTemp1236,nativeStatementTemp1237);}());const auto nativeStatementTemp1233=binary("+",nativeStatementTemp1234,nativeStatementTemp1235);setReference(nativeStatementTemp1231,nativeStatementTemp1232,nativeStatementTemp1233);return nativeStatementTemp1233;}());
([&]()->JSValue{auto nativeStatementTemp1254=te;const auto nativeStatementTemp1255=JSValue(14);const auto nativeStatementTemp1257=nativeStatementTemp1254.get(nativeStatementTemp1255);const auto nativeStatementTemp1258=([&]()->JSValue{const auto nativeStatementTemp1259=([&]()->JSValue{const auto nativeStatementTemp1261=([&]()->JSValue{const auto nativeStatementTemp1263=pz;const auto nativeStatementTemp1264=([&]()->JSValue{const auto nativeStatementTemp1265=([&]()->JSValue{const auto nativeStatementTemp1267=te;const auto nativeStatementTemp1268=JSValue(2);return nativeStatementTemp1267.get(nativeStatementTemp1268);}());const auto nativeStatementTemp1266=px;return binary("*",nativeStatementTemp1265,nativeStatementTemp1266);}());return binary("-",nativeStatementTemp1263,nativeStatementTemp1264);}());const auto nativeStatementTemp1262=([&]()->JSValue{const auto nativeStatementTemp1269=([&]()->JSValue{const auto nativeStatementTemp1271=te;const auto nativeStatementTemp1272=JSValue(6);return nativeStatementTemp1271.get(nativeStatementTemp1272);}());const auto nativeStatementTemp1270=py;return binary("*",nativeStatementTemp1269,nativeStatementTemp1270);}());return binary("-",nativeStatementTemp1261,nativeStatementTemp1262);}());const auto nativeStatementTemp1260=([&]()->JSValue{const auto nativeStatementTemp1273=([&]()->JSValue{const auto nativeStatementTemp1275=te;const auto nativeStatementTemp1276=JSValue(10);return nativeStatementTemp1275.get(nativeStatementTemp1276);}());const auto nativeStatementTemp1274=pz;return binary("*",nativeStatementTemp1273,nativeStatementTemp1274);}());return binary("-",nativeStatementTemp1259,nativeStatementTemp1260);}());const auto nativeStatementTemp1256=binary("+",nativeStatementTemp1257,nativeStatementTemp1258);setReference(nativeStatementTemp1254,nativeStatementTemp1255,nativeStatementTemp1256);return nativeStatementTemp1256;}());

}
([&]()->JSValue{auto nativeStatementTemp1277=nativeStatementTemp1179;const auto nativeStatementTemp1278=JSValue("matrixWorldNeedsUpdate");const auto nativeStatementTemp1279=JSValue(true);setReference(nativeStatementTemp1277,nativeStatementTemp1278,nativeStatementTemp1279);return nativeStatementTemp1279;}());
return JSValue{};
}

JSValue method42(const JSValue& nativeStatementTemp1282,const Args& nativeStatementTemp1283,const Args& nativeStatementTemp1284){
auto nativeStatementTemp1285=nativeStatementTemp1284[0];auto nativeStatementTemp1286=nativeStatementTemp1282;
JSValue force=arg(nativeStatementTemp1283,0);
if(([&]()->JSValue{const auto nativeStatementTemp1288=nativeStatementTemp1286;const auto nativeStatementTemp1289=JSValue("matrixAutoUpdate");return nativeStatementTemp1288.get(nativeStatementTemp1289);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp1293=nativeStatementTemp1286;const auto nativeStatementTemp1294=JSValue("updateMatrix");const auto nativeStatementTemp1290=nativeStatementTemp1293.get(nativeStatementTemp1294);const auto nativeStatementTemp1291=nativeStatementTemp1293;Args nativeStatementTemp1292;return nativeStatementTemp1290.call(nativeStatementTemp1291,nativeStatementTemp1292);}());
if(([&]()->JSValue{const auto nativeStatementTemp1295=([&]()->JSValue{const auto nativeStatementTemp1296=nativeStatementTemp1286;const auto nativeStatementTemp1297=JSValue("matrixWorldNeedsUpdate");return nativeStatementTemp1296.get(nativeStatementTemp1297);}());if(nativeStatementTemp1295.toBoolean())return nativeStatementTemp1295;return force;}()).toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp1298=([&]()->JSValue{const auto nativeStatementTemp1300=nativeStatementTemp1286;const auto nativeStatementTemp1301=JSValue("matrixWorldAutoUpdate");return nativeStatementTemp1300.get(nativeStatementTemp1301);}());const auto nativeStatementTemp1299=JSValue(true);return binary("===",nativeStatementTemp1298,nativeStatementTemp1299);}()).toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp1302=([&]()->JSValue{const auto nativeStatementTemp1304=nativeStatementTemp1286;const auto nativeStatementTemp1305=JSValue("parent");return nativeStatementTemp1304.get(nativeStatementTemp1305);}());const auto nativeStatementTemp1303=JSValue(nullptr);return binary("===",nativeStatementTemp1302,nativeStatementTemp1303);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp1309=([&]()->JSValue{const auto nativeStatementTemp1311=nativeStatementTemp1286;const auto nativeStatementTemp1312=JSValue("matrixWorld");return nativeStatementTemp1311.get(nativeStatementTemp1312);}());const auto nativeStatementTemp1310=JSValue("copy");const auto nativeStatementTemp1306=nativeStatementTemp1309.get(nativeStatementTemp1310);const auto nativeStatementTemp1307=nativeStatementTemp1309;Args nativeStatementTemp1308;nativeStatementTemp1308.push_back(([&]()->JSValue{const auto nativeStatementTemp1313=nativeStatementTemp1286;const auto nativeStatementTemp1314=JSValue("matrix");return nativeStatementTemp1313.get(nativeStatementTemp1314);}()));return nativeStatementTemp1306.call(nativeStatementTemp1307,nativeStatementTemp1308);}());

}
else {
([&]()->JSValue{auto nativeStatementTemp1318=([&]()->JSValue{const auto nativeStatementTemp1320=nativeStatementTemp1286;const auto nativeStatementTemp1321=JSValue("matrixWorld");return nativeStatementTemp1320.get(nativeStatementTemp1321);}());const auto nativeStatementTemp1319=JSValue("multiplyMatrices");const auto nativeStatementTemp1315=nativeStatementTemp1318.get(nativeStatementTemp1319);const auto nativeStatementTemp1316=nativeStatementTemp1318;Args nativeStatementTemp1317;nativeStatementTemp1317.push_back(([&]()->JSValue{const auto nativeStatementTemp1322=([&]()->JSValue{const auto nativeStatementTemp1324=nativeStatementTemp1286;const auto nativeStatementTemp1325=JSValue("parent");return nativeStatementTemp1324.get(nativeStatementTemp1325);}());const auto nativeStatementTemp1323=JSValue("matrixWorld");return nativeStatementTemp1322.get(nativeStatementTemp1323);}()));nativeStatementTemp1317.push_back(([&]()->JSValue{const auto nativeStatementTemp1326=nativeStatementTemp1286;const auto nativeStatementTemp1327=JSValue("matrix");return nativeStatementTemp1326.get(nativeStatementTemp1327);}()));return nativeStatementTemp1315.call(nativeStatementTemp1316,nativeStatementTemp1317);}());

}

}
([&]()->JSValue{auto nativeStatementTemp1328=nativeStatementTemp1286;const auto nativeStatementTemp1329=JSValue("matrixWorldNeedsUpdate");const auto nativeStatementTemp1330=JSValue(false);setReference(nativeStatementTemp1328,nativeStatementTemp1329,nativeStatementTemp1330);return nativeStatementTemp1330;}());
([&]()->JSValue{const auto nativeStatementTemp1333=JSValue(true);force=nativeStatementTemp1333;return nativeStatementTemp1333;}());

}
JSValue children=([&]()->JSValue{const auto nativeStatementTemp1336=nativeStatementTemp1286;const auto nativeStatementTemp1337=JSValue("children");return nativeStatementTemp1336.get(nativeStatementTemp1337);}());
{JSValue i=JSValue(0);
JSValue l=([&]()->JSValue{const auto nativeStatementTemp1338=children;const auto nativeStatementTemp1339=JSValue("length");return nativeStatementTemp1338.get(nativeStatementTemp1339);}());
for(;([&]()->JSValue{const auto nativeStatementTemp1340=i;const auto nativeStatementTemp1341=l;return binary("<",nativeStatementTemp1340,nativeStatementTemp1341);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1342=i.toNumber();const auto nativeStatementTemp1343=nativeStatementTemp1342+1;i=nativeStatementTemp1343;return nativeStatementTemp1342;}())){
JSValue child=([&]()->JSValue{const auto nativeStatementTemp1344=children;const auto nativeStatementTemp1345=i;return nativeStatementTemp1344.get(nativeStatementTemp1345);}());
([&]()->JSValue{auto nativeStatementTemp1349=child;const auto nativeStatementTemp1350=JSValue("updateMatrixWorld");const auto nativeStatementTemp1346=nativeStatementTemp1349.get(nativeStatementTemp1350);const auto nativeStatementTemp1347=nativeStatementTemp1349;Args nativeStatementTemp1348;nativeStatementTemp1348.push_back(force);return nativeStatementTemp1346.call(nativeStatementTemp1347,nativeStatementTemp1348);}());

}
}
return JSValue{};
}

JSValue method43(const JSValue& nativeStatementTemp1351,const Args& nativeStatementTemp1352,const Args& nativeStatementTemp1353){
auto nativeStatementTemp1354=nativeStatementTemp1353[0];auto nativeStatementTemp1355=nativeStatementTemp1351;
JSValue updateParents=arg(nativeStatementTemp1352,0);
JSValue updateChildren=arg(nativeStatementTemp1352,1);
JSValue force=arg(nativeStatementTemp1352,2);
if(force.isUndefined())force=JSValue(false);
JSValue parent=([&]()->JSValue{const auto nativeStatementTemp1357=nativeStatementTemp1355;const auto nativeStatementTemp1358=JSValue("parent");return nativeStatementTemp1357.get(nativeStatementTemp1358);}());
if(([&]()->JSValue{const auto nativeStatementTemp1359=([&]()->JSValue{const auto nativeStatementTemp1360=updateParents;const auto nativeStatementTemp1361=JSValue(true);return binary("===",nativeStatementTemp1360,nativeStatementTemp1361);}());if(!nativeStatementTemp1359.toBoolean())return nativeStatementTemp1359;return ([&]()->JSValue{const auto nativeStatementTemp1362=parent;const auto nativeStatementTemp1363=JSValue(nullptr);return binary("!==",nativeStatementTemp1362,nativeStatementTemp1363);}());}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp1367=parent;const auto nativeStatementTemp1368=JSValue("updateWorldMatrix");const auto nativeStatementTemp1364=nativeStatementTemp1367.get(nativeStatementTemp1368);const auto nativeStatementTemp1365=nativeStatementTemp1367;Args nativeStatementTemp1366;nativeStatementTemp1366.push_back(JSValue(true));nativeStatementTemp1366.push_back(JSValue(false));return nativeStatementTemp1364.call(nativeStatementTemp1365,nativeStatementTemp1366);}());

}
if(([&]()->JSValue{const auto nativeStatementTemp1369=nativeStatementTemp1355;const auto nativeStatementTemp1370=JSValue("matrixAutoUpdate");return nativeStatementTemp1369.get(nativeStatementTemp1370);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp1374=nativeStatementTemp1355;const auto nativeStatementTemp1375=JSValue("updateMatrix");const auto nativeStatementTemp1371=nativeStatementTemp1374.get(nativeStatementTemp1375);const auto nativeStatementTemp1372=nativeStatementTemp1374;Args nativeStatementTemp1373;return nativeStatementTemp1371.call(nativeStatementTemp1372,nativeStatementTemp1373);}());
if(([&]()->JSValue{const auto nativeStatementTemp1376=([&]()->JSValue{const auto nativeStatementTemp1377=nativeStatementTemp1355;const auto nativeStatementTemp1378=JSValue("matrixWorldNeedsUpdate");return nativeStatementTemp1377.get(nativeStatementTemp1378);}());if(nativeStatementTemp1376.toBoolean())return nativeStatementTemp1376;return force;}()).toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp1379=([&]()->JSValue{const auto nativeStatementTemp1381=nativeStatementTemp1355;const auto nativeStatementTemp1382=JSValue("matrixWorldAutoUpdate");return nativeStatementTemp1381.get(nativeStatementTemp1382);}());const auto nativeStatementTemp1380=JSValue(true);return binary("===",nativeStatementTemp1379,nativeStatementTemp1380);}()).toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp1383=([&]()->JSValue{const auto nativeStatementTemp1385=nativeStatementTemp1355;const auto nativeStatementTemp1386=JSValue("parent");return nativeStatementTemp1385.get(nativeStatementTemp1386);}());const auto nativeStatementTemp1384=JSValue(nullptr);return binary("===",nativeStatementTemp1383,nativeStatementTemp1384);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp1390=([&]()->JSValue{const auto nativeStatementTemp1392=nativeStatementTemp1355;const auto nativeStatementTemp1393=JSValue("matrixWorld");return nativeStatementTemp1392.get(nativeStatementTemp1393);}());const auto nativeStatementTemp1391=JSValue("copy");const auto nativeStatementTemp1387=nativeStatementTemp1390.get(nativeStatementTemp1391);const auto nativeStatementTemp1388=nativeStatementTemp1390;Args nativeStatementTemp1389;nativeStatementTemp1389.push_back(([&]()->JSValue{const auto nativeStatementTemp1394=nativeStatementTemp1355;const auto nativeStatementTemp1395=JSValue("matrix");return nativeStatementTemp1394.get(nativeStatementTemp1395);}()));return nativeStatementTemp1387.call(nativeStatementTemp1388,nativeStatementTemp1389);}());

}
else {
([&]()->JSValue{auto nativeStatementTemp1399=([&]()->JSValue{const auto nativeStatementTemp1401=nativeStatementTemp1355;const auto nativeStatementTemp1402=JSValue("matrixWorld");return nativeStatementTemp1401.get(nativeStatementTemp1402);}());const auto nativeStatementTemp1400=JSValue("multiplyMatrices");const auto nativeStatementTemp1396=nativeStatementTemp1399.get(nativeStatementTemp1400);const auto nativeStatementTemp1397=nativeStatementTemp1399;Args nativeStatementTemp1398;nativeStatementTemp1398.push_back(([&]()->JSValue{const auto nativeStatementTemp1403=([&]()->JSValue{const auto nativeStatementTemp1405=nativeStatementTemp1355;const auto nativeStatementTemp1406=JSValue("parent");return nativeStatementTemp1405.get(nativeStatementTemp1406);}());const auto nativeStatementTemp1404=JSValue("matrixWorld");return nativeStatementTemp1403.get(nativeStatementTemp1404);}()));nativeStatementTemp1398.push_back(([&]()->JSValue{const auto nativeStatementTemp1407=nativeStatementTemp1355;const auto nativeStatementTemp1408=JSValue("matrix");return nativeStatementTemp1407.get(nativeStatementTemp1408);}()));return nativeStatementTemp1396.call(nativeStatementTemp1397,nativeStatementTemp1398);}());

}

}
([&]()->JSValue{auto nativeStatementTemp1409=nativeStatementTemp1355;const auto nativeStatementTemp1410=JSValue("matrixWorldNeedsUpdate");const auto nativeStatementTemp1411=JSValue(false);setReference(nativeStatementTemp1409,nativeStatementTemp1410,nativeStatementTemp1411);return nativeStatementTemp1411;}());
([&]()->JSValue{const auto nativeStatementTemp1414=JSValue(true);force=nativeStatementTemp1414;return nativeStatementTemp1414;}());

}
if(([&]()->JSValue{const auto nativeStatementTemp1417=updateChildren;const auto nativeStatementTemp1418=JSValue(true);return binary("===",nativeStatementTemp1417,nativeStatementTemp1418);}()).toBoolean()){
JSValue children=([&]()->JSValue{const auto nativeStatementTemp1419=nativeStatementTemp1355;const auto nativeStatementTemp1420=JSValue("children");return nativeStatementTemp1419.get(nativeStatementTemp1420);}());
{JSValue i=JSValue(0);
JSValue l=([&]()->JSValue{const auto nativeStatementTemp1421=children;const auto nativeStatementTemp1422=JSValue("length");return nativeStatementTemp1421.get(nativeStatementTemp1422);}());
for(;([&]()->JSValue{const auto nativeStatementTemp1423=i;const auto nativeStatementTemp1424=l;return binary("<",nativeStatementTemp1423,nativeStatementTemp1424);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1425=i.toNumber();const auto nativeStatementTemp1426=nativeStatementTemp1425+1;i=nativeStatementTemp1426;return nativeStatementTemp1425;}())){
JSValue child=([&]()->JSValue{const auto nativeStatementTemp1427=children;const auto nativeStatementTemp1428=i;return nativeStatementTemp1427.get(nativeStatementTemp1428);}());
([&]()->JSValue{auto nativeStatementTemp1432=child;const auto nativeStatementTemp1433=JSValue("updateWorldMatrix");const auto nativeStatementTemp1429=nativeStatementTemp1432.get(nativeStatementTemp1433);const auto nativeStatementTemp1430=nativeStatementTemp1432;Args nativeStatementTemp1431;nativeStatementTemp1431.push_back(JSValue(false));nativeStatementTemp1431.push_back(JSValue(true));nativeStatementTemp1431.push_back(force);return nativeStatementTemp1429.call(nativeStatementTemp1430,nativeStatementTemp1431);}());

}
}

}
return JSValue{};
}

JSValue method44(const JSValue& nativeStatementTemp1434,const Args& nativeStatementTemp1435,const Args& nativeStatementTemp1436){
auto nativeStatementTemp1437=nativeStatementTemp1436[0];auto nativeStatementTemp1438=nativeStatementTemp1434;
JSValue meta=arg(nativeStatementTemp1435,0);
JSValue extractFromCache=([&]()->JSValue{auto nativeStatementTemp1458=JSValue::constructor(nested2,{nativeStatementTemp1437},"extractFromCache",1,JSValue::ConstructorKind::Callable);nativeStatementTemp1458.setFunctionSourceText("function extractFromCache( cache ) {\r\n\r\n\t\t\tconst values = [];\r\n\t\t\tfor ( const key in cache ) {\r\n\r\n\t\t\t\tconst data = cache[ key ];\r\n\t\t\t\tdelete data.metadata;\r\n\t\t\t\tvalues.push( data );\r\n\r\n\t\t\t}\r\n\r\n\t\t\treturn values;\r\n\r\n\t\t}");return nativeStatementTemp1458;}());
JSValue isRootObject=([&]()->JSValue{const auto nativeStatementTemp1459=([&]()->JSValue{const auto nativeStatementTemp1460=meta;const auto nativeStatementTemp1461=JSValue{};return binary("===",nativeStatementTemp1460,nativeStatementTemp1461);}());if(nativeStatementTemp1459.toBoolean())return nativeStatementTemp1459;return ([&]()->JSValue{const auto nativeStatementTemp1462=([&]()->JSValue{const auto nativeStatementTemp1464=meta;return typeOf(nativeStatementTemp1464);}());const auto nativeStatementTemp1463=JSValue("string");return binary("===",nativeStatementTemp1462,nativeStatementTemp1463);}());}());
JSValue output=([&]()->JSValue{auto nativeStatementTemp1465=JSValue::object();return nativeStatementTemp1465;}());
if(isRootObject.toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp1466=([&]()->JSValue{auto nativeStatementTemp1469=JSValue::object();{const auto nativeStatementTemp1470=JSValue("geometries");const auto nativeStatementTemp1471=([&]()->JSValue{auto nativeStatementTemp1472=JSValue::object();return nativeStatementTemp1472;}());own(nativeStatementTemp1469,nativeStatementTemp1470,nativeStatementTemp1471);}{const auto nativeStatementTemp1473=JSValue("materials");const auto nativeStatementTemp1474=([&]()->JSValue{auto nativeStatementTemp1475=JSValue::object();return nativeStatementTemp1475;}());own(nativeStatementTemp1469,nativeStatementTemp1473,nativeStatementTemp1474);}{const auto nativeStatementTemp1476=JSValue("textures");const auto nativeStatementTemp1477=([&]()->JSValue{auto nativeStatementTemp1478=JSValue::object();return nativeStatementTemp1478;}());own(nativeStatementTemp1469,nativeStatementTemp1476,nativeStatementTemp1477);}{const auto nativeStatementTemp1479=JSValue("images");const auto nativeStatementTemp1480=([&]()->JSValue{auto nativeStatementTemp1481=JSValue::object();return nativeStatementTemp1481;}());own(nativeStatementTemp1469,nativeStatementTemp1479,nativeStatementTemp1480);}{const auto nativeStatementTemp1482=JSValue("shapes");const auto nativeStatementTemp1483=([&]()->JSValue{auto nativeStatementTemp1484=JSValue::object();return nativeStatementTemp1484;}());own(nativeStatementTemp1469,nativeStatementTemp1482,nativeStatementTemp1483);}{const auto nativeStatementTemp1485=JSValue("skeletons");const auto nativeStatementTemp1486=([&]()->JSValue{auto nativeStatementTemp1487=JSValue::object();return nativeStatementTemp1487;}());own(nativeStatementTemp1469,nativeStatementTemp1485,nativeStatementTemp1486);}{const auto nativeStatementTemp1488=JSValue("animations");const auto nativeStatementTemp1489=([&]()->JSValue{auto nativeStatementTemp1490=JSValue::object();return nativeStatementTemp1490;}());own(nativeStatementTemp1469,nativeStatementTemp1488,nativeStatementTemp1489);}{const auto nativeStatementTemp1491=JSValue("nodes");const auto nativeStatementTemp1492=([&]()->JSValue{auto nativeStatementTemp1493=JSValue::object();return nativeStatementTemp1493;}());own(nativeStatementTemp1469,nativeStatementTemp1491,nativeStatementTemp1492);}return nativeStatementTemp1469;}());meta=nativeStatementTemp1466;return nativeStatementTemp1466;}());
([&]()->JSValue{auto nativeStatementTemp1494=output;const auto nativeStatementTemp1495=JSValue("metadata");const auto nativeStatementTemp1496=([&]()->JSValue{auto nativeStatementTemp1499=JSValue::object();{const auto nativeStatementTemp1500=JSValue("version");const auto nativeStatementTemp1501=JSValue(4.7);own(nativeStatementTemp1499,nativeStatementTemp1500,nativeStatementTemp1501);}{const auto nativeStatementTemp1502=JSValue("type");const auto nativeStatementTemp1503=JSValue("Object");own(nativeStatementTemp1499,nativeStatementTemp1502,nativeStatementTemp1503);}{const auto nativeStatementTemp1504=JSValue("generator");const auto nativeStatementTemp1505=JSValue("Object3D.toJSON");own(nativeStatementTemp1499,nativeStatementTemp1504,nativeStatementTemp1505);}return nativeStatementTemp1499;}());setReference(nativeStatementTemp1494,nativeStatementTemp1495,nativeStatementTemp1496);return nativeStatementTemp1496;}());

}
JSValue object=([&]()->JSValue{auto nativeStatementTemp1506=JSValue::object();return nativeStatementTemp1506;}());
([&]()->JSValue{auto nativeStatementTemp1507=object;const auto nativeStatementTemp1508=JSValue("uuid");const auto nativeStatementTemp1509=([&]()->JSValue{const auto nativeStatementTemp1512=nativeStatementTemp1438;const auto nativeStatementTemp1513=JSValue("uuid");return nativeStatementTemp1512.get(nativeStatementTemp1513);}());setReference(nativeStatementTemp1507,nativeStatementTemp1508,nativeStatementTemp1509);return nativeStatementTemp1509;}());
([&]()->JSValue{auto nativeStatementTemp1514=object;const auto nativeStatementTemp1515=JSValue("type");const auto nativeStatementTemp1516=([&]()->JSValue{const auto nativeStatementTemp1519=nativeStatementTemp1438;const auto nativeStatementTemp1520=JSValue("type");return nativeStatementTemp1519.get(nativeStatementTemp1520);}());setReference(nativeStatementTemp1514,nativeStatementTemp1515,nativeStatementTemp1516);return nativeStatementTemp1516;}());
([&]()->JSValue{auto nativeStatementTemp1521=object;const auto nativeStatementTemp1522=JSValue("name");const auto nativeStatementTemp1523=([&]()->JSValue{const auto nativeStatementTemp1526=nativeStatementTemp1438;const auto nativeStatementTemp1527=JSValue("name");return nativeStatementTemp1526.get(nativeStatementTemp1527);}());setReference(nativeStatementTemp1521,nativeStatementTemp1522,nativeStatementTemp1523);return nativeStatementTemp1523;}());
([&]()->JSValue{auto nativeStatementTemp1528=object;const auto nativeStatementTemp1529=JSValue("castShadow");const auto nativeStatementTemp1530=([&]()->JSValue{const auto nativeStatementTemp1533=nativeStatementTemp1438;const auto nativeStatementTemp1534=JSValue("castShadow");return nativeStatementTemp1533.get(nativeStatementTemp1534);}());setReference(nativeStatementTemp1528,nativeStatementTemp1529,nativeStatementTemp1530);return nativeStatementTemp1530;}());
([&]()->JSValue{auto nativeStatementTemp1535=object;const auto nativeStatementTemp1536=JSValue("receiveShadow");const auto nativeStatementTemp1537=([&]()->JSValue{const auto nativeStatementTemp1540=nativeStatementTemp1438;const auto nativeStatementTemp1541=JSValue("receiveShadow");return nativeStatementTemp1540.get(nativeStatementTemp1541);}());setReference(nativeStatementTemp1535,nativeStatementTemp1536,nativeStatementTemp1537);return nativeStatementTemp1537;}());
([&]()->JSValue{auto nativeStatementTemp1542=object;const auto nativeStatementTemp1543=JSValue("visible");const auto nativeStatementTemp1544=([&]()->JSValue{const auto nativeStatementTemp1547=nativeStatementTemp1438;const auto nativeStatementTemp1548=JSValue("visible");return nativeStatementTemp1547.get(nativeStatementTemp1548);}());setReference(nativeStatementTemp1542,nativeStatementTemp1543,nativeStatementTemp1544);return nativeStatementTemp1544;}());
([&]()->JSValue{auto nativeStatementTemp1549=object;const auto nativeStatementTemp1550=JSValue("frustumCulled");const auto nativeStatementTemp1551=([&]()->JSValue{const auto nativeStatementTemp1554=nativeStatementTemp1438;const auto nativeStatementTemp1555=JSValue("frustumCulled");return nativeStatementTemp1554.get(nativeStatementTemp1555);}());setReference(nativeStatementTemp1549,nativeStatementTemp1550,nativeStatementTemp1551);return nativeStatementTemp1551;}());
([&]()->JSValue{auto nativeStatementTemp1556=object;const auto nativeStatementTemp1557=JSValue("renderOrder");const auto nativeStatementTemp1558=([&]()->JSValue{const auto nativeStatementTemp1561=nativeStatementTemp1438;const auto nativeStatementTemp1562=JSValue("renderOrder");return nativeStatementTemp1561.get(nativeStatementTemp1562);}());setReference(nativeStatementTemp1556,nativeStatementTemp1557,nativeStatementTemp1558);return nativeStatementTemp1558;}());
([&]()->JSValue{auto nativeStatementTemp1563=object;const auto nativeStatementTemp1564=JSValue("static");const auto nativeStatementTemp1565=([&]()->JSValue{const auto nativeStatementTemp1568=nativeStatementTemp1438;const auto nativeStatementTemp1569=JSValue("static");return nativeStatementTemp1568.get(nativeStatementTemp1569);}());setReference(nativeStatementTemp1563,nativeStatementTemp1564,nativeStatementTemp1565);return nativeStatementTemp1565;}());
([&]()->JSValue{auto nativeStatementTemp1570=object;const auto nativeStatementTemp1571=JSValue("matrixAutoUpdate");const auto nativeStatementTemp1572=([&]()->JSValue{const auto nativeStatementTemp1575=nativeStatementTemp1438;const auto nativeStatementTemp1576=JSValue("matrixAutoUpdate");return nativeStatementTemp1575.get(nativeStatementTemp1576);}());setReference(nativeStatementTemp1570,nativeStatementTemp1571,nativeStatementTemp1572);return nativeStatementTemp1572;}());
if(([&]()->JSValue{const auto nativeStatementTemp1577=([&]()->JSValue{const auto nativeStatementTemp1579=([&]()->JSValue{auto nativeStatementTemp1584=nativeGlobalBinding("Object");const auto nativeStatementTemp1585=JSValue("keys");const auto nativeStatementTemp1581=nativeStatementTemp1584.get(nativeStatementTemp1585);const auto nativeStatementTemp1582=nativeStatementTemp1584;Args nativeStatementTemp1583;nativeStatementTemp1583.push_back(([&]()->JSValue{const auto nativeStatementTemp1586=nativeStatementTemp1438;const auto nativeStatementTemp1587=JSValue("userData");return nativeStatementTemp1586.get(nativeStatementTemp1587);}()));return nativeStatementTemp1581.call(nativeStatementTemp1582,nativeStatementTemp1583);}());const auto nativeStatementTemp1580=JSValue("length");return nativeStatementTemp1579.get(nativeStatementTemp1580);}());const auto nativeStatementTemp1578=JSValue(0);return binary(">",nativeStatementTemp1577,nativeStatementTemp1578);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp1588=object;const auto nativeStatementTemp1589=JSValue("userData");const auto nativeStatementTemp1590=([&]()->JSValue{const auto nativeStatementTemp1593=nativeStatementTemp1438;const auto nativeStatementTemp1594=JSValue("userData");return nativeStatementTemp1593.get(nativeStatementTemp1594);}());setReference(nativeStatementTemp1588,nativeStatementTemp1589,nativeStatementTemp1590);return nativeStatementTemp1590;}());
([&]()->JSValue{auto nativeStatementTemp1595=object;const auto nativeStatementTemp1596=JSValue("layers");const auto nativeStatementTemp1597=([&]()->JSValue{const auto nativeStatementTemp1600=([&]()->JSValue{const auto nativeStatementTemp1602=nativeStatementTemp1438;const auto nativeStatementTemp1603=JSValue("layers");return nativeStatementTemp1602.get(nativeStatementTemp1603);}());const auto nativeStatementTemp1601=JSValue("mask");return nativeStatementTemp1600.get(nativeStatementTemp1601);}());setReference(nativeStatementTemp1595,nativeStatementTemp1596,nativeStatementTemp1597);return nativeStatementTemp1597;}());
([&]()->JSValue{auto nativeStatementTemp1604=object;const auto nativeStatementTemp1605=JSValue("matrix");const auto nativeStatementTemp1606=([&]()->JSValue{auto nativeStatementTemp1612=([&]()->JSValue{const auto nativeStatementTemp1614=nativeStatementTemp1438;const auto nativeStatementTemp1615=JSValue("matrix");return nativeStatementTemp1614.get(nativeStatementTemp1615);}());const auto nativeStatementTemp1613=JSValue("toArray");const auto nativeStatementTemp1609=nativeStatementTemp1612.get(nativeStatementTemp1613);const auto nativeStatementTemp1610=nativeStatementTemp1612;Args nativeStatementTemp1611;return nativeStatementTemp1609.call(nativeStatementTemp1610,nativeStatementTemp1611);}());setReference(nativeStatementTemp1604,nativeStatementTemp1605,nativeStatementTemp1606);return nativeStatementTemp1606;}());
([&]()->JSValue{auto nativeStatementTemp1616=object;const auto nativeStatementTemp1617=JSValue("up");const auto nativeStatementTemp1618=([&]()->JSValue{auto nativeStatementTemp1624=([&]()->JSValue{const auto nativeStatementTemp1626=nativeStatementTemp1438;const auto nativeStatementTemp1627=JSValue("up");return nativeStatementTemp1626.get(nativeStatementTemp1627);}());const auto nativeStatementTemp1625=JSValue("toArray");const auto nativeStatementTemp1621=nativeStatementTemp1624.get(nativeStatementTemp1625);const auto nativeStatementTemp1622=nativeStatementTemp1624;Args nativeStatementTemp1623;return nativeStatementTemp1621.call(nativeStatementTemp1622,nativeStatementTemp1623);}());setReference(nativeStatementTemp1616,nativeStatementTemp1617,nativeStatementTemp1618);return nativeStatementTemp1618;}());
if(([&]()->JSValue{const auto nativeStatementTemp1628=([&]()->JSValue{const auto nativeStatementTemp1630=nativeStatementTemp1438;const auto nativeStatementTemp1631=JSValue("pivot");return nativeStatementTemp1630.get(nativeStatementTemp1631);}());const auto nativeStatementTemp1629=JSValue(nullptr);return binary("!==",nativeStatementTemp1628,nativeStatementTemp1629);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp1632=object;const auto nativeStatementTemp1633=JSValue("pivot");const auto nativeStatementTemp1634=([&]()->JSValue{auto nativeStatementTemp1640=([&]()->JSValue{const auto nativeStatementTemp1642=nativeStatementTemp1438;const auto nativeStatementTemp1643=JSValue("pivot");return nativeStatementTemp1642.get(nativeStatementTemp1643);}());const auto nativeStatementTemp1641=JSValue("toArray");const auto nativeStatementTemp1637=nativeStatementTemp1640.get(nativeStatementTemp1641);const auto nativeStatementTemp1638=nativeStatementTemp1640;Args nativeStatementTemp1639;return nativeStatementTemp1637.call(nativeStatementTemp1638,nativeStatementTemp1639);}());setReference(nativeStatementTemp1632,nativeStatementTemp1633,nativeStatementTemp1634);return nativeStatementTemp1634;}());
if(([&]()->JSValue{const auto nativeStatementTemp1644=([&]()->JSValue{const auto nativeStatementTemp1646=nativeStatementTemp1438;const auto nativeStatementTemp1647=JSValue("morphTargetDictionary");return nativeStatementTemp1646.get(nativeStatementTemp1647);}());const auto nativeStatementTemp1645=JSValue{};return binary("!==",nativeStatementTemp1644,nativeStatementTemp1645);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp1648=object;const auto nativeStatementTemp1649=JSValue("morphTargetDictionary");const auto nativeStatementTemp1650=([&]()->JSValue{auto nativeStatementTemp1656=nativeGlobalBinding("Object");const auto nativeStatementTemp1657=JSValue("assign");const auto nativeStatementTemp1653=nativeStatementTemp1656.get(nativeStatementTemp1657);const auto nativeStatementTemp1654=nativeStatementTemp1656;Args nativeStatementTemp1655;nativeStatementTemp1655.push_back(([&]()->JSValue{auto nativeStatementTemp1658=JSValue::object();return nativeStatementTemp1658;}()));nativeStatementTemp1655.push_back(([&]()->JSValue{const auto nativeStatementTemp1659=nativeStatementTemp1438;const auto nativeStatementTemp1660=JSValue("morphTargetDictionary");return nativeStatementTemp1659.get(nativeStatementTemp1660);}()));return nativeStatementTemp1653.call(nativeStatementTemp1654,nativeStatementTemp1655);}());setReference(nativeStatementTemp1648,nativeStatementTemp1649,nativeStatementTemp1650);return nativeStatementTemp1650;}());
if(([&]()->JSValue{const auto nativeStatementTemp1661=([&]()->JSValue{const auto nativeStatementTemp1663=nativeStatementTemp1438;const auto nativeStatementTemp1664=JSValue("morphTargetInfluences");return nativeStatementTemp1663.get(nativeStatementTemp1664);}());const auto nativeStatementTemp1662=JSValue{};return binary("!==",nativeStatementTemp1661,nativeStatementTemp1662);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp1665=object;const auto nativeStatementTemp1666=JSValue("morphTargetInfluences");const auto nativeStatementTemp1667=([&]()->JSValue{auto nativeStatementTemp1673=([&]()->JSValue{const auto nativeStatementTemp1675=nativeStatementTemp1438;const auto nativeStatementTemp1676=JSValue("morphTargetInfluences");return nativeStatementTemp1675.get(nativeStatementTemp1676);}());const auto nativeStatementTemp1674=JSValue("slice");const auto nativeStatementTemp1670=nativeStatementTemp1673.get(nativeStatementTemp1674);const auto nativeStatementTemp1671=nativeStatementTemp1673;Args nativeStatementTemp1672;return nativeStatementTemp1670.call(nativeStatementTemp1671,nativeStatementTemp1672);}());setReference(nativeStatementTemp1665,nativeStatementTemp1666,nativeStatementTemp1667);return nativeStatementTemp1667;}());
if(([&]()->JSValue{const auto nativeStatementTemp1677=nativeStatementTemp1438;const auto nativeStatementTemp1678=JSValue("isInstancedMesh");return nativeStatementTemp1677.get(nativeStatementTemp1678);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp1679=object;const auto nativeStatementTemp1680=JSValue("type");const auto nativeStatementTemp1681=JSValue("InstancedMesh");setReference(nativeStatementTemp1679,nativeStatementTemp1680,nativeStatementTemp1681);return nativeStatementTemp1681;}());
([&]()->JSValue{auto nativeStatementTemp1684=object;const auto nativeStatementTemp1685=JSValue("count");const auto nativeStatementTemp1686=([&]()->JSValue{const auto nativeStatementTemp1689=nativeStatementTemp1438;const auto nativeStatementTemp1690=JSValue("count");return nativeStatementTemp1689.get(nativeStatementTemp1690);}());setReference(nativeStatementTemp1684,nativeStatementTemp1685,nativeStatementTemp1686);return nativeStatementTemp1686;}());
([&]()->JSValue{auto nativeStatementTemp1691=object;const auto nativeStatementTemp1692=JSValue("instanceMatrix");const auto nativeStatementTemp1693=([&]()->JSValue{auto nativeStatementTemp1699=([&]()->JSValue{const auto nativeStatementTemp1701=nativeStatementTemp1438;const auto nativeStatementTemp1702=JSValue("instanceMatrix");return nativeStatementTemp1701.get(nativeStatementTemp1702);}());const auto nativeStatementTemp1700=JSValue("toJSON");const auto nativeStatementTemp1696=nativeStatementTemp1699.get(nativeStatementTemp1700);const auto nativeStatementTemp1697=nativeStatementTemp1699;Args nativeStatementTemp1698;return nativeStatementTemp1696.call(nativeStatementTemp1697,nativeStatementTemp1698);}());setReference(nativeStatementTemp1691,nativeStatementTemp1692,nativeStatementTemp1693);return nativeStatementTemp1693;}());
if(([&]()->JSValue{const auto nativeStatementTemp1703=([&]()->JSValue{const auto nativeStatementTemp1705=nativeStatementTemp1438;const auto nativeStatementTemp1706=JSValue("instanceColor");return nativeStatementTemp1705.get(nativeStatementTemp1706);}());const auto nativeStatementTemp1704=JSValue(nullptr);return binary("!==",nativeStatementTemp1703,nativeStatementTemp1704);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp1707=object;const auto nativeStatementTemp1708=JSValue("instanceColor");const auto nativeStatementTemp1709=([&]()->JSValue{auto nativeStatementTemp1715=([&]()->JSValue{const auto nativeStatementTemp1717=nativeStatementTemp1438;const auto nativeStatementTemp1718=JSValue("instanceColor");return nativeStatementTemp1717.get(nativeStatementTemp1718);}());const auto nativeStatementTemp1716=JSValue("toJSON");const auto nativeStatementTemp1712=nativeStatementTemp1715.get(nativeStatementTemp1716);const auto nativeStatementTemp1713=nativeStatementTemp1715;Args nativeStatementTemp1714;return nativeStatementTemp1712.call(nativeStatementTemp1713,nativeStatementTemp1714);}());setReference(nativeStatementTemp1707,nativeStatementTemp1708,nativeStatementTemp1709);return nativeStatementTemp1709;}());

}
if(([&]()->JSValue{const auto nativeStatementTemp1719=nativeStatementTemp1438;const auto nativeStatementTemp1720=JSValue("isBatchedMesh");return nativeStatementTemp1719.get(nativeStatementTemp1720);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp1721=object;const auto nativeStatementTemp1722=JSValue("type");const auto nativeStatementTemp1723=JSValue("BatchedMesh");setReference(nativeStatementTemp1721,nativeStatementTemp1722,nativeStatementTemp1723);return nativeStatementTemp1723;}());
([&]()->JSValue{auto nativeStatementTemp1726=object;const auto nativeStatementTemp1727=JSValue("perObjectFrustumCulled");const auto nativeStatementTemp1728=([&]()->JSValue{const auto nativeStatementTemp1731=nativeStatementTemp1438;const auto nativeStatementTemp1732=JSValue("perObjectFrustumCulled");return nativeStatementTemp1731.get(nativeStatementTemp1732);}());setReference(nativeStatementTemp1726,nativeStatementTemp1727,nativeStatementTemp1728);return nativeStatementTemp1728;}());
([&]()->JSValue{auto nativeStatementTemp1733=object;const auto nativeStatementTemp1734=JSValue("sortObjects");const auto nativeStatementTemp1735=([&]()->JSValue{const auto nativeStatementTemp1738=nativeStatementTemp1438;const auto nativeStatementTemp1739=JSValue("sortObjects");return nativeStatementTemp1738.get(nativeStatementTemp1739);}());setReference(nativeStatementTemp1733,nativeStatementTemp1734,nativeStatementTemp1735);return nativeStatementTemp1735;}());
([&]()->JSValue{auto nativeStatementTemp1740=object;const auto nativeStatementTemp1741=JSValue("drawRanges");const auto nativeStatementTemp1742=([&]()->JSValue{const auto nativeStatementTemp1745=nativeStatementTemp1438;const auto nativeStatementTemp1746=JSValue("_drawRanges");return nativeStatementTemp1745.get(nativeStatementTemp1746);}());setReference(nativeStatementTemp1740,nativeStatementTemp1741,nativeStatementTemp1742);return nativeStatementTemp1742;}());
([&]()->JSValue{auto nativeStatementTemp1747=object;const auto nativeStatementTemp1748=JSValue("reservedRanges");const auto nativeStatementTemp1749=([&]()->JSValue{const auto nativeStatementTemp1752=nativeStatementTemp1438;const auto nativeStatementTemp1753=JSValue("_reservedRanges");return nativeStatementTemp1752.get(nativeStatementTemp1753);}());setReference(nativeStatementTemp1747,nativeStatementTemp1748,nativeStatementTemp1749);return nativeStatementTemp1749;}());
([&]()->JSValue{auto nativeStatementTemp1754=object;const auto nativeStatementTemp1755=JSValue("geometryInfo");const auto nativeStatementTemp1756=([&]()->JSValue{auto nativeStatementTemp1762=([&]()->JSValue{const auto nativeStatementTemp1764=nativeStatementTemp1438;const auto nativeStatementTemp1765=JSValue("_geometryInfo");return nativeStatementTemp1764.get(nativeStatementTemp1765);}());const auto nativeStatementTemp1763=JSValue("map");const auto nativeStatementTemp1759=nativeStatementTemp1762.get(nativeStatementTemp1763);const auto nativeStatementTemp1760=nativeStatementTemp1762;Args nativeStatementTemp1761;nativeStatementTemp1761.push_back(([&]()->JSValue{auto nativeStatementTemp1795=JSValue::function(nested3,{nativeStatementTemp1437},"",1);nativeStatementTemp1795.setFunctionSourceText("info => ( {\r\n\t\t\t\t...info,\r\n\t\t\t\tboundingBox: info.boundingBox ? info.boundingBox.toJSON() : undefined,\r\n\t\t\t\tboundingSphere: info.boundingSphere ? info.boundingSphere.toJSON() : undefined\r\n\t\t\t} )");return nativeStatementTemp1795;}()));return nativeStatementTemp1759.call(nativeStatementTemp1760,nativeStatementTemp1761);}());setReference(nativeStatementTemp1754,nativeStatementTemp1755,nativeStatementTemp1756);return nativeStatementTemp1756;}());
([&]()->JSValue{auto nativeStatementTemp1796=object;const auto nativeStatementTemp1797=JSValue("instanceInfo");const auto nativeStatementTemp1798=([&]()->JSValue{auto nativeStatementTemp1804=([&]()->JSValue{const auto nativeStatementTemp1806=nativeStatementTemp1438;const auto nativeStatementTemp1807=JSValue("_instanceInfo");return nativeStatementTemp1806.get(nativeStatementTemp1807);}());const auto nativeStatementTemp1805=JSValue("map");const auto nativeStatementTemp1801=nativeStatementTemp1804.get(nativeStatementTemp1805);const auto nativeStatementTemp1802=nativeStatementTemp1804;Args nativeStatementTemp1803;nativeStatementTemp1803.push_back(([&]()->JSValue{auto nativeStatementTemp1815=JSValue::function(nested4,{nativeStatementTemp1437},"",1);nativeStatementTemp1815.setFunctionSourceText("info => ( { ...info } )");return nativeStatementTemp1815;}()));return nativeStatementTemp1801.call(nativeStatementTemp1802,nativeStatementTemp1803);}());setReference(nativeStatementTemp1796,nativeStatementTemp1797,nativeStatementTemp1798);return nativeStatementTemp1798;}());
([&]()->JSValue{auto nativeStatementTemp1816=object;const auto nativeStatementTemp1817=JSValue("availableInstanceIds");const auto nativeStatementTemp1818=([&]()->JSValue{auto nativeStatementTemp1824=([&]()->JSValue{const auto nativeStatementTemp1826=nativeStatementTemp1438;const auto nativeStatementTemp1827=JSValue("_availableInstanceIds");return nativeStatementTemp1826.get(nativeStatementTemp1827);}());const auto nativeStatementTemp1825=JSValue("slice");const auto nativeStatementTemp1821=nativeStatementTemp1824.get(nativeStatementTemp1825);const auto nativeStatementTemp1822=nativeStatementTemp1824;Args nativeStatementTemp1823;return nativeStatementTemp1821.call(nativeStatementTemp1822,nativeStatementTemp1823);}());setReference(nativeStatementTemp1816,nativeStatementTemp1817,nativeStatementTemp1818);return nativeStatementTemp1818;}());
([&]()->JSValue{auto nativeStatementTemp1828=object;const auto nativeStatementTemp1829=JSValue("availableGeometryIds");const auto nativeStatementTemp1830=([&]()->JSValue{auto nativeStatementTemp1836=([&]()->JSValue{const auto nativeStatementTemp1838=nativeStatementTemp1438;const auto nativeStatementTemp1839=JSValue("_availableGeometryIds");return nativeStatementTemp1838.get(nativeStatementTemp1839);}());const auto nativeStatementTemp1837=JSValue("slice");const auto nativeStatementTemp1833=nativeStatementTemp1836.get(nativeStatementTemp1837);const auto nativeStatementTemp1834=nativeStatementTemp1836;Args nativeStatementTemp1835;return nativeStatementTemp1833.call(nativeStatementTemp1834,nativeStatementTemp1835);}());setReference(nativeStatementTemp1828,nativeStatementTemp1829,nativeStatementTemp1830);return nativeStatementTemp1830;}());
([&]()->JSValue{auto nativeStatementTemp1840=object;const auto nativeStatementTemp1841=JSValue("nextIndexStart");const auto nativeStatementTemp1842=([&]()->JSValue{const auto nativeStatementTemp1845=nativeStatementTemp1438;const auto nativeStatementTemp1846=JSValue("_nextIndexStart");return nativeStatementTemp1845.get(nativeStatementTemp1846);}());setReference(nativeStatementTemp1840,nativeStatementTemp1841,nativeStatementTemp1842);return nativeStatementTemp1842;}());
([&]()->JSValue{auto nativeStatementTemp1847=object;const auto nativeStatementTemp1848=JSValue("nextVertexStart");const auto nativeStatementTemp1849=([&]()->JSValue{const auto nativeStatementTemp1852=nativeStatementTemp1438;const auto nativeStatementTemp1853=JSValue("_nextVertexStart");return nativeStatementTemp1852.get(nativeStatementTemp1853);}());setReference(nativeStatementTemp1847,nativeStatementTemp1848,nativeStatementTemp1849);return nativeStatementTemp1849;}());
([&]()->JSValue{auto nativeStatementTemp1854=object;const auto nativeStatementTemp1855=JSValue("geometryCount");const auto nativeStatementTemp1856=([&]()->JSValue{const auto nativeStatementTemp1859=nativeStatementTemp1438;const auto nativeStatementTemp1860=JSValue("_geometryCount");return nativeStatementTemp1859.get(nativeStatementTemp1860);}());setReference(nativeStatementTemp1854,nativeStatementTemp1855,nativeStatementTemp1856);return nativeStatementTemp1856;}());
([&]()->JSValue{auto nativeStatementTemp1861=object;const auto nativeStatementTemp1862=JSValue("maxInstanceCount");const auto nativeStatementTemp1863=([&]()->JSValue{const auto nativeStatementTemp1866=nativeStatementTemp1438;const auto nativeStatementTemp1867=JSValue("_maxInstanceCount");return nativeStatementTemp1866.get(nativeStatementTemp1867);}());setReference(nativeStatementTemp1861,nativeStatementTemp1862,nativeStatementTemp1863);return nativeStatementTemp1863;}());
([&]()->JSValue{auto nativeStatementTemp1868=object;const auto nativeStatementTemp1869=JSValue("maxVertexCount");const auto nativeStatementTemp1870=([&]()->JSValue{const auto nativeStatementTemp1873=nativeStatementTemp1438;const auto nativeStatementTemp1874=JSValue("_maxVertexCount");return nativeStatementTemp1873.get(nativeStatementTemp1874);}());setReference(nativeStatementTemp1868,nativeStatementTemp1869,nativeStatementTemp1870);return nativeStatementTemp1870;}());
([&]()->JSValue{auto nativeStatementTemp1875=object;const auto nativeStatementTemp1876=JSValue("maxIndexCount");const auto nativeStatementTemp1877=([&]()->JSValue{const auto nativeStatementTemp1880=nativeStatementTemp1438;const auto nativeStatementTemp1881=JSValue("_maxIndexCount");return nativeStatementTemp1880.get(nativeStatementTemp1881);}());setReference(nativeStatementTemp1875,nativeStatementTemp1876,nativeStatementTemp1877);return nativeStatementTemp1877;}());
([&]()->JSValue{auto nativeStatementTemp1882=object;const auto nativeStatementTemp1883=JSValue("geometryInitialized");const auto nativeStatementTemp1884=([&]()->JSValue{const auto nativeStatementTemp1887=nativeStatementTemp1438;const auto nativeStatementTemp1888=JSValue("_geometryInitialized");return nativeStatementTemp1887.get(nativeStatementTemp1888);}());setReference(nativeStatementTemp1882,nativeStatementTemp1883,nativeStatementTemp1884);return nativeStatementTemp1884;}());
([&]()->JSValue{auto nativeStatementTemp1889=object;const auto nativeStatementTemp1890=JSValue("matricesTexture");const auto nativeStatementTemp1891=([&]()->JSValue{auto nativeStatementTemp1897=([&]()->JSValue{const auto nativeStatementTemp1899=nativeStatementTemp1438;const auto nativeStatementTemp1900=JSValue("_matricesTexture");return nativeStatementTemp1899.get(nativeStatementTemp1900);}());const auto nativeStatementTemp1898=JSValue("toJSON");const auto nativeStatementTemp1894=nativeStatementTemp1897.get(nativeStatementTemp1898);const auto nativeStatementTemp1895=nativeStatementTemp1897;Args nativeStatementTemp1896;nativeStatementTemp1896.push_back(meta);return nativeStatementTemp1894.call(nativeStatementTemp1895,nativeStatementTemp1896);}());setReference(nativeStatementTemp1889,nativeStatementTemp1890,nativeStatementTemp1891);return nativeStatementTemp1891;}());
([&]()->JSValue{auto nativeStatementTemp1901=object;const auto nativeStatementTemp1902=JSValue("indirectTexture");const auto nativeStatementTemp1903=([&]()->JSValue{auto nativeStatementTemp1909=([&]()->JSValue{const auto nativeStatementTemp1911=nativeStatementTemp1438;const auto nativeStatementTemp1912=JSValue("_indirectTexture");return nativeStatementTemp1911.get(nativeStatementTemp1912);}());const auto nativeStatementTemp1910=JSValue("toJSON");const auto nativeStatementTemp1906=nativeStatementTemp1909.get(nativeStatementTemp1910);const auto nativeStatementTemp1907=nativeStatementTemp1909;Args nativeStatementTemp1908;nativeStatementTemp1908.push_back(meta);return nativeStatementTemp1906.call(nativeStatementTemp1907,nativeStatementTemp1908);}());setReference(nativeStatementTemp1901,nativeStatementTemp1902,nativeStatementTemp1903);return nativeStatementTemp1903;}());
if(([&]()->JSValue{const auto nativeStatementTemp1913=([&]()->JSValue{const auto nativeStatementTemp1915=nativeStatementTemp1438;const auto nativeStatementTemp1916=JSValue("_colorsTexture");return nativeStatementTemp1915.get(nativeStatementTemp1916);}());const auto nativeStatementTemp1914=JSValue(nullptr);return binary("!==",nativeStatementTemp1913,nativeStatementTemp1914);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp1917=object;const auto nativeStatementTemp1918=JSValue("colorsTexture");const auto nativeStatementTemp1919=([&]()->JSValue{auto nativeStatementTemp1925=([&]()->JSValue{const auto nativeStatementTemp1927=nativeStatementTemp1438;const auto nativeStatementTemp1928=JSValue("_colorsTexture");return nativeStatementTemp1927.get(nativeStatementTemp1928);}());const auto nativeStatementTemp1926=JSValue("toJSON");const auto nativeStatementTemp1922=nativeStatementTemp1925.get(nativeStatementTemp1926);const auto nativeStatementTemp1923=nativeStatementTemp1925;Args nativeStatementTemp1924;nativeStatementTemp1924.push_back(meta);return nativeStatementTemp1922.call(nativeStatementTemp1923,nativeStatementTemp1924);}());setReference(nativeStatementTemp1917,nativeStatementTemp1918,nativeStatementTemp1919);return nativeStatementTemp1919;}());

}
if(([&]()->JSValue{const auto nativeStatementTemp1929=([&]()->JSValue{const auto nativeStatementTemp1931=nativeStatementTemp1438;const auto nativeStatementTemp1932=JSValue("boundingSphere");return nativeStatementTemp1931.get(nativeStatementTemp1932);}());const auto nativeStatementTemp1930=JSValue(nullptr);return binary("!==",nativeStatementTemp1929,nativeStatementTemp1930);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp1933=object;const auto nativeStatementTemp1934=JSValue("boundingSphere");const auto nativeStatementTemp1935=([&]()->JSValue{auto nativeStatementTemp1941=([&]()->JSValue{const auto nativeStatementTemp1943=nativeStatementTemp1438;const auto nativeStatementTemp1944=JSValue("boundingSphere");return nativeStatementTemp1943.get(nativeStatementTemp1944);}());const auto nativeStatementTemp1942=JSValue("toJSON");const auto nativeStatementTemp1938=nativeStatementTemp1941.get(nativeStatementTemp1942);const auto nativeStatementTemp1939=nativeStatementTemp1941;Args nativeStatementTemp1940;return nativeStatementTemp1938.call(nativeStatementTemp1939,nativeStatementTemp1940);}());setReference(nativeStatementTemp1933,nativeStatementTemp1934,nativeStatementTemp1935);return nativeStatementTemp1935;}());

}
if(([&]()->JSValue{const auto nativeStatementTemp1945=([&]()->JSValue{const auto nativeStatementTemp1947=nativeStatementTemp1438;const auto nativeStatementTemp1948=JSValue("boundingBox");return nativeStatementTemp1947.get(nativeStatementTemp1948);}());const auto nativeStatementTemp1946=JSValue(nullptr);return binary("!==",nativeStatementTemp1945,nativeStatementTemp1946);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp1949=object;const auto nativeStatementTemp1950=JSValue("boundingBox");const auto nativeStatementTemp1951=([&]()->JSValue{auto nativeStatementTemp1957=([&]()->JSValue{const auto nativeStatementTemp1959=nativeStatementTemp1438;const auto nativeStatementTemp1960=JSValue("boundingBox");return nativeStatementTemp1959.get(nativeStatementTemp1960);}());const auto nativeStatementTemp1958=JSValue("toJSON");const auto nativeStatementTemp1954=nativeStatementTemp1957.get(nativeStatementTemp1958);const auto nativeStatementTemp1955=nativeStatementTemp1957;Args nativeStatementTemp1956;return nativeStatementTemp1954.call(nativeStatementTemp1955,nativeStatementTemp1956);}());setReference(nativeStatementTemp1949,nativeStatementTemp1950,nativeStatementTemp1951);return nativeStatementTemp1951;}());

}

}
JSValue serialize=([&]()->JSValue{auto nativeStatementTemp1987=JSValue::constructor(nested5,{nativeStatementTemp1437,meta},"serialize",2,JSValue::ConstructorKind::Callable);nativeStatementTemp1987.setFunctionSourceText("function serialize( library, element ) {\r\n\r\n\t\t\tif ( library[ element.uuid ] === undefined ) {\r\n\r\n\t\t\t\tlibrary[ element.uuid ] = element.toJSON( meta );\r\n\r\n\t\t\t}\r\n\r\n\t\t\treturn element.uuid;\r\n\r\n\t\t}");return nativeStatementTemp1987;}());
if(([&]()->JSValue{const auto nativeStatementTemp1988=nativeStatementTemp1438;const auto nativeStatementTemp1989=JSValue("isScene");return nativeStatementTemp1988.get(nativeStatementTemp1989);}()).toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp1990=nativeStatementTemp1438;const auto nativeStatementTemp1991=JSValue("background");return nativeStatementTemp1990.get(nativeStatementTemp1991);}()).toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp1992=([&]()->JSValue{const auto nativeStatementTemp1994=nativeStatementTemp1438;const auto nativeStatementTemp1995=JSValue("background");return nativeStatementTemp1994.get(nativeStatementTemp1995);}());const auto nativeStatementTemp1993=JSValue("isColor");return nativeStatementTemp1992.get(nativeStatementTemp1993);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp1996=object;const auto nativeStatementTemp1997=JSValue("background");const auto nativeStatementTemp1998=([&]()->JSValue{auto nativeStatementTemp2004=([&]()->JSValue{const auto nativeStatementTemp2006=nativeStatementTemp1438;const auto nativeStatementTemp2007=JSValue("background");return nativeStatementTemp2006.get(nativeStatementTemp2007);}());const auto nativeStatementTemp2005=JSValue("toJSON");const auto nativeStatementTemp2001=nativeStatementTemp2004.get(nativeStatementTemp2005);const auto nativeStatementTemp2002=nativeStatementTemp2004;Args nativeStatementTemp2003;return nativeStatementTemp2001.call(nativeStatementTemp2002,nativeStatementTemp2003);}());setReference(nativeStatementTemp1996,nativeStatementTemp1997,nativeStatementTemp1998);return nativeStatementTemp1998;}());

}
else if(([&]()->JSValue{const auto nativeStatementTemp2008=([&]()->JSValue{const auto nativeStatementTemp2010=nativeStatementTemp1438;const auto nativeStatementTemp2011=JSValue("background");return nativeStatementTemp2010.get(nativeStatementTemp2011);}());const auto nativeStatementTemp2009=JSValue("isTexture");return nativeStatementTemp2008.get(nativeStatementTemp2009);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp2012=object;const auto nativeStatementTemp2013=JSValue("background");const auto nativeStatementTemp2014=([&]()->JSValue{const auto nativeStatementTemp2017=([&]()->JSValue{auto nativeStatementTemp2022=([&]()->JSValue{const auto nativeStatementTemp2024=nativeStatementTemp1438;const auto nativeStatementTemp2025=JSValue("background");return nativeStatementTemp2024.get(nativeStatementTemp2025);}());const auto nativeStatementTemp2023=JSValue("toJSON");const auto nativeStatementTemp2019=nativeStatementTemp2022.get(nativeStatementTemp2023);const auto nativeStatementTemp2020=nativeStatementTemp2022;Args nativeStatementTemp2021;nativeStatementTemp2021.push_back(meta);return nativeStatementTemp2019.call(nativeStatementTemp2020,nativeStatementTemp2021);}());const auto nativeStatementTemp2018=JSValue("uuid");return nativeStatementTemp2017.get(nativeStatementTemp2018);}());setReference(nativeStatementTemp2012,nativeStatementTemp2013,nativeStatementTemp2014);return nativeStatementTemp2014;}());

}

}
if(([&]()->JSValue{const auto nativeStatementTemp2026=([&]()->JSValue{const auto nativeStatementTemp2027=([&]()->JSValue{const auto nativeStatementTemp2028=nativeStatementTemp1438;const auto nativeStatementTemp2029=JSValue("environment");return nativeStatementTemp2028.get(nativeStatementTemp2029);}());if(!nativeStatementTemp2027.toBoolean())return nativeStatementTemp2027;return ([&]()->JSValue{const auto nativeStatementTemp2030=([&]()->JSValue{const auto nativeStatementTemp2032=nativeStatementTemp1438;const auto nativeStatementTemp2033=JSValue("environment");return nativeStatementTemp2032.get(nativeStatementTemp2033);}());const auto nativeStatementTemp2031=JSValue("isTexture");return nativeStatementTemp2030.get(nativeStatementTemp2031);}());}());if(!nativeStatementTemp2026.toBoolean())return nativeStatementTemp2026;return ([&]()->JSValue{const auto nativeStatementTemp2034=([&]()->JSValue{const auto nativeStatementTemp2036=([&]()->JSValue{const auto nativeStatementTemp2038=nativeStatementTemp1438;const auto nativeStatementTemp2039=JSValue("environment");return nativeStatementTemp2038.get(nativeStatementTemp2039);}());const auto nativeStatementTemp2037=JSValue("isRenderTargetTexture");return nativeStatementTemp2036.get(nativeStatementTemp2037);}());const auto nativeStatementTemp2035=JSValue(true);return binary("!==",nativeStatementTemp2034,nativeStatementTemp2035);}());}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp2040=object;const auto nativeStatementTemp2041=JSValue("environment");const auto nativeStatementTemp2042=([&]()->JSValue{const auto nativeStatementTemp2045=([&]()->JSValue{auto nativeStatementTemp2050=([&]()->JSValue{const auto nativeStatementTemp2052=nativeStatementTemp1438;const auto nativeStatementTemp2053=JSValue("environment");return nativeStatementTemp2052.get(nativeStatementTemp2053);}());const auto nativeStatementTemp2051=JSValue("toJSON");const auto nativeStatementTemp2047=nativeStatementTemp2050.get(nativeStatementTemp2051);const auto nativeStatementTemp2048=nativeStatementTemp2050;Args nativeStatementTemp2049;nativeStatementTemp2049.push_back(meta);return nativeStatementTemp2047.call(nativeStatementTemp2048,nativeStatementTemp2049);}());const auto nativeStatementTemp2046=JSValue("uuid");return nativeStatementTemp2045.get(nativeStatementTemp2046);}());setReference(nativeStatementTemp2040,nativeStatementTemp2041,nativeStatementTemp2042);return nativeStatementTemp2042;}());

}

}
else if(([&]()->JSValue{const auto nativeStatementTemp2054=([&]()->JSValue{const auto nativeStatementTemp2055=([&]()->JSValue{const auto nativeStatementTemp2056=nativeStatementTemp1438;const auto nativeStatementTemp2057=JSValue("isMesh");return nativeStatementTemp2056.get(nativeStatementTemp2057);}());if(nativeStatementTemp2055.toBoolean())return nativeStatementTemp2055;return ([&]()->JSValue{const auto nativeStatementTemp2058=nativeStatementTemp1438;const auto nativeStatementTemp2059=JSValue("isLine");return nativeStatementTemp2058.get(nativeStatementTemp2059);}());}());if(nativeStatementTemp2054.toBoolean())return nativeStatementTemp2054;return ([&]()->JSValue{const auto nativeStatementTemp2060=nativeStatementTemp1438;const auto nativeStatementTemp2061=JSValue("isPoints");return nativeStatementTemp2060.get(nativeStatementTemp2061);}());}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp2062=object;const auto nativeStatementTemp2063=JSValue("geometry");const auto nativeStatementTemp2064=([&]()->JSValue{const auto nativeStatementTemp2067=serialize;const JSValue nativeStatementTemp2068;Args nativeStatementTemp2069;nativeStatementTemp2069.push_back(([&]()->JSValue{const auto nativeStatementTemp2070=meta;const auto nativeStatementTemp2071=JSValue("geometries");return nativeStatementTemp2070.get(nativeStatementTemp2071);}()));nativeStatementTemp2069.push_back(([&]()->JSValue{const auto nativeStatementTemp2072=nativeStatementTemp1438;const auto nativeStatementTemp2073=JSValue("geometry");return nativeStatementTemp2072.get(nativeStatementTemp2073);}()));return nativeStatementTemp2067.call(nativeStatementTemp2068,nativeStatementTemp2069);}());setReference(nativeStatementTemp2062,nativeStatementTemp2063,nativeStatementTemp2064);return nativeStatementTemp2064;}());
JSValue parameters=([&]()->JSValue{const auto nativeStatementTemp2074=([&]()->JSValue{const auto nativeStatementTemp2076=nativeStatementTemp1438;const auto nativeStatementTemp2077=JSValue("geometry");return nativeStatementTemp2076.get(nativeStatementTemp2077);}());const auto nativeStatementTemp2075=JSValue("parameters");return nativeStatementTemp2074.get(nativeStatementTemp2075);}());
if(([&]()->JSValue{const auto nativeStatementTemp2078=([&]()->JSValue{const auto nativeStatementTemp2079=parameters;const auto nativeStatementTemp2080=JSValue{};return binary("!==",nativeStatementTemp2079,nativeStatementTemp2080);}());if(!nativeStatementTemp2078.toBoolean())return nativeStatementTemp2078;return ([&]()->JSValue{const auto nativeStatementTemp2081=([&]()->JSValue{const auto nativeStatementTemp2083=parameters;const auto nativeStatementTemp2084=JSValue("shapes");return nativeStatementTemp2083.get(nativeStatementTemp2084);}());const auto nativeStatementTemp2082=JSValue{};return binary("!==",nativeStatementTemp2081,nativeStatementTemp2082);}());}()).toBoolean()){
JSValue shapes=([&]()->JSValue{const auto nativeStatementTemp2085=parameters;const auto nativeStatementTemp2086=JSValue("shapes");return nativeStatementTemp2085.get(nativeStatementTemp2086);}());
if(([&]()->JSValue{auto nativeStatementTemp2090=nativeGlobalBinding("Array");const auto nativeStatementTemp2091=JSValue("isArray");const auto nativeStatementTemp2087=nativeStatementTemp2090.get(nativeStatementTemp2091);const auto nativeStatementTemp2088=nativeStatementTemp2090;Args nativeStatementTemp2089;nativeStatementTemp2089.push_back(shapes);return nativeStatementTemp2087.call(nativeStatementTemp2088,nativeStatementTemp2089);}()).toBoolean()){
{JSValue i=JSValue(0);
JSValue l=([&]()->JSValue{const auto nativeStatementTemp2092=shapes;const auto nativeStatementTemp2093=JSValue("length");return nativeStatementTemp2092.get(nativeStatementTemp2093);}());
for(;([&]()->JSValue{const auto nativeStatementTemp2094=i;const auto nativeStatementTemp2095=l;return binary("<",nativeStatementTemp2094,nativeStatementTemp2095);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp2096=i.toNumber();const auto nativeStatementTemp2097=nativeStatementTemp2096+1;i=nativeStatementTemp2097;return nativeStatementTemp2096;}())){
JSValue shape=([&]()->JSValue{const auto nativeStatementTemp2098=shapes;const auto nativeStatementTemp2099=i;return nativeStatementTemp2098.get(nativeStatementTemp2099);}());
([&]()->JSValue{const auto nativeStatementTemp2100=serialize;const JSValue nativeStatementTemp2101;Args nativeStatementTemp2102;nativeStatementTemp2102.push_back(([&]()->JSValue{const auto nativeStatementTemp2103=meta;const auto nativeStatementTemp2104=JSValue("shapes");return nativeStatementTemp2103.get(nativeStatementTemp2104);}()));nativeStatementTemp2102.push_back(shape);return nativeStatementTemp2100.call(nativeStatementTemp2101,nativeStatementTemp2102);}());

}
}

}
else {
([&]()->JSValue{const auto nativeStatementTemp2105=serialize;const JSValue nativeStatementTemp2106;Args nativeStatementTemp2107;nativeStatementTemp2107.push_back(([&]()->JSValue{const auto nativeStatementTemp2108=meta;const auto nativeStatementTemp2109=JSValue("shapes");return nativeStatementTemp2108.get(nativeStatementTemp2109);}()));nativeStatementTemp2107.push_back(shapes);return nativeStatementTemp2105.call(nativeStatementTemp2106,nativeStatementTemp2107);}());

}

}

}
if(([&]()->JSValue{const auto nativeStatementTemp2110=nativeStatementTemp1438;const auto nativeStatementTemp2111=JSValue("isSkinnedMesh");return nativeStatementTemp2110.get(nativeStatementTemp2111);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp2112=object;const auto nativeStatementTemp2113=JSValue("bindMode");const auto nativeStatementTemp2114=([&]()->JSValue{const auto nativeStatementTemp2117=nativeStatementTemp1438;const auto nativeStatementTemp2118=JSValue("bindMode");return nativeStatementTemp2117.get(nativeStatementTemp2118);}());setReference(nativeStatementTemp2112,nativeStatementTemp2113,nativeStatementTemp2114);return nativeStatementTemp2114;}());
([&]()->JSValue{auto nativeStatementTemp2119=object;const auto nativeStatementTemp2120=JSValue("bindMatrix");const auto nativeStatementTemp2121=([&]()->JSValue{auto nativeStatementTemp2127=([&]()->JSValue{const auto nativeStatementTemp2129=nativeStatementTemp1438;const auto nativeStatementTemp2130=JSValue("bindMatrix");return nativeStatementTemp2129.get(nativeStatementTemp2130);}());const auto nativeStatementTemp2128=JSValue("toArray");const auto nativeStatementTemp2124=nativeStatementTemp2127.get(nativeStatementTemp2128);const auto nativeStatementTemp2125=nativeStatementTemp2127;Args nativeStatementTemp2126;return nativeStatementTemp2124.call(nativeStatementTemp2125,nativeStatementTemp2126);}());setReference(nativeStatementTemp2119,nativeStatementTemp2120,nativeStatementTemp2121);return nativeStatementTemp2121;}());
if(([&]()->JSValue{const auto nativeStatementTemp2131=([&]()->JSValue{const auto nativeStatementTemp2133=nativeStatementTemp1438;const auto nativeStatementTemp2134=JSValue("skeleton");return nativeStatementTemp2133.get(nativeStatementTemp2134);}());const auto nativeStatementTemp2132=JSValue{};return binary("!==",nativeStatementTemp2131,nativeStatementTemp2132);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp2135=serialize;const JSValue nativeStatementTemp2136;Args nativeStatementTemp2137;nativeStatementTemp2137.push_back(([&]()->JSValue{const auto nativeStatementTemp2138=meta;const auto nativeStatementTemp2139=JSValue("skeletons");return nativeStatementTemp2138.get(nativeStatementTemp2139);}()));nativeStatementTemp2137.push_back(([&]()->JSValue{const auto nativeStatementTemp2140=nativeStatementTemp1438;const auto nativeStatementTemp2141=JSValue("skeleton");return nativeStatementTemp2140.get(nativeStatementTemp2141);}()));return nativeStatementTemp2135.call(nativeStatementTemp2136,nativeStatementTemp2137);}());
([&]()->JSValue{auto nativeStatementTemp2142=object;const auto nativeStatementTemp2143=JSValue("skeleton");const auto nativeStatementTemp2144=([&]()->JSValue{const auto nativeStatementTemp2147=([&]()->JSValue{const auto nativeStatementTemp2149=nativeStatementTemp1438;const auto nativeStatementTemp2150=JSValue("skeleton");return nativeStatementTemp2149.get(nativeStatementTemp2150);}());const auto nativeStatementTemp2148=JSValue("uuid");return nativeStatementTemp2147.get(nativeStatementTemp2148);}());setReference(nativeStatementTemp2142,nativeStatementTemp2143,nativeStatementTemp2144);return nativeStatementTemp2144;}());

}

}
if(([&]()->JSValue{const auto nativeStatementTemp2151=([&]()->JSValue{const auto nativeStatementTemp2153=nativeStatementTemp1438;const auto nativeStatementTemp2154=JSValue("material");return nativeStatementTemp2153.get(nativeStatementTemp2154);}());const auto nativeStatementTemp2152=JSValue{};return binary("!==",nativeStatementTemp2151,nativeStatementTemp2152);}()).toBoolean()){
if(([&]()->JSValue{auto nativeStatementTemp2158=nativeGlobalBinding("Array");const auto nativeStatementTemp2159=JSValue("isArray");const auto nativeStatementTemp2155=nativeStatementTemp2158.get(nativeStatementTemp2159);const auto nativeStatementTemp2156=nativeStatementTemp2158;Args nativeStatementTemp2157;nativeStatementTemp2157.push_back(([&]()->JSValue{const auto nativeStatementTemp2160=nativeStatementTemp1438;const auto nativeStatementTemp2161=JSValue("material");return nativeStatementTemp2160.get(nativeStatementTemp2161);}()));return nativeStatementTemp2155.call(nativeStatementTemp2156,nativeStatementTemp2157);}()).toBoolean()){
JSValue uuids=([&]()->JSValue{auto nativeStatementTemp2162=JSValue::array();return nativeStatementTemp2162;}());
{JSValue i=JSValue(0);
JSValue l=([&]()->JSValue{const auto nativeStatementTemp2163=([&]()->JSValue{const auto nativeStatementTemp2165=nativeStatementTemp1438;const auto nativeStatementTemp2166=JSValue("material");return nativeStatementTemp2165.get(nativeStatementTemp2166);}());const auto nativeStatementTemp2164=JSValue("length");return nativeStatementTemp2163.get(nativeStatementTemp2164);}());
for(;([&]()->JSValue{const auto nativeStatementTemp2167=i;const auto nativeStatementTemp2168=l;return binary("<",nativeStatementTemp2167,nativeStatementTemp2168);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp2169=i.toNumber();const auto nativeStatementTemp2170=nativeStatementTemp2169+1;i=nativeStatementTemp2170;return nativeStatementTemp2169;}())){
([&]()->JSValue{auto nativeStatementTemp2174=uuids;const auto nativeStatementTemp2175=JSValue("push");const auto nativeStatementTemp2171=nativeStatementTemp2174.get(nativeStatementTemp2175);const auto nativeStatementTemp2172=nativeStatementTemp2174;Args nativeStatementTemp2173;nativeStatementTemp2173.push_back(([&]()->JSValue{const auto nativeStatementTemp2176=serialize;const JSValue nativeStatementTemp2177;Args nativeStatementTemp2178;nativeStatementTemp2178.push_back(([&]()->JSValue{const auto nativeStatementTemp2179=meta;const auto nativeStatementTemp2180=JSValue("materials");return nativeStatementTemp2179.get(nativeStatementTemp2180);}()));nativeStatementTemp2178.push_back(([&]()->JSValue{const auto nativeStatementTemp2181=([&]()->JSValue{const auto nativeStatementTemp2183=nativeStatementTemp1438;const auto nativeStatementTemp2184=JSValue("material");return nativeStatementTemp2183.get(nativeStatementTemp2184);}());const auto nativeStatementTemp2182=i;return nativeStatementTemp2181.get(nativeStatementTemp2182);}()));return nativeStatementTemp2176.call(nativeStatementTemp2177,nativeStatementTemp2178);}()));return nativeStatementTemp2171.call(nativeStatementTemp2172,nativeStatementTemp2173);}());

}
}
([&]()->JSValue{auto nativeStatementTemp2185=object;const auto nativeStatementTemp2186=JSValue("material");const auto nativeStatementTemp2187=uuids;setReference(nativeStatementTemp2185,nativeStatementTemp2186,nativeStatementTemp2187);return nativeStatementTemp2187;}());

}
else {
([&]()->JSValue{auto nativeStatementTemp2190=object;const auto nativeStatementTemp2191=JSValue("material");const auto nativeStatementTemp2192=([&]()->JSValue{const auto nativeStatementTemp2195=serialize;const JSValue nativeStatementTemp2196;Args nativeStatementTemp2197;nativeStatementTemp2197.push_back(([&]()->JSValue{const auto nativeStatementTemp2198=meta;const auto nativeStatementTemp2199=JSValue("materials");return nativeStatementTemp2198.get(nativeStatementTemp2199);}()));nativeStatementTemp2197.push_back(([&]()->JSValue{const auto nativeStatementTemp2200=nativeStatementTemp1438;const auto nativeStatementTemp2201=JSValue("material");return nativeStatementTemp2200.get(nativeStatementTemp2201);}()));return nativeStatementTemp2195.call(nativeStatementTemp2196,nativeStatementTemp2197);}());setReference(nativeStatementTemp2190,nativeStatementTemp2191,nativeStatementTemp2192);return nativeStatementTemp2192;}());

}

}
if(([&]()->JSValue{const auto nativeStatementTemp2202=([&]()->JSValue{const auto nativeStatementTemp2204=([&]()->JSValue{const auto nativeStatementTemp2206=nativeStatementTemp1438;const auto nativeStatementTemp2207=JSValue("children");return nativeStatementTemp2206.get(nativeStatementTemp2207);}());const auto nativeStatementTemp2205=JSValue("length");return nativeStatementTemp2204.get(nativeStatementTemp2205);}());const auto nativeStatementTemp2203=JSValue(0);return binary(">",nativeStatementTemp2202,nativeStatementTemp2203);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp2208=object;const auto nativeStatementTemp2209=JSValue("children");const auto nativeStatementTemp2210=([&]()->JSValue{auto nativeStatementTemp2213=JSValue::array();return nativeStatementTemp2213;}());setReference(nativeStatementTemp2208,nativeStatementTemp2209,nativeStatementTemp2210);return nativeStatementTemp2210;}());
{JSValue i=JSValue(0);
for(;([&]()->JSValue{const auto nativeStatementTemp2214=i;const auto nativeStatementTemp2215=([&]()->JSValue{const auto nativeStatementTemp2216=([&]()->JSValue{const auto nativeStatementTemp2218=nativeStatementTemp1438;const auto nativeStatementTemp2219=JSValue("children");return nativeStatementTemp2218.get(nativeStatementTemp2219);}());const auto nativeStatementTemp2217=JSValue("length");return nativeStatementTemp2216.get(nativeStatementTemp2217);}());return binary("<",nativeStatementTemp2214,nativeStatementTemp2215);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp2220=i.toNumber();const auto nativeStatementTemp2221=nativeStatementTemp2220+1;i=nativeStatementTemp2221;return nativeStatementTemp2220;}())){
([&]()->JSValue{auto nativeStatementTemp2225=([&]()->JSValue{const auto nativeStatementTemp2227=object;const auto nativeStatementTemp2228=JSValue("children");return nativeStatementTemp2227.get(nativeStatementTemp2228);}());const auto nativeStatementTemp2226=JSValue("push");const auto nativeStatementTemp2222=nativeStatementTemp2225.get(nativeStatementTemp2226);const auto nativeStatementTemp2223=nativeStatementTemp2225;Args nativeStatementTemp2224;nativeStatementTemp2224.push_back(([&]()->JSValue{const auto nativeStatementTemp2229=([&]()->JSValue{auto nativeStatementTemp2234=([&]()->JSValue{const auto nativeStatementTemp2236=([&]()->JSValue{const auto nativeStatementTemp2238=nativeStatementTemp1438;const auto nativeStatementTemp2239=JSValue("children");return nativeStatementTemp2238.get(nativeStatementTemp2239);}());const auto nativeStatementTemp2237=i;return nativeStatementTemp2236.get(nativeStatementTemp2237);}());const auto nativeStatementTemp2235=JSValue("toJSON");const auto nativeStatementTemp2231=nativeStatementTemp2234.get(nativeStatementTemp2235);const auto nativeStatementTemp2232=nativeStatementTemp2234;Args nativeStatementTemp2233;nativeStatementTemp2233.push_back(meta);return nativeStatementTemp2231.call(nativeStatementTemp2232,nativeStatementTemp2233);}());const auto nativeStatementTemp2230=JSValue("object");return nativeStatementTemp2229.get(nativeStatementTemp2230);}()));return nativeStatementTemp2222.call(nativeStatementTemp2223,nativeStatementTemp2224);}());

}
}

}
if(([&]()->JSValue{const auto nativeStatementTemp2240=([&]()->JSValue{const auto nativeStatementTemp2242=([&]()->JSValue{const auto nativeStatementTemp2244=nativeStatementTemp1438;const auto nativeStatementTemp2245=JSValue("animations");return nativeStatementTemp2244.get(nativeStatementTemp2245);}());const auto nativeStatementTemp2243=JSValue("length");return nativeStatementTemp2242.get(nativeStatementTemp2243);}());const auto nativeStatementTemp2241=JSValue(0);return binary(">",nativeStatementTemp2240,nativeStatementTemp2241);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp2246=object;const auto nativeStatementTemp2247=JSValue("animations");const auto nativeStatementTemp2248=([&]()->JSValue{auto nativeStatementTemp2251=JSValue::array();return nativeStatementTemp2251;}());setReference(nativeStatementTemp2246,nativeStatementTemp2247,nativeStatementTemp2248);return nativeStatementTemp2248;}());
{JSValue i=JSValue(0);
for(;([&]()->JSValue{const auto nativeStatementTemp2252=i;const auto nativeStatementTemp2253=([&]()->JSValue{const auto nativeStatementTemp2254=([&]()->JSValue{const auto nativeStatementTemp2256=nativeStatementTemp1438;const auto nativeStatementTemp2257=JSValue("animations");return nativeStatementTemp2256.get(nativeStatementTemp2257);}());const auto nativeStatementTemp2255=JSValue("length");return nativeStatementTemp2254.get(nativeStatementTemp2255);}());return binary("<",nativeStatementTemp2252,nativeStatementTemp2253);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp2258=i.toNumber();const auto nativeStatementTemp2259=nativeStatementTemp2258+1;i=nativeStatementTemp2259;return nativeStatementTemp2258;}())){
JSValue animation=([&]()->JSValue{const auto nativeStatementTemp2260=([&]()->JSValue{const auto nativeStatementTemp2262=nativeStatementTemp1438;const auto nativeStatementTemp2263=JSValue("animations");return nativeStatementTemp2262.get(nativeStatementTemp2263);}());const auto nativeStatementTemp2261=i;return nativeStatementTemp2260.get(nativeStatementTemp2261);}());
([&]()->JSValue{auto nativeStatementTemp2267=([&]()->JSValue{const auto nativeStatementTemp2269=object;const auto nativeStatementTemp2270=JSValue("animations");return nativeStatementTemp2269.get(nativeStatementTemp2270);}());const auto nativeStatementTemp2268=JSValue("push");const auto nativeStatementTemp2264=nativeStatementTemp2267.get(nativeStatementTemp2268);const auto nativeStatementTemp2265=nativeStatementTemp2267;Args nativeStatementTemp2266;nativeStatementTemp2266.push_back(([&]()->JSValue{const auto nativeStatementTemp2271=serialize;const JSValue nativeStatementTemp2272;Args nativeStatementTemp2273;nativeStatementTemp2273.push_back(([&]()->JSValue{const auto nativeStatementTemp2274=meta;const auto nativeStatementTemp2275=JSValue("animations");return nativeStatementTemp2274.get(nativeStatementTemp2275);}()));nativeStatementTemp2273.push_back(animation);return nativeStatementTemp2271.call(nativeStatementTemp2272,nativeStatementTemp2273);}()));return nativeStatementTemp2264.call(nativeStatementTemp2265,nativeStatementTemp2266);}());

}
}

}
if(isRootObject.toBoolean()){
JSValue geometries=([&]()->JSValue{const auto nativeStatementTemp2276=extractFromCache;const JSValue nativeStatementTemp2277;Args nativeStatementTemp2278;nativeStatementTemp2278.push_back(([&]()->JSValue{const auto nativeStatementTemp2279=meta;const auto nativeStatementTemp2280=JSValue("geometries");return nativeStatementTemp2279.get(nativeStatementTemp2280);}()));return nativeStatementTemp2276.call(nativeStatementTemp2277,nativeStatementTemp2278);}());
JSValue materials=([&]()->JSValue{const auto nativeStatementTemp2281=extractFromCache;const JSValue nativeStatementTemp2282;Args nativeStatementTemp2283;nativeStatementTemp2283.push_back(([&]()->JSValue{const auto nativeStatementTemp2284=meta;const auto nativeStatementTemp2285=JSValue("materials");return nativeStatementTemp2284.get(nativeStatementTemp2285);}()));return nativeStatementTemp2281.call(nativeStatementTemp2282,nativeStatementTemp2283);}());
JSValue textures=([&]()->JSValue{const auto nativeStatementTemp2286=extractFromCache;const JSValue nativeStatementTemp2287;Args nativeStatementTemp2288;nativeStatementTemp2288.push_back(([&]()->JSValue{const auto nativeStatementTemp2289=meta;const auto nativeStatementTemp2290=JSValue("textures");return nativeStatementTemp2289.get(nativeStatementTemp2290);}()));return nativeStatementTemp2286.call(nativeStatementTemp2287,nativeStatementTemp2288);}());
JSValue images=([&]()->JSValue{const auto nativeStatementTemp2291=extractFromCache;const JSValue nativeStatementTemp2292;Args nativeStatementTemp2293;nativeStatementTemp2293.push_back(([&]()->JSValue{const auto nativeStatementTemp2294=meta;const auto nativeStatementTemp2295=JSValue("images");return nativeStatementTemp2294.get(nativeStatementTemp2295);}()));return nativeStatementTemp2291.call(nativeStatementTemp2292,nativeStatementTemp2293);}());
JSValue shapes=([&]()->JSValue{const auto nativeStatementTemp2296=extractFromCache;const JSValue nativeStatementTemp2297;Args nativeStatementTemp2298;nativeStatementTemp2298.push_back(([&]()->JSValue{const auto nativeStatementTemp2299=meta;const auto nativeStatementTemp2300=JSValue("shapes");return nativeStatementTemp2299.get(nativeStatementTemp2300);}()));return nativeStatementTemp2296.call(nativeStatementTemp2297,nativeStatementTemp2298);}());
JSValue skeletons=([&]()->JSValue{const auto nativeStatementTemp2301=extractFromCache;const JSValue nativeStatementTemp2302;Args nativeStatementTemp2303;nativeStatementTemp2303.push_back(([&]()->JSValue{const auto nativeStatementTemp2304=meta;const auto nativeStatementTemp2305=JSValue("skeletons");return nativeStatementTemp2304.get(nativeStatementTemp2305);}()));return nativeStatementTemp2301.call(nativeStatementTemp2302,nativeStatementTemp2303);}());
JSValue animations=([&]()->JSValue{const auto nativeStatementTemp2306=extractFromCache;const JSValue nativeStatementTemp2307;Args nativeStatementTemp2308;nativeStatementTemp2308.push_back(([&]()->JSValue{const auto nativeStatementTemp2309=meta;const auto nativeStatementTemp2310=JSValue("animations");return nativeStatementTemp2309.get(nativeStatementTemp2310);}()));return nativeStatementTemp2306.call(nativeStatementTemp2307,nativeStatementTemp2308);}());
JSValue nodes=([&]()->JSValue{const auto nativeStatementTemp2311=extractFromCache;const JSValue nativeStatementTemp2312;Args nativeStatementTemp2313;nativeStatementTemp2313.push_back(([&]()->JSValue{const auto nativeStatementTemp2314=meta;const auto nativeStatementTemp2315=JSValue("nodes");return nativeStatementTemp2314.get(nativeStatementTemp2315);}()));return nativeStatementTemp2311.call(nativeStatementTemp2312,nativeStatementTemp2313);}());
if(([&]()->JSValue{const auto nativeStatementTemp2316=([&]()->JSValue{const auto nativeStatementTemp2318=geometries;const auto nativeStatementTemp2319=JSValue("length");return nativeStatementTemp2318.get(nativeStatementTemp2319);}());const auto nativeStatementTemp2317=JSValue(0);return binary(">",nativeStatementTemp2316,nativeStatementTemp2317);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp2320=output;const auto nativeStatementTemp2321=JSValue("geometries");const auto nativeStatementTemp2322=geometries;setReference(nativeStatementTemp2320,nativeStatementTemp2321,nativeStatementTemp2322);return nativeStatementTemp2322;}());
if(([&]()->JSValue{const auto nativeStatementTemp2325=([&]()->JSValue{const auto nativeStatementTemp2327=materials;const auto nativeStatementTemp2328=JSValue("length");return nativeStatementTemp2327.get(nativeStatementTemp2328);}());const auto nativeStatementTemp2326=JSValue(0);return binary(">",nativeStatementTemp2325,nativeStatementTemp2326);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp2329=output;const auto nativeStatementTemp2330=JSValue("materials");const auto nativeStatementTemp2331=materials;setReference(nativeStatementTemp2329,nativeStatementTemp2330,nativeStatementTemp2331);return nativeStatementTemp2331;}());
if(([&]()->JSValue{const auto nativeStatementTemp2334=([&]()->JSValue{const auto nativeStatementTemp2336=textures;const auto nativeStatementTemp2337=JSValue("length");return nativeStatementTemp2336.get(nativeStatementTemp2337);}());const auto nativeStatementTemp2335=JSValue(0);return binary(">",nativeStatementTemp2334,nativeStatementTemp2335);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp2338=output;const auto nativeStatementTemp2339=JSValue("textures");const auto nativeStatementTemp2340=textures;setReference(nativeStatementTemp2338,nativeStatementTemp2339,nativeStatementTemp2340);return nativeStatementTemp2340;}());
if(([&]()->JSValue{const auto nativeStatementTemp2343=([&]()->JSValue{const auto nativeStatementTemp2345=images;const auto nativeStatementTemp2346=JSValue("length");return nativeStatementTemp2345.get(nativeStatementTemp2346);}());const auto nativeStatementTemp2344=JSValue(0);return binary(">",nativeStatementTemp2343,nativeStatementTemp2344);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp2347=output;const auto nativeStatementTemp2348=JSValue("images");const auto nativeStatementTemp2349=images;setReference(nativeStatementTemp2347,nativeStatementTemp2348,nativeStatementTemp2349);return nativeStatementTemp2349;}());
if(([&]()->JSValue{const auto nativeStatementTemp2352=([&]()->JSValue{const auto nativeStatementTemp2354=shapes;const auto nativeStatementTemp2355=JSValue("length");return nativeStatementTemp2354.get(nativeStatementTemp2355);}());const auto nativeStatementTemp2353=JSValue(0);return binary(">",nativeStatementTemp2352,nativeStatementTemp2353);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp2356=output;const auto nativeStatementTemp2357=JSValue("shapes");const auto nativeStatementTemp2358=shapes;setReference(nativeStatementTemp2356,nativeStatementTemp2357,nativeStatementTemp2358);return nativeStatementTemp2358;}());
if(([&]()->JSValue{const auto nativeStatementTemp2361=([&]()->JSValue{const auto nativeStatementTemp2363=skeletons;const auto nativeStatementTemp2364=JSValue("length");return nativeStatementTemp2363.get(nativeStatementTemp2364);}());const auto nativeStatementTemp2362=JSValue(0);return binary(">",nativeStatementTemp2361,nativeStatementTemp2362);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp2365=output;const auto nativeStatementTemp2366=JSValue("skeletons");const auto nativeStatementTemp2367=skeletons;setReference(nativeStatementTemp2365,nativeStatementTemp2366,nativeStatementTemp2367);return nativeStatementTemp2367;}());
if(([&]()->JSValue{const auto nativeStatementTemp2370=([&]()->JSValue{const auto nativeStatementTemp2372=animations;const auto nativeStatementTemp2373=JSValue("length");return nativeStatementTemp2372.get(nativeStatementTemp2373);}());const auto nativeStatementTemp2371=JSValue(0);return binary(">",nativeStatementTemp2370,nativeStatementTemp2371);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp2374=output;const auto nativeStatementTemp2375=JSValue("animations");const auto nativeStatementTemp2376=animations;setReference(nativeStatementTemp2374,nativeStatementTemp2375,nativeStatementTemp2376);return nativeStatementTemp2376;}());
if(([&]()->JSValue{const auto nativeStatementTemp2379=([&]()->JSValue{const auto nativeStatementTemp2381=nodes;const auto nativeStatementTemp2382=JSValue("length");return nativeStatementTemp2381.get(nativeStatementTemp2382);}());const auto nativeStatementTemp2380=JSValue(0);return binary(">",nativeStatementTemp2379,nativeStatementTemp2380);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp2383=output;const auto nativeStatementTemp2384=JSValue("nodes");const auto nativeStatementTemp2385=nodes;setReference(nativeStatementTemp2383,nativeStatementTemp2384,nativeStatementTemp2385);return nativeStatementTemp2385;}());

}
([&]()->JSValue{auto nativeStatementTemp2388=output;const auto nativeStatementTemp2389=JSValue("object");const auto nativeStatementTemp2390=object;setReference(nativeStatementTemp2388,nativeStatementTemp2389,nativeStatementTemp2390);return nativeStatementTemp2390;}());
return output;
return JSValue{};
}

JSValue method45(const JSValue& nativeStatementTemp2393,const Args& nativeStatementTemp2394,const Args& nativeStatementTemp2395){
auto nativeStatementTemp2396=nativeStatementTemp2395[0];auto nativeStatementTemp2397=nativeStatementTemp2393;
JSValue recursive=arg(nativeStatementTemp2394,0);
return ([&]()->JSValue{auto nativeStatementTemp2402=([&]()->JSValue{const auto nativeStatementTemp2404=([&]()->JSValue{const auto nativeStatementTemp2406=nativeStatementTemp2397;const auto nativeStatementTemp2407=JSValue("constructor");return nativeStatementTemp2406.get(nativeStatementTemp2407);}());Args nativeStatementTemp2405;return nativeStatementTemp2404.construct(nativeStatementTemp2405);}());const auto nativeStatementTemp2403=JSValue("copy");const auto nativeStatementTemp2399=nativeStatementTemp2402.get(nativeStatementTemp2403);const auto nativeStatementTemp2400=nativeStatementTemp2402;Args nativeStatementTemp2401;nativeStatementTemp2401.push_back(nativeStatementTemp2397);nativeStatementTemp2401.push_back(recursive);return nativeStatementTemp2399.call(nativeStatementTemp2400,nativeStatementTemp2401);}());
return JSValue{};
}

JSValue method46(const JSValue& nativeStatementTemp2408,const Args& nativeStatementTemp2409,const Args& nativeStatementTemp2410){
auto nativeStatementTemp2411=nativeStatementTemp2410[0];auto nativeStatementTemp2412=nativeStatementTemp2408;
JSValue source=arg(nativeStatementTemp2409,0);
JSValue recursive=arg(nativeStatementTemp2409,1);
if(recursive.isUndefined())recursive=JSValue(true);
([&]()->JSValue{auto nativeStatementTemp2414=nativeStatementTemp2412;const auto nativeStatementTemp2415=JSValue("name");const auto nativeStatementTemp2416=([&]()->JSValue{const auto nativeStatementTemp2419=source;const auto nativeStatementTemp2420=JSValue("name");return nativeStatementTemp2419.get(nativeStatementTemp2420);}());setReference(nativeStatementTemp2414,nativeStatementTemp2415,nativeStatementTemp2416);return nativeStatementTemp2416;}());
([&]()->JSValue{auto nativeStatementTemp2424=([&]()->JSValue{const auto nativeStatementTemp2426=nativeStatementTemp2412;const auto nativeStatementTemp2427=JSValue("up");return nativeStatementTemp2426.get(nativeStatementTemp2427);}());const auto nativeStatementTemp2425=JSValue("copy");const auto nativeStatementTemp2421=nativeStatementTemp2424.get(nativeStatementTemp2425);const auto nativeStatementTemp2422=nativeStatementTemp2424;Args nativeStatementTemp2423;nativeStatementTemp2423.push_back(([&]()->JSValue{const auto nativeStatementTemp2428=source;const auto nativeStatementTemp2429=JSValue("up");return nativeStatementTemp2428.get(nativeStatementTemp2429);}()));return nativeStatementTemp2421.call(nativeStatementTemp2422,nativeStatementTemp2423);}());
([&]()->JSValue{auto nativeStatementTemp2433=([&]()->JSValue{const auto nativeStatementTemp2435=nativeStatementTemp2412;const auto nativeStatementTemp2436=JSValue("position");return nativeStatementTemp2435.get(nativeStatementTemp2436);}());const auto nativeStatementTemp2434=JSValue("copy");const auto nativeStatementTemp2430=nativeStatementTemp2433.get(nativeStatementTemp2434);const auto nativeStatementTemp2431=nativeStatementTemp2433;Args nativeStatementTemp2432;nativeStatementTemp2432.push_back(([&]()->JSValue{const auto nativeStatementTemp2437=source;const auto nativeStatementTemp2438=JSValue("position");return nativeStatementTemp2437.get(nativeStatementTemp2438);}()));return nativeStatementTemp2430.call(nativeStatementTemp2431,nativeStatementTemp2432);}());
([&]()->JSValue{auto nativeStatementTemp2439=([&]()->JSValue{const auto nativeStatementTemp2441=nativeStatementTemp2412;const auto nativeStatementTemp2442=JSValue("rotation");return nativeStatementTemp2441.get(nativeStatementTemp2442);}());const auto nativeStatementTemp2440=JSValue("order");const auto nativeStatementTemp2443=([&]()->JSValue{const auto nativeStatementTemp2446=([&]()->JSValue{const auto nativeStatementTemp2448=source;const auto nativeStatementTemp2449=JSValue("rotation");return nativeStatementTemp2448.get(nativeStatementTemp2449);}());const auto nativeStatementTemp2447=JSValue("order");return nativeStatementTemp2446.get(nativeStatementTemp2447);}());setReference(nativeStatementTemp2439,nativeStatementTemp2440,nativeStatementTemp2443);return nativeStatementTemp2443;}());
([&]()->JSValue{auto nativeStatementTemp2453=([&]()->JSValue{const auto nativeStatementTemp2455=nativeStatementTemp2412;const auto nativeStatementTemp2456=JSValue("quaternion");return nativeStatementTemp2455.get(nativeStatementTemp2456);}());const auto nativeStatementTemp2454=JSValue("copy");const auto nativeStatementTemp2450=nativeStatementTemp2453.get(nativeStatementTemp2454);const auto nativeStatementTemp2451=nativeStatementTemp2453;Args nativeStatementTemp2452;nativeStatementTemp2452.push_back(([&]()->JSValue{const auto nativeStatementTemp2457=source;const auto nativeStatementTemp2458=JSValue("quaternion");return nativeStatementTemp2457.get(nativeStatementTemp2458);}()));return nativeStatementTemp2450.call(nativeStatementTemp2451,nativeStatementTemp2452);}());
([&]()->JSValue{auto nativeStatementTemp2462=([&]()->JSValue{const auto nativeStatementTemp2464=nativeStatementTemp2412;const auto nativeStatementTemp2465=JSValue("scale");return nativeStatementTemp2464.get(nativeStatementTemp2465);}());const auto nativeStatementTemp2463=JSValue("copy");const auto nativeStatementTemp2459=nativeStatementTemp2462.get(nativeStatementTemp2463);const auto nativeStatementTemp2460=nativeStatementTemp2462;Args nativeStatementTemp2461;nativeStatementTemp2461.push_back(([&]()->JSValue{const auto nativeStatementTemp2466=source;const auto nativeStatementTemp2467=JSValue("scale");return nativeStatementTemp2466.get(nativeStatementTemp2467);}()));return nativeStatementTemp2459.call(nativeStatementTemp2460,nativeStatementTemp2461);}());
([&]()->JSValue{auto nativeStatementTemp2468=nativeStatementTemp2412;const auto nativeStatementTemp2469=JSValue("pivot");const auto nativeStatementTemp2470=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp2473=([&]()->JSValue{const auto nativeStatementTemp2475=source;const auto nativeStatementTemp2476=JSValue("pivot");return nativeStatementTemp2475.get(nativeStatementTemp2476);}());const auto nativeStatementTemp2474=JSValue(nullptr);return binary("!==",nativeStatementTemp2473,nativeStatementTemp2474);}()).toBoolean())return ([&]()->JSValue{auto nativeStatementTemp2480=([&]()->JSValue{const auto nativeStatementTemp2482=source;const auto nativeStatementTemp2483=JSValue("pivot");return nativeStatementTemp2482.get(nativeStatementTemp2483);}());const auto nativeStatementTemp2481=JSValue("clone");const auto nativeStatementTemp2477=nativeStatementTemp2480.get(nativeStatementTemp2481);const auto nativeStatementTemp2478=nativeStatementTemp2480;Args nativeStatementTemp2479;return nativeStatementTemp2477.call(nativeStatementTemp2478,nativeStatementTemp2479);}());return JSValue(nullptr);}());setReference(nativeStatementTemp2468,nativeStatementTemp2469,nativeStatementTemp2470);return nativeStatementTemp2470;}());
([&]()->JSValue{auto nativeStatementTemp2487=([&]()->JSValue{const auto nativeStatementTemp2489=nativeStatementTemp2412;const auto nativeStatementTemp2490=JSValue("matrix");return nativeStatementTemp2489.get(nativeStatementTemp2490);}());const auto nativeStatementTemp2488=JSValue("copy");const auto nativeStatementTemp2484=nativeStatementTemp2487.get(nativeStatementTemp2488);const auto nativeStatementTemp2485=nativeStatementTemp2487;Args nativeStatementTemp2486;nativeStatementTemp2486.push_back(([&]()->JSValue{const auto nativeStatementTemp2491=source;const auto nativeStatementTemp2492=JSValue("matrix");return nativeStatementTemp2491.get(nativeStatementTemp2492);}()));return nativeStatementTemp2484.call(nativeStatementTemp2485,nativeStatementTemp2486);}());
([&]()->JSValue{auto nativeStatementTemp2496=([&]()->JSValue{const auto nativeStatementTemp2498=nativeStatementTemp2412;const auto nativeStatementTemp2499=JSValue("matrixWorld");return nativeStatementTemp2498.get(nativeStatementTemp2499);}());const auto nativeStatementTemp2497=JSValue("copy");const auto nativeStatementTemp2493=nativeStatementTemp2496.get(nativeStatementTemp2497);const auto nativeStatementTemp2494=nativeStatementTemp2496;Args nativeStatementTemp2495;nativeStatementTemp2495.push_back(([&]()->JSValue{const auto nativeStatementTemp2500=source;const auto nativeStatementTemp2501=JSValue("matrixWorld");return nativeStatementTemp2500.get(nativeStatementTemp2501);}()));return nativeStatementTemp2493.call(nativeStatementTemp2494,nativeStatementTemp2495);}());
([&]()->JSValue{auto nativeStatementTemp2502=nativeStatementTemp2412;const auto nativeStatementTemp2503=JSValue("matrixAutoUpdate");const auto nativeStatementTemp2504=([&]()->JSValue{const auto nativeStatementTemp2507=source;const auto nativeStatementTemp2508=JSValue("matrixAutoUpdate");return nativeStatementTemp2507.get(nativeStatementTemp2508);}());setReference(nativeStatementTemp2502,nativeStatementTemp2503,nativeStatementTemp2504);return nativeStatementTemp2504;}());
([&]()->JSValue{auto nativeStatementTemp2509=nativeStatementTemp2412;const auto nativeStatementTemp2510=JSValue("matrixWorldAutoUpdate");const auto nativeStatementTemp2511=([&]()->JSValue{const auto nativeStatementTemp2514=source;const auto nativeStatementTemp2515=JSValue("matrixWorldAutoUpdate");return nativeStatementTemp2514.get(nativeStatementTemp2515);}());setReference(nativeStatementTemp2509,nativeStatementTemp2510,nativeStatementTemp2511);return nativeStatementTemp2511;}());
([&]()->JSValue{auto nativeStatementTemp2516=nativeStatementTemp2412;const auto nativeStatementTemp2517=JSValue("matrixWorldNeedsUpdate");const auto nativeStatementTemp2518=([&]()->JSValue{const auto nativeStatementTemp2521=source;const auto nativeStatementTemp2522=JSValue("matrixWorldNeedsUpdate");return nativeStatementTemp2521.get(nativeStatementTemp2522);}());setReference(nativeStatementTemp2516,nativeStatementTemp2517,nativeStatementTemp2518);return nativeStatementTemp2518;}());
([&]()->JSValue{auto nativeStatementTemp2523=([&]()->JSValue{const auto nativeStatementTemp2525=nativeStatementTemp2412;const auto nativeStatementTemp2526=JSValue("layers");return nativeStatementTemp2525.get(nativeStatementTemp2526);}());const auto nativeStatementTemp2524=JSValue("mask");const auto nativeStatementTemp2527=([&]()->JSValue{const auto nativeStatementTemp2530=([&]()->JSValue{const auto nativeStatementTemp2532=source;const auto nativeStatementTemp2533=JSValue("layers");return nativeStatementTemp2532.get(nativeStatementTemp2533);}());const auto nativeStatementTemp2531=JSValue("mask");return nativeStatementTemp2530.get(nativeStatementTemp2531);}());setReference(nativeStatementTemp2523,nativeStatementTemp2524,nativeStatementTemp2527);return nativeStatementTemp2527;}());
([&]()->JSValue{auto nativeStatementTemp2534=nativeStatementTemp2412;const auto nativeStatementTemp2535=JSValue("visible");const auto nativeStatementTemp2536=([&]()->JSValue{const auto nativeStatementTemp2539=source;const auto nativeStatementTemp2540=JSValue("visible");return nativeStatementTemp2539.get(nativeStatementTemp2540);}());setReference(nativeStatementTemp2534,nativeStatementTemp2535,nativeStatementTemp2536);return nativeStatementTemp2536;}());
([&]()->JSValue{auto nativeStatementTemp2541=nativeStatementTemp2412;const auto nativeStatementTemp2542=JSValue("castShadow");const auto nativeStatementTemp2543=([&]()->JSValue{const auto nativeStatementTemp2546=source;const auto nativeStatementTemp2547=JSValue("castShadow");return nativeStatementTemp2546.get(nativeStatementTemp2547);}());setReference(nativeStatementTemp2541,nativeStatementTemp2542,nativeStatementTemp2543);return nativeStatementTemp2543;}());
([&]()->JSValue{auto nativeStatementTemp2548=nativeStatementTemp2412;const auto nativeStatementTemp2549=JSValue("receiveShadow");const auto nativeStatementTemp2550=([&]()->JSValue{const auto nativeStatementTemp2553=source;const auto nativeStatementTemp2554=JSValue("receiveShadow");return nativeStatementTemp2553.get(nativeStatementTemp2554);}());setReference(nativeStatementTemp2548,nativeStatementTemp2549,nativeStatementTemp2550);return nativeStatementTemp2550;}());
([&]()->JSValue{auto nativeStatementTemp2555=nativeStatementTemp2412;const auto nativeStatementTemp2556=JSValue("frustumCulled");const auto nativeStatementTemp2557=([&]()->JSValue{const auto nativeStatementTemp2560=source;const auto nativeStatementTemp2561=JSValue("frustumCulled");return nativeStatementTemp2560.get(nativeStatementTemp2561);}());setReference(nativeStatementTemp2555,nativeStatementTemp2556,nativeStatementTemp2557);return nativeStatementTemp2557;}());
([&]()->JSValue{auto nativeStatementTemp2562=nativeStatementTemp2412;const auto nativeStatementTemp2563=JSValue("renderOrder");const auto nativeStatementTemp2564=([&]()->JSValue{const auto nativeStatementTemp2567=source;const auto nativeStatementTemp2568=JSValue("renderOrder");return nativeStatementTemp2567.get(nativeStatementTemp2568);}());setReference(nativeStatementTemp2562,nativeStatementTemp2563,nativeStatementTemp2564);return nativeStatementTemp2564;}());
([&]()->JSValue{auto nativeStatementTemp2569=nativeStatementTemp2412;const auto nativeStatementTemp2570=JSValue("static");const auto nativeStatementTemp2571=([&]()->JSValue{const auto nativeStatementTemp2574=source;const auto nativeStatementTemp2575=JSValue("static");return nativeStatementTemp2574.get(nativeStatementTemp2575);}());setReference(nativeStatementTemp2569,nativeStatementTemp2570,nativeStatementTemp2571);return nativeStatementTemp2571;}());
([&]()->JSValue{auto nativeStatementTemp2576=nativeStatementTemp2412;const auto nativeStatementTemp2577=JSValue("animations");const auto nativeStatementTemp2578=([&]()->JSValue{auto nativeStatementTemp2584=([&]()->JSValue{const auto nativeStatementTemp2586=source;const auto nativeStatementTemp2587=JSValue("animations");return nativeStatementTemp2586.get(nativeStatementTemp2587);}());const auto nativeStatementTemp2585=JSValue("slice");const auto nativeStatementTemp2581=nativeStatementTemp2584.get(nativeStatementTemp2585);const auto nativeStatementTemp2582=nativeStatementTemp2584;Args nativeStatementTemp2583;return nativeStatementTemp2581.call(nativeStatementTemp2582,nativeStatementTemp2583);}());setReference(nativeStatementTemp2576,nativeStatementTemp2577,nativeStatementTemp2578);return nativeStatementTemp2578;}());
([&]()->JSValue{auto nativeStatementTemp2588=nativeStatementTemp2412;const auto nativeStatementTemp2589=JSValue("userData");const auto nativeStatementTemp2590=([&]()->JSValue{auto nativeStatementTemp2596=nativeGlobalBinding("JSON");const auto nativeStatementTemp2597=JSValue("parse");const auto nativeStatementTemp2593=nativeStatementTemp2596.get(nativeStatementTemp2597);const auto nativeStatementTemp2594=nativeStatementTemp2596;Args nativeStatementTemp2595;nativeStatementTemp2595.push_back(([&]()->JSValue{auto nativeStatementTemp2601=nativeGlobalBinding("JSON");const auto nativeStatementTemp2602=JSValue("stringify");const auto nativeStatementTemp2598=nativeStatementTemp2601.get(nativeStatementTemp2602);const auto nativeStatementTemp2599=nativeStatementTemp2601;Args nativeStatementTemp2600;nativeStatementTemp2600.push_back(([&]()->JSValue{const auto nativeStatementTemp2603=source;const auto nativeStatementTemp2604=JSValue("userData");return nativeStatementTemp2603.get(nativeStatementTemp2604);}()));return nativeStatementTemp2598.call(nativeStatementTemp2599,nativeStatementTemp2600);}()));return nativeStatementTemp2593.call(nativeStatementTemp2594,nativeStatementTemp2595);}());setReference(nativeStatementTemp2588,nativeStatementTemp2589,nativeStatementTemp2590);return nativeStatementTemp2590;}());
if(([&]()->JSValue{const auto nativeStatementTemp2605=recursive;const auto nativeStatementTemp2606=JSValue(true);return binary("===",nativeStatementTemp2605,nativeStatementTemp2606);}()).toBoolean()){
{JSValue i=JSValue(0);
for(;([&]()->JSValue{const auto nativeStatementTemp2607=i;const auto nativeStatementTemp2608=([&]()->JSValue{const auto nativeStatementTemp2609=([&]()->JSValue{const auto nativeStatementTemp2611=source;const auto nativeStatementTemp2612=JSValue("children");return nativeStatementTemp2611.get(nativeStatementTemp2612);}());const auto nativeStatementTemp2610=JSValue("length");return nativeStatementTemp2609.get(nativeStatementTemp2610);}());return binary("<",nativeStatementTemp2607,nativeStatementTemp2608);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp2613=i.toNumber();const auto nativeStatementTemp2614=nativeStatementTemp2613+1;i=nativeStatementTemp2614;return nativeStatementTemp2613;}())){
JSValue child=([&]()->JSValue{const auto nativeStatementTemp2615=([&]()->JSValue{const auto nativeStatementTemp2617=source;const auto nativeStatementTemp2618=JSValue("children");return nativeStatementTemp2617.get(nativeStatementTemp2618);}());const auto nativeStatementTemp2616=i;return nativeStatementTemp2615.get(nativeStatementTemp2616);}());
([&]()->JSValue{auto nativeStatementTemp2622=nativeStatementTemp2412;const auto nativeStatementTemp2623=JSValue("add");const auto nativeStatementTemp2619=nativeStatementTemp2622.get(nativeStatementTemp2623);const auto nativeStatementTemp2620=nativeStatementTemp2622;Args nativeStatementTemp2621;nativeStatementTemp2621.push_back(([&]()->JSValue{auto nativeStatementTemp2627=child;const auto nativeStatementTemp2628=JSValue("clone");const auto nativeStatementTemp2624=nativeStatementTemp2627.get(nativeStatementTemp2628);const auto nativeStatementTemp2625=nativeStatementTemp2627;Args nativeStatementTemp2626;return nativeStatementTemp2624.call(nativeStatementTemp2625,nativeStatementTemp2626);}()));return nativeStatementTemp2619.call(nativeStatementTemp2620,nativeStatementTemp2621);}());

}
}

}
return nativeStatementTemp2412;
return JSValue{};
}

JSValue method47(const JSValue& nativeStatementTemp2629,const Args& nativeStatementTemp2630,const Args& nativeStatementTemp2631){
auto nativeStatementTemp2632=nativeStatementTemp2631[0];auto nativeStatementTemp2633=nativeStatementTemp2629;
([&]()->JSValue{auto nativeStatementTemp2638=nativeStatementTemp2633;const auto nativeStatementTemp2639=JSValue("dispatchEvent");const auto nativeStatementTemp2635=nativeStatementTemp2638.get(nativeStatementTemp2639);const auto nativeStatementTemp2636=nativeStatementTemp2638;Args nativeStatementTemp2637;nativeStatementTemp2637.push_back(([&]()->JSValue{auto nativeStatementTemp2640=JSValue::object();{const auto nativeStatementTemp2641=JSValue("type");const auto nativeStatementTemp2642=JSValue("dispose");own(nativeStatementTemp2640,nativeStatementTemp2641,nativeStatementTemp2642);}return nativeStatementTemp2640;}()));return nativeStatementTemp2635.call(nativeStatementTemp2636,nativeStatementTemp2637);}());
return JSValue{};
}
}
JSValue nativeObject3DModule(){
using namespace Object3DManaged;static thread_local JSWeakValue weak;if(const auto existing=weak.lock();existing&&existing->isObject())return *existing;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Object3D module");
auto module=JSValue::object(nullptr);
own(module,"Quaternion",nativeQuaternionConstructor());
own(module,"Vector3",nativeVector3Constructor());
own(module,"Matrix4",nativeMatrix4Constructor());
own(module,"EventDispatcher",EventDispatcher::runtimeConstructor());
own(module,"Euler",nativeEulerConstructor());
own(module,"Layers",nativeLayersConstructor());
own(module,"Matrix3",nativeMatrix3Constructor());
own(module,"generateUUID",nativeMathUtilsGenerateUUID());
own(module,"error",ThreeUtils::nativeErrorFunction());
own(module,"_object3DId",JSValue(0));
own(module,"_v1",([&]()->JSValue{const auto nativeStatementTemp2643=module.get("Vector3");Args nativeStatementTemp2644;return nativeStatementTemp2643.construct(nativeStatementTemp2644);}()));
own(module,"_q1",([&]()->JSValue{const auto nativeStatementTemp2645=module.get("Quaternion");Args nativeStatementTemp2646;return nativeStatementTemp2645.construct(nativeStatementTemp2646);}()));
own(module,"_m1",([&]()->JSValue{const auto nativeStatementTemp2647=module.get("Matrix4");Args nativeStatementTemp2648;return nativeStatementTemp2647.construct(nativeStatementTemp2648);}()));
own(module,"_target",([&]()->JSValue{const auto nativeStatementTemp2649=module.get("Vector3");Args nativeStatementTemp2650;return nativeStatementTemp2649.construct(nativeStatementTemp2650);}()));
own(module,"_position",([&]()->JSValue{const auto nativeStatementTemp2651=module.get("Vector3");Args nativeStatementTemp2652;return nativeStatementTemp2651.construct(nativeStatementTemp2652);}()));
own(module,"_scale",([&]()->JSValue{const auto nativeStatementTemp2653=module.get("Vector3");Args nativeStatementTemp2654;return nativeStatementTemp2653.construct(nativeStatementTemp2654);}()));
own(module,"_quaternion",([&]()->JSValue{const auto nativeStatementTemp2655=module.get("Quaternion");Args nativeStatementTemp2656;return nativeStatementTemp2655.construct(nativeStatementTemp2656);}()));
own(module,"_xAxis",([&]()->JSValue{const auto nativeStatementTemp2657=module.get("Vector3");Args nativeStatementTemp2658;nativeStatementTemp2658.push_back(JSValue(1));nativeStatementTemp2658.push_back(JSValue(0));nativeStatementTemp2658.push_back(JSValue(0));return nativeStatementTemp2657.construct(nativeStatementTemp2658);}()));
own(module,"_yAxis",([&]()->JSValue{const auto nativeStatementTemp2659=module.get("Vector3");Args nativeStatementTemp2660;nativeStatementTemp2660.push_back(JSValue(0));nativeStatementTemp2660.push_back(JSValue(1));nativeStatementTemp2660.push_back(JSValue(0));return nativeStatementTemp2659.construct(nativeStatementTemp2660);}()));
own(module,"_zAxis",([&]()->JSValue{const auto nativeStatementTemp2661=module.get("Vector3");Args nativeStatementTemp2662;nativeStatementTemp2662.push_back(JSValue(0));nativeStatementTemp2662.push_back(JSValue(0));nativeStatementTemp2662.push_back(JSValue(1));return nativeStatementTemp2661.construct(nativeStatementTemp2662);}()));
own(module,"_addedEvent",([&]()->JSValue{auto nativeStatementTemp2663=JSValue::object();{const auto nativeStatementTemp2664=JSValue("type");const auto nativeStatementTemp2665=JSValue("added");own(nativeStatementTemp2663,nativeStatementTemp2664,nativeStatementTemp2665);}return nativeStatementTemp2663;}()));
own(module,"_removedEvent",([&]()->JSValue{auto nativeStatementTemp2666=JSValue::object();{const auto nativeStatementTemp2667=JSValue("type");const auto nativeStatementTemp2668=JSValue("removed");own(nativeStatementTemp2666,nativeStatementTemp2667,nativeStatementTemp2668);}return nativeStatementTemp2666;}()));
own(module,"_childaddedEvent",([&]()->JSValue{auto nativeStatementTemp2669=JSValue::object();{const auto nativeStatementTemp2670=JSValue("type");const auto nativeStatementTemp2671=JSValue("childadded");own(nativeStatementTemp2669,nativeStatementTemp2670,nativeStatementTemp2671);}{const auto nativeStatementTemp2672=JSValue("child");const auto nativeStatementTemp2673=JSValue(nullptr);own(nativeStatementTemp2669,nativeStatementTemp2672,nativeStatementTemp2673);}return nativeStatementTemp2669;}()));
own(module,"_childremovedEvent",([&]()->JSValue{auto nativeStatementTemp2674=JSValue::object();{const auto nativeStatementTemp2675=JSValue("type");const auto nativeStatementTemp2676=JSValue("childremoved");own(nativeStatementTemp2674,nativeStatementTemp2675,nativeStatementTemp2676);}{const auto nativeStatementTemp2677=JSValue("child");const auto nativeStatementTemp2678=JSValue(nullptr);own(nativeStatementTemp2674,nativeStatementTemp2677,nativeStatementTemp2678);}return nativeStatementTemp2674;}()));
auto ctor=JSValue::builtinConstructor(method0,{module},"Object3D",0);ctor.setFunctionSourceText(Object3DRuntimeSource::classSource);ctor.setPrototype(module.get("EventDispatcher"));auto prototype=ctor.get("prototype");prototype.setPrototype(module.get("EventDispatcher").get("prototype"));own(module,"Object3D",ctor);
{auto fn=JSValue::function(method1,{module},"onBeforeShadow",0);fn.setFunctionSourceText(Object3DRuntimeSource::method1);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("onBeforeShadow",d);}
{auto fn=JSValue::function(method2,{module},"onAfterShadow",0);fn.setFunctionSourceText(Object3DRuntimeSource::method2);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("onAfterShadow",d);}
{auto fn=JSValue::function(method3,{module},"onBeforeRender",0);fn.setFunctionSourceText(Object3DRuntimeSource::method3);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("onBeforeRender",d);}
{auto fn=JSValue::function(method4,{module},"onAfterRender",0);fn.setFunctionSourceText(Object3DRuntimeSource::method4);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("onAfterRender",d);}
{auto fn=JSValue::function(method5,{module},"applyMatrix4",1);fn.setFunctionSourceText(Object3DRuntimeSource::method5);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("applyMatrix4",d);}
{auto fn=JSValue::function(method6,{module},"applyQuaternion",1);fn.setFunctionSourceText(Object3DRuntimeSource::method6);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("applyQuaternion",d);}
{auto fn=JSValue::function(method7,{module},"setRotationFromAxisAngle",2);fn.setFunctionSourceText(Object3DRuntimeSource::method7);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("setRotationFromAxisAngle",d);}
{auto fn=JSValue::function(method8,{module},"setRotationFromEuler",1);fn.setFunctionSourceText(Object3DRuntimeSource::method8);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("setRotationFromEuler",d);}
{auto fn=JSValue::function(method9,{module},"setRotationFromMatrix",1);fn.setFunctionSourceText(Object3DRuntimeSource::method9);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("setRotationFromMatrix",d);}
{auto fn=JSValue::function(method10,{module},"setRotationFromQuaternion",1);fn.setFunctionSourceText(Object3DRuntimeSource::method10);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("setRotationFromQuaternion",d);}
{auto fn=JSValue::function(method11,{module},"rotateOnAxis",2);fn.setFunctionSourceText(Object3DRuntimeSource::method11);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("rotateOnAxis",d);}
{auto fn=JSValue::function(method12,{module},"rotateOnWorldAxis",2);fn.setFunctionSourceText(Object3DRuntimeSource::method12);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("rotateOnWorldAxis",d);}
{auto fn=JSValue::function(method13,{module},"rotateX",1);fn.setFunctionSourceText(Object3DRuntimeSource::method13);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("rotateX",d);}
{auto fn=JSValue::function(method14,{module},"rotateY",1);fn.setFunctionSourceText(Object3DRuntimeSource::method14);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("rotateY",d);}
{auto fn=JSValue::function(method15,{module},"rotateZ",1);fn.setFunctionSourceText(Object3DRuntimeSource::method15);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("rotateZ",d);}
{auto fn=JSValue::function(method16,{module},"translateOnAxis",2);fn.setFunctionSourceText(Object3DRuntimeSource::method16);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("translateOnAxis",d);}
{auto fn=JSValue::function(method17,{module},"translateX",1);fn.setFunctionSourceText(Object3DRuntimeSource::method17);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("translateX",d);}
{auto fn=JSValue::function(method18,{module},"translateY",1);fn.setFunctionSourceText(Object3DRuntimeSource::method18);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("translateY",d);}
{auto fn=JSValue::function(method19,{module},"translateZ",1);fn.setFunctionSourceText(Object3DRuntimeSource::method19);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("translateZ",d);}
{auto fn=JSValue::function(method20,{module},"localToWorld",1);fn.setFunctionSourceText(Object3DRuntimeSource::method20);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("localToWorld",d);}
{auto fn=JSValue::function(method21,{module},"worldToLocal",1);fn.setFunctionSourceText(Object3DRuntimeSource::method21);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("worldToLocal",d);}
{auto fn=JSValue::function(method22,{module},"lookAt",3);fn.setFunctionSourceText(Object3DRuntimeSource::method22);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("lookAt",d);}
{auto fn=JSValue::function(method23,{module},"add",1);fn.setFunctionSourceText(Object3DRuntimeSource::method23);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("add",d);}
{auto fn=JSValue::function(method24,{module},"remove",1);fn.setFunctionSourceText(Object3DRuntimeSource::method24);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("remove",d);}
{auto fn=JSValue::function(method25,{module},"removeFromParent",0);fn.setFunctionSourceText(Object3DRuntimeSource::method25);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("removeFromParent",d);}
{auto fn=JSValue::function(method26,{module},"clear",0);fn.setFunctionSourceText(Object3DRuntimeSource::method26);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("clear",d);}
{auto fn=JSValue::function(method27,{module},"attach",1);fn.setFunctionSourceText(Object3DRuntimeSource::method27);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("attach",d);}
{auto fn=JSValue::function(method28,{module},"getObjectById",1);fn.setFunctionSourceText(Object3DRuntimeSource::method28);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("getObjectById",d);}
{auto fn=JSValue::function(method29,{module},"getObjectByName",1);fn.setFunctionSourceText(Object3DRuntimeSource::method29);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("getObjectByName",d);}
{auto fn=JSValue::function(method30,{module},"getObjectByProperty",2);fn.setFunctionSourceText(Object3DRuntimeSource::method30);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("getObjectByProperty",d);}
{auto fn=JSValue::function(method31,{module},"getObjectsByProperty",2);fn.setFunctionSourceText(Object3DRuntimeSource::method31);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("getObjectsByProperty",d);}
{auto fn=JSValue::function(method32,{module},"getWorldPosition",1);fn.setFunctionSourceText(Object3DRuntimeSource::method32);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("getWorldPosition",d);}
{auto fn=JSValue::function(method33,{module},"getWorldQuaternion",1);fn.setFunctionSourceText(Object3DRuntimeSource::method33);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("getWorldQuaternion",d);}
{auto fn=JSValue::function(method34,{module},"getWorldScale",1);fn.setFunctionSourceText(Object3DRuntimeSource::method34);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("getWorldScale",d);}
{auto fn=JSValue::function(method35,{module},"getWorldDirection",1);fn.setFunctionSourceText(Object3DRuntimeSource::method35);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("getWorldDirection",d);}
{auto fn=JSValue::function(method36,{module},"raycast",0);fn.setFunctionSourceText(Object3DRuntimeSource::method36);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("raycast",d);}
{auto fn=JSValue::function(method37,{module},"intersectsFrustum",0);fn.setFunctionSourceText(Object3DRuntimeSource::method37);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("intersectsFrustum",d);}
{auto fn=JSValue::function(method38,{module},"traverse",1);fn.setFunctionSourceText(Object3DRuntimeSource::method38);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("traverse",d);}
{auto fn=JSValue::function(method39,{module},"traverseVisible",1);fn.setFunctionSourceText(Object3DRuntimeSource::method39);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("traverseVisible",d);}
{auto fn=JSValue::function(method40,{module},"traverseAncestors",1);fn.setFunctionSourceText(Object3DRuntimeSource::method40);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("traverseAncestors",d);}
{auto fn=JSValue::function(method41,{module},"updateMatrix",0);fn.setFunctionSourceText(Object3DRuntimeSource::method41);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("updateMatrix",d);}
{auto fn=JSValue::function(method42,{module},"updateMatrixWorld",1);fn.setFunctionSourceText(Object3DRuntimeSource::method42);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("updateMatrixWorld",d);}
{auto fn=JSValue::function(method43,{module},"updateWorldMatrix",2);fn.setFunctionSourceText(Object3DRuntimeSource::method43);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("updateWorldMatrix",d);}
{auto fn=JSValue::function(method44,{module},"toJSON",1);fn.setFunctionSourceText(Object3DRuntimeSource::method44);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("toJSON",d);}
{auto fn=JSValue::function(method45,{module},"clone",1);fn.setFunctionSourceText(Object3DRuntimeSource::method45);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("clone",d);}
{auto fn=JSValue::function(method46,{module},"copy",1);fn.setFunctionSourceText(Object3DRuntimeSource::method46);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("copy",d);}
{auto fn=JSValue::function(method47,{module},"dispose",0);fn.setFunctionSourceText(Object3DRuntimeSource::method47);JSPropertyDescriptor d;d.value=fn;d.writable=d.configurable=true;prototype.defineProperty("dispose",d);}
([&]()->JSValue{auto nativeStatementTemp2679=module.get("Object3D");const auto nativeStatementTemp2680=JSValue("DEFAULT_UP");const auto nativeStatementTemp2681=([&]()->JSValue{const auto nativeStatementTemp2684=module.get("Vector3");Args nativeStatementTemp2685;nativeStatementTemp2685.push_back(JSValue(0));nativeStatementTemp2685.push_back(JSValue(1));nativeStatementTemp2685.push_back(JSValue(0));return nativeStatementTemp2684.construct(nativeStatementTemp2685);}());setReference(nativeStatementTemp2679,nativeStatementTemp2680,nativeStatementTemp2681);return nativeStatementTemp2681;}());
([&]()->JSValue{auto nativeStatementTemp2686=module.get("Object3D");const auto nativeStatementTemp2687=JSValue("DEFAULT_MATRIX_AUTO_UPDATE");const auto nativeStatementTemp2688=JSValue(true);setReference(nativeStatementTemp2686,nativeStatementTemp2687,nativeStatementTemp2688);return nativeStatementTemp2688;}());
([&]()->JSValue{auto nativeStatementTemp2691=module.get("Object3D");const auto nativeStatementTemp2692=JSValue("DEFAULT_MATRIX_WORLD_AUTO_UPDATE");const auto nativeStatementTemp2693=JSValue(true);setReference(nativeStatementTemp2691,nativeStatementTemp2692,nativeStatementTemp2693);return nativeStatementTemp2693;}());
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto exports=JSValue::object();own(exports,"Object3D",module.get("Object3D"));exports.retainForHeapLifetime();weak=JSWeakValue(exports);return exports;
}
JSValue nativeObject3DBinding(const std::string& name){const auto keeper=nativeObject3DModule();return Object3DManaged::lexical().lock()->get(name);}
JSValue nativeObject3DConstructor(){return nativeObject3DBinding("Object3D");}
