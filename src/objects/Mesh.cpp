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

// Complete native managed Mesh source class; typed implementation above remains intact.
#include "core/JSGlobal.h"
#include "core/JSModuleObject.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "math/JSNumberMath.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Ray.h"
#include "math/Triangle.h"
#include "core/Object3D.h"
#include "core/BufferGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "constants.h"
#include <string_view>
#include <unordered_set>
namespace MeshManaged {

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

JSWeakValue& lexical(){static thread_local JSWeakValue value;return value;}
JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
const auto nativeStatementTemp5=nativeStatementTemp0;nativeStatementTemp4={};
JSValue geometry=arg(nativeStatementTemp1,0);
if(geometry.isUndefined())geometry=([&]()->JSValue{const auto nativeStatementTemp6=nativeStatementTemp3.get("BufferGeometry");Args nativeStatementTemp7;return nativeStatementTemp6.construct(nativeStatementTemp7);}());
JSValue material=arg(nativeStatementTemp1,1);
if(material.isUndefined())material=([&]()->JSValue{const auto nativeStatementTemp8=nativeStatementTemp3.get("MeshBasicMaterial");Args nativeStatementTemp9;return nativeStatementTemp8.construct(nativeStatementTemp9);}());
([&]()->JSValue{const auto nativeStatementTemp13=nativeStatementTemp3.get("Mesh").getPrototype();Args nativeStatementTemp12;nativeStatementTemp4=nativeStatementTemp13.construct(nativeStatementTemp12,nativeStatementTemp5);return nativeStatementTemp4;}());
([&]()->JSValue{auto nativeStatementTemp14=nativeStatementTemp4;const auto nativeStatementTemp15=JSValue("isMesh");const auto nativeStatementTemp16=JSValue(true);setReference(nativeStatementTemp14,nativeStatementTemp15,nativeStatementTemp16);return nativeStatementTemp16;}());
([&]()->JSValue{auto nativeStatementTemp19=nativeStatementTemp4;const auto nativeStatementTemp20=JSValue("type");const auto nativeStatementTemp21=JSValue("Mesh");setReference(nativeStatementTemp19,nativeStatementTemp20,nativeStatementTemp21);return nativeStatementTemp21;}());
([&]()->JSValue{auto nativeStatementTemp24=nativeStatementTemp4;const auto nativeStatementTemp25=JSValue("geometry");const auto nativeStatementTemp26=geometry;setReference(nativeStatementTemp24,nativeStatementTemp25,nativeStatementTemp26);return nativeStatementTemp26;}());
([&]()->JSValue{auto nativeStatementTemp29=nativeStatementTemp4;const auto nativeStatementTemp30=JSValue("material");const auto nativeStatementTemp31=material;setReference(nativeStatementTemp29,nativeStatementTemp30,nativeStatementTemp31);return nativeStatementTemp31;}());
([&]()->JSValue{auto nativeStatementTemp34=nativeStatementTemp4;const auto nativeStatementTemp35=JSValue("morphTargetDictionary");const auto nativeStatementTemp36=JSValue{};setReference(nativeStatementTemp34,nativeStatementTemp35,nativeStatementTemp36);return nativeStatementTemp36;}());
([&]()->JSValue{auto nativeStatementTemp39=nativeStatementTemp4;const auto nativeStatementTemp40=JSValue("morphTargetInfluences");const auto nativeStatementTemp41=JSValue{};setReference(nativeStatementTemp39,nativeStatementTemp40,nativeStatementTemp41);return nativeStatementTemp41;}());
([&]()->JSValue{auto nativeStatementTemp44=nativeStatementTemp4;const auto nativeStatementTemp45=JSValue("count");const auto nativeStatementTemp46=JSValue(1);setReference(nativeStatementTemp44,nativeStatementTemp45,nativeStatementTemp46);return nativeStatementTemp46;}());
([&]()->JSValue{auto nativeStatementTemp52=nativeStatementTemp4;const auto nativeStatementTemp53=JSValue("updateMorphTargets");const auto nativeStatementTemp49=nativeStatementTemp52.get(nativeStatementTemp53);const auto nativeStatementTemp50=nativeStatementTemp52;Args nativeStatementTemp51;return nativeStatementTemp49.call(nativeStatementTemp50,nativeStatementTemp51);}());
return nativeStatementTemp4;
}

JSValue method1(const JSValue& nativeStatementTemp54,const Args& nativeStatementTemp55,const Args& nativeStatementTemp56){
auto nativeStatementTemp57=nativeStatementTemp56[0];auto nativeStatementTemp58=nativeStatementTemp54;
JSValue source=arg(nativeStatementTemp55,0);
JSValue recursive=arg(nativeStatementTemp55,1);
([&]()->JSValue{const auto nativeStatementTemp63=nativeStatementTemp57.get("_home").getPrototype();const auto nativeStatementTemp64=JSValue("copy");const auto nativeStatementTemp60=nativeStatementTemp63.get(nativeStatementTemp64,nativeStatementTemp58);const auto nativeStatementTemp61=nativeStatementTemp58;Args nativeStatementTemp62;nativeStatementTemp62.push_back(source);nativeStatementTemp62.push_back(recursive);return nativeStatementTemp60.call(nativeStatementTemp61,nativeStatementTemp62);}());
if(([&]()->JSValue{const auto nativeStatementTemp65=([&]()->JSValue{const auto nativeStatementTemp67=source;const auto nativeStatementTemp68=JSValue("morphTargetInfluences");return nativeStatementTemp67.get(nativeStatementTemp68);}());const auto nativeStatementTemp66=JSValue{};return binary("!==",nativeStatementTemp65,nativeStatementTemp66);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp69=nativeStatementTemp58;const auto nativeStatementTemp70=JSValue("morphTargetInfluences");const auto nativeStatementTemp71=([&]()->JSValue{auto nativeStatementTemp77=([&]()->JSValue{const auto nativeStatementTemp79=source;const auto nativeStatementTemp80=JSValue("morphTargetInfluences");return nativeStatementTemp79.get(nativeStatementTemp80);}());const auto nativeStatementTemp78=JSValue("slice");const auto nativeStatementTemp74=nativeStatementTemp77.get(nativeStatementTemp78);const auto nativeStatementTemp75=nativeStatementTemp77;Args nativeStatementTemp76;return nativeStatementTemp74.call(nativeStatementTemp75,nativeStatementTemp76);}());setReference(nativeStatementTemp69,nativeStatementTemp70,nativeStatementTemp71);return nativeStatementTemp71;}());

}
else {
([&]()->JSValue{auto nativeStatementTemp81=nativeStatementTemp58;const auto nativeStatementTemp82=JSValue("morphTargetInfluences");const auto nativeStatementTemp83=JSValue{};setReference(nativeStatementTemp81,nativeStatementTemp82,nativeStatementTemp83);return nativeStatementTemp83;}());

}
if(([&]()->JSValue{const auto nativeStatementTemp86=([&]()->JSValue{const auto nativeStatementTemp88=source;const auto nativeStatementTemp89=JSValue("morphTargetDictionary");return nativeStatementTemp88.get(nativeStatementTemp89);}());const auto nativeStatementTemp87=JSValue{};return binary("!==",nativeStatementTemp86,nativeStatementTemp87);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp90=nativeStatementTemp58;const auto nativeStatementTemp91=JSValue("morphTargetDictionary");const auto nativeStatementTemp92=([&]()->JSValue{auto nativeStatementTemp98=nativeGlobalBinding("Object");const auto nativeStatementTemp99=JSValue("assign");const auto nativeStatementTemp95=nativeStatementTemp98.get(nativeStatementTemp99);const auto nativeStatementTemp96=nativeStatementTemp98;Args nativeStatementTemp97;nativeStatementTemp97.push_back(([&]()->JSValue{auto nativeStatementTemp100=JSValue::object();return nativeStatementTemp100;}()));nativeStatementTemp97.push_back(([&]()->JSValue{const auto nativeStatementTemp101=source;const auto nativeStatementTemp102=JSValue("morphTargetDictionary");return nativeStatementTemp101.get(nativeStatementTemp102);}()));return nativeStatementTemp95.call(nativeStatementTemp96,nativeStatementTemp97);}());setReference(nativeStatementTemp90,nativeStatementTemp91,nativeStatementTemp92);return nativeStatementTemp92;}());

}
else {
([&]()->JSValue{auto nativeStatementTemp103=nativeStatementTemp58;const auto nativeStatementTemp104=JSValue("morphTargetDictionary");const auto nativeStatementTemp105=JSValue{};setReference(nativeStatementTemp103,nativeStatementTemp104,nativeStatementTemp105);return nativeStatementTemp105;}());

}
([&]()->JSValue{auto nativeStatementTemp108=nativeStatementTemp58;const auto nativeStatementTemp109=JSValue("material");const auto nativeStatementTemp110=([&]()->JSValue{if(([&]()->JSValue{auto nativeStatementTemp116=nativeGlobalBinding("Array");const auto nativeStatementTemp117=JSValue("isArray");const auto nativeStatementTemp113=nativeStatementTemp116.get(nativeStatementTemp117);const auto nativeStatementTemp114=nativeStatementTemp116;Args nativeStatementTemp115;nativeStatementTemp115.push_back(([&]()->JSValue{const auto nativeStatementTemp118=source;const auto nativeStatementTemp119=JSValue("material");return nativeStatementTemp118.get(nativeStatementTemp119);}()));return nativeStatementTemp113.call(nativeStatementTemp114,nativeStatementTemp115);}()).toBoolean())return ([&]()->JSValue{auto nativeStatementTemp123=([&]()->JSValue{const auto nativeStatementTemp125=source;const auto nativeStatementTemp126=JSValue("material");return nativeStatementTemp125.get(nativeStatementTemp126);}());const auto nativeStatementTemp124=JSValue("slice");const auto nativeStatementTemp120=nativeStatementTemp123.get(nativeStatementTemp124);const auto nativeStatementTemp121=nativeStatementTemp123;Args nativeStatementTemp122;return nativeStatementTemp120.call(nativeStatementTemp121,nativeStatementTemp122);}());return ([&]()->JSValue{const auto nativeStatementTemp127=source;const auto nativeStatementTemp128=JSValue("material");return nativeStatementTemp127.get(nativeStatementTemp128);}());}());setReference(nativeStatementTemp108,nativeStatementTemp109,nativeStatementTemp110);return nativeStatementTemp110;}());
([&]()->JSValue{auto nativeStatementTemp129=nativeStatementTemp58;const auto nativeStatementTemp130=JSValue("geometry");const auto nativeStatementTemp131=([&]()->JSValue{const auto nativeStatementTemp134=source;const auto nativeStatementTemp135=JSValue("geometry");return nativeStatementTemp134.get(nativeStatementTemp135);}());setReference(nativeStatementTemp129,nativeStatementTemp130,nativeStatementTemp131);return nativeStatementTemp131;}());
return nativeStatementTemp58;
return JSValue{};
}

JSValue method2(const JSValue& nativeStatementTemp136,const Args& nativeStatementTemp137,const Args& nativeStatementTemp138){
auto nativeStatementTemp139=nativeStatementTemp138[0];auto nativeStatementTemp140=nativeStatementTemp136;
JSValue geometry=([&]()->JSValue{const auto nativeStatementTemp142=nativeStatementTemp140;const auto nativeStatementTemp143=JSValue("geometry");return nativeStatementTemp142.get(nativeStatementTemp143);}());
JSValue morphAttributes=([&]()->JSValue{const auto nativeStatementTemp144=geometry;const auto nativeStatementTemp145=JSValue("morphAttributes");return nativeStatementTemp144.get(nativeStatementTemp145);}());
JSValue keys=([&]()->JSValue{auto nativeStatementTemp149=nativeGlobalBinding("Object");const auto nativeStatementTemp150=JSValue("keys");const auto nativeStatementTemp146=nativeStatementTemp149.get(nativeStatementTemp150);const auto nativeStatementTemp147=nativeStatementTemp149;Args nativeStatementTemp148;nativeStatementTemp148.push_back(morphAttributes);return nativeStatementTemp146.call(nativeStatementTemp147,nativeStatementTemp148);}());
if(([&]()->JSValue{const auto nativeStatementTemp151=([&]()->JSValue{const auto nativeStatementTemp153=keys;const auto nativeStatementTemp154=JSValue("length");return nativeStatementTemp153.get(nativeStatementTemp154);}());const auto nativeStatementTemp152=JSValue(0);return binary(">",nativeStatementTemp151,nativeStatementTemp152);}()).toBoolean()){
JSValue morphAttribute=([&]()->JSValue{const auto nativeStatementTemp155=morphAttributes;const auto nativeStatementTemp156=([&]()->JSValue{const auto nativeStatementTemp157=keys;const auto nativeStatementTemp158=JSValue(0);return nativeStatementTemp157.get(nativeStatementTemp158);}());return nativeStatementTemp155.get(nativeStatementTemp156);}());
if(([&]()->JSValue{const auto nativeStatementTemp159=morphAttribute;const auto nativeStatementTemp160=JSValue{};return binary("!==",nativeStatementTemp159,nativeStatementTemp160);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp161=nativeStatementTemp140;const auto nativeStatementTemp162=JSValue("morphTargetInfluences");const auto nativeStatementTemp163=([&]()->JSValue{auto nativeStatementTemp166=JSValue::array();return nativeStatementTemp166;}());setReference(nativeStatementTemp161,nativeStatementTemp162,nativeStatementTemp163);return nativeStatementTemp163;}());
([&]()->JSValue{auto nativeStatementTemp167=nativeStatementTemp140;const auto nativeStatementTemp168=JSValue("morphTargetDictionary");const auto nativeStatementTemp169=([&]()->JSValue{auto nativeStatementTemp172=JSValue::object();return nativeStatementTemp172;}());setReference(nativeStatementTemp167,nativeStatementTemp168,nativeStatementTemp169);return nativeStatementTemp169;}());
{JSValue m=JSValue(0);
JSValue ml=([&]()->JSValue{const auto nativeStatementTemp173=morphAttribute;const auto nativeStatementTemp174=JSValue("length");return nativeStatementTemp173.get(nativeStatementTemp174);}());
for(;([&]()->JSValue{const auto nativeStatementTemp175=m;const auto nativeStatementTemp176=ml;return binary("<",nativeStatementTemp175,nativeStatementTemp176);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp177=m.toNumber();const auto nativeStatementTemp178=nativeStatementTemp177+1;m=nativeStatementTemp178;return nativeStatementTemp177;}())){
JSValue name=([&]()->JSValue{const auto nativeStatementTemp179=([&]()->JSValue{const auto nativeStatementTemp180=([&]()->JSValue{const auto nativeStatementTemp182=morphAttribute;const auto nativeStatementTemp183=m;return nativeStatementTemp182.get(nativeStatementTemp183);}());const auto nativeStatementTemp181=JSValue("name");return nativeStatementTemp180.get(nativeStatementTemp181);}());if(nativeStatementTemp179.toBoolean())return nativeStatementTemp179;return ([&]()->JSValue{const auto nativeStatementTemp184=nativeGlobalBinding("String");const JSValue nativeStatementTemp185;Args nativeStatementTemp186;nativeStatementTemp186.push_back(m);return nativeStatementTemp184.call(nativeStatementTemp185,nativeStatementTemp186);}());}());
([&]()->JSValue{auto nativeStatementTemp190=([&]()->JSValue{const auto nativeStatementTemp192=nativeStatementTemp140;const auto nativeStatementTemp193=JSValue("morphTargetInfluences");return nativeStatementTemp192.get(nativeStatementTemp193);}());const auto nativeStatementTemp191=JSValue("push");const auto nativeStatementTemp187=nativeStatementTemp190.get(nativeStatementTemp191);const auto nativeStatementTemp188=nativeStatementTemp190;Args nativeStatementTemp189;nativeStatementTemp189.push_back(JSValue(0));return nativeStatementTemp187.call(nativeStatementTemp188,nativeStatementTemp189);}());
([&]()->JSValue{auto nativeStatementTemp194=([&]()->JSValue{const auto nativeStatementTemp196=nativeStatementTemp140;const auto nativeStatementTemp197=JSValue("morphTargetDictionary");return nativeStatementTemp196.get(nativeStatementTemp197);}());const auto nativeStatementTemp195=name;const auto nativeStatementTemp198=m;setReference(nativeStatementTemp194,nativeStatementTemp195,nativeStatementTemp198);return nativeStatementTemp198;}());

}
}

}

}
return JSValue{};
}

JSValue method3(const JSValue& nativeStatementTemp201,const Args& nativeStatementTemp202,const Args& nativeStatementTemp203){
auto nativeStatementTemp204=nativeStatementTemp203[0];auto nativeStatementTemp205=nativeStatementTemp201;
JSValue index=arg(nativeStatementTemp202,0);
JSValue target=arg(nativeStatementTemp202,1);
JSValue geometry=([&]()->JSValue{const auto nativeStatementTemp207=nativeStatementTemp205;const auto nativeStatementTemp208=JSValue("geometry");return nativeStatementTemp207.get(nativeStatementTemp208);}());
JSValue position=([&]()->JSValue{const auto nativeStatementTemp209=([&]()->JSValue{const auto nativeStatementTemp211=geometry;const auto nativeStatementTemp212=JSValue("attributes");return nativeStatementTemp211.get(nativeStatementTemp212);}());const auto nativeStatementTemp210=JSValue("position");return nativeStatementTemp209.get(nativeStatementTemp210);}());
JSValue morphPosition=([&]()->JSValue{const auto nativeStatementTemp213=([&]()->JSValue{const auto nativeStatementTemp215=geometry;const auto nativeStatementTemp216=JSValue("morphAttributes");return nativeStatementTemp215.get(nativeStatementTemp216);}());const auto nativeStatementTemp214=JSValue("position");return nativeStatementTemp213.get(nativeStatementTemp214);}());
JSValue morphTargetsRelative=([&]()->JSValue{const auto nativeStatementTemp217=geometry;const auto nativeStatementTemp218=JSValue("morphTargetsRelative");return nativeStatementTemp217.get(nativeStatementTemp218);}());
([&]()->JSValue{auto nativeStatementTemp222=target;const auto nativeStatementTemp223=JSValue("fromBufferAttribute");const auto nativeStatementTemp219=nativeStatementTemp222.get(nativeStatementTemp223);const auto nativeStatementTemp220=nativeStatementTemp222;Args nativeStatementTemp221;nativeStatementTemp221.push_back(position);nativeStatementTemp221.push_back(index);return nativeStatementTemp219.call(nativeStatementTemp220,nativeStatementTemp221);}());
JSValue morphInfluences=([&]()->JSValue{const auto nativeStatementTemp224=nativeStatementTemp205;const auto nativeStatementTemp225=JSValue("morphTargetInfluences");return nativeStatementTemp224.get(nativeStatementTemp225);}());
if(([&]()->JSValue{const auto nativeStatementTemp226=morphPosition;if(!nativeStatementTemp226.toBoolean())return nativeStatementTemp226;return morphInfluences;}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp230=nativeStatementTemp204.get("_morphA");const auto nativeStatementTemp231=JSValue("set");const auto nativeStatementTemp227=nativeStatementTemp230.get(nativeStatementTemp231);const auto nativeStatementTemp228=nativeStatementTemp230;Args nativeStatementTemp229;nativeStatementTemp229.push_back(JSValue(0));nativeStatementTemp229.push_back(JSValue(0));nativeStatementTemp229.push_back(JSValue(0));return nativeStatementTemp227.call(nativeStatementTemp228,nativeStatementTemp229);}());
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp232=morphPosition;const auto nativeStatementTemp233=JSValue("length");return nativeStatementTemp232.get(nativeStatementTemp233);}());
for(;([&]()->JSValue{const auto nativeStatementTemp234=i;const auto nativeStatementTemp235=il;return binary("<",nativeStatementTemp234,nativeStatementTemp235);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp236=i.toNumber();const auto nativeStatementTemp237=nativeStatementTemp236+1;i=nativeStatementTemp237;return nativeStatementTemp236;}())){
JSValue influence=([&]()->JSValue{const auto nativeStatementTemp238=morphInfluences;const auto nativeStatementTemp239=i;return nativeStatementTemp238.get(nativeStatementTemp239);}());
JSValue morphAttribute=([&]()->JSValue{const auto nativeStatementTemp240=morphPosition;const auto nativeStatementTemp241=i;return nativeStatementTemp240.get(nativeStatementTemp241);}());
if(([&]()->JSValue{const auto nativeStatementTemp242=influence;const auto nativeStatementTemp243=JSValue(0);return binary("===",nativeStatementTemp242,nativeStatementTemp243);}()).toBoolean())continue;
([&]()->JSValue{auto nativeStatementTemp247=nativeStatementTemp204.get("_tempA");const auto nativeStatementTemp248=JSValue("fromBufferAttribute");const auto nativeStatementTemp244=nativeStatementTemp247.get(nativeStatementTemp248);const auto nativeStatementTemp245=nativeStatementTemp247;Args nativeStatementTemp246;nativeStatementTemp246.push_back(morphAttribute);nativeStatementTemp246.push_back(index);return nativeStatementTemp244.call(nativeStatementTemp245,nativeStatementTemp246);}());
if(morphTargetsRelative.toBoolean()){
([&]()->JSValue{auto nativeStatementTemp252=nativeStatementTemp204.get("_morphA");const auto nativeStatementTemp253=JSValue("addScaledVector");const auto nativeStatementTemp249=nativeStatementTemp252.get(nativeStatementTemp253);const auto nativeStatementTemp250=nativeStatementTemp252;Args nativeStatementTemp251;nativeStatementTemp251.push_back(nativeStatementTemp204.get("_tempA"));nativeStatementTemp251.push_back(influence);return nativeStatementTemp249.call(nativeStatementTemp250,nativeStatementTemp251);}());

}
else {
([&]()->JSValue{auto nativeStatementTemp257=nativeStatementTemp204.get("_morphA");const auto nativeStatementTemp258=JSValue("addScaledVector");const auto nativeStatementTemp254=nativeStatementTemp257.get(nativeStatementTemp258);const auto nativeStatementTemp255=nativeStatementTemp257;Args nativeStatementTemp256;nativeStatementTemp256.push_back(([&]()->JSValue{auto nativeStatementTemp262=nativeStatementTemp204.get("_tempA");const auto nativeStatementTemp263=JSValue("sub");const auto nativeStatementTemp259=nativeStatementTemp262.get(nativeStatementTemp263);const auto nativeStatementTemp260=nativeStatementTemp262;Args nativeStatementTemp261;nativeStatementTemp261.push_back(target);return nativeStatementTemp259.call(nativeStatementTemp260,nativeStatementTemp261);}()));nativeStatementTemp256.push_back(influence);return nativeStatementTemp254.call(nativeStatementTemp255,nativeStatementTemp256);}());

}

}
}
([&]()->JSValue{auto nativeStatementTemp267=target;const auto nativeStatementTemp268=JSValue("add");const auto nativeStatementTemp264=nativeStatementTemp267.get(nativeStatementTemp268);const auto nativeStatementTemp265=nativeStatementTemp267;Args nativeStatementTemp266;nativeStatementTemp266.push_back(nativeStatementTemp204.get("_morphA"));return nativeStatementTemp264.call(nativeStatementTemp265,nativeStatementTemp266);}());

}
return target;
return JSValue{};
}

JSValue method4(const JSValue& nativeStatementTemp269,const Args& nativeStatementTemp270,const Args& nativeStatementTemp271){
auto nativeStatementTemp272=nativeStatementTemp271[0];auto nativeStatementTemp273=nativeStatementTemp269;
JSValue frustum=arg(nativeStatementTemp270,0);
return ([&]()->JSValue{auto nativeStatementTemp278=frustum;const auto nativeStatementTemp279=JSValue("intersectsObject");const auto nativeStatementTemp275=nativeStatementTemp278.get(nativeStatementTemp279);const auto nativeStatementTemp276=nativeStatementTemp278;Args nativeStatementTemp277;nativeStatementTemp277.push_back(nativeStatementTemp273);return nativeStatementTemp275.call(nativeStatementTemp276,nativeStatementTemp277);}());
return JSValue{};
}

JSValue method5(const JSValue& nativeStatementTemp280,const Args& nativeStatementTemp281,const Args& nativeStatementTemp282){
auto nativeStatementTemp283=nativeStatementTemp282[0];auto nativeStatementTemp284=nativeStatementTemp280;
JSValue raycaster=arg(nativeStatementTemp281,0);
JSValue intersects=arg(nativeStatementTemp281,1);
JSValue geometry=([&]()->JSValue{const auto nativeStatementTemp286=nativeStatementTemp284;const auto nativeStatementTemp287=JSValue("geometry");return nativeStatementTemp286.get(nativeStatementTemp287);}());
JSValue material=([&]()->JSValue{const auto nativeStatementTemp288=nativeStatementTemp284;const auto nativeStatementTemp289=JSValue("material");return nativeStatementTemp288.get(nativeStatementTemp289);}());
JSValue matrixWorld=([&]()->JSValue{const auto nativeStatementTemp290=nativeStatementTemp284;const auto nativeStatementTemp291=JSValue("matrixWorld");return nativeStatementTemp290.get(nativeStatementTemp291);}());
if(([&]()->JSValue{const auto nativeStatementTemp292=material;const auto nativeStatementTemp293=JSValue{};return binary("===",nativeStatementTemp292,nativeStatementTemp293);}()).toBoolean())return JSValue{};
if(([&]()->JSValue{const auto nativeStatementTemp294=([&]()->JSValue{const auto nativeStatementTemp296=geometry;const auto nativeStatementTemp297=JSValue("boundingSphere");return nativeStatementTemp296.get(nativeStatementTemp297);}());const auto nativeStatementTemp295=JSValue(nullptr);return binary("===",nativeStatementTemp294,nativeStatementTemp295);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp301=geometry;const auto nativeStatementTemp302=JSValue("computeBoundingSphere");const auto nativeStatementTemp298=nativeStatementTemp301.get(nativeStatementTemp302);const auto nativeStatementTemp299=nativeStatementTemp301;Args nativeStatementTemp300;return nativeStatementTemp298.call(nativeStatementTemp299,nativeStatementTemp300);}());
([&]()->JSValue{auto nativeStatementTemp306=nativeStatementTemp283.get("_sphere");const auto nativeStatementTemp307=JSValue("copy");const auto nativeStatementTemp303=nativeStatementTemp306.get(nativeStatementTemp307);const auto nativeStatementTemp304=nativeStatementTemp306;Args nativeStatementTemp305;nativeStatementTemp305.push_back(([&]()->JSValue{const auto nativeStatementTemp308=geometry;const auto nativeStatementTemp309=JSValue("boundingSphere");return nativeStatementTemp308.get(nativeStatementTemp309);}()));return nativeStatementTemp303.call(nativeStatementTemp304,nativeStatementTemp305);}());
([&]()->JSValue{auto nativeStatementTemp313=nativeStatementTemp283.get("_sphere");const auto nativeStatementTemp314=JSValue("applyMatrix4");const auto nativeStatementTemp310=nativeStatementTemp313.get(nativeStatementTemp314);const auto nativeStatementTemp311=nativeStatementTemp313;Args nativeStatementTemp312;nativeStatementTemp312.push_back(matrixWorld);return nativeStatementTemp310.call(nativeStatementTemp311,nativeStatementTemp312);}());
([&]()->JSValue{auto nativeStatementTemp318=([&]()->JSValue{auto nativeStatementTemp323=nativeStatementTemp283.get("_ray");const auto nativeStatementTemp324=JSValue("copy");const auto nativeStatementTemp320=nativeStatementTemp323.get(nativeStatementTemp324);const auto nativeStatementTemp321=nativeStatementTemp323;Args nativeStatementTemp322;nativeStatementTemp322.push_back(([&]()->JSValue{const auto nativeStatementTemp325=raycaster;const auto nativeStatementTemp326=JSValue("ray");return nativeStatementTemp325.get(nativeStatementTemp326);}()));return nativeStatementTemp320.call(nativeStatementTemp321,nativeStatementTemp322);}());const auto nativeStatementTemp319=JSValue("recast");const auto nativeStatementTemp315=nativeStatementTemp318.get(nativeStatementTemp319);const auto nativeStatementTemp316=nativeStatementTemp318;Args nativeStatementTemp317;nativeStatementTemp317.push_back(([&]()->JSValue{const auto nativeStatementTemp327=raycaster;const auto nativeStatementTemp328=JSValue("near");return nativeStatementTemp327.get(nativeStatementTemp328);}()));return nativeStatementTemp315.call(nativeStatementTemp316,nativeStatementTemp317);}());
if(([&]()->JSValue{const auto nativeStatementTemp329=([&]()->JSValue{auto nativeStatementTemp334=nativeStatementTemp283.get("_sphere");const auto nativeStatementTemp335=JSValue("containsPoint");const auto nativeStatementTemp331=nativeStatementTemp334.get(nativeStatementTemp335);const auto nativeStatementTemp332=nativeStatementTemp334;Args nativeStatementTemp333;nativeStatementTemp333.push_back(([&]()->JSValue{const auto nativeStatementTemp336=nativeStatementTemp283.get("_ray");const auto nativeStatementTemp337=JSValue("origin");return nativeStatementTemp336.get(nativeStatementTemp337);}()));return nativeStatementTemp331.call(nativeStatementTemp332,nativeStatementTemp333);}());const auto nativeStatementTemp330=JSValue(false);return binary("===",nativeStatementTemp329,nativeStatementTemp330);}()).toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp338=([&]()->JSValue{auto nativeStatementTemp343=nativeStatementTemp283.get("_ray");const auto nativeStatementTemp344=JSValue("intersectSphere");const auto nativeStatementTemp340=nativeStatementTemp343.get(nativeStatementTemp344);const auto nativeStatementTemp341=nativeStatementTemp343;Args nativeStatementTemp342;nativeStatementTemp342.push_back(nativeStatementTemp283.get("_sphere"));nativeStatementTemp342.push_back(nativeStatementTemp283.get("_sphereHitAt"));return nativeStatementTemp340.call(nativeStatementTemp341,nativeStatementTemp342);}());const auto nativeStatementTemp339=JSValue(nullptr);return binary("===",nativeStatementTemp338,nativeStatementTemp339);}()).toBoolean())return JSValue{};
if(([&]()->JSValue{const auto nativeStatementTemp345=([&]()->JSValue{auto nativeStatementTemp350=([&]()->JSValue{const auto nativeStatementTemp352=nativeStatementTemp283.get("_ray");const auto nativeStatementTemp353=JSValue("origin");return nativeStatementTemp352.get(nativeStatementTemp353);}());const auto nativeStatementTemp351=JSValue("distanceToSquared");const auto nativeStatementTemp347=nativeStatementTemp350.get(nativeStatementTemp351);const auto nativeStatementTemp348=nativeStatementTemp350;Args nativeStatementTemp349;nativeStatementTemp349.push_back(nativeStatementTemp283.get("_sphereHitAt"));return nativeStatementTemp347.call(nativeStatementTemp348,nativeStatementTemp349);}());const auto nativeStatementTemp346=([&]()->JSValue{const auto nativeStatementTemp354=([&]()->JSValue{const auto nativeStatementTemp356=([&]()->JSValue{const auto nativeStatementTemp358=raycaster;const auto nativeStatementTemp359=JSValue("far");return nativeStatementTemp358.get(nativeStatementTemp359);}());const auto nativeStatementTemp357=([&]()->JSValue{const auto nativeStatementTemp360=raycaster;const auto nativeStatementTemp361=JSValue("near");return nativeStatementTemp360.get(nativeStatementTemp361);}());return binary("-",nativeStatementTemp356,nativeStatementTemp357);}());const auto nativeStatementTemp355=JSValue(2);return binary("**",nativeStatementTemp354,nativeStatementTemp355);}());return binary(">",nativeStatementTemp345,nativeStatementTemp346);}()).toBoolean())return JSValue{};

}
([&]()->JSValue{auto nativeStatementTemp365=([&]()->JSValue{auto nativeStatementTemp370=nativeStatementTemp283.get("_inverseMatrix");const auto nativeStatementTemp371=JSValue("copy");const auto nativeStatementTemp367=nativeStatementTemp370.get(nativeStatementTemp371);const auto nativeStatementTemp368=nativeStatementTemp370;Args nativeStatementTemp369;nativeStatementTemp369.push_back(matrixWorld);return nativeStatementTemp367.call(nativeStatementTemp368,nativeStatementTemp369);}());const auto nativeStatementTemp366=JSValue("invert");const auto nativeStatementTemp362=nativeStatementTemp365.get(nativeStatementTemp366);const auto nativeStatementTemp363=nativeStatementTemp365;Args nativeStatementTemp364;return nativeStatementTemp362.call(nativeStatementTemp363,nativeStatementTemp364);}());
([&]()->JSValue{auto nativeStatementTemp375=([&]()->JSValue{auto nativeStatementTemp380=nativeStatementTemp283.get("_ray");const auto nativeStatementTemp381=JSValue("copy");const auto nativeStatementTemp377=nativeStatementTemp380.get(nativeStatementTemp381);const auto nativeStatementTemp378=nativeStatementTemp380;Args nativeStatementTemp379;nativeStatementTemp379.push_back(([&]()->JSValue{const auto nativeStatementTemp382=raycaster;const auto nativeStatementTemp383=JSValue("ray");return nativeStatementTemp382.get(nativeStatementTemp383);}()));return nativeStatementTemp377.call(nativeStatementTemp378,nativeStatementTemp379);}());const auto nativeStatementTemp376=JSValue("applyMatrix4");const auto nativeStatementTemp372=nativeStatementTemp375.get(nativeStatementTemp376);const auto nativeStatementTemp373=nativeStatementTemp375;Args nativeStatementTemp374;nativeStatementTemp374.push_back(nativeStatementTemp283.get("_inverseMatrix"));return nativeStatementTemp372.call(nativeStatementTemp373,nativeStatementTemp374);}());
if(([&]()->JSValue{const auto nativeStatementTemp384=([&]()->JSValue{const auto nativeStatementTemp386=geometry;const auto nativeStatementTemp387=JSValue("boundingBox");return nativeStatementTemp386.get(nativeStatementTemp387);}());const auto nativeStatementTemp385=JSValue(nullptr);return binary("!==",nativeStatementTemp384,nativeStatementTemp385);}()).toBoolean()){
if(([&]()->JSValue{const auto nativeStatementTemp388=([&]()->JSValue{auto nativeStatementTemp393=nativeStatementTemp283.get("_ray");const auto nativeStatementTemp394=JSValue("intersectsBox");const auto nativeStatementTemp390=nativeStatementTemp393.get(nativeStatementTemp394);const auto nativeStatementTemp391=nativeStatementTemp393;Args nativeStatementTemp392;nativeStatementTemp392.push_back(([&]()->JSValue{const auto nativeStatementTemp395=geometry;const auto nativeStatementTemp396=JSValue("boundingBox");return nativeStatementTemp395.get(nativeStatementTemp396);}()));return nativeStatementTemp390.call(nativeStatementTemp391,nativeStatementTemp392);}());const auto nativeStatementTemp389=JSValue(false);return binary("===",nativeStatementTemp388,nativeStatementTemp389);}()).toBoolean())return JSValue{};

}
([&]()->JSValue{auto nativeStatementTemp400=nativeStatementTemp284;const auto nativeStatementTemp401=JSValue("_computeIntersections");const auto nativeStatementTemp397=nativeStatementTemp400.get(nativeStatementTemp401);const auto nativeStatementTemp398=nativeStatementTemp400;Args nativeStatementTemp399;nativeStatementTemp399.push_back(raycaster);nativeStatementTemp399.push_back(intersects);nativeStatementTemp399.push_back(nativeStatementTemp283.get("_ray"));return nativeStatementTemp397.call(nativeStatementTemp398,nativeStatementTemp399);}());
return JSValue{};
}

JSValue method6(const JSValue& nativeStatementTemp402,const Args& nativeStatementTemp403,const Args& nativeStatementTemp404){
auto nativeStatementTemp405=nativeStatementTemp404[0];auto nativeStatementTemp406=nativeStatementTemp402;
JSValue raycaster=arg(nativeStatementTemp403,0);
JSValue intersects=arg(nativeStatementTemp403,1);
JSValue rayLocalSpace=arg(nativeStatementTemp403,2);
JSValue intersection=JSValue{};
JSValue geometry=([&]()->JSValue{const auto nativeStatementTemp408=nativeStatementTemp406;const auto nativeStatementTemp409=JSValue("geometry");return nativeStatementTemp408.get(nativeStatementTemp409);}());
JSValue material=([&]()->JSValue{const auto nativeStatementTemp410=nativeStatementTemp406;const auto nativeStatementTemp411=JSValue("material");return nativeStatementTemp410.get(nativeStatementTemp411);}());
JSValue index=([&]()->JSValue{const auto nativeStatementTemp412=geometry;const auto nativeStatementTemp413=JSValue("index");return nativeStatementTemp412.get(nativeStatementTemp413);}());
JSValue position=([&]()->JSValue{const auto nativeStatementTemp414=([&]()->JSValue{const auto nativeStatementTemp416=geometry;const auto nativeStatementTemp417=JSValue("attributes");return nativeStatementTemp416.get(nativeStatementTemp417);}());const auto nativeStatementTemp415=JSValue("position");return nativeStatementTemp414.get(nativeStatementTemp415);}());
JSValue uv=([&]()->JSValue{const auto nativeStatementTemp418=([&]()->JSValue{const auto nativeStatementTemp420=geometry;const auto nativeStatementTemp421=JSValue("attributes");return nativeStatementTemp420.get(nativeStatementTemp421);}());const auto nativeStatementTemp419=JSValue("uv");return nativeStatementTemp418.get(nativeStatementTemp419);}());
JSValue uv1=([&]()->JSValue{const auto nativeStatementTemp422=([&]()->JSValue{const auto nativeStatementTemp424=geometry;const auto nativeStatementTemp425=JSValue("attributes");return nativeStatementTemp424.get(nativeStatementTemp425);}());const auto nativeStatementTemp423=JSValue("uv1");return nativeStatementTemp422.get(nativeStatementTemp423);}());
JSValue normal=([&]()->JSValue{const auto nativeStatementTemp426=([&]()->JSValue{const auto nativeStatementTemp428=geometry;const auto nativeStatementTemp429=JSValue("attributes");return nativeStatementTemp428.get(nativeStatementTemp429);}());const auto nativeStatementTemp427=JSValue("normal");return nativeStatementTemp426.get(nativeStatementTemp427);}());
JSValue groups=([&]()->JSValue{const auto nativeStatementTemp430=geometry;const auto nativeStatementTemp431=JSValue("groups");return nativeStatementTemp430.get(nativeStatementTemp431);}());
JSValue drawRange=([&]()->JSValue{const auto nativeStatementTemp432=geometry;const auto nativeStatementTemp433=JSValue("drawRange");return nativeStatementTemp432.get(nativeStatementTemp433);}());
if(([&]()->JSValue{const auto nativeStatementTemp434=index;const auto nativeStatementTemp435=JSValue(nullptr);return binary("!==",nativeStatementTemp434,nativeStatementTemp435);}()).toBoolean()){
if(([&]()->JSValue{auto nativeStatementTemp439=nativeGlobalBinding("Array");const auto nativeStatementTemp440=JSValue("isArray");const auto nativeStatementTemp436=nativeStatementTemp439.get(nativeStatementTemp440);const auto nativeStatementTemp437=nativeStatementTemp439;Args nativeStatementTemp438;nativeStatementTemp438.push_back(material);return nativeStatementTemp436.call(nativeStatementTemp437,nativeStatementTemp438);}()).toBoolean()){
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp441=groups;const auto nativeStatementTemp442=JSValue("length");return nativeStatementTemp441.get(nativeStatementTemp442);}());
for(;([&]()->JSValue{const auto nativeStatementTemp443=i;const auto nativeStatementTemp444=il;return binary("<",nativeStatementTemp443,nativeStatementTemp444);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp445=i.toNumber();const auto nativeStatementTemp446=nativeStatementTemp445+1;i=nativeStatementTemp446;return nativeStatementTemp445;}())){
JSValue group=([&]()->JSValue{const auto nativeStatementTemp447=groups;const auto nativeStatementTemp448=i;return nativeStatementTemp447.get(nativeStatementTemp448);}());
JSValue groupMaterial=([&]()->JSValue{const auto nativeStatementTemp449=material;const auto nativeStatementTemp450=([&]()->JSValue{const auto nativeStatementTemp451=group;const auto nativeStatementTemp452=JSValue("materialIndex");return nativeStatementTemp451.get(nativeStatementTemp452);}());return nativeStatementTemp449.get(nativeStatementTemp450);}());
JSValue start=([&]()->JSValue{auto nativeStatementTemp456=nativeGlobalBinding("Math");const auto nativeStatementTemp457=JSValue("max");const auto nativeStatementTemp453=nativeStatementTemp456.get(nativeStatementTemp457);const auto nativeStatementTemp454=nativeStatementTemp456;Args nativeStatementTemp455;nativeStatementTemp455.push_back(([&]()->JSValue{const auto nativeStatementTemp458=group;const auto nativeStatementTemp459=JSValue("start");return nativeStatementTemp458.get(nativeStatementTemp459);}()));nativeStatementTemp455.push_back(([&]()->JSValue{const auto nativeStatementTemp460=drawRange;const auto nativeStatementTemp461=JSValue("start");return nativeStatementTemp460.get(nativeStatementTemp461);}()));return nativeStatementTemp453.call(nativeStatementTemp454,nativeStatementTemp455);}());
JSValue end=([&]()->JSValue{auto nativeStatementTemp465=nativeGlobalBinding("Math");const auto nativeStatementTemp466=JSValue("min");const auto nativeStatementTemp462=nativeStatementTemp465.get(nativeStatementTemp466);const auto nativeStatementTemp463=nativeStatementTemp465;Args nativeStatementTemp464;nativeStatementTemp464.push_back(([&]()->JSValue{const auto nativeStatementTemp467=index;const auto nativeStatementTemp468=JSValue("count");return nativeStatementTemp467.get(nativeStatementTemp468);}()));nativeStatementTemp464.push_back(([&]()->JSValue{auto nativeStatementTemp472=nativeGlobalBinding("Math");const auto nativeStatementTemp473=JSValue("min");const auto nativeStatementTemp469=nativeStatementTemp472.get(nativeStatementTemp473);const auto nativeStatementTemp470=nativeStatementTemp472;Args nativeStatementTemp471;nativeStatementTemp471.push_back(([&]()->JSValue{const auto nativeStatementTemp474=([&]()->JSValue{const auto nativeStatementTemp476=group;const auto nativeStatementTemp477=JSValue("start");return nativeStatementTemp476.get(nativeStatementTemp477);}());const auto nativeStatementTemp475=([&]()->JSValue{const auto nativeStatementTemp478=group;const auto nativeStatementTemp479=JSValue("count");return nativeStatementTemp478.get(nativeStatementTemp479);}());return binary("+",nativeStatementTemp474,nativeStatementTemp475);}()));nativeStatementTemp471.push_back(([&]()->JSValue{const auto nativeStatementTemp480=([&]()->JSValue{const auto nativeStatementTemp482=drawRange;const auto nativeStatementTemp483=JSValue("start");return nativeStatementTemp482.get(nativeStatementTemp483);}());const auto nativeStatementTemp481=([&]()->JSValue{const auto nativeStatementTemp484=drawRange;const auto nativeStatementTemp485=JSValue("count");return nativeStatementTemp484.get(nativeStatementTemp485);}());return binary("+",nativeStatementTemp480,nativeStatementTemp481);}()));return nativeStatementTemp469.call(nativeStatementTemp470,nativeStatementTemp471);}()));return nativeStatementTemp462.call(nativeStatementTemp463,nativeStatementTemp464);}());
{JSValue j=start;
JSValue jl=end;
for(;([&]()->JSValue{const auto nativeStatementTemp486=j;const auto nativeStatementTemp487=jl;return binary("<",nativeStatementTemp486,nativeStatementTemp487);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp489=j;const auto nativeStatementTemp490=JSValue(3);const auto nativeStatementTemp488=binary("+",nativeStatementTemp489,nativeStatementTemp490);j=nativeStatementTemp488;return nativeStatementTemp488;}())){
JSValue a=([&]()->JSValue{auto nativeStatementTemp494=index;const auto nativeStatementTemp495=JSValue("getX");const auto nativeStatementTemp491=nativeStatementTemp494.get(nativeStatementTemp495);const auto nativeStatementTemp492=nativeStatementTemp494;Args nativeStatementTemp493;nativeStatementTemp493.push_back(j);return nativeStatementTemp491.call(nativeStatementTemp492,nativeStatementTemp493);}());
JSValue b=([&]()->JSValue{auto nativeStatementTemp499=index;const auto nativeStatementTemp500=JSValue("getX");const auto nativeStatementTemp496=nativeStatementTemp499.get(nativeStatementTemp500);const auto nativeStatementTemp497=nativeStatementTemp499;Args nativeStatementTemp498;nativeStatementTemp498.push_back(([&]()->JSValue{const auto nativeStatementTemp501=j;const auto nativeStatementTemp502=JSValue(1);return binary("+",nativeStatementTemp501,nativeStatementTemp502);}()));return nativeStatementTemp496.call(nativeStatementTemp497,nativeStatementTemp498);}());
JSValue c=([&]()->JSValue{auto nativeStatementTemp506=index;const auto nativeStatementTemp507=JSValue("getX");const auto nativeStatementTemp503=nativeStatementTemp506.get(nativeStatementTemp507);const auto nativeStatementTemp504=nativeStatementTemp506;Args nativeStatementTemp505;nativeStatementTemp505.push_back(([&]()->JSValue{const auto nativeStatementTemp508=j;const auto nativeStatementTemp509=JSValue(2);return binary("+",nativeStatementTemp508,nativeStatementTemp509);}()));return nativeStatementTemp503.call(nativeStatementTemp504,nativeStatementTemp505);}());
([&]()->JSValue{const auto nativeStatementTemp510=([&]()->JSValue{const auto nativeStatementTemp513=nativeStatementTemp405.get("checkGeometryIntersection");const JSValue nativeStatementTemp514;Args nativeStatementTemp515;nativeStatementTemp515.push_back(nativeStatementTemp406);nativeStatementTemp515.push_back(groupMaterial);nativeStatementTemp515.push_back(raycaster);nativeStatementTemp515.push_back(rayLocalSpace);nativeStatementTemp515.push_back(uv);nativeStatementTemp515.push_back(uv1);nativeStatementTemp515.push_back(normal);nativeStatementTemp515.push_back(a);nativeStatementTemp515.push_back(b);nativeStatementTemp515.push_back(c);return nativeStatementTemp513.call(nativeStatementTemp514,nativeStatementTemp515);}());intersection=nativeStatementTemp510;return nativeStatementTemp510;}());
if(intersection.toBoolean()){
([&]()->JSValue{auto nativeStatementTemp516=intersection;const auto nativeStatementTemp517=JSValue("faceIndex");const auto nativeStatementTemp518=([&]()->JSValue{auto nativeStatementTemp524=nativeGlobalBinding("Math");const auto nativeStatementTemp525=JSValue("floor");const auto nativeStatementTemp521=nativeStatementTemp524.get(nativeStatementTemp525);const auto nativeStatementTemp522=nativeStatementTemp524;Args nativeStatementTemp523;nativeStatementTemp523.push_back(([&]()->JSValue{const auto nativeStatementTemp526=j;const auto nativeStatementTemp527=JSValue(3);return binary("/",nativeStatementTemp526,nativeStatementTemp527);}()));return nativeStatementTemp521.call(nativeStatementTemp522,nativeStatementTemp523);}());setReference(nativeStatementTemp516,nativeStatementTemp517,nativeStatementTemp518);return nativeStatementTemp518;}());
([&]()->JSValue{auto nativeStatementTemp528=([&]()->JSValue{const auto nativeStatementTemp530=intersection;const auto nativeStatementTemp531=JSValue("face");return nativeStatementTemp530.get(nativeStatementTemp531);}());const auto nativeStatementTemp529=JSValue("materialIndex");const auto nativeStatementTemp532=([&]()->JSValue{const auto nativeStatementTemp535=group;const auto nativeStatementTemp536=JSValue("materialIndex");return nativeStatementTemp535.get(nativeStatementTemp536);}());setReference(nativeStatementTemp528,nativeStatementTemp529,nativeStatementTemp532);return nativeStatementTemp532;}());
([&]()->JSValue{auto nativeStatementTemp540=intersects;const auto nativeStatementTemp541=JSValue("push");const auto nativeStatementTemp537=nativeStatementTemp540.get(nativeStatementTemp541);const auto nativeStatementTemp538=nativeStatementTemp540;Args nativeStatementTemp539;nativeStatementTemp539.push_back(intersection);return nativeStatementTemp537.call(nativeStatementTemp538,nativeStatementTemp539);}());

}

}
}

}
}

}
else {
JSValue start=([&]()->JSValue{auto nativeStatementTemp545=nativeGlobalBinding("Math");const auto nativeStatementTemp546=JSValue("max");const auto nativeStatementTemp542=nativeStatementTemp545.get(nativeStatementTemp546);const auto nativeStatementTemp543=nativeStatementTemp545;Args nativeStatementTemp544;nativeStatementTemp544.push_back(JSValue(0));nativeStatementTemp544.push_back(([&]()->JSValue{const auto nativeStatementTemp547=drawRange;const auto nativeStatementTemp548=JSValue("start");return nativeStatementTemp547.get(nativeStatementTemp548);}()));return nativeStatementTemp542.call(nativeStatementTemp543,nativeStatementTemp544);}());
JSValue end=([&]()->JSValue{auto nativeStatementTemp552=nativeGlobalBinding("Math");const auto nativeStatementTemp553=JSValue("min");const auto nativeStatementTemp549=nativeStatementTemp552.get(nativeStatementTemp553);const auto nativeStatementTemp550=nativeStatementTemp552;Args nativeStatementTemp551;nativeStatementTemp551.push_back(([&]()->JSValue{const auto nativeStatementTemp554=index;const auto nativeStatementTemp555=JSValue("count");return nativeStatementTemp554.get(nativeStatementTemp555);}()));nativeStatementTemp551.push_back(([&]()->JSValue{const auto nativeStatementTemp556=([&]()->JSValue{const auto nativeStatementTemp558=drawRange;const auto nativeStatementTemp559=JSValue("start");return nativeStatementTemp558.get(nativeStatementTemp559);}());const auto nativeStatementTemp557=([&]()->JSValue{const auto nativeStatementTemp560=drawRange;const auto nativeStatementTemp561=JSValue("count");return nativeStatementTemp560.get(nativeStatementTemp561);}());return binary("+",nativeStatementTemp556,nativeStatementTemp557);}()));return nativeStatementTemp549.call(nativeStatementTemp550,nativeStatementTemp551);}());
{JSValue i=start;
JSValue il=end;
for(;([&]()->JSValue{const auto nativeStatementTemp562=i;const auto nativeStatementTemp563=il;return binary("<",nativeStatementTemp562,nativeStatementTemp563);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp565=i;const auto nativeStatementTemp566=JSValue(3);const auto nativeStatementTemp564=binary("+",nativeStatementTemp565,nativeStatementTemp566);i=nativeStatementTemp564;return nativeStatementTemp564;}())){
JSValue a=([&]()->JSValue{auto nativeStatementTemp570=index;const auto nativeStatementTemp571=JSValue("getX");const auto nativeStatementTemp567=nativeStatementTemp570.get(nativeStatementTemp571);const auto nativeStatementTemp568=nativeStatementTemp570;Args nativeStatementTemp569;nativeStatementTemp569.push_back(i);return nativeStatementTemp567.call(nativeStatementTemp568,nativeStatementTemp569);}());
JSValue b=([&]()->JSValue{auto nativeStatementTemp575=index;const auto nativeStatementTemp576=JSValue("getX");const auto nativeStatementTemp572=nativeStatementTemp575.get(nativeStatementTemp576);const auto nativeStatementTemp573=nativeStatementTemp575;Args nativeStatementTemp574;nativeStatementTemp574.push_back(([&]()->JSValue{const auto nativeStatementTemp577=i;const auto nativeStatementTemp578=JSValue(1);return binary("+",nativeStatementTemp577,nativeStatementTemp578);}()));return nativeStatementTemp572.call(nativeStatementTemp573,nativeStatementTemp574);}());
JSValue c=([&]()->JSValue{auto nativeStatementTemp582=index;const auto nativeStatementTemp583=JSValue("getX");const auto nativeStatementTemp579=nativeStatementTemp582.get(nativeStatementTemp583);const auto nativeStatementTemp580=nativeStatementTemp582;Args nativeStatementTemp581;nativeStatementTemp581.push_back(([&]()->JSValue{const auto nativeStatementTemp584=i;const auto nativeStatementTemp585=JSValue(2);return binary("+",nativeStatementTemp584,nativeStatementTemp585);}()));return nativeStatementTemp579.call(nativeStatementTemp580,nativeStatementTemp581);}());
([&]()->JSValue{const auto nativeStatementTemp586=([&]()->JSValue{const auto nativeStatementTemp589=nativeStatementTemp405.get("checkGeometryIntersection");const JSValue nativeStatementTemp590;Args nativeStatementTemp591;nativeStatementTemp591.push_back(nativeStatementTemp406);nativeStatementTemp591.push_back(material);nativeStatementTemp591.push_back(raycaster);nativeStatementTemp591.push_back(rayLocalSpace);nativeStatementTemp591.push_back(uv);nativeStatementTemp591.push_back(uv1);nativeStatementTemp591.push_back(normal);nativeStatementTemp591.push_back(a);nativeStatementTemp591.push_back(b);nativeStatementTemp591.push_back(c);return nativeStatementTemp589.call(nativeStatementTemp590,nativeStatementTemp591);}());intersection=nativeStatementTemp586;return nativeStatementTemp586;}());
if(intersection.toBoolean()){
([&]()->JSValue{auto nativeStatementTemp592=intersection;const auto nativeStatementTemp593=JSValue("faceIndex");const auto nativeStatementTemp594=([&]()->JSValue{auto nativeStatementTemp600=nativeGlobalBinding("Math");const auto nativeStatementTemp601=JSValue("floor");const auto nativeStatementTemp597=nativeStatementTemp600.get(nativeStatementTemp601);const auto nativeStatementTemp598=nativeStatementTemp600;Args nativeStatementTemp599;nativeStatementTemp599.push_back(([&]()->JSValue{const auto nativeStatementTemp602=i;const auto nativeStatementTemp603=JSValue(3);return binary("/",nativeStatementTemp602,nativeStatementTemp603);}()));return nativeStatementTemp597.call(nativeStatementTemp598,nativeStatementTemp599);}());setReference(nativeStatementTemp592,nativeStatementTemp593,nativeStatementTemp594);return nativeStatementTemp594;}());
([&]()->JSValue{auto nativeStatementTemp607=intersects;const auto nativeStatementTemp608=JSValue("push");const auto nativeStatementTemp604=nativeStatementTemp607.get(nativeStatementTemp608);const auto nativeStatementTemp605=nativeStatementTemp607;Args nativeStatementTemp606;nativeStatementTemp606.push_back(intersection);return nativeStatementTemp604.call(nativeStatementTemp605,nativeStatementTemp606);}());

}

}
}

}

}
else if(([&]()->JSValue{const auto nativeStatementTemp609=position;const auto nativeStatementTemp610=JSValue{};return binary("!==",nativeStatementTemp609,nativeStatementTemp610);}()).toBoolean()){
if(([&]()->JSValue{auto nativeStatementTemp614=nativeGlobalBinding("Array");const auto nativeStatementTemp615=JSValue("isArray");const auto nativeStatementTemp611=nativeStatementTemp614.get(nativeStatementTemp615);const auto nativeStatementTemp612=nativeStatementTemp614;Args nativeStatementTemp613;nativeStatementTemp613.push_back(material);return nativeStatementTemp611.call(nativeStatementTemp612,nativeStatementTemp613);}()).toBoolean()){
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp616=groups;const auto nativeStatementTemp617=JSValue("length");return nativeStatementTemp616.get(nativeStatementTemp617);}());
for(;([&]()->JSValue{const auto nativeStatementTemp618=i;const auto nativeStatementTemp619=il;return binary("<",nativeStatementTemp618,nativeStatementTemp619);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp620=i.toNumber();const auto nativeStatementTemp621=nativeStatementTemp620+1;i=nativeStatementTemp621;return nativeStatementTemp620;}())){
JSValue group=([&]()->JSValue{const auto nativeStatementTemp622=groups;const auto nativeStatementTemp623=i;return nativeStatementTemp622.get(nativeStatementTemp623);}());
JSValue groupMaterial=([&]()->JSValue{const auto nativeStatementTemp624=material;const auto nativeStatementTemp625=([&]()->JSValue{const auto nativeStatementTemp626=group;const auto nativeStatementTemp627=JSValue("materialIndex");return nativeStatementTemp626.get(nativeStatementTemp627);}());return nativeStatementTemp624.get(nativeStatementTemp625);}());
JSValue start=([&]()->JSValue{auto nativeStatementTemp631=nativeGlobalBinding("Math");const auto nativeStatementTemp632=JSValue("max");const auto nativeStatementTemp628=nativeStatementTemp631.get(nativeStatementTemp632);const auto nativeStatementTemp629=nativeStatementTemp631;Args nativeStatementTemp630;nativeStatementTemp630.push_back(([&]()->JSValue{const auto nativeStatementTemp633=group;const auto nativeStatementTemp634=JSValue("start");return nativeStatementTemp633.get(nativeStatementTemp634);}()));nativeStatementTemp630.push_back(([&]()->JSValue{const auto nativeStatementTemp635=drawRange;const auto nativeStatementTemp636=JSValue("start");return nativeStatementTemp635.get(nativeStatementTemp636);}()));return nativeStatementTemp628.call(nativeStatementTemp629,nativeStatementTemp630);}());
JSValue end=([&]()->JSValue{auto nativeStatementTemp640=nativeGlobalBinding("Math");const auto nativeStatementTemp641=JSValue("min");const auto nativeStatementTemp637=nativeStatementTemp640.get(nativeStatementTemp641);const auto nativeStatementTemp638=nativeStatementTemp640;Args nativeStatementTemp639;nativeStatementTemp639.push_back(([&]()->JSValue{const auto nativeStatementTemp642=position;const auto nativeStatementTemp643=JSValue("count");return nativeStatementTemp642.get(nativeStatementTemp643);}()));nativeStatementTemp639.push_back(([&]()->JSValue{auto nativeStatementTemp647=nativeGlobalBinding("Math");const auto nativeStatementTemp648=JSValue("min");const auto nativeStatementTemp644=nativeStatementTemp647.get(nativeStatementTemp648);const auto nativeStatementTemp645=nativeStatementTemp647;Args nativeStatementTemp646;nativeStatementTemp646.push_back(([&]()->JSValue{const auto nativeStatementTemp649=([&]()->JSValue{const auto nativeStatementTemp651=group;const auto nativeStatementTemp652=JSValue("start");return nativeStatementTemp651.get(nativeStatementTemp652);}());const auto nativeStatementTemp650=([&]()->JSValue{const auto nativeStatementTemp653=group;const auto nativeStatementTemp654=JSValue("count");return nativeStatementTemp653.get(nativeStatementTemp654);}());return binary("+",nativeStatementTemp649,nativeStatementTemp650);}()));nativeStatementTemp646.push_back(([&]()->JSValue{const auto nativeStatementTemp655=([&]()->JSValue{const auto nativeStatementTemp657=drawRange;const auto nativeStatementTemp658=JSValue("start");return nativeStatementTemp657.get(nativeStatementTemp658);}());const auto nativeStatementTemp656=([&]()->JSValue{const auto nativeStatementTemp659=drawRange;const auto nativeStatementTemp660=JSValue("count");return nativeStatementTemp659.get(nativeStatementTemp660);}());return binary("+",nativeStatementTemp655,nativeStatementTemp656);}()));return nativeStatementTemp644.call(nativeStatementTemp645,nativeStatementTemp646);}()));return nativeStatementTemp637.call(nativeStatementTemp638,nativeStatementTemp639);}());
{JSValue j=start;
JSValue jl=end;
for(;([&]()->JSValue{const auto nativeStatementTemp661=j;const auto nativeStatementTemp662=jl;return binary("<",nativeStatementTemp661,nativeStatementTemp662);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp664=j;const auto nativeStatementTemp665=JSValue(3);const auto nativeStatementTemp663=binary("+",nativeStatementTemp664,nativeStatementTemp665);j=nativeStatementTemp663;return nativeStatementTemp663;}())){
JSValue a=j;
JSValue b=([&]()->JSValue{const auto nativeStatementTemp666=j;const auto nativeStatementTemp667=JSValue(1);return binary("+",nativeStatementTemp666,nativeStatementTemp667);}());
JSValue c=([&]()->JSValue{const auto nativeStatementTemp668=j;const auto nativeStatementTemp669=JSValue(2);return binary("+",nativeStatementTemp668,nativeStatementTemp669);}());
([&]()->JSValue{const auto nativeStatementTemp670=([&]()->JSValue{const auto nativeStatementTemp673=nativeStatementTemp405.get("checkGeometryIntersection");const JSValue nativeStatementTemp674;Args nativeStatementTemp675;nativeStatementTemp675.push_back(nativeStatementTemp406);nativeStatementTemp675.push_back(groupMaterial);nativeStatementTemp675.push_back(raycaster);nativeStatementTemp675.push_back(rayLocalSpace);nativeStatementTemp675.push_back(uv);nativeStatementTemp675.push_back(uv1);nativeStatementTemp675.push_back(normal);nativeStatementTemp675.push_back(a);nativeStatementTemp675.push_back(b);nativeStatementTemp675.push_back(c);return nativeStatementTemp673.call(nativeStatementTemp674,nativeStatementTemp675);}());intersection=nativeStatementTemp670;return nativeStatementTemp670;}());
if(intersection.toBoolean()){
([&]()->JSValue{auto nativeStatementTemp676=intersection;const auto nativeStatementTemp677=JSValue("faceIndex");const auto nativeStatementTemp678=([&]()->JSValue{auto nativeStatementTemp684=nativeGlobalBinding("Math");const auto nativeStatementTemp685=JSValue("floor");const auto nativeStatementTemp681=nativeStatementTemp684.get(nativeStatementTemp685);const auto nativeStatementTemp682=nativeStatementTemp684;Args nativeStatementTemp683;nativeStatementTemp683.push_back(([&]()->JSValue{const auto nativeStatementTemp686=j;const auto nativeStatementTemp687=JSValue(3);return binary("/",nativeStatementTemp686,nativeStatementTemp687);}()));return nativeStatementTemp681.call(nativeStatementTemp682,nativeStatementTemp683);}());setReference(nativeStatementTemp676,nativeStatementTemp677,nativeStatementTemp678);return nativeStatementTemp678;}());
([&]()->JSValue{auto nativeStatementTemp688=([&]()->JSValue{const auto nativeStatementTemp690=intersection;const auto nativeStatementTemp691=JSValue("face");return nativeStatementTemp690.get(nativeStatementTemp691);}());const auto nativeStatementTemp689=JSValue("materialIndex");const auto nativeStatementTemp692=([&]()->JSValue{const auto nativeStatementTemp695=group;const auto nativeStatementTemp696=JSValue("materialIndex");return nativeStatementTemp695.get(nativeStatementTemp696);}());setReference(nativeStatementTemp688,nativeStatementTemp689,nativeStatementTemp692);return nativeStatementTemp692;}());
([&]()->JSValue{auto nativeStatementTemp700=intersects;const auto nativeStatementTemp701=JSValue("push");const auto nativeStatementTemp697=nativeStatementTemp700.get(nativeStatementTemp701);const auto nativeStatementTemp698=nativeStatementTemp700;Args nativeStatementTemp699;nativeStatementTemp699.push_back(intersection);return nativeStatementTemp697.call(nativeStatementTemp698,nativeStatementTemp699);}());

}

}
}

}
}

}
else {
JSValue start=([&]()->JSValue{auto nativeStatementTemp705=nativeGlobalBinding("Math");const auto nativeStatementTemp706=JSValue("max");const auto nativeStatementTemp702=nativeStatementTemp705.get(nativeStatementTemp706);const auto nativeStatementTemp703=nativeStatementTemp705;Args nativeStatementTemp704;nativeStatementTemp704.push_back(JSValue(0));nativeStatementTemp704.push_back(([&]()->JSValue{const auto nativeStatementTemp707=drawRange;const auto nativeStatementTemp708=JSValue("start");return nativeStatementTemp707.get(nativeStatementTemp708);}()));return nativeStatementTemp702.call(nativeStatementTemp703,nativeStatementTemp704);}());
JSValue end=([&]()->JSValue{auto nativeStatementTemp712=nativeGlobalBinding("Math");const auto nativeStatementTemp713=JSValue("min");const auto nativeStatementTemp709=nativeStatementTemp712.get(nativeStatementTemp713);const auto nativeStatementTemp710=nativeStatementTemp712;Args nativeStatementTemp711;nativeStatementTemp711.push_back(([&]()->JSValue{const auto nativeStatementTemp714=position;const auto nativeStatementTemp715=JSValue("count");return nativeStatementTemp714.get(nativeStatementTemp715);}()));nativeStatementTemp711.push_back(([&]()->JSValue{const auto nativeStatementTemp716=([&]()->JSValue{const auto nativeStatementTemp718=drawRange;const auto nativeStatementTemp719=JSValue("start");return nativeStatementTemp718.get(nativeStatementTemp719);}());const auto nativeStatementTemp717=([&]()->JSValue{const auto nativeStatementTemp720=drawRange;const auto nativeStatementTemp721=JSValue("count");return nativeStatementTemp720.get(nativeStatementTemp721);}());return binary("+",nativeStatementTemp716,nativeStatementTemp717);}()));return nativeStatementTemp709.call(nativeStatementTemp710,nativeStatementTemp711);}());
{JSValue i=start;
JSValue il=end;
for(;([&]()->JSValue{const auto nativeStatementTemp722=i;const auto nativeStatementTemp723=il;return binary("<",nativeStatementTemp722,nativeStatementTemp723);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp725=i;const auto nativeStatementTemp726=JSValue(3);const auto nativeStatementTemp724=binary("+",nativeStatementTemp725,nativeStatementTemp726);i=nativeStatementTemp724;return nativeStatementTemp724;}())){
JSValue a=i;
JSValue b=([&]()->JSValue{const auto nativeStatementTemp727=i;const auto nativeStatementTemp728=JSValue(1);return binary("+",nativeStatementTemp727,nativeStatementTemp728);}());
JSValue c=([&]()->JSValue{const auto nativeStatementTemp729=i;const auto nativeStatementTemp730=JSValue(2);return binary("+",nativeStatementTemp729,nativeStatementTemp730);}());
([&]()->JSValue{const auto nativeStatementTemp731=([&]()->JSValue{const auto nativeStatementTemp734=nativeStatementTemp405.get("checkGeometryIntersection");const JSValue nativeStatementTemp735;Args nativeStatementTemp736;nativeStatementTemp736.push_back(nativeStatementTemp406);nativeStatementTemp736.push_back(material);nativeStatementTemp736.push_back(raycaster);nativeStatementTemp736.push_back(rayLocalSpace);nativeStatementTemp736.push_back(uv);nativeStatementTemp736.push_back(uv1);nativeStatementTemp736.push_back(normal);nativeStatementTemp736.push_back(a);nativeStatementTemp736.push_back(b);nativeStatementTemp736.push_back(c);return nativeStatementTemp734.call(nativeStatementTemp735,nativeStatementTemp736);}());intersection=nativeStatementTemp731;return nativeStatementTemp731;}());
if(intersection.toBoolean()){
([&]()->JSValue{auto nativeStatementTemp737=intersection;const auto nativeStatementTemp738=JSValue("faceIndex");const auto nativeStatementTemp739=([&]()->JSValue{auto nativeStatementTemp745=nativeGlobalBinding("Math");const auto nativeStatementTemp746=JSValue("floor");const auto nativeStatementTemp742=nativeStatementTemp745.get(nativeStatementTemp746);const auto nativeStatementTemp743=nativeStatementTemp745;Args nativeStatementTemp744;nativeStatementTemp744.push_back(([&]()->JSValue{const auto nativeStatementTemp747=i;const auto nativeStatementTemp748=JSValue(3);return binary("/",nativeStatementTemp747,nativeStatementTemp748);}()));return nativeStatementTemp742.call(nativeStatementTemp743,nativeStatementTemp744);}());setReference(nativeStatementTemp737,nativeStatementTemp738,nativeStatementTemp739);return nativeStatementTemp739;}());
([&]()->JSValue{auto nativeStatementTemp752=intersects;const auto nativeStatementTemp753=JSValue("push");const auto nativeStatementTemp749=nativeStatementTemp752.get(nativeStatementTemp753);const auto nativeStatementTemp750=nativeStatementTemp752;Args nativeStatementTemp751;nativeStatementTemp751.push_back(intersection);return nativeStatementTemp749.call(nativeStatementTemp750,nativeStatementTemp751);}());

}

}
}

}

}
return JSValue{};
}
JSValue helper0(const JSValue& nativeStatementTemp754,const Args& nativeStatementTemp755,const Args& nativeStatementTemp756){
auto nativeStatementTemp757=nativeStatementTemp756[0];auto nativeStatementTemp758=nativeStatementTemp754;
JSValue object=arg(nativeStatementTemp755,0);
JSValue material=arg(nativeStatementTemp755,1);
JSValue raycaster=arg(nativeStatementTemp755,2);
JSValue ray=arg(nativeStatementTemp755,3);
JSValue pA=arg(nativeStatementTemp755,4);
JSValue pB=arg(nativeStatementTemp755,5);
JSValue pC=arg(nativeStatementTemp755,6);
JSValue point=arg(nativeStatementTemp755,7);
JSValue intersect=JSValue{};
if(([&]()->JSValue{const auto nativeStatementTemp760=([&]()->JSValue{const auto nativeStatementTemp762=material;const auto nativeStatementTemp763=JSValue("side");return nativeStatementTemp762.get(nativeStatementTemp763);}());const auto nativeStatementTemp761=nativeStatementTemp757.get("BackSide");return binary("===",nativeStatementTemp760,nativeStatementTemp761);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp764=([&]()->JSValue{auto nativeStatementTemp770=ray;const auto nativeStatementTemp771=JSValue("intersectTriangle");const auto nativeStatementTemp767=nativeStatementTemp770.get(nativeStatementTemp771);const auto nativeStatementTemp768=nativeStatementTemp770;Args nativeStatementTemp769;nativeStatementTemp769.push_back(pC);nativeStatementTemp769.push_back(pB);nativeStatementTemp769.push_back(pA);nativeStatementTemp769.push_back(JSValue(true));nativeStatementTemp769.push_back(point);return nativeStatementTemp767.call(nativeStatementTemp768,nativeStatementTemp769);}());intersect=nativeStatementTemp764;return nativeStatementTemp764;}());

}
else {
([&]()->JSValue{const auto nativeStatementTemp772=([&]()->JSValue{auto nativeStatementTemp778=ray;const auto nativeStatementTemp779=JSValue("intersectTriangle");const auto nativeStatementTemp775=nativeStatementTemp778.get(nativeStatementTemp779);const auto nativeStatementTemp776=nativeStatementTemp778;Args nativeStatementTemp777;nativeStatementTemp777.push_back(pA);nativeStatementTemp777.push_back(pB);nativeStatementTemp777.push_back(pC);nativeStatementTemp777.push_back(([&]()->JSValue{const auto nativeStatementTemp780=([&]()->JSValue{const auto nativeStatementTemp782=material;const auto nativeStatementTemp783=JSValue("side");return nativeStatementTemp782.get(nativeStatementTemp783);}());const auto nativeStatementTemp781=nativeStatementTemp757.get("FrontSide");return binary("===",nativeStatementTemp780,nativeStatementTemp781);}()));nativeStatementTemp777.push_back(point);return nativeStatementTemp775.call(nativeStatementTemp776,nativeStatementTemp777);}());intersect=nativeStatementTemp772;return nativeStatementTemp772;}());

}
if(([&]()->JSValue{const auto nativeStatementTemp784=intersect;const auto nativeStatementTemp785=JSValue(nullptr);return binary("===",nativeStatementTemp784,nativeStatementTemp785);}()).toBoolean())return JSValue(nullptr);
([&]()->JSValue{auto nativeStatementTemp789=nativeStatementTemp757.get("_intersectionPointWorld");const auto nativeStatementTemp790=JSValue("copy");const auto nativeStatementTemp786=nativeStatementTemp789.get(nativeStatementTemp790);const auto nativeStatementTemp787=nativeStatementTemp789;Args nativeStatementTemp788;nativeStatementTemp788.push_back(point);return nativeStatementTemp786.call(nativeStatementTemp787,nativeStatementTemp788);}());
([&]()->JSValue{auto nativeStatementTemp794=nativeStatementTemp757.get("_intersectionPointWorld");const auto nativeStatementTemp795=JSValue("applyMatrix4");const auto nativeStatementTemp791=nativeStatementTemp794.get(nativeStatementTemp795);const auto nativeStatementTemp792=nativeStatementTemp794;Args nativeStatementTemp793;nativeStatementTemp793.push_back(([&]()->JSValue{const auto nativeStatementTemp796=object;const auto nativeStatementTemp797=JSValue("matrixWorld");return nativeStatementTemp796.get(nativeStatementTemp797);}()));return nativeStatementTemp791.call(nativeStatementTemp792,nativeStatementTemp793);}());
JSValue distance=([&]()->JSValue{auto nativeStatementTemp801=([&]()->JSValue{const auto nativeStatementTemp803=([&]()->JSValue{const auto nativeStatementTemp805=raycaster;const auto nativeStatementTemp806=JSValue("ray");return nativeStatementTemp805.get(nativeStatementTemp806);}());const auto nativeStatementTemp804=JSValue("origin");return nativeStatementTemp803.get(nativeStatementTemp804);}());const auto nativeStatementTemp802=JSValue("distanceTo");const auto nativeStatementTemp798=nativeStatementTemp801.get(nativeStatementTemp802);const auto nativeStatementTemp799=nativeStatementTemp801;Args nativeStatementTemp800;nativeStatementTemp800.push_back(nativeStatementTemp757.get("_intersectionPointWorld"));return nativeStatementTemp798.call(nativeStatementTemp799,nativeStatementTemp800);}());
if(([&]()->JSValue{const auto nativeStatementTemp807=([&]()->JSValue{const auto nativeStatementTemp808=distance;const auto nativeStatementTemp809=([&]()->JSValue{const auto nativeStatementTemp810=raycaster;const auto nativeStatementTemp811=JSValue("near");return nativeStatementTemp810.get(nativeStatementTemp811);}());return binary("<",nativeStatementTemp808,nativeStatementTemp809);}());if(nativeStatementTemp807.toBoolean())return nativeStatementTemp807;return ([&]()->JSValue{const auto nativeStatementTemp812=distance;const auto nativeStatementTemp813=([&]()->JSValue{const auto nativeStatementTemp814=raycaster;const auto nativeStatementTemp815=JSValue("far");return nativeStatementTemp814.get(nativeStatementTemp815);}());return binary(">",nativeStatementTemp812,nativeStatementTemp813);}());}()).toBoolean())return JSValue(nullptr);
return ([&]()->JSValue{auto nativeStatementTemp816=JSValue::object();{const auto nativeStatementTemp817=JSValue("distance");const auto nativeStatementTemp818=distance;own(nativeStatementTemp816,nativeStatementTemp817,nativeStatementTemp818);}{const auto nativeStatementTemp819=JSValue("point");const auto nativeStatementTemp820=([&]()->JSValue{auto nativeStatementTemp824=nativeStatementTemp757.get("_intersectionPointWorld");const auto nativeStatementTemp825=JSValue("clone");const auto nativeStatementTemp821=nativeStatementTemp824.get(nativeStatementTemp825);const auto nativeStatementTemp822=nativeStatementTemp824;Args nativeStatementTemp823;return nativeStatementTemp821.call(nativeStatementTemp822,nativeStatementTemp823);}());own(nativeStatementTemp816,nativeStatementTemp819,nativeStatementTemp820);}{const auto nativeStatementTemp826=JSValue("object");const auto nativeStatementTemp827=object;own(nativeStatementTemp816,nativeStatementTemp826,nativeStatementTemp827);}return nativeStatementTemp816;}());
return JSValue{};
}
JSValue helper1(const JSValue& nativeStatementTemp828,const Args& nativeStatementTemp829,const Args& nativeStatementTemp830){
auto nativeStatementTemp831=nativeStatementTemp830[0];auto nativeStatementTemp832=nativeStatementTemp828;
JSValue object=arg(nativeStatementTemp829,0);
JSValue material=arg(nativeStatementTemp829,1);
JSValue raycaster=arg(nativeStatementTemp829,2);
JSValue ray=arg(nativeStatementTemp829,3);
JSValue uv=arg(nativeStatementTemp829,4);
JSValue uv1=arg(nativeStatementTemp829,5);
JSValue normal=arg(nativeStatementTemp829,6);
JSValue a=arg(nativeStatementTemp829,7);
JSValue b=arg(nativeStatementTemp829,8);
JSValue c=arg(nativeStatementTemp829,9);
([&]()->JSValue{auto nativeStatementTemp837=object;const auto nativeStatementTemp838=JSValue("getVertexPosition");const auto nativeStatementTemp834=nativeStatementTemp837.get(nativeStatementTemp838);const auto nativeStatementTemp835=nativeStatementTemp837;Args nativeStatementTemp836;nativeStatementTemp836.push_back(a);nativeStatementTemp836.push_back(nativeStatementTemp831.get("_vA"));return nativeStatementTemp834.call(nativeStatementTemp835,nativeStatementTemp836);}());
([&]()->JSValue{auto nativeStatementTemp842=object;const auto nativeStatementTemp843=JSValue("getVertexPosition");const auto nativeStatementTemp839=nativeStatementTemp842.get(nativeStatementTemp843);const auto nativeStatementTemp840=nativeStatementTemp842;Args nativeStatementTemp841;nativeStatementTemp841.push_back(b);nativeStatementTemp841.push_back(nativeStatementTemp831.get("_vB"));return nativeStatementTemp839.call(nativeStatementTemp840,nativeStatementTemp841);}());
([&]()->JSValue{auto nativeStatementTemp847=object;const auto nativeStatementTemp848=JSValue("getVertexPosition");const auto nativeStatementTemp844=nativeStatementTemp847.get(nativeStatementTemp848);const auto nativeStatementTemp845=nativeStatementTemp847;Args nativeStatementTemp846;nativeStatementTemp846.push_back(c);nativeStatementTemp846.push_back(nativeStatementTemp831.get("_vC"));return nativeStatementTemp844.call(nativeStatementTemp845,nativeStatementTemp846);}());
JSValue intersection=([&]()->JSValue{const auto nativeStatementTemp849=nativeStatementTemp831.get("checkIntersection");const JSValue nativeStatementTemp850;Args nativeStatementTemp851;nativeStatementTemp851.push_back(object);nativeStatementTemp851.push_back(material);nativeStatementTemp851.push_back(raycaster);nativeStatementTemp851.push_back(ray);nativeStatementTemp851.push_back(nativeStatementTemp831.get("_vA"));nativeStatementTemp851.push_back(nativeStatementTemp831.get("_vB"));nativeStatementTemp851.push_back(nativeStatementTemp831.get("_vC"));nativeStatementTemp851.push_back(nativeStatementTemp831.get("_intersectionPoint"));return nativeStatementTemp849.call(nativeStatementTemp850,nativeStatementTemp851);}());
if(intersection.toBoolean()){
JSValue barycoord=([&]()->JSValue{const auto nativeStatementTemp852=nativeStatementTemp831.get("Vector3");Args nativeStatementTemp853;return nativeStatementTemp852.construct(nativeStatementTemp853);}());
([&]()->JSValue{auto nativeStatementTemp857=nativeStatementTemp831.get("Triangle");const auto nativeStatementTemp858=JSValue("getBarycoord");const auto nativeStatementTemp854=nativeStatementTemp857.get(nativeStatementTemp858);const auto nativeStatementTemp855=nativeStatementTemp857;Args nativeStatementTemp856;nativeStatementTemp856.push_back(nativeStatementTemp831.get("_intersectionPoint"));nativeStatementTemp856.push_back(nativeStatementTemp831.get("_vA"));nativeStatementTemp856.push_back(nativeStatementTemp831.get("_vB"));nativeStatementTemp856.push_back(nativeStatementTemp831.get("_vC"));nativeStatementTemp856.push_back(barycoord);return nativeStatementTemp854.call(nativeStatementTemp855,nativeStatementTemp856);}());
if(uv.toBoolean()){
([&]()->JSValue{auto nativeStatementTemp859=intersection;const auto nativeStatementTemp860=JSValue("uv");const auto nativeStatementTemp861=([&]()->JSValue{auto nativeStatementTemp867=nativeStatementTemp831.get("Triangle");const auto nativeStatementTemp868=JSValue("getInterpolatedAttribute");const auto nativeStatementTemp864=nativeStatementTemp867.get(nativeStatementTemp868);const auto nativeStatementTemp865=nativeStatementTemp867;Args nativeStatementTemp866;nativeStatementTemp866.push_back(uv);nativeStatementTemp866.push_back(a);nativeStatementTemp866.push_back(b);nativeStatementTemp866.push_back(c);nativeStatementTemp866.push_back(barycoord);nativeStatementTemp866.push_back(([&]()->JSValue{const auto nativeStatementTemp869=nativeStatementTemp831.get("Vector2");Args nativeStatementTemp870;return nativeStatementTemp869.construct(nativeStatementTemp870);}()));return nativeStatementTemp864.call(nativeStatementTemp865,nativeStatementTemp866);}());setReference(nativeStatementTemp859,nativeStatementTemp860,nativeStatementTemp861);return nativeStatementTemp861;}());

}
if(uv1.toBoolean()){
([&]()->JSValue{auto nativeStatementTemp871=intersection;const auto nativeStatementTemp872=JSValue("uv1");const auto nativeStatementTemp873=([&]()->JSValue{auto nativeStatementTemp879=nativeStatementTemp831.get("Triangle");const auto nativeStatementTemp880=JSValue("getInterpolatedAttribute");const auto nativeStatementTemp876=nativeStatementTemp879.get(nativeStatementTemp880);const auto nativeStatementTemp877=nativeStatementTemp879;Args nativeStatementTemp878;nativeStatementTemp878.push_back(uv1);nativeStatementTemp878.push_back(a);nativeStatementTemp878.push_back(b);nativeStatementTemp878.push_back(c);nativeStatementTemp878.push_back(barycoord);nativeStatementTemp878.push_back(([&]()->JSValue{const auto nativeStatementTemp881=nativeStatementTemp831.get("Vector2");Args nativeStatementTemp882;return nativeStatementTemp881.construct(nativeStatementTemp882);}()));return nativeStatementTemp876.call(nativeStatementTemp877,nativeStatementTemp878);}());setReference(nativeStatementTemp871,nativeStatementTemp872,nativeStatementTemp873);return nativeStatementTemp873;}());

}
if(normal.toBoolean()){
([&]()->JSValue{auto nativeStatementTemp883=intersection;const auto nativeStatementTemp884=JSValue("normal");const auto nativeStatementTemp885=([&]()->JSValue{auto nativeStatementTemp891=nativeStatementTemp831.get("Triangle");const auto nativeStatementTemp892=JSValue("getInterpolatedAttribute");const auto nativeStatementTemp888=nativeStatementTemp891.get(nativeStatementTemp892);const auto nativeStatementTemp889=nativeStatementTemp891;Args nativeStatementTemp890;nativeStatementTemp890.push_back(normal);nativeStatementTemp890.push_back(a);nativeStatementTemp890.push_back(b);nativeStatementTemp890.push_back(c);nativeStatementTemp890.push_back(barycoord);nativeStatementTemp890.push_back(([&]()->JSValue{const auto nativeStatementTemp893=nativeStatementTemp831.get("Vector3");Args nativeStatementTemp894;return nativeStatementTemp893.construct(nativeStatementTemp894);}()));return nativeStatementTemp888.call(nativeStatementTemp889,nativeStatementTemp890);}());setReference(nativeStatementTemp883,nativeStatementTemp884,nativeStatementTemp885);return nativeStatementTemp885;}());
if(([&]()->JSValue{const auto nativeStatementTemp895=([&]()->JSValue{auto nativeStatementTemp900=([&]()->JSValue{const auto nativeStatementTemp902=intersection;const auto nativeStatementTemp903=JSValue("normal");return nativeStatementTemp902.get(nativeStatementTemp903);}());const auto nativeStatementTemp901=JSValue("dot");const auto nativeStatementTemp897=nativeStatementTemp900.get(nativeStatementTemp901);const auto nativeStatementTemp898=nativeStatementTemp900;Args nativeStatementTemp899;nativeStatementTemp899.push_back(([&]()->JSValue{const auto nativeStatementTemp904=ray;const auto nativeStatementTemp905=JSValue("direction");return nativeStatementTemp904.get(nativeStatementTemp905);}()));return nativeStatementTemp897.call(nativeStatementTemp898,nativeStatementTemp899);}());const auto nativeStatementTemp896=JSValue(0);return binary(">",nativeStatementTemp895,nativeStatementTemp896);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp909=([&]()->JSValue{const auto nativeStatementTemp911=intersection;const auto nativeStatementTemp912=JSValue("normal");return nativeStatementTemp911.get(nativeStatementTemp912);}());const auto nativeStatementTemp910=JSValue("multiplyScalar");const auto nativeStatementTemp906=nativeStatementTemp909.get(nativeStatementTemp910);const auto nativeStatementTemp907=nativeStatementTemp909;Args nativeStatementTemp908;nativeStatementTemp908.push_back(([&]()->JSValue{const auto nativeStatementTemp913=JSValue(1);return -nativeStatementTemp913.toNumber();}()));return nativeStatementTemp906.call(nativeStatementTemp907,nativeStatementTemp908);}());

}

}
JSValue face=([&]()->JSValue{auto nativeStatementTemp914=JSValue::object();{const auto nativeStatementTemp915=JSValue("a");const auto nativeStatementTemp916=a;own(nativeStatementTemp914,nativeStatementTemp915,nativeStatementTemp916);}{const auto nativeStatementTemp917=JSValue("b");const auto nativeStatementTemp918=b;own(nativeStatementTemp914,nativeStatementTemp917,nativeStatementTemp918);}{const auto nativeStatementTemp919=JSValue("c");const auto nativeStatementTemp920=c;own(nativeStatementTemp914,nativeStatementTemp919,nativeStatementTemp920);}{const auto nativeStatementTemp921=JSValue("normal");const auto nativeStatementTemp922=([&]()->JSValue{const auto nativeStatementTemp923=nativeStatementTemp831.get("Vector3");Args nativeStatementTemp924;return nativeStatementTemp923.construct(nativeStatementTemp924);}());own(nativeStatementTemp914,nativeStatementTemp921,nativeStatementTemp922);}{const auto nativeStatementTemp925=JSValue("materialIndex");const auto nativeStatementTemp926=JSValue(0);own(nativeStatementTemp914,nativeStatementTemp925,nativeStatementTemp926);}return nativeStatementTemp914;}());
([&]()->JSValue{auto nativeStatementTemp930=nativeStatementTemp831.get("Triangle");const auto nativeStatementTemp931=JSValue("getNormal");const auto nativeStatementTemp927=nativeStatementTemp930.get(nativeStatementTemp931);const auto nativeStatementTemp928=nativeStatementTemp930;Args nativeStatementTemp929;nativeStatementTemp929.push_back(nativeStatementTemp831.get("_vA"));nativeStatementTemp929.push_back(nativeStatementTemp831.get("_vB"));nativeStatementTemp929.push_back(nativeStatementTemp831.get("_vC"));nativeStatementTemp929.push_back(([&]()->JSValue{const auto nativeStatementTemp932=face;const auto nativeStatementTemp933=JSValue("normal");return nativeStatementTemp932.get(nativeStatementTemp933);}()));return nativeStatementTemp927.call(nativeStatementTemp928,nativeStatementTemp929);}());
([&]()->JSValue{auto nativeStatementTemp934=intersection;const auto nativeStatementTemp935=JSValue("face");const auto nativeStatementTemp936=face;setReference(nativeStatementTemp934,nativeStatementTemp935,nativeStatementTemp936);return nativeStatementTemp936;}());
([&]()->JSValue{auto nativeStatementTemp939=intersection;const auto nativeStatementTemp940=JSValue("barycoord");const auto nativeStatementTemp941=barycoord;setReference(nativeStatementTemp939,nativeStatementTemp940,nativeStatementTemp941);return nativeStatementTemp941;}());

}
return intersection;
return JSValue{};
}
}
JSValue nativeMeshModule(){using namespace MeshManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"Mesh module");auto module=JSValue::object(nullptr);
own(module,"Vector3",nativeVector3Constructor());
own(module,"Vector2",nativeVector2Constructor());
own(module,"Sphere",nativeSphereConstructor());
own(module,"Ray",nativeRayConstructor());
own(module,"Matrix4",nativeMatrix4Constructor());
own(module,"Object3D",nativeObject3DConstructor());
own(module,"Triangle",nativeTriangleConstructor());
own(module,"BackSide",nativeConstantsBinding("BackSide"));
own(module,"FrontSide",nativeConstantsBinding("FrontSide"));
own(module,"MeshBasicMaterial",nativeMeshBasicMaterialConstructor());
own(module,"BufferGeometry",nativeBufferGeometryConstructor());
{auto function=JSValue::constructor(helper0,{module},"checkIntersection",8,JSValue::ConstructorKind::Callable);function.setFunctionSourceText("function checkIntersection( object, material, raycaster, ray, pA, pB, pC, point ) {\r\n\r\n\tlet intersect;\r\n\r\n\tif ( material.side === BackSide ) {\r\n\r\n\t\tintersect = ray.intersectTriangle( pC, pB, pA, true, point );\r\n\r\n\t} else {\r\n\r\n\t\tintersect = ray.intersectTriangle( pA, pB, pC, ( material.side === FrontSide ), point );\r\n\r\n\t}\r\n\r\n\tif ( intersect === null ) return null;\r\n\r\n\t_intersectionPointWorld.copy( point );\r\n\t_intersectionPointWorld.applyMatrix4( object.matrixWorld );\r\n\r\n\tconst distance = raycaster.ray.origin.distanceTo( _intersectionPointWorld );\r\n\r\n\tif ( distance < raycaster.near || distance > raycaster.far ) return null;\r\n\r\n\treturn {\r\n\t\tdistance: distance,\r\n\t\tpoint: _intersectionPointWorld.clone(),\r\n\t\tobject: object\r\n\t};\r\n\r\n}");own(module,"checkIntersection",function);}
{auto function=JSValue::constructor(helper1,{module},"checkGeometryIntersection",10,JSValue::ConstructorKind::Callable);function.setFunctionSourceText("function checkGeometryIntersection( object, material, raycaster, ray, uv, uv1, normal, a, b, c ) {\r\n\r\n\tobject.getVertexPosition( a, _vA );\r\n\tobject.getVertexPosition( b, _vB );\r\n\tobject.getVertexPosition( c, _vC );\r\n\r\n\tconst intersection = checkIntersection( object, material, raycaster, ray, _vA, _vB, _vC, _intersectionPoint );\r\n\r\n\tif ( intersection ) {\r\n\r\n\t\tconst barycoord = new Vector3();\r\n\t\tTriangle.getBarycoord( _intersectionPoint, _vA, _vB, _vC, barycoord );\r\n\r\n\t\tif ( uv ) {\r\n\r\n\t\t\tintersection.uv = Triangle.getInterpolatedAttribute( uv, a, b, c, barycoord, new Vector2() );\r\n\r\n\t\t}\r\n\r\n\t\tif ( uv1 ) {\r\n\r\n\t\t\tintersection.uv1 = Triangle.getInterpolatedAttribute( uv1, a, b, c, barycoord, new Vector2() );\r\n\r\n\t\t}\r\n\r\n\t\tif ( normal ) {\r\n\r\n\t\t\tintersection.normal = Triangle.getInterpolatedAttribute( normal, a, b, c, barycoord, new Vector3() );\r\n\r\n\t\t\tif ( intersection.normal.dot( ray.direction ) > 0 ) {\r\n\r\n\t\t\t\tintersection.normal.multiplyScalar( - 1 );\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t\tconst face = {\r\n\t\t\ta: a,\r\n\t\t\tb: b,\r\n\t\t\tc: c,\r\n\t\t\tnormal: new Vector3(),\r\n\t\t\tmaterialIndex: 0\r\n\t\t};\r\n\r\n\t\tTriangle.getNormal( _vA, _vB, _vC, face.normal );\r\n\r\n\t\tintersection.face = face;\r\n\t\tintersection.barycoord = barycoord;\r\n\r\n\t}\r\n\r\n\treturn intersection;\r\n\r\n}");own(module,"checkGeometryIntersection",function);}
own(module,"_inverseMatrix",([&]()->JSValue{const auto nativeStatementTemp944=module.get("Matrix4");Args nativeStatementTemp945;return nativeStatementTemp944.construct(nativeStatementTemp945);}()));
own(module,"_ray",([&]()->JSValue{const auto nativeStatementTemp946=module.get("Ray");Args nativeStatementTemp947;return nativeStatementTemp946.construct(nativeStatementTemp947);}()));
own(module,"_sphere",([&]()->JSValue{const auto nativeStatementTemp948=module.get("Sphere");Args nativeStatementTemp949;return nativeStatementTemp948.construct(nativeStatementTemp949);}()));
own(module,"_sphereHitAt",([&]()->JSValue{const auto nativeStatementTemp950=module.get("Vector3");Args nativeStatementTemp951;return nativeStatementTemp950.construct(nativeStatementTemp951);}()));
own(module,"_vA",([&]()->JSValue{const auto nativeStatementTemp952=module.get("Vector3");Args nativeStatementTemp953;return nativeStatementTemp952.construct(nativeStatementTemp953);}()));
own(module,"_vB",([&]()->JSValue{const auto nativeStatementTemp954=module.get("Vector3");Args nativeStatementTemp955;return nativeStatementTemp954.construct(nativeStatementTemp955);}()));
own(module,"_vC",([&]()->JSValue{const auto nativeStatementTemp956=module.get("Vector3");Args nativeStatementTemp957;return nativeStatementTemp956.construct(nativeStatementTemp957);}()));
own(module,"_tempA",([&]()->JSValue{const auto nativeStatementTemp958=module.get("Vector3");Args nativeStatementTemp959;return nativeStatementTemp958.construct(nativeStatementTemp959);}()));
own(module,"_morphA",([&]()->JSValue{const auto nativeStatementTemp960=module.get("Vector3");Args nativeStatementTemp961;return nativeStatementTemp960.construct(nativeStatementTemp961);}()));
own(module,"_intersectionPoint",([&]()->JSValue{const auto nativeStatementTemp962=module.get("Vector3");Args nativeStatementTemp963;return nativeStatementTemp962.construct(nativeStatementTemp963);}()));
own(module,"_intersectionPointWorld",([&]()->JSValue{const auto nativeStatementTemp964=module.get("Vector3");Args nativeStatementTemp965;return nativeStatementTemp964.construct(nativeStatementTemp965);}()));
auto ctor=JSValue::builtinConstructor(method0,{module},"Mesh",0);ctor.setFunctionSourceText("class Mesh extends Object3D {\r\n\r\n\t/**\r\n\t * Constructs a new mesh.\r\n\t *\r\n\t * @param {BufferGeometry} [geometry] - The mesh geometry.\r\n\t * @param {Material|Array<Material>} [material] - The mesh material.\r\n\t */\r\n\tconstructor( geometry = new BufferGeometry(), material = new MeshBasicMaterial() ) {\r\n\r\n\t\tsuper();\r\n\r\n\t\t/**\r\n\t\t * This flag can be used for type testing.\r\n\t\t *\r\n\t\t * @type {boolean}\r\n\t\t * @readonly\r\n\t\t * @default true\r\n\t\t */\r\n\t\tthis.isMesh = true;\r\n\r\n\t\tthis.type = 'Mesh';\r\n\r\n\t\t/**\r\n\t\t * The mesh geometry.\r\n\t\t *\r\n\t\t * @type {BufferGeometry}\r\n\t\t */\r\n\t\tthis.geometry = geometry;\r\n\r\n\t\t/**\r\n\t\t * The mesh material.\r\n\t\t *\r\n\t\t * @type {Material|Array<Material>}\r\n\t\t * @default MeshBasicMaterial\r\n\t\t */\r\n\t\tthis.material = material;\r\n\r\n\t\t/**\r\n\t\t * A dictionary representing the morph targets in the geometry. The key is the\r\n\t\t * morph targets name, the value its attribute index. This member is `undefined`\r\n\t\t * by default and only set when morph targets are detected in the geometry.\r\n\t\t *\r\n\t\t * @type {Object<string,number>|undefined}\r\n\t\t * @default undefined\r\n\t\t */\r\n\t\tthis.morphTargetDictionary = undefined;\r\n\r\n\t\t/**\r\n\t\t * An array of weights typically in the range `[0,1]` that specify how much of the morph\r\n\t\t * is applied. This member is `undefined` by default and only set when morph targets are\r\n\t\t * detected in the geometry.\r\n\t\t *\r\n\t\t * @type {Array<number>|undefined}\r\n\t\t * @default undefined\r\n\t\t */\r\n\t\tthis.morphTargetInfluences = undefined;\r\n\r\n\t\t/**\r\n\t\t * The number of instances of this mesh.\r\n\t\t * Can only be used with {@link WebGPURenderer}.\r\n\t\t *\r\n\t\t * @type {number}\r\n\t\t * @default 1\r\n\t\t */\r\n\t\tthis.count = 1;\r\n\r\n\t\tthis.updateMorphTargets();\r\n\r\n\t}\r\n\r\n\tcopy( source, recursive ) {\r\n\r\n\t\tsuper.copy( source, recursive );\r\n\r\n\t\tif ( source.morphTargetInfluences !== undefined ) {\r\n\r\n\t\t\tthis.morphTargetInfluences = source.morphTargetInfluences.slice();\r\n\r\n\t\t} else {\r\n\r\n\t\t\tthis.morphTargetInfluences = undefined;\r\n\r\n\t\t}\r\n\r\n\t\tif ( source.morphTargetDictionary !== undefined ) {\r\n\r\n\t\t\tthis.morphTargetDictionary = Object.assign( {}, source.morphTargetDictionary );\r\n\r\n\t\t} else {\r\n\r\n\t\t\tthis.morphTargetDictionary = undefined;\r\n\r\n\t\t}\r\n\r\n\t\tthis.material = Array.isArray( source.material ) ? source.material.slice() : source.material;\r\n\t\tthis.geometry = source.geometry;\r\n\r\n\t\treturn this;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Sets the values of {@link Mesh#morphTargetDictionary} and {@link Mesh#morphTargetInfluences}\r\n\t * to make sure existing morph targets can influence this 3D object.\r\n\t */\r\n\tupdateMorphTargets() {\r\n\r\n\t\tconst geometry = this.geometry;\r\n\r\n\t\tconst morphAttributes = geometry.morphAttributes;\r\n\t\tconst keys = Object.keys( morphAttributes );\r\n\r\n\t\tif ( keys.length > 0 ) {\r\n\r\n\t\t\tconst morphAttribute = morphAttributes[ keys[ 0 ] ];\r\n\r\n\t\t\tif ( morphAttribute !== undefined ) {\r\n\r\n\t\t\t\tthis.morphTargetInfluences = [];\r\n\t\t\t\tthis.morphTargetDictionary = {};\r\n\r\n\t\t\t\tfor ( let m = 0, ml = morphAttribute.length; m < ml; m ++ ) {\r\n\r\n\t\t\t\t\tconst name = morphAttribute[ m ].name || String( m );\r\n\r\n\t\t\t\t\tthis.morphTargetInfluences.push( 0 );\r\n\t\t\t\t\tthis.morphTargetDictionary[ name ] = m;\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns the local-space position of the vertex at the given index, taking into\r\n\t * account the current animation state of both morph targets and skinning.\r\n\t *\r\n\t * @param {number} index - The vertex index.\r\n\t * @param {Vector3} target - The target object that is used to store the method's result.\r\n\t * @return {Vector3} The vertex position in local space.\r\n\t */\r\n\tgetVertexPosition( index, target ) {\r\n\r\n\t\tconst geometry = this.geometry;\r\n\t\tconst position = geometry.attributes.position;\r\n\t\tconst morphPosition = geometry.morphAttributes.position;\r\n\t\tconst morphTargetsRelative = geometry.morphTargetsRelative;\r\n\r\n\t\ttarget.fromBufferAttribute( position, index );\r\n\r\n\t\tconst morphInfluences = this.morphTargetInfluences;\r\n\r\n\t\tif ( morphPosition && morphInfluences ) {\r\n\r\n\t\t\t_morphA.set( 0, 0, 0 );\r\n\r\n\t\t\tfor ( let i = 0, il = morphPosition.length; i < il; i ++ ) {\r\n\r\n\t\t\t\tconst influence = morphInfluences[ i ];\r\n\t\t\t\tconst morphAttribute = morphPosition[ i ];\r\n\r\n\t\t\t\tif ( influence === 0 ) continue;\r\n\r\n\t\t\t\t_tempA.fromBufferAttribute( morphAttribute, index );\r\n\r\n\t\t\t\tif ( morphTargetsRelative ) {\r\n\r\n\t\t\t\t\t_morphA.addScaledVector( _tempA, influence );\r\n\r\n\t\t\t\t} else {\r\n\r\n\t\t\t\t\t_morphA.addScaledVector( _tempA.sub( target ), influence );\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t\ttarget.add( _morphA );\r\n\r\n\t\t}\r\n\r\n\t\treturn target;\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Returns `true` if this mesh intersects the given frustum.\r\n\t *\r\n\t * @param {Frustum|FrustumArray} frustum - The frustum to test.\r\n\t * @return {boolean} Whether this mesh intersects the given frustum or not.\r\n\t */\r\n\tintersectsFrustum( frustum ) {\r\n\r\n\t\treturn frustum.intersectsObject( this );\r\n\r\n\t}\r\n\r\n\t/**\r\n\t * Computes intersection points between a casted ray and this line.\r\n\t *\r\n\t * @param {Raycaster} raycaster - The raycaster.\r\n\t * @param {Array<Object>} intersects - The target array that holds the intersection points.\r\n\t */\r\n\traycast( raycaster, intersects ) {\r\n\r\n\t\tconst geometry = this.geometry;\r\n\t\tconst material = this.material;\r\n\t\tconst matrixWorld = this.matrixWorld;\r\n\r\n\t\tif ( material === undefined ) return;\r\n\r\n\t\t// test with bounding sphere in world space\r\n\r\n\t\tif ( geometry.boundingSphere === null ) geometry.computeBoundingSphere();\r\n\r\n\t\t_sphere.copy( geometry.boundingSphere );\r\n\t\t_sphere.applyMatrix4( matrixWorld );\r\n\r\n\t\t// check distance from ray origin to bounding sphere\r\n\r\n\t\t_ray.copy( raycaster.ray ).recast( raycaster.near );\r\n\r\n\t\tif ( _sphere.containsPoint( _ray.origin ) === false ) {\r\n\r\n\t\t\tif ( _ray.intersectSphere( _sphere, _sphereHitAt ) === null ) return;\r\n\r\n\t\t\tif ( _ray.origin.distanceToSquared( _sphereHitAt ) > ( raycaster.far - raycaster.near ) ** 2 ) return;\r\n\r\n\t\t}\r\n\r\n\t\t// convert ray to local space of mesh\r\n\r\n\t\t_inverseMatrix.copy( matrixWorld ).invert();\r\n\t\t_ray.copy( raycaster.ray ).applyMatrix4( _inverseMatrix );\r\n\r\n\t\t// test with bounding box in local space\r\n\r\n\t\tif ( geometry.boundingBox !== null ) {\r\n\r\n\t\t\tif ( _ray.intersectsBox( geometry.boundingBox ) === false ) return;\r\n\r\n\t\t}\r\n\r\n\t\t// test for intersections with geometry\r\n\r\n\t\tthis._computeIntersections( raycaster, intersects, _ray );\r\n\r\n\t}\r\n\r\n\t_computeIntersections( raycaster, intersects, rayLocalSpace ) {\r\n\r\n\t\tlet intersection;\r\n\r\n\t\tconst geometry = this.geometry;\r\n\t\tconst material = this.material;\r\n\r\n\t\tconst index = geometry.index;\r\n\t\tconst position = geometry.attributes.position;\r\n\t\tconst uv = geometry.attributes.uv;\r\n\t\tconst uv1 = geometry.attributes.uv1;\r\n\t\tconst normal = geometry.attributes.normal;\r\n\t\tconst groups = geometry.groups;\r\n\t\tconst drawRange = geometry.drawRange;\r\n\r\n\t\tif ( index !== null ) {\r\n\r\n\t\t\t// indexed buffer geometry\r\n\r\n\t\t\tif ( Array.isArray( material ) ) {\r\n\r\n\t\t\t\tfor ( let i = 0, il = groups.length; i < il; i ++ ) {\r\n\r\n\t\t\t\t\tconst group = groups[ i ];\r\n\t\t\t\t\tconst groupMaterial = material[ group.materialIndex ];\r\n\r\n\t\t\t\t\tconst start = Math.max( group.start, drawRange.start );\r\n\t\t\t\t\tconst end = Math.min( index.count, Math.min( ( group.start + group.count ), ( drawRange.start + drawRange.count ) ) );\r\n\r\n\t\t\t\t\tfor ( let j = start, jl = end; j < jl; j += 3 ) {\r\n\r\n\t\t\t\t\t\tconst a = index.getX( j );\r\n\t\t\t\t\t\tconst b = index.getX( j + 1 );\r\n\t\t\t\t\t\tconst c = index.getX( j + 2 );\r\n\r\n\t\t\t\t\t\tintersection = checkGeometryIntersection( this, groupMaterial, raycaster, rayLocalSpace, uv, uv1, normal, a, b, c );\r\n\r\n\t\t\t\t\t\tif ( intersection ) {\r\n\r\n\t\t\t\t\t\t\tintersection.faceIndex = Math.floor( j / 3 ); // triangle number in indexed buffer semantics\r\n\t\t\t\t\t\t\tintersection.face.materialIndex = group.materialIndex;\r\n\t\t\t\t\t\t\tintersects.push( intersection );\r\n\r\n\t\t\t\t\t\t}\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t} else {\r\n\r\n\t\t\t\tconst start = Math.max( 0, drawRange.start );\r\n\t\t\t\tconst end = Math.min( index.count, ( drawRange.start + drawRange.count ) );\r\n\r\n\t\t\t\tfor ( let i = start, il = end; i < il; i += 3 ) {\r\n\r\n\t\t\t\t\tconst a = index.getX( i );\r\n\t\t\t\t\tconst b = index.getX( i + 1 );\r\n\t\t\t\t\tconst c = index.getX( i + 2 );\r\n\r\n\t\t\t\t\tintersection = checkGeometryIntersection( this, material, raycaster, rayLocalSpace, uv, uv1, normal, a, b, c );\r\n\r\n\t\t\t\t\tif ( intersection ) {\r\n\r\n\t\t\t\t\t\tintersection.faceIndex = Math.floor( i / 3 ); // triangle number in indexed buffer semantics\r\n\t\t\t\t\t\tintersects.push( intersection );\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t} else if ( position !== undefined ) {\r\n\r\n\t\t\t// non-indexed buffer geometry\r\n\r\n\t\t\tif ( Array.isArray( material ) ) {\r\n\r\n\t\t\t\tfor ( let i = 0, il = groups.length; i < il; i ++ ) {\r\n\r\n\t\t\t\t\tconst group = groups[ i ];\r\n\t\t\t\t\tconst groupMaterial = material[ group.materialIndex ];\r\n\r\n\t\t\t\t\tconst start = Math.max( group.start, drawRange.start );\r\n\t\t\t\t\tconst end = Math.min( position.count, Math.min( ( group.start + group.count ), ( drawRange.start + drawRange.count ) ) );\r\n\r\n\t\t\t\t\tfor ( let j = start, jl = end; j < jl; j += 3 ) {\r\n\r\n\t\t\t\t\t\tconst a = j;\r\n\t\t\t\t\t\tconst b = j + 1;\r\n\t\t\t\t\t\tconst c = j + 2;\r\n\r\n\t\t\t\t\t\tintersection = checkGeometryIntersection( this, groupMaterial, raycaster, rayLocalSpace, uv, uv1, normal, a, b, c );\r\n\r\n\t\t\t\t\t\tif ( intersection ) {\r\n\r\n\t\t\t\t\t\t\tintersection.faceIndex = Math.floor( j / 3 ); // triangle number in non-indexed buffer semantics\r\n\t\t\t\t\t\t\tintersection.face.materialIndex = group.materialIndex;\r\n\t\t\t\t\t\t\tintersects.push( intersection );\r\n\r\n\t\t\t\t\t\t}\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t} else {\r\n\r\n\t\t\t\tconst start = Math.max( 0, drawRange.start );\r\n\t\t\t\tconst end = Math.min( position.count, ( drawRange.start + drawRange.count ) );\r\n\r\n\t\t\t\tfor ( let i = start, il = end; i < il; i += 3 ) {\r\n\r\n\t\t\t\t\tconst a = i;\r\n\t\t\t\t\tconst b = i + 1;\r\n\t\t\t\t\tconst c = i + 2;\r\n\r\n\t\t\t\t\tintersection = checkGeometryIntersection( this, material, raycaster, rayLocalSpace, uv, uv1, normal, a, b, c );\r\n\r\n\t\t\t\t\tif ( intersection ) {\r\n\r\n\t\t\t\t\t\tintersection.faceIndex = Math.floor( i / 3 ); // triangle number in non-indexed buffer semantics\r\n\t\t\t\t\t\tintersects.push( intersection );\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t}\r\n\r\n}");auto prototype=ctor.get("prototype");own(module,"Mesh",ctor);own(module,"_home",prototype);
ctor.setPrototype(module.get("Object3D"));prototype.setPrototype(module.get("Object3D").get("prototype"));
{auto function=JSValue::function(method1,{module},"copy",2);function.setFunctionSourceText("copy( source, recursive ) {\r\n\r\n\t\tsuper.copy( source, recursive );\r\n\r\n\t\tif ( source.morphTargetInfluences !== undefined ) {\r\n\r\n\t\t\tthis.morphTargetInfluences = source.morphTargetInfluences.slice();\r\n\r\n\t\t} else {\r\n\r\n\t\t\tthis.morphTargetInfluences = undefined;\r\n\r\n\t\t}\r\n\r\n\t\tif ( source.morphTargetDictionary !== undefined ) {\r\n\r\n\t\t\tthis.morphTargetDictionary = Object.assign( {}, source.morphTargetDictionary );\r\n\r\n\t\t} else {\r\n\r\n\t\t\tthis.morphTargetDictionary = undefined;\r\n\r\n\t\t}\r\n\r\n\t\tthis.material = Array.isArray( source.material ) ? source.material.slice() : source.material;\r\n\t\tthis.geometry = source.geometry;\r\n\r\n\t\treturn this;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("copy",descriptor);}
{auto function=JSValue::function(method2,{module},"updateMorphTargets",0);function.setFunctionSourceText("updateMorphTargets() {\r\n\r\n\t\tconst geometry = this.geometry;\r\n\r\n\t\tconst morphAttributes = geometry.morphAttributes;\r\n\t\tconst keys = Object.keys( morphAttributes );\r\n\r\n\t\tif ( keys.length > 0 ) {\r\n\r\n\t\t\tconst morphAttribute = morphAttributes[ keys[ 0 ] ];\r\n\r\n\t\t\tif ( morphAttribute !== undefined ) {\r\n\r\n\t\t\t\tthis.morphTargetInfluences = [];\r\n\t\t\t\tthis.morphTargetDictionary = {};\r\n\r\n\t\t\t\tfor ( let m = 0, ml = morphAttribute.length; m < ml; m ++ ) {\r\n\r\n\t\t\t\t\tconst name = morphAttribute[ m ].name || String( m );\r\n\r\n\t\t\t\t\tthis.morphTargetInfluences.push( 0 );\r\n\t\t\t\t\tthis.morphTargetDictionary[ name ] = m;\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("updateMorphTargets",descriptor);}
{auto function=JSValue::function(method3,{module},"getVertexPosition",2);function.setFunctionSourceText("getVertexPosition( index, target ) {\r\n\r\n\t\tconst geometry = this.geometry;\r\n\t\tconst position = geometry.attributes.position;\r\n\t\tconst morphPosition = geometry.morphAttributes.position;\r\n\t\tconst morphTargetsRelative = geometry.morphTargetsRelative;\r\n\r\n\t\ttarget.fromBufferAttribute( position, index );\r\n\r\n\t\tconst morphInfluences = this.morphTargetInfluences;\r\n\r\n\t\tif ( morphPosition && morphInfluences ) {\r\n\r\n\t\t\t_morphA.set( 0, 0, 0 );\r\n\r\n\t\t\tfor ( let i = 0, il = morphPosition.length; i < il; i ++ ) {\r\n\r\n\t\t\t\tconst influence = morphInfluences[ i ];\r\n\t\t\t\tconst morphAttribute = morphPosition[ i ];\r\n\r\n\t\t\t\tif ( influence === 0 ) continue;\r\n\r\n\t\t\t\t_tempA.fromBufferAttribute( morphAttribute, index );\r\n\r\n\t\t\t\tif ( morphTargetsRelative ) {\r\n\r\n\t\t\t\t\t_morphA.addScaledVector( _tempA, influence );\r\n\r\n\t\t\t\t} else {\r\n\r\n\t\t\t\t\t_morphA.addScaledVector( _tempA.sub( target ), influence );\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t\ttarget.add( _morphA );\r\n\r\n\t\t}\r\n\r\n\t\treturn target;\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("getVertexPosition",descriptor);}
{auto function=JSValue::function(method4,{module},"intersectsFrustum",1);function.setFunctionSourceText("intersectsFrustum( frustum ) {\r\n\r\n\t\treturn frustum.intersectsObject( this );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("intersectsFrustum",descriptor);}
{auto function=JSValue::function(method5,{module},"raycast",2);function.setFunctionSourceText("raycast( raycaster, intersects ) {\r\n\r\n\t\tconst geometry = this.geometry;\r\n\t\tconst material = this.material;\r\n\t\tconst matrixWorld = this.matrixWorld;\r\n\r\n\t\tif ( material === undefined ) return;\r\n\r\n\t\t// test with bounding sphere in world space\r\n\r\n\t\tif ( geometry.boundingSphere === null ) geometry.computeBoundingSphere();\r\n\r\n\t\t_sphere.copy( geometry.boundingSphere );\r\n\t\t_sphere.applyMatrix4( matrixWorld );\r\n\r\n\t\t// check distance from ray origin to bounding sphere\r\n\r\n\t\t_ray.copy( raycaster.ray ).recast( raycaster.near );\r\n\r\n\t\tif ( _sphere.containsPoint( _ray.origin ) === false ) {\r\n\r\n\t\t\tif ( _ray.intersectSphere( _sphere, _sphereHitAt ) === null ) return;\r\n\r\n\t\t\tif ( _ray.origin.distanceToSquared( _sphereHitAt ) > ( raycaster.far - raycaster.near ) ** 2 ) return;\r\n\r\n\t\t}\r\n\r\n\t\t// convert ray to local space of mesh\r\n\r\n\t\t_inverseMatrix.copy( matrixWorld ).invert();\r\n\t\t_ray.copy( raycaster.ray ).applyMatrix4( _inverseMatrix );\r\n\r\n\t\t// test with bounding box in local space\r\n\r\n\t\tif ( geometry.boundingBox !== null ) {\r\n\r\n\t\t\tif ( _ray.intersectsBox( geometry.boundingBox ) === false ) return;\r\n\r\n\t\t}\r\n\r\n\t\t// test for intersections with geometry\r\n\r\n\t\tthis._computeIntersections( raycaster, intersects, _ray );\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("raycast",descriptor);}
{auto function=JSValue::function(method6,{module},"_computeIntersections",3);function.setFunctionSourceText("_computeIntersections( raycaster, intersects, rayLocalSpace ) {\r\n\r\n\t\tlet intersection;\r\n\r\n\t\tconst geometry = this.geometry;\r\n\t\tconst material = this.material;\r\n\r\n\t\tconst index = geometry.index;\r\n\t\tconst position = geometry.attributes.position;\r\n\t\tconst uv = geometry.attributes.uv;\r\n\t\tconst uv1 = geometry.attributes.uv1;\r\n\t\tconst normal = geometry.attributes.normal;\r\n\t\tconst groups = geometry.groups;\r\n\t\tconst drawRange = geometry.drawRange;\r\n\r\n\t\tif ( index !== null ) {\r\n\r\n\t\t\t// indexed buffer geometry\r\n\r\n\t\t\tif ( Array.isArray( material ) ) {\r\n\r\n\t\t\t\tfor ( let i = 0, il = groups.length; i < il; i ++ ) {\r\n\r\n\t\t\t\t\tconst group = groups[ i ];\r\n\t\t\t\t\tconst groupMaterial = material[ group.materialIndex ];\r\n\r\n\t\t\t\t\tconst start = Math.max( group.start, drawRange.start );\r\n\t\t\t\t\tconst end = Math.min( index.count, Math.min( ( group.start + group.count ), ( drawRange.start + drawRange.count ) ) );\r\n\r\n\t\t\t\t\tfor ( let j = start, jl = end; j < jl; j += 3 ) {\r\n\r\n\t\t\t\t\t\tconst a = index.getX( j );\r\n\t\t\t\t\t\tconst b = index.getX( j + 1 );\r\n\t\t\t\t\t\tconst c = index.getX( j + 2 );\r\n\r\n\t\t\t\t\t\tintersection = checkGeometryIntersection( this, groupMaterial, raycaster, rayLocalSpace, uv, uv1, normal, a, b, c );\r\n\r\n\t\t\t\t\t\tif ( intersection ) {\r\n\r\n\t\t\t\t\t\t\tintersection.faceIndex = Math.floor( j / 3 ); // triangle number in indexed buffer semantics\r\n\t\t\t\t\t\t\tintersection.face.materialIndex = group.materialIndex;\r\n\t\t\t\t\t\t\tintersects.push( intersection );\r\n\r\n\t\t\t\t\t\t}\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t} else {\r\n\r\n\t\t\t\tconst start = Math.max( 0, drawRange.start );\r\n\t\t\t\tconst end = Math.min( index.count, ( drawRange.start + drawRange.count ) );\r\n\r\n\t\t\t\tfor ( let i = start, il = end; i < il; i += 3 ) {\r\n\r\n\t\t\t\t\tconst a = index.getX( i );\r\n\t\t\t\t\tconst b = index.getX( i + 1 );\r\n\t\t\t\t\tconst c = index.getX( i + 2 );\r\n\r\n\t\t\t\t\tintersection = checkGeometryIntersection( this, material, raycaster, rayLocalSpace, uv, uv1, normal, a, b, c );\r\n\r\n\t\t\t\t\tif ( intersection ) {\r\n\r\n\t\t\t\t\t\tintersection.faceIndex = Math.floor( i / 3 ); // triangle number in indexed buffer semantics\r\n\t\t\t\t\t\tintersects.push( intersection );\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t} else if ( position !== undefined ) {\r\n\r\n\t\t\t// non-indexed buffer geometry\r\n\r\n\t\t\tif ( Array.isArray( material ) ) {\r\n\r\n\t\t\t\tfor ( let i = 0, il = groups.length; i < il; i ++ ) {\r\n\r\n\t\t\t\t\tconst group = groups[ i ];\r\n\t\t\t\t\tconst groupMaterial = material[ group.materialIndex ];\r\n\r\n\t\t\t\t\tconst start = Math.max( group.start, drawRange.start );\r\n\t\t\t\t\tconst end = Math.min( position.count, Math.min( ( group.start + group.count ), ( drawRange.start + drawRange.count ) ) );\r\n\r\n\t\t\t\t\tfor ( let j = start, jl = end; j < jl; j += 3 ) {\r\n\r\n\t\t\t\t\t\tconst a = j;\r\n\t\t\t\t\t\tconst b = j + 1;\r\n\t\t\t\t\t\tconst c = j + 2;\r\n\r\n\t\t\t\t\t\tintersection = checkGeometryIntersection( this, groupMaterial, raycaster, rayLocalSpace, uv, uv1, normal, a, b, c );\r\n\r\n\t\t\t\t\t\tif ( intersection ) {\r\n\r\n\t\t\t\t\t\t\tintersection.faceIndex = Math.floor( j / 3 ); // triangle number in non-indexed buffer semantics\r\n\t\t\t\t\t\t\tintersection.face.materialIndex = group.materialIndex;\r\n\t\t\t\t\t\t\tintersects.push( intersection );\r\n\r\n\t\t\t\t\t\t}\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t} else {\r\n\r\n\t\t\t\tconst start = Math.max( 0, drawRange.start );\r\n\t\t\t\tconst end = Math.min( position.count, ( drawRange.start + drawRange.count ) );\r\n\r\n\t\t\t\tfor ( let i = start, il = end; i < il; i += 3 ) {\r\n\r\n\t\t\t\t\tconst a = i;\r\n\t\t\t\t\tconst b = i + 1;\r\n\t\t\t\t\tconst c = i + 2;\r\n\r\n\t\t\t\t\tintersection = checkGeometryIntersection( this, material, raycaster, rayLocalSpace, uv, uv1, normal, a, b, c );\r\n\r\n\t\t\t\t\tif ( intersection ) {\r\n\r\n\t\t\t\t\t\tintersection.faceIndex = Math.floor( i / 3 ); // triangle number in non-indexed buffer semantics\r\n\t\t\t\t\t\tintersects.push( intersection );\r\n\r\n\t\t\t\t\t}\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t}\r\n\r\n\t}");JSPropertyDescriptor descriptor;descriptor.value=function;descriptor.writable=descriptor.configurable=true;prototype.defineProperty("_computeIntersections",descriptor);}
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();own(facade,"Mesh",ctor);facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;}
JSValue nativeMeshBinding(const std::string& name){const auto keeper=nativeMeshModule();const auto state=MeshManaged::lexical().lock();return name=="Mesh"?state->get(name):JSValue{};}
JSValue nativeMeshConstructor(){return nativeMeshBinding("Mesh");}
