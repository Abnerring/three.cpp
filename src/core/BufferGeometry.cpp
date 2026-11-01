#include "core/BufferGeometry.h"
#include "core/Object3D.h"
#include "core/JSDataSnapshot.h"
#include "core/NativeObject.h"
#include "math/MathUtils.h"
#include <algorithm>
#include <cmath>

BufferGeometry::BufferGeometry() : uuid(MathUtils::generateUUID()) {}
BufferGeometry& BufferGeometry::setIndex(const std::vector<std::uint32_t>& values) {
    // 65535 is the primitive restart value, matching arrayNeedsUint32().
    if (std::any_of(values.begin(), values.end(), [](auto v) { return v >= 65535; }))
        index = std::make_shared<Uint32BufferAttribute>(values, 1);
    else
        index = std::make_shared<Uint16BufferAttribute>(
            std::vector<std::uint16_t>(values.begin(), values.end()), 1);
    return *this;
}
std::shared_ptr<VertexAttribute> BufferGeometry::getAttribute(const std::string& name) const {
    const auto it = attributes.find(name);
    return it == attributes.end() ? nullptr : it->second;
}
BufferGeometry& BufferGeometry::setAttribute(const std::string& name,
                                             std::shared_ptr<VertexAttribute> a) {
    attributes[name] = std::move(a);
    return *this;
}
BufferGeometry& BufferGeometry::deleteAttribute(const std::string& name) {
    attributes.erase(name);
    return *this;
}
bool BufferGeometry::hasAttribute(const std::string& name) const {
    return attributes.contains(name);
}
void BufferGeometry::addGroup(double start, double count, double materialIndex) {
    groups.push_back({start, count, materialIndex});
}
void BufferGeometry::clearGroups() {
    groups.clear();
}
void BufferGeometry::setDrawRange(double start, double count) {
    drawRange = {start, count};
}
BufferGeometry& BufferGeometry::applyMatrix4(const Matrix4& m) {
    if (auto a = getAttribute("position")) {
        a->applyMatrix4(m);
        a->setNeedsUpdate(true);
    }
    if (auto a = getAttribute("normal")) {
        Matrix3 n;
        n.getNormalMatrix(m);
        a->applyNormalMatrix(n);
        a->setNeedsUpdate(true);
    }
    if (auto a = getAttribute("tangent")) {
        a->transformDirection(m);
        a->setNeedsUpdate(true);
    }
    if (boundingBox)
        computeBoundingBox();
    if (boundingSphere)
        computeBoundingSphere();
    _transformed = true;
    return *this;
}
BufferGeometry& BufferGeometry::applyQuaternion(const Quaternion& q) {
    Matrix4 m;
    m.makeRotationFromQuaternion(q);
    return applyMatrix4(m);
}
BufferGeometry& BufferGeometry::rotateX(double a) {
    Matrix4 m;
    m.makeRotationX(a);
    return applyMatrix4(m);
}
BufferGeometry& BufferGeometry::rotateY(double a) {
    Matrix4 m;
    m.makeRotationY(a);
    return applyMatrix4(m);
}
BufferGeometry& BufferGeometry::rotateZ(double a) {
    Matrix4 m;
    m.makeRotationZ(a);
    return applyMatrix4(m);
}
BufferGeometry& BufferGeometry::translate(double x, double y, double z) {
    Matrix4 m;
    m.makeTranslation(x, y, z);
    return applyMatrix4(m);
}
BufferGeometry& BufferGeometry::scale(double x, double y, double z) {
    Matrix4 m;
    m.makeScale(x, y, z);
    return applyMatrix4(m);
}
BufferGeometry& BufferGeometry::center() {
    computeBoundingBox();
    Vector3 v;
    boundingBox->getCenter(v);
    v.negate();
    return translate(v.x, v.y, v.z);
}
BufferGeometry& BufferGeometry::setFromPoints(const std::vector<Vector3>& points) {
    auto a = getAttribute("position");
    if (!a) {
        std::vector<float> values;
        values.reserve(points.size() * 3);
        for (const auto& p : points) {
            values.push_back(static_cast<float>(p.x));
            values.push_back(static_cast<float>(p.y));
            values.push_back(static_cast<float>(p.z == 0 || std::isnan(p.z) ? 0 : p.z));
        }
        setAttribute("position", std::make_shared<Float32BufferAttribute>(std::move(values), 3));
    } else {
        for (std::size_t i = 0; i < MathUtils::min(a->getCount(), double(points.size())); ++i)
            a->setXYZ(i, points[i].x, points[i].y,
                      points[i].z == 0 || std::isnan(points[i].z) ? 0 : points[i].z);
        a->setNeedsUpdate(true);
    }
    return *this;
}
void BufferGeometry::computeBoundingBox() {
    if (!boundingBox)
        boundingBox.emplace();
    auto p = getAttribute("position");
    if (!p) {
        boundingBox->makeEmpty();
        return;
    }
    boundingBox->setFromBufferAttribute(*p);
    if (auto it = morphAttributes.find("position"); it != morphAttributes.end())
        for (const auto& a : it->second) {
            Box3 b;
            b.setFromBufferAttribute(*a);
            Vector3 v;
            if (morphTargetsRelative) {
                v.addVectors(boundingBox->min, b.min);
                boundingBox->expandByPoint(v);
                v.addVectors(boundingBox->max, b.max);
                boundingBox->expandByPoint(v);
            } else {
                boundingBox->expandByPoint(b.min);
                boundingBox->expandByPoint(b.max);
            }
        }
}
void BufferGeometry::computeBoundingSphere() {
    if (!boundingSphere)
        boundingSphere.emplace();
    auto p = getAttribute("position");
    if (!p)
        return;
    Box3 b;
    b.setFromBufferAttribute(*p);
    auto it = morphAttributes.find("position");
    if (it != morphAttributes.end())
        for (const auto& a : it->second) {
            Box3 mb;
            mb.setFromBufferAttribute(*a);
            Vector3 v;
            if (morphTargetsRelative) {
                v.addVectors(b.min, mb.min);
                b.expandByPoint(v);
                v.addVectors(b.max, mb.max);
                b.expandByPoint(v);
            } else {
                b.expandByPoint(mb.min);
                b.expandByPoint(mb.max);
            }
        }
    b.getCenter(boundingSphere->center);
    double radiusSq = 0;
    for (std::size_t i = 0; i < p->getCount(); ++i) {
        Vector3 v;v.fromBufferAttribute(*p,double(i));
        radiusSq = MathUtils::max(radiusSq, v.distanceToSquared(boundingSphere->center));
    }
    if (it != morphAttributes.end())
        for (const auto& a : it->second)
            for (std::size_t i = 0; i < a->getCount(); ++i) {
                Vector3 v;v.fromBufferAttribute(*a,double(i));
                if (morphTargetsRelative) {
                    Vector3 base;base.fromBufferAttribute(*p,double(i));
                    v.add(base);
                }
                radiusSq = MathUtils::max(radiusSq, v.distanceToSquared(boundingSphere->center));
            }
    boundingSphere->radius = std::sqrt(radiusSq);
}
void BufferGeometry::computeVertexNormals() {
    auto p = getAttribute("position");
    if (!p)
        return;
    auto n = getAttribute("normal");
    if (!n || n->getCount() != p->getCount()) {
        n = std::make_shared<BufferAttribute>(Float32Array(p->getCount() * 3), 3);
        setAttribute("normal", n);
    } else
        for (double i = 0, limit = n->getCount(); i < limit; ++i)
            n->setXYZ(i, 0, 0, 0);
    const auto length = index ? index->count : p->getCount();
    for (double i = 0; i < length; i += 3) {
        const double a = index ? index->getX(i) : i,
                     b = index ? index->getX(i + 1) : i + 1,
                     c = index ? index->getX(i + 2) : i + 2;
          Vector3 pa, pb, pc, cb, ab;
          pa.fromBufferAttribute(*p, a); pb.fromBufferAttribute(*p, b); pc.fromBufferAttribute(*p, c);
        cb.subVectors(pc, pb);
        ab.subVectors(pa, pb);
        cb.cross(ab);
        if (index) {
              Vector3 na, nb, nc;
              na.fromBufferAttribute(*n, a); nb.fromBufferAttribute(*n, b); nc.fromBufferAttribute(*n, c);
            na.add(cb); nb.add(cb); nc.add(cb);
            n->setXYZ(a, na.x, na.y, na.z);
            n->setXYZ(b, nb.x, nb.y, nb.z);
            n->setXYZ(c, nc.x, nc.y, nc.z);
        } else {
            for (auto v : {a, b, c}) n->setXYZ(v, cb.x, cb.y, cb.z);
        }
    }
    normalizeNormals();
    n->setNeedsUpdate(true);
}
void BufferGeometry::normalizeNormals() {
    auto n = getAttribute("normal");
    if (!n)
        return;
    for (double i = 0, limit = n->getCount(); i < limit; ++i) {
        Vector3 v;
        v.fromBufferAttribute(*n, i);
        v.normalize();
        n->setXYZ(i, v.x, v.y, v.z);
    }
}
std::shared_ptr<BufferGeometry> BufferGeometry::toNonIndexed() {
    if (!index)
        return shared_from_this();
    auto result = std::make_shared<BufferGeometry>();
    const auto indices=nativeArray(index->getArray());
    const auto convert = [&](const std::shared_ptr<VertexAttribute>& a)->std::shared_ptr<BufferAttribute> {
        const auto array=nativeArray(a->getArray()),size=a->getProperty("itemSize"),normalized=a->getProperty("normalized");
        const auto constructor=array.get("constructor");const auto length=indices.get("length");
        auto values=constructor.construct({Interpolant_detail::multiply(length,size)});
        double outputIndex=0;
        const auto limit=indices.get("length");
        for(double i=0;i<limit.toNumber();++i) {
            JSValue offset;
            if(a->getProperty("isInterleavedBufferAttribute").toBoolean()) {
                const auto input=indices.get(JSValue(i).toString()),stride=a->getProperty("data").get("stride");
                const auto product=Interpolant_detail::multiply(input,stride);offset=Interpolant_detail::add(product,a->getProperty("offset"));
            } else offset=Interpolant_detail::multiply(indices.get(JSValue(i).toString()),size);
            for(double c=0;c<size.toNumber();++c) {
                const auto oldOffset=offset.toNumber();offset=oldOffset+1;
                const auto value=array.get(JSValue(oldOffset).toString());values.set(JSValue(outputIndex++).toString(),value);
            }
        }
        const auto storage=animationArray(values);
        return std::visit([&](const auto& input)->std::shared_ptr<BufferAttribute> {
            if constexpr(std::is_same_v<std::decay_t<decltype(input)>,JSValue>) throw JSTypeError("Expected TypedArray");
            else return std::make_shared<BufferAttribute>(input,size.toNumber(),normalized.toBoolean());
        },storage.storage());
    };
    for (const auto& [name, a] : attributes)
        result->setAttribute(name, convert(a));
    for (const auto& [name, as] : morphAttributes)
        for (const auto& a : as)
            result->morphAttributes[name].push_back(convert(a));
    result->morphTargetsRelative = morphTargetsRelative;
    for (const auto& group : groups)
        result->addGroup(group.start, group.count, group.materialIndex);
    return result;
}
std::shared_ptr<BufferGeometry> BufferGeometry::clone() const {
    auto result = std::make_shared<BufferGeometry>();
    result->copy(*this);
    return result;
}
BufferGeometry& BufferGeometry::copy(const BufferGeometry& source) {
    index.reset();
    attributes.clear();
    morphAttributes = SharedOrderedMap<SharedArray<std::shared_ptr<VertexAttribute>>>{};
    groups.clear();boundingBox.reset();boundingSphere.reset();
    auto metadata=JSValue::object();name=source.name;
    index=source.index?source.index->clone():nullptr;
    for (const auto& [name, a] : source.attributes)
        attributes[name] = a->cloneAttribute(metadata);
    for (const auto& [name, as] : source.morphAttributes) {
        auto& cloned = morphAttributes[name];
        for (const auto& a : as)
            cloned.push_back(a->cloneAttribute(metadata));
    }
    morphTargetsRelative = source.morphTargetsRelative;
    for (const auto& group : source.groups)
        addGroup(group.start, group.count, group.materialIndex);
    boundingBox = source.boundingBox;
    boundingSphere = source.boundingSphere;
    drawRange = source.drawRange;
    userData = source.userData.sharedCopy();
    _transformed = source._transformed;
    return *this;
}
void BufferGeometry::dispose() {
    Event event;
    event.type = "dispose";
    dispatchEvent(event);
}
JsonValue BufferGeometry::toJSON() const {
    JsonValue result =
        JsonValue::Object{{"metadata", JsonValue::Object{{"version", 4.7},
                                                         {"type", "BufferGeometry"},
                                                         {"generator", "BufferGeometry.toJSON"}}},
                          {"uuid", uuid},
                          {"type", type},
                          {"name", name}};
    if (userData.size())
        result["userData"] = userData.sharedCopy();
    auto data=JSValue::object(),attributesJSON=JSValue::object();data.set("attributes",attributesJSON);
    if(index) {
        auto serialized=index->toJSON();
        data.set("index",JSValue::fromJSON(JsonValue::Object{{"type",serialized.at("type")},{"array",serialized.at("array")}}));
    }
    for(const auto& [key,attribute]:attributes) attributesJSON.set(key,attribute->serializeAttribute(data));
    auto morphs=JSValue::object();
    for(const auto& [key,items]:morphAttributes) {
        if(items.empty()) continue;
        auto serialized=JSValue::array();
        for(const auto& attribute:items) serialized.get("push").call(serialized,{attribute->serializeAttribute(data)});
        morphs.set(key,serialized);
    }
    if(!morphs.keys().empty()) {data.set("morphAttributes",morphs);data.set("morphTargetsRelative",morphTargetsRelative);}
    if(!groups.empty()) {
        JsonValue::Array serialized;
        for(const auto& group:groups) serialized.push_back(JsonValue::Object{{"start",group.start},{"count",group.count},{"materialIndex",group.materialIndex}});
        data.set("groups",JSValue::fromJSON(JsonValue(serialized).copyJSON()));
    }
    if(boundingSphere) data.set("boundingSphere",JSValue::fromJSON(JsonValue::Object{{"radius",boundingSphere->radius},
        {"center",JsonValue::Array{boundingSphere->center.x,boundingSphere->center.y,boundingSphere->center.z}}}));
    result["data"] = *snapshotData(data);
    return result;
}

BufferGeometry& BufferGeometry::lookAt(const Vector3& vector) {
    // Upstream uses a scratch object at the origin: lookAt gives it the rotation, updateMatrix composes it
    // with that origin, and the geometry is transformed by the result.
    Object3D scratch;
    scratch.lookAt(vector);
    scratch.updateMatrix();
    return applyMatrix4(scratch.matrix);
}
void BufferGeometry::computeTangents() {
    // Based on http://www.terathon.com/code/tangent.html, per vertex tangents.
    const auto positionAttribute = std::dynamic_pointer_cast<BufferAttribute>(getAttribute("position"));
    const auto normalAttribute = std::dynamic_pointer_cast<BufferAttribute>(getAttribute("normal"));
    const auto uvAttribute = std::dynamic_pointer_cast<BufferAttribute>(getAttribute("uv"));
    if (!index || !positionAttribute || !normalAttribute || !uvAttribute) {
        // Upstream reports the missing attributes on the console and returns; nothing changes here.
        return;
    }
    const auto positionCount = static_cast<std::size_t>(positionAttribute->count);
    auto tangent = std::dynamic_pointer_cast<BufferAttribute>(getAttribute("tangent"));
    if (!tangent || tangent->count != positionAttribute->count) {
        tangent = std::make_shared<Float32BufferAttribute>(std::vector<float>(4 * positionCount, 0.0f), 4);
        setAttribute("tangent", tangent);
    }
    std::vector<Vector3> tan1(positionCount), tan2(positionCount);
    std::vector<std::uint8_t> used(positionCount, 0);
    Vector3 vA, vB, vC, sdir, tdir;
    Vector2 uvA, uvB, uvC;
    const auto handleTriangle = [&](double a, double b, double c) {
        used[static_cast<std::size_t>(a)] = used[static_cast<std::size_t>(b)] = used[static_cast<std::size_t>(c)] = 1;
        const auto read3 = [](const std::shared_ptr<BufferAttribute>& attribute, double i, Vector3& target) {
            target.x = attribute->getX(i); target.y = attribute->getY(i); target.z = attribute->getZ(i);
        };
        const auto read2 = [](const std::shared_ptr<BufferAttribute>& attribute, double i, Vector2& target) {
            target.x = attribute->getX(i); target.y = attribute->getY(i);
        };
        read3(positionAttribute, a, vA); read3(positionAttribute, b, vB); read3(positionAttribute, c, vC);
        read2(uvAttribute, a, uvA); read2(uvAttribute, b, uvB); read2(uvAttribute, c, uvC);
        vB.sub(vA); vC.sub(vA);
        uvB.sub(uvA); uvC.sub(uvA);
        const double r = 1.0 / (uvB.x * uvC.y - uvC.x * uvB.y);
        // Degenerate uv triangles are ignored, exactly as upstream does when r is not finite.
        if (!std::isfinite(r)) return;
        sdir.copy(vB).multiplyScalar(uvC.y).addScaledVector(vC, -uvB.y).multiplyScalar(r);
        tdir.copy(vC).multiplyScalar(uvB.x).addScaledVector(vB, -uvC.x).multiplyScalar(r);
        tan1[static_cast<std::size_t>(a)].add(sdir);
        tan1[static_cast<std::size_t>(b)].add(sdir);
        tan1[static_cast<std::size_t>(c)].add(sdir);
        tan2[static_cast<std::size_t>(a)].add(tdir);
        tan2[static_cast<std::size_t>(b)].add(tdir);
        tan2[static_cast<std::size_t>(c)].add(tdir);
    };
    const auto indexCount = index->count;
    if (groups.empty()) {
        // Upstream falls back to a single implicit group covering the whole index buffer.
        for (double j = 0; j < indexCount; j += 3)
            handleTriangle(index->getX(j + 0), index->getX(j + 1), index->getX(j + 2));
    } else {
        for (std::size_t g = 0; g < groups.size(); ++g) {
            const auto& group = groups[g];
            for (double j = group.start; j < group.start + group.count; j += 3)
                handleTriangle(index->getX(j + 0), index->getX(j + 1), index->getX(j + 2));
        }
    }
    Vector3 tmp, tmp2, n, n2;
    for (std::size_t v = 0; v < positionCount; ++v) {
        if (!used[v]) continue;
        n.x = normalAttribute->getX(static_cast<double>(v));
        n.y = normalAttribute->getY(static_cast<double>(v));
        n.z = normalAttribute->getZ(static_cast<double>(v));
        n2.copy(n);
        tmp.copy(tan1[v]);
        // Gram-Schmidt orthogonalize, then take the handedness from the bitangent.
        tmp.sub(n.multiplyScalar(n.dot(tan1[v]))).normalize();
        tmp2.crossVectors(n2, tan1[v]);
        const double test = tmp2.dot(tan2[v]);
        const double w = (test < 0.0) ? -1.0 : 1.0;
        tangent->setXYZW(static_cast<double>(v), tmp.x, tmp.y, tmp.z, w);
    }
    this->_transformed = true;
}

// Native statements translated from pinned BufferGeometry.js; see THREEJS-LICENSE.
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include "math/JSNumberMath.h"
#include "core/JSIterator.h"
#include "core/JSStringEncoding.h"
#include "core/BufferGeometryRuntimeSource.inc"
#include "math/Matrix3.h"
#include "math/Vector2.h"
#include "utils.h"
#include <string_view>
#include <unordered_set>
namespace BufferGeometryManaged {

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
JSValue nested0(const JSValue& nativeStatementTemp1060,const Args& nativeStatementTemp1061,const Args& nativeStatementTemp1062){
auto nativeStatementTemp1063=nativeStatementTemp1062[0];auto nativeStatementTemp1064=nativeStatementTemp1060;
JSValue a=arg(nativeStatementTemp1061,0);
JSValue b=arg(nativeStatementTemp1061,1);
JSValue c=arg(nativeStatementTemp1061,2);
([&]()->JSValue{auto nativeStatementTemp1066=nativeStatementTemp1062[1];const auto nativeStatementTemp1067=a;const auto nativeStatementTemp1068=([&]()->JSValue{auto nativeStatementTemp1071=nativeStatementTemp1062[1];const auto nativeStatementTemp1072=b;const auto nativeStatementTemp1073=([&]()->JSValue{auto nativeStatementTemp1076=nativeStatementTemp1062[1];const auto nativeStatementTemp1077=c;const auto nativeStatementTemp1078=JSValue(1);setReference(nativeStatementTemp1076,nativeStatementTemp1077,nativeStatementTemp1078);return nativeStatementTemp1078;}());setReference(nativeStatementTemp1071,nativeStatementTemp1072,nativeStatementTemp1073);return nativeStatementTemp1073;}());setReference(nativeStatementTemp1066,nativeStatementTemp1067,nativeStatementTemp1068);return nativeStatementTemp1068;}());
([&]()->JSValue{auto nativeStatementTemp1084=nativeStatementTemp1062[2];const auto nativeStatementTemp1085=JSValue("fromBufferAttribute");const auto nativeStatementTemp1081=nativeStatementTemp1084.get(nativeStatementTemp1085);const auto nativeStatementTemp1082=nativeStatementTemp1084;Args nativeStatementTemp1083;nativeStatementTemp1083.push_back(nativeStatementTemp1062[3]);nativeStatementTemp1083.push_back(a);return nativeStatementTemp1081.call(nativeStatementTemp1082,nativeStatementTemp1083);}());
([&]()->JSValue{auto nativeStatementTemp1089=nativeStatementTemp1062[4];const auto nativeStatementTemp1090=JSValue("fromBufferAttribute");const auto nativeStatementTemp1086=nativeStatementTemp1089.get(nativeStatementTemp1090);const auto nativeStatementTemp1087=nativeStatementTemp1089;Args nativeStatementTemp1088;nativeStatementTemp1088.push_back(nativeStatementTemp1062[3]);nativeStatementTemp1088.push_back(b);return nativeStatementTemp1086.call(nativeStatementTemp1087,nativeStatementTemp1088);}());
([&]()->JSValue{auto nativeStatementTemp1094=nativeStatementTemp1062[5];const auto nativeStatementTemp1095=JSValue("fromBufferAttribute");const auto nativeStatementTemp1091=nativeStatementTemp1094.get(nativeStatementTemp1095);const auto nativeStatementTemp1092=nativeStatementTemp1094;Args nativeStatementTemp1093;nativeStatementTemp1093.push_back(nativeStatementTemp1062[3]);nativeStatementTemp1093.push_back(c);return nativeStatementTemp1091.call(nativeStatementTemp1092,nativeStatementTemp1093);}());
([&]()->JSValue{auto nativeStatementTemp1099=nativeStatementTemp1062[6];const auto nativeStatementTemp1100=JSValue("fromBufferAttribute");const auto nativeStatementTemp1096=nativeStatementTemp1099.get(nativeStatementTemp1100);const auto nativeStatementTemp1097=nativeStatementTemp1099;Args nativeStatementTemp1098;nativeStatementTemp1098.push_back(nativeStatementTemp1062[7]);nativeStatementTemp1098.push_back(a);return nativeStatementTemp1096.call(nativeStatementTemp1097,nativeStatementTemp1098);}());
([&]()->JSValue{auto nativeStatementTemp1104=nativeStatementTemp1062[8];const auto nativeStatementTemp1105=JSValue("fromBufferAttribute");const auto nativeStatementTemp1101=nativeStatementTemp1104.get(nativeStatementTemp1105);const auto nativeStatementTemp1102=nativeStatementTemp1104;Args nativeStatementTemp1103;nativeStatementTemp1103.push_back(nativeStatementTemp1062[7]);nativeStatementTemp1103.push_back(b);return nativeStatementTemp1101.call(nativeStatementTemp1102,nativeStatementTemp1103);}());
([&]()->JSValue{auto nativeStatementTemp1109=nativeStatementTemp1062[9];const auto nativeStatementTemp1110=JSValue("fromBufferAttribute");const auto nativeStatementTemp1106=nativeStatementTemp1109.get(nativeStatementTemp1110);const auto nativeStatementTemp1107=nativeStatementTemp1109;Args nativeStatementTemp1108;nativeStatementTemp1108.push_back(nativeStatementTemp1062[7]);nativeStatementTemp1108.push_back(c);return nativeStatementTemp1106.call(nativeStatementTemp1107,nativeStatementTemp1108);}());
([&]()->JSValue{auto nativeStatementTemp1114=nativeStatementTemp1062[4];const auto nativeStatementTemp1115=JSValue("sub");const auto nativeStatementTemp1111=nativeStatementTemp1114.get(nativeStatementTemp1115);const auto nativeStatementTemp1112=nativeStatementTemp1114;Args nativeStatementTemp1113;nativeStatementTemp1113.push_back(nativeStatementTemp1062[2]);return nativeStatementTemp1111.call(nativeStatementTemp1112,nativeStatementTemp1113);}());
([&]()->JSValue{auto nativeStatementTemp1119=nativeStatementTemp1062[5];const auto nativeStatementTemp1120=JSValue("sub");const auto nativeStatementTemp1116=nativeStatementTemp1119.get(nativeStatementTemp1120);const auto nativeStatementTemp1117=nativeStatementTemp1119;Args nativeStatementTemp1118;nativeStatementTemp1118.push_back(nativeStatementTemp1062[2]);return nativeStatementTemp1116.call(nativeStatementTemp1117,nativeStatementTemp1118);}());
([&]()->JSValue{auto nativeStatementTemp1124=nativeStatementTemp1062[8];const auto nativeStatementTemp1125=JSValue("sub");const auto nativeStatementTemp1121=nativeStatementTemp1124.get(nativeStatementTemp1125);const auto nativeStatementTemp1122=nativeStatementTemp1124;Args nativeStatementTemp1123;nativeStatementTemp1123.push_back(nativeStatementTemp1062[6]);return nativeStatementTemp1121.call(nativeStatementTemp1122,nativeStatementTemp1123);}());
([&]()->JSValue{auto nativeStatementTemp1129=nativeStatementTemp1062[9];const auto nativeStatementTemp1130=JSValue("sub");const auto nativeStatementTemp1126=nativeStatementTemp1129.get(nativeStatementTemp1130);const auto nativeStatementTemp1127=nativeStatementTemp1129;Args nativeStatementTemp1128;nativeStatementTemp1128.push_back(nativeStatementTemp1062[6]);return nativeStatementTemp1126.call(nativeStatementTemp1127,nativeStatementTemp1128);}());
JSValue r=([&]()->JSValue{const auto nativeStatementTemp1131=JSValue(1);const auto nativeStatementTemp1132=([&]()->JSValue{const auto nativeStatementTemp1133=([&]()->JSValue{const auto nativeStatementTemp1135=([&]()->JSValue{const auto nativeStatementTemp1137=nativeStatementTemp1062[8];const auto nativeStatementTemp1138=JSValue("x");return nativeStatementTemp1137.get(nativeStatementTemp1138);}());const auto nativeStatementTemp1136=([&]()->JSValue{const auto nativeStatementTemp1139=nativeStatementTemp1062[9];const auto nativeStatementTemp1140=JSValue("y");return nativeStatementTemp1139.get(nativeStatementTemp1140);}());return binary("*",nativeStatementTemp1135,nativeStatementTemp1136);}());const auto nativeStatementTemp1134=([&]()->JSValue{const auto nativeStatementTemp1141=([&]()->JSValue{const auto nativeStatementTemp1143=nativeStatementTemp1062[9];const auto nativeStatementTemp1144=JSValue("x");return nativeStatementTemp1143.get(nativeStatementTemp1144);}());const auto nativeStatementTemp1142=([&]()->JSValue{const auto nativeStatementTemp1145=nativeStatementTemp1062[8];const auto nativeStatementTemp1146=JSValue("y");return nativeStatementTemp1145.get(nativeStatementTemp1146);}());return binary("*",nativeStatementTemp1141,nativeStatementTemp1142);}());return binary("-",nativeStatementTemp1133,nativeStatementTemp1134);}());return binary("/",nativeStatementTemp1131,nativeStatementTemp1132);}());
if(([&]()->JSValue{const auto nativeStatementTemp1147=([&]()->JSValue{const auto nativeStatementTemp1148=nativeGlobalBinding("isFinite");const JSValue nativeStatementTemp1149;Args nativeStatementTemp1150;nativeStatementTemp1150.push_back(r);return nativeStatementTemp1148.call(nativeStatementTemp1149,nativeStatementTemp1150);}());return !nativeStatementTemp1147.toBoolean();}()).toBoolean())return JSValue{};
([&]()->JSValue{auto nativeStatementTemp1154=([&]()->JSValue{auto nativeStatementTemp1159=([&]()->JSValue{auto nativeStatementTemp1164=([&]()->JSValue{auto nativeStatementTemp1169=nativeStatementTemp1062[10];const auto nativeStatementTemp1170=JSValue("copy");const auto nativeStatementTemp1166=nativeStatementTemp1169.get(nativeStatementTemp1170);const auto nativeStatementTemp1167=nativeStatementTemp1169;Args nativeStatementTemp1168;nativeStatementTemp1168.push_back(nativeStatementTemp1062[4]);return nativeStatementTemp1166.call(nativeStatementTemp1167,nativeStatementTemp1168);}());const auto nativeStatementTemp1165=JSValue("multiplyScalar");const auto nativeStatementTemp1161=nativeStatementTemp1164.get(nativeStatementTemp1165);const auto nativeStatementTemp1162=nativeStatementTemp1164;Args nativeStatementTemp1163;nativeStatementTemp1163.push_back(([&]()->JSValue{const auto nativeStatementTemp1171=nativeStatementTemp1062[9];const auto nativeStatementTemp1172=JSValue("y");return nativeStatementTemp1171.get(nativeStatementTemp1172);}()));return nativeStatementTemp1161.call(nativeStatementTemp1162,nativeStatementTemp1163);}());const auto nativeStatementTemp1160=JSValue("addScaledVector");const auto nativeStatementTemp1156=nativeStatementTemp1159.get(nativeStatementTemp1160);const auto nativeStatementTemp1157=nativeStatementTemp1159;Args nativeStatementTemp1158;nativeStatementTemp1158.push_back(nativeStatementTemp1062[5]);nativeStatementTemp1158.push_back(([&]()->JSValue{const auto nativeStatementTemp1173=([&]()->JSValue{const auto nativeStatementTemp1174=nativeStatementTemp1062[8];const auto nativeStatementTemp1175=JSValue("y");return nativeStatementTemp1174.get(nativeStatementTemp1175);}());return -nativeStatementTemp1173.toNumber();}()));return nativeStatementTemp1156.call(nativeStatementTemp1157,nativeStatementTemp1158);}());const auto nativeStatementTemp1155=JSValue("multiplyScalar");const auto nativeStatementTemp1151=nativeStatementTemp1154.get(nativeStatementTemp1155);const auto nativeStatementTemp1152=nativeStatementTemp1154;Args nativeStatementTemp1153;nativeStatementTemp1153.push_back(r);return nativeStatementTemp1151.call(nativeStatementTemp1152,nativeStatementTemp1153);}());
([&]()->JSValue{auto nativeStatementTemp1179=([&]()->JSValue{auto nativeStatementTemp1184=([&]()->JSValue{auto nativeStatementTemp1189=([&]()->JSValue{auto nativeStatementTemp1194=nativeStatementTemp1062[11];const auto nativeStatementTemp1195=JSValue("copy");const auto nativeStatementTemp1191=nativeStatementTemp1194.get(nativeStatementTemp1195);const auto nativeStatementTemp1192=nativeStatementTemp1194;Args nativeStatementTemp1193;nativeStatementTemp1193.push_back(nativeStatementTemp1062[5]);return nativeStatementTemp1191.call(nativeStatementTemp1192,nativeStatementTemp1193);}());const auto nativeStatementTemp1190=JSValue("multiplyScalar");const auto nativeStatementTemp1186=nativeStatementTemp1189.get(nativeStatementTemp1190);const auto nativeStatementTemp1187=nativeStatementTemp1189;Args nativeStatementTemp1188;nativeStatementTemp1188.push_back(([&]()->JSValue{const auto nativeStatementTemp1196=nativeStatementTemp1062[8];const auto nativeStatementTemp1197=JSValue("x");return nativeStatementTemp1196.get(nativeStatementTemp1197);}()));return nativeStatementTemp1186.call(nativeStatementTemp1187,nativeStatementTemp1188);}());const auto nativeStatementTemp1185=JSValue("addScaledVector");const auto nativeStatementTemp1181=nativeStatementTemp1184.get(nativeStatementTemp1185);const auto nativeStatementTemp1182=nativeStatementTemp1184;Args nativeStatementTemp1183;nativeStatementTemp1183.push_back(nativeStatementTemp1062[4]);nativeStatementTemp1183.push_back(([&]()->JSValue{const auto nativeStatementTemp1198=([&]()->JSValue{const auto nativeStatementTemp1199=nativeStatementTemp1062[9];const auto nativeStatementTemp1200=JSValue("x");return nativeStatementTemp1199.get(nativeStatementTemp1200);}());return -nativeStatementTemp1198.toNumber();}()));return nativeStatementTemp1181.call(nativeStatementTemp1182,nativeStatementTemp1183);}());const auto nativeStatementTemp1180=JSValue("multiplyScalar");const auto nativeStatementTemp1176=nativeStatementTemp1179.get(nativeStatementTemp1180);const auto nativeStatementTemp1177=nativeStatementTemp1179;Args nativeStatementTemp1178;nativeStatementTemp1178.push_back(r);return nativeStatementTemp1176.call(nativeStatementTemp1177,nativeStatementTemp1178);}());
([&]()->JSValue{auto nativeStatementTemp1204=([&]()->JSValue{const auto nativeStatementTemp1206=nativeStatementTemp1062[12];const auto nativeStatementTemp1207=a;return nativeStatementTemp1206.get(nativeStatementTemp1207);}());const auto nativeStatementTemp1205=JSValue("add");const auto nativeStatementTemp1201=nativeStatementTemp1204.get(nativeStatementTemp1205);const auto nativeStatementTemp1202=nativeStatementTemp1204;Args nativeStatementTemp1203;nativeStatementTemp1203.push_back(nativeStatementTemp1062[10]);return nativeStatementTemp1201.call(nativeStatementTemp1202,nativeStatementTemp1203);}());
([&]()->JSValue{auto nativeStatementTemp1211=([&]()->JSValue{const auto nativeStatementTemp1213=nativeStatementTemp1062[12];const auto nativeStatementTemp1214=b;return nativeStatementTemp1213.get(nativeStatementTemp1214);}());const auto nativeStatementTemp1212=JSValue("add");const auto nativeStatementTemp1208=nativeStatementTemp1211.get(nativeStatementTemp1212);const auto nativeStatementTemp1209=nativeStatementTemp1211;Args nativeStatementTemp1210;nativeStatementTemp1210.push_back(nativeStatementTemp1062[10]);return nativeStatementTemp1208.call(nativeStatementTemp1209,nativeStatementTemp1210);}());
([&]()->JSValue{auto nativeStatementTemp1218=([&]()->JSValue{const auto nativeStatementTemp1220=nativeStatementTemp1062[12];const auto nativeStatementTemp1221=c;return nativeStatementTemp1220.get(nativeStatementTemp1221);}());const auto nativeStatementTemp1219=JSValue("add");const auto nativeStatementTemp1215=nativeStatementTemp1218.get(nativeStatementTemp1219);const auto nativeStatementTemp1216=nativeStatementTemp1218;Args nativeStatementTemp1217;nativeStatementTemp1217.push_back(nativeStatementTemp1062[10]);return nativeStatementTemp1215.call(nativeStatementTemp1216,nativeStatementTemp1217);}());
([&]()->JSValue{auto nativeStatementTemp1225=([&]()->JSValue{const auto nativeStatementTemp1227=nativeStatementTemp1062[13];const auto nativeStatementTemp1228=a;return nativeStatementTemp1227.get(nativeStatementTemp1228);}());const auto nativeStatementTemp1226=JSValue("add");const auto nativeStatementTemp1222=nativeStatementTemp1225.get(nativeStatementTemp1226);const auto nativeStatementTemp1223=nativeStatementTemp1225;Args nativeStatementTemp1224;nativeStatementTemp1224.push_back(nativeStatementTemp1062[11]);return nativeStatementTemp1222.call(nativeStatementTemp1223,nativeStatementTemp1224);}());
([&]()->JSValue{auto nativeStatementTemp1232=([&]()->JSValue{const auto nativeStatementTemp1234=nativeStatementTemp1062[13];const auto nativeStatementTemp1235=b;return nativeStatementTemp1234.get(nativeStatementTemp1235);}());const auto nativeStatementTemp1233=JSValue("add");const auto nativeStatementTemp1229=nativeStatementTemp1232.get(nativeStatementTemp1233);const auto nativeStatementTemp1230=nativeStatementTemp1232;Args nativeStatementTemp1231;nativeStatementTemp1231.push_back(nativeStatementTemp1062[11]);return nativeStatementTemp1229.call(nativeStatementTemp1230,nativeStatementTemp1231);}());
([&]()->JSValue{auto nativeStatementTemp1239=([&]()->JSValue{const auto nativeStatementTemp1241=nativeStatementTemp1062[13];const auto nativeStatementTemp1242=c;return nativeStatementTemp1241.get(nativeStatementTemp1242);}());const auto nativeStatementTemp1240=JSValue("add");const auto nativeStatementTemp1236=nativeStatementTemp1239.get(nativeStatementTemp1240);const auto nativeStatementTemp1237=nativeStatementTemp1239;Args nativeStatementTemp1238;nativeStatementTemp1238.push_back(nativeStatementTemp1062[11]);return nativeStatementTemp1236.call(nativeStatementTemp1237,nativeStatementTemp1238);}());
return JSValue{};
}

JSValue nested1(const JSValue& nativeStatementTemp1312,const Args& nativeStatementTemp1313,const Args& nativeStatementTemp1314){
auto nativeStatementTemp1315=nativeStatementTemp1314[0];auto nativeStatementTemp1316=nativeStatementTemp1312;
JSValue v=arg(nativeStatementTemp1313,0);
([&]()->JSValue{auto nativeStatementTemp1321=nativeStatementTemp1314[1];const auto nativeStatementTemp1322=JSValue("fromBufferAttribute");const auto nativeStatementTemp1318=nativeStatementTemp1321.get(nativeStatementTemp1322);const auto nativeStatementTemp1319=nativeStatementTemp1321;Args nativeStatementTemp1320;nativeStatementTemp1320.push_back(nativeStatementTemp1314[2]);nativeStatementTemp1320.push_back(v);return nativeStatementTemp1318.call(nativeStatementTemp1319,nativeStatementTemp1320);}());
([&]()->JSValue{auto nativeStatementTemp1326=nativeStatementTemp1314[3];const auto nativeStatementTemp1327=JSValue("copy");const auto nativeStatementTemp1323=nativeStatementTemp1326.get(nativeStatementTemp1327);const auto nativeStatementTemp1324=nativeStatementTemp1326;Args nativeStatementTemp1325;nativeStatementTemp1325.push_back(nativeStatementTemp1314[1]);return nativeStatementTemp1323.call(nativeStatementTemp1324,nativeStatementTemp1325);}());
JSValue t=([&]()->JSValue{const auto nativeStatementTemp1328=nativeStatementTemp1314[4];const auto nativeStatementTemp1329=v;return nativeStatementTemp1328.get(nativeStatementTemp1329);}());
([&]()->JSValue{auto nativeStatementTemp1333=nativeStatementTemp1314[5];const auto nativeStatementTemp1334=JSValue("copy");const auto nativeStatementTemp1330=nativeStatementTemp1333.get(nativeStatementTemp1334);const auto nativeStatementTemp1331=nativeStatementTemp1333;Args nativeStatementTemp1332;nativeStatementTemp1332.push_back(t);return nativeStatementTemp1330.call(nativeStatementTemp1331,nativeStatementTemp1332);}());
([&]()->JSValue{auto nativeStatementTemp1338=([&]()->JSValue{auto nativeStatementTemp1343=nativeStatementTemp1314[5];const auto nativeStatementTemp1344=JSValue("sub");const auto nativeStatementTemp1340=nativeStatementTemp1343.get(nativeStatementTemp1344);const auto nativeStatementTemp1341=nativeStatementTemp1343;Args nativeStatementTemp1342;nativeStatementTemp1342.push_back(([&]()->JSValue{auto nativeStatementTemp1348=nativeStatementTemp1314[1];const auto nativeStatementTemp1349=JSValue("multiplyScalar");const auto nativeStatementTemp1345=nativeStatementTemp1348.get(nativeStatementTemp1349);const auto nativeStatementTemp1346=nativeStatementTemp1348;Args nativeStatementTemp1347;nativeStatementTemp1347.push_back(([&]()->JSValue{auto nativeStatementTemp1353=nativeStatementTemp1314[1];const auto nativeStatementTemp1354=JSValue("dot");const auto nativeStatementTemp1350=nativeStatementTemp1353.get(nativeStatementTemp1354);const auto nativeStatementTemp1351=nativeStatementTemp1353;Args nativeStatementTemp1352;nativeStatementTemp1352.push_back(t);return nativeStatementTemp1350.call(nativeStatementTemp1351,nativeStatementTemp1352);}()));return nativeStatementTemp1345.call(nativeStatementTemp1346,nativeStatementTemp1347);}()));return nativeStatementTemp1340.call(nativeStatementTemp1341,nativeStatementTemp1342);}());const auto nativeStatementTemp1339=JSValue("normalize");const auto nativeStatementTemp1335=nativeStatementTemp1338.get(nativeStatementTemp1339);const auto nativeStatementTemp1336=nativeStatementTemp1338;Args nativeStatementTemp1337;return nativeStatementTemp1335.call(nativeStatementTemp1336,nativeStatementTemp1337);}());
([&]()->JSValue{auto nativeStatementTemp1358=nativeStatementTemp1314[6];const auto nativeStatementTemp1359=JSValue("crossVectors");const auto nativeStatementTemp1355=nativeStatementTemp1358.get(nativeStatementTemp1359);const auto nativeStatementTemp1356=nativeStatementTemp1358;Args nativeStatementTemp1357;nativeStatementTemp1357.push_back(nativeStatementTemp1314[3]);nativeStatementTemp1357.push_back(t);return nativeStatementTemp1355.call(nativeStatementTemp1356,nativeStatementTemp1357);}());
JSValue test=([&]()->JSValue{auto nativeStatementTemp1363=nativeStatementTemp1314[6];const auto nativeStatementTemp1364=JSValue("dot");const auto nativeStatementTemp1360=nativeStatementTemp1363.get(nativeStatementTemp1364);const auto nativeStatementTemp1361=nativeStatementTemp1363;Args nativeStatementTemp1362;nativeStatementTemp1362.push_back(([&]()->JSValue{const auto nativeStatementTemp1365=nativeStatementTemp1314[7];const auto nativeStatementTemp1366=v;return nativeStatementTemp1365.get(nativeStatementTemp1366);}()));return nativeStatementTemp1360.call(nativeStatementTemp1361,nativeStatementTemp1362);}());
JSValue w=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp1367=test;const auto nativeStatementTemp1368=JSValue(0);return binary("<",nativeStatementTemp1367,nativeStatementTemp1368);}()).toBoolean())return ([&]()->JSValue{const auto nativeStatementTemp1369=JSValue(1);return -nativeStatementTemp1369.toNumber();}());return JSValue(1);}());
([&]()->JSValue{auto nativeStatementTemp1373=nativeStatementTemp1314[8];const auto nativeStatementTemp1374=JSValue("setXYZW");const auto nativeStatementTemp1370=nativeStatementTemp1373.get(nativeStatementTemp1374);const auto nativeStatementTemp1371=nativeStatementTemp1373;Args nativeStatementTemp1372;nativeStatementTemp1372.push_back(v);nativeStatementTemp1372.push_back(([&]()->JSValue{const auto nativeStatementTemp1375=nativeStatementTemp1314[5];const auto nativeStatementTemp1376=JSValue("x");return nativeStatementTemp1375.get(nativeStatementTemp1376);}()));nativeStatementTemp1372.push_back(([&]()->JSValue{const auto nativeStatementTemp1377=nativeStatementTemp1314[5];const auto nativeStatementTemp1378=JSValue("y");return nativeStatementTemp1377.get(nativeStatementTemp1378);}()));nativeStatementTemp1372.push_back(([&]()->JSValue{const auto nativeStatementTemp1379=nativeStatementTemp1314[5];const auto nativeStatementTemp1380=JSValue("z");return nativeStatementTemp1379.get(nativeStatementTemp1380);}()));nativeStatementTemp1372.push_back(w);return nativeStatementTemp1370.call(nativeStatementTemp1371,nativeStatementTemp1372);}());
return JSValue{};
}

JSValue nested2(const JSValue& nativeStatementTemp1726,const Args& nativeStatementTemp1727,const Args& nativeStatementTemp1728){
auto nativeStatementTemp1729=nativeStatementTemp1728[0];auto nativeStatementTemp1730=nativeStatementTemp1726;
JSValue attribute=arg(nativeStatementTemp1727,0);
JSValue indices=arg(nativeStatementTemp1727,1);
JSValue array=([&]()->JSValue{const auto nativeStatementTemp1732=attribute;const auto nativeStatementTemp1733=JSValue("array");return nativeStatementTemp1732.get(nativeStatementTemp1733);}());
JSValue itemSize=([&]()->JSValue{const auto nativeStatementTemp1734=attribute;const auto nativeStatementTemp1735=JSValue("itemSize");return nativeStatementTemp1734.get(nativeStatementTemp1735);}());
JSValue normalized=([&]()->JSValue{const auto nativeStatementTemp1736=attribute;const auto nativeStatementTemp1737=JSValue("normalized");return nativeStatementTemp1736.get(nativeStatementTemp1737);}());
JSValue array2=([&]()->JSValue{const auto nativeStatementTemp1738=([&]()->JSValue{const auto nativeStatementTemp1740=array;const auto nativeStatementTemp1741=JSValue("constructor");return nativeStatementTemp1740.get(nativeStatementTemp1741);}());Args nativeStatementTemp1739;nativeStatementTemp1739.push_back(([&]()->JSValue{const auto nativeStatementTemp1742=([&]()->JSValue{const auto nativeStatementTemp1744=indices;const auto nativeStatementTemp1745=JSValue("length");return nativeStatementTemp1744.get(nativeStatementTemp1745);}());const auto nativeStatementTemp1743=itemSize;return binary("*",nativeStatementTemp1742,nativeStatementTemp1743);}()));return nativeStatementTemp1738.construct(nativeStatementTemp1739);}());
JSValue index=JSValue(0);
JSValue index2=JSValue(0);
{JSValue i=JSValue(0);
JSValue l=([&]()->JSValue{const auto nativeStatementTemp1746=indices;const auto nativeStatementTemp1747=JSValue("length");return nativeStatementTemp1746.get(nativeStatementTemp1747);}());
for(;([&]()->JSValue{const auto nativeStatementTemp1748=i;const auto nativeStatementTemp1749=l;return binary("<",nativeStatementTemp1748,nativeStatementTemp1749);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1750=i.toNumber();const auto nativeStatementTemp1751=nativeStatementTemp1750+1;i=nativeStatementTemp1751;return nativeStatementTemp1750;}())){
if(([&]()->JSValue{const auto nativeStatementTemp1752=attribute;const auto nativeStatementTemp1753=JSValue("isInterleavedBufferAttribute");return nativeStatementTemp1752.get(nativeStatementTemp1753);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp1754=([&]()->JSValue{const auto nativeStatementTemp1757=([&]()->JSValue{const auto nativeStatementTemp1759=([&]()->JSValue{const auto nativeStatementTemp1761=indices;const auto nativeStatementTemp1762=i;return nativeStatementTemp1761.get(nativeStatementTemp1762);}());const auto nativeStatementTemp1760=([&]()->JSValue{const auto nativeStatementTemp1763=([&]()->JSValue{const auto nativeStatementTemp1765=attribute;const auto nativeStatementTemp1766=JSValue("data");return nativeStatementTemp1765.get(nativeStatementTemp1766);}());const auto nativeStatementTemp1764=JSValue("stride");return nativeStatementTemp1763.get(nativeStatementTemp1764);}());return binary("*",nativeStatementTemp1759,nativeStatementTemp1760);}());const auto nativeStatementTemp1758=([&]()->JSValue{const auto nativeStatementTemp1767=attribute;const auto nativeStatementTemp1768=JSValue("offset");return nativeStatementTemp1767.get(nativeStatementTemp1768);}());return binary("+",nativeStatementTemp1757,nativeStatementTemp1758);}());index=nativeStatementTemp1754;return nativeStatementTemp1754;}());

}
else {
([&]()->JSValue{const auto nativeStatementTemp1769=([&]()->JSValue{const auto nativeStatementTemp1772=([&]()->JSValue{const auto nativeStatementTemp1774=indices;const auto nativeStatementTemp1775=i;return nativeStatementTemp1774.get(nativeStatementTemp1775);}());const auto nativeStatementTemp1773=itemSize;return binary("*",nativeStatementTemp1772,nativeStatementTemp1773);}());index=nativeStatementTemp1769;return nativeStatementTemp1769;}());

}
{JSValue j=JSValue(0);
for(;([&]()->JSValue{const auto nativeStatementTemp1776=j;const auto nativeStatementTemp1777=itemSize;return binary("<",nativeStatementTemp1776,nativeStatementTemp1777);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1778=j.toNumber();const auto nativeStatementTemp1779=nativeStatementTemp1778+1;j=nativeStatementTemp1779;return nativeStatementTemp1778;}())){
([&]()->JSValue{auto nativeStatementTemp1780=array2;const auto nativeStatementTemp1781=([&]()->JSValue{const auto nativeStatementTemp1782=index2.toNumber();const auto nativeStatementTemp1783=nativeStatementTemp1782+1;index2=nativeStatementTemp1783;return nativeStatementTemp1782;}());const auto nativeStatementTemp1784=([&]()->JSValue{const auto nativeStatementTemp1787=array;const auto nativeStatementTemp1788=([&]()->JSValue{const auto nativeStatementTemp1789=index.toNumber();const auto nativeStatementTemp1790=nativeStatementTemp1789+1;index=nativeStatementTemp1790;return nativeStatementTemp1789;}());return nativeStatementTemp1787.get(nativeStatementTemp1788);}());setReference(nativeStatementTemp1780,nativeStatementTemp1781,nativeStatementTemp1784);return nativeStatementTemp1784;}());

}
}

}
}
return ([&]()->JSValue{const auto nativeStatementTemp1791=nativeStatementTemp1729.get("BufferAttribute");Args nativeStatementTemp1792;nativeStatementTemp1792.push_back(array2);nativeStatementTemp1792.push_back(itemSize);nativeStatementTemp1792.push_back(normalized);return nativeStatementTemp1791.construct(nativeStatementTemp1792);}());
return JSValue{};
}
JSValue method0(const JSValue& nativeStatementTemp0,const Args& nativeStatementTemp1,const Args& nativeStatementTemp2){
auto nativeStatementTemp3=nativeStatementTemp2[0];auto nativeStatementTemp4=nativeStatementTemp0;
const auto nativeStatementTemp5=nativeStatementTemp0;nativeStatementTemp4={};
([&]()->JSValue{const auto nativeStatementTemp9=nativeStatementTemp3.get("BufferGeometry").getPrototype();Args nativeStatementTemp8;nativeStatementTemp4=nativeStatementTemp9.construct(nativeStatementTemp8,nativeStatementTemp5);return nativeStatementTemp4;}());
([&]()->JSValue{auto nativeStatementTemp10=nativeStatementTemp4;const auto nativeStatementTemp11=JSValue("isBufferGeometry");const auto nativeStatementTemp12=JSValue(true);setReference(nativeStatementTemp10,nativeStatementTemp11,nativeStatementTemp12);return nativeStatementTemp12;}());
([&]()->JSValue{auto nativeStatementTemp18=nativeGlobalBinding("Object");const auto nativeStatementTemp19=JSValue("defineProperty");const auto nativeStatementTemp15=nativeStatementTemp18.get(nativeStatementTemp19);const auto nativeStatementTemp16=nativeStatementTemp18;Args nativeStatementTemp17;nativeStatementTemp17.push_back(nativeStatementTemp4);nativeStatementTemp17.push_back(JSValue("id"));nativeStatementTemp17.push_back(([&]()->JSValue{auto nativeStatementTemp20=JSValue::object();{const auto nativeStatementTemp21=JSValue("value");const auto nativeStatementTemp22=([&]()->JSValue{const auto nativeStatementTemp23=nativeStatementTemp3.get("_id").toNumber();const auto nativeStatementTemp24=nativeStatementTemp23+1;nativeStatementTemp3.set("_id",nativeStatementTemp24);return nativeStatementTemp23;}());own(nativeStatementTemp20,nativeStatementTemp21,nativeStatementTemp22);}return nativeStatementTemp20;}()));return nativeStatementTemp15.call(nativeStatementTemp16,nativeStatementTemp17);}());
([&]()->JSValue{auto nativeStatementTemp25=nativeStatementTemp4;const auto nativeStatementTemp26=JSValue("uuid");const auto nativeStatementTemp27=([&]()->JSValue{const auto nativeStatementTemp30=nativeStatementTemp3.get("generateUUID");const JSValue nativeStatementTemp31;Args nativeStatementTemp32;return nativeStatementTemp30.call(nativeStatementTemp31,nativeStatementTemp32);}());setReference(nativeStatementTemp25,nativeStatementTemp26,nativeStatementTemp27);return nativeStatementTemp27;}());
([&]()->JSValue{auto nativeStatementTemp33=nativeStatementTemp4;const auto nativeStatementTemp34=JSValue("name");const auto nativeStatementTemp35=JSValue("");setReference(nativeStatementTemp33,nativeStatementTemp34,nativeStatementTemp35);return nativeStatementTemp35;}());
([&]()->JSValue{auto nativeStatementTemp38=nativeStatementTemp4;const auto nativeStatementTemp39=JSValue("type");const auto nativeStatementTemp40=JSValue("BufferGeometry");setReference(nativeStatementTemp38,nativeStatementTemp39,nativeStatementTemp40);return nativeStatementTemp40;}());
([&]()->JSValue{auto nativeStatementTemp43=nativeStatementTemp4;const auto nativeStatementTemp44=JSValue("index");const auto nativeStatementTemp45=JSValue(nullptr);setReference(nativeStatementTemp43,nativeStatementTemp44,nativeStatementTemp45);return nativeStatementTemp45;}());
([&]()->JSValue{auto nativeStatementTemp48=nativeStatementTemp4;const auto nativeStatementTemp49=JSValue("indirect");const auto nativeStatementTemp50=JSValue(nullptr);setReference(nativeStatementTemp48,nativeStatementTemp49,nativeStatementTemp50);return nativeStatementTemp50;}());
([&]()->JSValue{auto nativeStatementTemp53=nativeStatementTemp4;const auto nativeStatementTemp54=JSValue("indirectOffset");const auto nativeStatementTemp55=JSValue(0);setReference(nativeStatementTemp53,nativeStatementTemp54,nativeStatementTemp55);return nativeStatementTemp55;}());
([&]()->JSValue{auto nativeStatementTemp58=nativeStatementTemp4;const auto nativeStatementTemp59=JSValue("attributes");const auto nativeStatementTemp60=([&]()->JSValue{auto nativeStatementTemp63=JSValue::object();return nativeStatementTemp63;}());setReference(nativeStatementTemp58,nativeStatementTemp59,nativeStatementTemp60);return nativeStatementTemp60;}());
([&]()->JSValue{auto nativeStatementTemp64=nativeStatementTemp4;const auto nativeStatementTemp65=JSValue("morphAttributes");const auto nativeStatementTemp66=([&]()->JSValue{auto nativeStatementTemp69=JSValue::object();return nativeStatementTemp69;}());setReference(nativeStatementTemp64,nativeStatementTemp65,nativeStatementTemp66);return nativeStatementTemp66;}());
([&]()->JSValue{auto nativeStatementTemp70=nativeStatementTemp4;const auto nativeStatementTemp71=JSValue("morphTargetsRelative");const auto nativeStatementTemp72=JSValue(false);setReference(nativeStatementTemp70,nativeStatementTemp71,nativeStatementTemp72);return nativeStatementTemp72;}());
([&]()->JSValue{auto nativeStatementTemp75=nativeStatementTemp4;const auto nativeStatementTemp76=JSValue("groups");const auto nativeStatementTemp77=([&]()->JSValue{auto nativeStatementTemp80=JSValue::array();return nativeStatementTemp80;}());setReference(nativeStatementTemp75,nativeStatementTemp76,nativeStatementTemp77);return nativeStatementTemp77;}());
([&]()->JSValue{auto nativeStatementTemp81=nativeStatementTemp4;const auto nativeStatementTemp82=JSValue("boundingBox");const auto nativeStatementTemp83=JSValue(nullptr);setReference(nativeStatementTemp81,nativeStatementTemp82,nativeStatementTemp83);return nativeStatementTemp83;}());
([&]()->JSValue{auto nativeStatementTemp86=nativeStatementTemp4;const auto nativeStatementTemp87=JSValue("boundingSphere");const auto nativeStatementTemp88=JSValue(nullptr);setReference(nativeStatementTemp86,nativeStatementTemp87,nativeStatementTemp88);return nativeStatementTemp88;}());
([&]()->JSValue{auto nativeStatementTemp91=nativeStatementTemp4;const auto nativeStatementTemp92=JSValue("drawRange");const auto nativeStatementTemp93=([&]()->JSValue{auto nativeStatementTemp96=JSValue::object();{const auto nativeStatementTemp97=JSValue("start");const auto nativeStatementTemp98=JSValue(0);own(nativeStatementTemp96,nativeStatementTemp97,nativeStatementTemp98);}{const auto nativeStatementTemp99=JSValue("count");const auto nativeStatementTemp100=nativeGlobalBinding("Infinity");own(nativeStatementTemp96,nativeStatementTemp99,nativeStatementTemp100);}return nativeStatementTemp96;}());setReference(nativeStatementTemp91,nativeStatementTemp92,nativeStatementTemp93);return nativeStatementTemp93;}());
([&]()->JSValue{auto nativeStatementTemp101=nativeStatementTemp4;const auto nativeStatementTemp102=JSValue("userData");const auto nativeStatementTemp103=([&]()->JSValue{auto nativeStatementTemp106=JSValue::object();return nativeStatementTemp106;}());setReference(nativeStatementTemp101,nativeStatementTemp102,nativeStatementTemp103);return nativeStatementTemp103;}());
([&]()->JSValue{auto nativeStatementTemp107=nativeStatementTemp4;const auto nativeStatementTemp108=JSValue("_transformed");const auto nativeStatementTemp109=JSValue(false);setReference(nativeStatementTemp107,nativeStatementTemp108,nativeStatementTemp109);return nativeStatementTemp109;}());
return nativeStatementTemp4;
}

JSValue method1(const JSValue& nativeStatementTemp112,const Args& nativeStatementTemp113,const Args& nativeStatementTemp114){
auto nativeStatementTemp115=nativeStatementTemp114[0];auto nativeStatementTemp116=nativeStatementTemp112;
return ([&]()->JSValue{const auto nativeStatementTemp118=nativeStatementTemp116;const auto nativeStatementTemp119=JSValue("index");return nativeStatementTemp118.get(nativeStatementTemp119);}());
return JSValue{};
}

JSValue method2(const JSValue& nativeStatementTemp120,const Args& nativeStatementTemp121,const Args& nativeStatementTemp122){
auto nativeStatementTemp123=nativeStatementTemp122[0];auto nativeStatementTemp124=nativeStatementTemp120;
JSValue index=arg(nativeStatementTemp121,0);
if(([&]()->JSValue{auto nativeStatementTemp129=nativeGlobalBinding("Array");const auto nativeStatementTemp130=JSValue("isArray");const auto nativeStatementTemp126=nativeStatementTemp129.get(nativeStatementTemp130);const auto nativeStatementTemp127=nativeStatementTemp129;Args nativeStatementTemp128;nativeStatementTemp128.push_back(index);return nativeStatementTemp126.call(nativeStatementTemp127,nativeStatementTemp128);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp131=nativeStatementTemp124;const auto nativeStatementTemp132=JSValue("index");const auto nativeStatementTemp133=([&]()->JSValue{const auto nativeStatementTemp136=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp138=nativeStatementTemp123.get("arrayNeedsUint32");const JSValue nativeStatementTemp139;Args nativeStatementTemp140;nativeStatementTemp140.push_back(index);return nativeStatementTemp138.call(nativeStatementTemp139,nativeStatementTemp140);}()).toBoolean())return nativeStatementTemp123.get("Uint32BufferAttribute");return nativeStatementTemp123.get("Uint16BufferAttribute");}());Args nativeStatementTemp137;nativeStatementTemp137.push_back(index);nativeStatementTemp137.push_back(JSValue(1));return nativeStatementTemp136.construct(nativeStatementTemp137);}());setReference(nativeStatementTemp131,nativeStatementTemp132,nativeStatementTemp133);return nativeStatementTemp133;}());

}
else {
([&]()->JSValue{auto nativeStatementTemp141=nativeStatementTemp124;const auto nativeStatementTemp142=JSValue("index");const auto nativeStatementTemp143=index;setReference(nativeStatementTemp141,nativeStatementTemp142,nativeStatementTemp143);return nativeStatementTemp143;}());

}
return nativeStatementTemp124;
return JSValue{};
}

JSValue method3(const JSValue& nativeStatementTemp146,const Args& nativeStatementTemp147,const Args& nativeStatementTemp148){
auto nativeStatementTemp149=nativeStatementTemp148[0];auto nativeStatementTemp150=nativeStatementTemp146;
JSValue indirect=arg(nativeStatementTemp147,0);
JSValue indirectOffset=arg(nativeStatementTemp147,1);
if(indirectOffset.isUndefined())indirectOffset=JSValue(0);
([&]()->JSValue{auto nativeStatementTemp152=nativeStatementTemp150;const auto nativeStatementTemp153=JSValue("indirect");const auto nativeStatementTemp154=indirect;setReference(nativeStatementTemp152,nativeStatementTemp153,nativeStatementTemp154);return nativeStatementTemp154;}());
([&]()->JSValue{auto nativeStatementTemp157=nativeStatementTemp150;const auto nativeStatementTemp158=JSValue("indirectOffset");const auto nativeStatementTemp159=indirectOffset;setReference(nativeStatementTemp157,nativeStatementTemp158,nativeStatementTemp159);return nativeStatementTemp159;}());
return nativeStatementTemp150;
return JSValue{};
}

JSValue method4(const JSValue& nativeStatementTemp162,const Args& nativeStatementTemp163,const Args& nativeStatementTemp164){
auto nativeStatementTemp165=nativeStatementTemp164[0];auto nativeStatementTemp166=nativeStatementTemp162;
return ([&]()->JSValue{const auto nativeStatementTemp168=nativeStatementTemp166;const auto nativeStatementTemp169=JSValue("indirect");return nativeStatementTemp168.get(nativeStatementTemp169);}());
return JSValue{};
}

JSValue method5(const JSValue& nativeStatementTemp170,const Args& nativeStatementTemp171,const Args& nativeStatementTemp172){
auto nativeStatementTemp173=nativeStatementTemp172[0];auto nativeStatementTemp174=nativeStatementTemp170;
JSValue name=arg(nativeStatementTemp171,0);
return ([&]()->JSValue{const auto nativeStatementTemp176=([&]()->JSValue{const auto nativeStatementTemp178=nativeStatementTemp174;const auto nativeStatementTemp179=JSValue("attributes");return nativeStatementTemp178.get(nativeStatementTemp179);}());const auto nativeStatementTemp177=name;return nativeStatementTemp176.get(nativeStatementTemp177);}());
return JSValue{};
}

JSValue method6(const JSValue& nativeStatementTemp180,const Args& nativeStatementTemp181,const Args& nativeStatementTemp182){
auto nativeStatementTemp183=nativeStatementTemp182[0];auto nativeStatementTemp184=nativeStatementTemp180;
JSValue name=arg(nativeStatementTemp181,0);
JSValue attribute=arg(nativeStatementTemp181,1);
([&]()->JSValue{auto nativeStatementTemp186=([&]()->JSValue{const auto nativeStatementTemp188=nativeStatementTemp184;const auto nativeStatementTemp189=JSValue("attributes");return nativeStatementTemp188.get(nativeStatementTemp189);}());const auto nativeStatementTemp187=name;const auto nativeStatementTemp190=attribute;setReference(nativeStatementTemp186,nativeStatementTemp187,nativeStatementTemp190);return nativeStatementTemp190;}());
return nativeStatementTemp184;
return JSValue{};
}

JSValue method7(const JSValue& nativeStatementTemp193,const Args& nativeStatementTemp194,const Args& nativeStatementTemp195){
auto nativeStatementTemp196=nativeStatementTemp195[0];auto nativeStatementTemp197=nativeStatementTemp193;
JSValue name=arg(nativeStatementTemp194,0);
([&]()->JSValue{auto nativeStatementTemp199=([&]()->JSValue{const auto nativeStatementTemp201=nativeStatementTemp197;const auto nativeStatementTemp202=JSValue("attributes");return nativeStatementTemp201.get(nativeStatementTemp202);}());const auto nativeStatementTemp200=name;return deleteReference(nativeStatementTemp199,nativeStatementTemp200);}());
return nativeStatementTemp197;
return JSValue{};
}

JSValue method8(const JSValue& nativeStatementTemp203,const Args& nativeStatementTemp204,const Args& nativeStatementTemp205){
auto nativeStatementTemp206=nativeStatementTemp205[0];auto nativeStatementTemp207=nativeStatementTemp203;
JSValue name=arg(nativeStatementTemp204,0);
return ([&]()->JSValue{const auto nativeStatementTemp209=([&]()->JSValue{const auto nativeStatementTemp211=([&]()->JSValue{const auto nativeStatementTemp213=nativeStatementTemp207;const auto nativeStatementTemp214=JSValue("attributes");return nativeStatementTemp213.get(nativeStatementTemp214);}());const auto nativeStatementTemp212=name;return nativeStatementTemp211.get(nativeStatementTemp212);}());const auto nativeStatementTemp210=JSValue{};return binary("!==",nativeStatementTemp209,nativeStatementTemp210);}());
return JSValue{};
}

JSValue method9(const JSValue& nativeStatementTemp215,const Args& nativeStatementTemp216,const Args& nativeStatementTemp217){
auto nativeStatementTemp218=nativeStatementTemp217[0];auto nativeStatementTemp219=nativeStatementTemp215;
JSValue start=arg(nativeStatementTemp216,0);
JSValue count=arg(nativeStatementTemp216,1);
JSValue materialIndex=arg(nativeStatementTemp216,2);
if(materialIndex.isUndefined())materialIndex=JSValue(0);
([&]()->JSValue{auto nativeStatementTemp224=([&]()->JSValue{const auto nativeStatementTemp226=nativeStatementTemp219;const auto nativeStatementTemp227=JSValue("groups");return nativeStatementTemp226.get(nativeStatementTemp227);}());const auto nativeStatementTemp225=JSValue("push");const auto nativeStatementTemp221=nativeStatementTemp224.get(nativeStatementTemp225);const auto nativeStatementTemp222=nativeStatementTemp224;Args nativeStatementTemp223;nativeStatementTemp223.push_back(([&]()->JSValue{auto nativeStatementTemp228=JSValue::object();{const auto nativeStatementTemp229=JSValue("start");const auto nativeStatementTemp230=start;own(nativeStatementTemp228,nativeStatementTemp229,nativeStatementTemp230);}{const auto nativeStatementTemp231=JSValue("count");const auto nativeStatementTemp232=count;own(nativeStatementTemp228,nativeStatementTemp231,nativeStatementTemp232);}{const auto nativeStatementTemp233=JSValue("materialIndex");const auto nativeStatementTemp234=materialIndex;own(nativeStatementTemp228,nativeStatementTemp233,nativeStatementTemp234);}return nativeStatementTemp228;}()));return nativeStatementTemp221.call(nativeStatementTemp222,nativeStatementTemp223);}());
return JSValue{};
}

JSValue method10(const JSValue& nativeStatementTemp235,const Args& nativeStatementTemp236,const Args& nativeStatementTemp237){
auto nativeStatementTemp238=nativeStatementTemp237[0];auto nativeStatementTemp239=nativeStatementTemp235;
([&]()->JSValue{auto nativeStatementTemp241=nativeStatementTemp239;const auto nativeStatementTemp242=JSValue("groups");const auto nativeStatementTemp243=([&]()->JSValue{auto nativeStatementTemp246=JSValue::array();return nativeStatementTemp246;}());setReference(nativeStatementTemp241,nativeStatementTemp242,nativeStatementTemp243);return nativeStatementTemp243;}());
return JSValue{};
}

JSValue method11(const JSValue& nativeStatementTemp247,const Args& nativeStatementTemp248,const Args& nativeStatementTemp249){
auto nativeStatementTemp250=nativeStatementTemp249[0];auto nativeStatementTemp251=nativeStatementTemp247;
JSValue start=arg(nativeStatementTemp248,0);
JSValue count=arg(nativeStatementTemp248,1);
([&]()->JSValue{auto nativeStatementTemp253=([&]()->JSValue{const auto nativeStatementTemp255=nativeStatementTemp251;const auto nativeStatementTemp256=JSValue("drawRange");return nativeStatementTemp255.get(nativeStatementTemp256);}());const auto nativeStatementTemp254=JSValue("start");const auto nativeStatementTemp257=start;setReference(nativeStatementTemp253,nativeStatementTemp254,nativeStatementTemp257);return nativeStatementTemp257;}());
([&]()->JSValue{auto nativeStatementTemp260=([&]()->JSValue{const auto nativeStatementTemp262=nativeStatementTemp251;const auto nativeStatementTemp263=JSValue("drawRange");return nativeStatementTemp262.get(nativeStatementTemp263);}());const auto nativeStatementTemp261=JSValue("count");const auto nativeStatementTemp264=count;setReference(nativeStatementTemp260,nativeStatementTemp261,nativeStatementTemp264);return nativeStatementTemp264;}());
return JSValue{};
}

JSValue method12(const JSValue& nativeStatementTemp267,const Args& nativeStatementTemp268,const Args& nativeStatementTemp269){
auto nativeStatementTemp270=nativeStatementTemp269[0];auto nativeStatementTemp271=nativeStatementTemp267;
JSValue matrix=arg(nativeStatementTemp268,0);
JSValue position=([&]()->JSValue{const auto nativeStatementTemp273=([&]()->JSValue{const auto nativeStatementTemp275=nativeStatementTemp271;const auto nativeStatementTemp276=JSValue("attributes");return nativeStatementTemp275.get(nativeStatementTemp276);}());const auto nativeStatementTemp274=JSValue("position");return nativeStatementTemp273.get(nativeStatementTemp274);}());
if(([&]()->JSValue{const auto nativeStatementTemp277=position;const auto nativeStatementTemp278=JSValue{};return binary("!==",nativeStatementTemp277,nativeStatementTemp278);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp282=position;const auto nativeStatementTemp283=JSValue("applyMatrix4");const auto nativeStatementTemp279=nativeStatementTemp282.get(nativeStatementTemp283);const auto nativeStatementTemp280=nativeStatementTemp282;Args nativeStatementTemp281;nativeStatementTemp281.push_back(matrix);return nativeStatementTemp279.call(nativeStatementTemp280,nativeStatementTemp281);}());
([&]()->JSValue{auto nativeStatementTemp284=position;const auto nativeStatementTemp285=JSValue("needsUpdate");const auto nativeStatementTemp286=JSValue(true);setReference(nativeStatementTemp284,nativeStatementTemp285,nativeStatementTemp286);return nativeStatementTemp286;}());

}
JSValue normal=([&]()->JSValue{const auto nativeStatementTemp289=([&]()->JSValue{const auto nativeStatementTemp291=nativeStatementTemp271;const auto nativeStatementTemp292=JSValue("attributes");return nativeStatementTemp291.get(nativeStatementTemp292);}());const auto nativeStatementTemp290=JSValue("normal");return nativeStatementTemp289.get(nativeStatementTemp290);}());
if(([&]()->JSValue{const auto nativeStatementTemp293=normal;const auto nativeStatementTemp294=JSValue{};return binary("!==",nativeStatementTemp293,nativeStatementTemp294);}()).toBoolean()){
JSValue normalMatrix=([&]()->JSValue{auto nativeStatementTemp298=([&]()->JSValue{const auto nativeStatementTemp300=nativeStatementTemp270.get("Matrix3");Args nativeStatementTemp301;return nativeStatementTemp300.construct(nativeStatementTemp301);}());const auto nativeStatementTemp299=JSValue("getNormalMatrix");const auto nativeStatementTemp295=nativeStatementTemp298.get(nativeStatementTemp299);const auto nativeStatementTemp296=nativeStatementTemp298;Args nativeStatementTemp297;nativeStatementTemp297.push_back(matrix);return nativeStatementTemp295.call(nativeStatementTemp296,nativeStatementTemp297);}());
([&]()->JSValue{auto nativeStatementTemp305=normal;const auto nativeStatementTemp306=JSValue("applyNormalMatrix");const auto nativeStatementTemp302=nativeStatementTemp305.get(nativeStatementTemp306);const auto nativeStatementTemp303=nativeStatementTemp305;Args nativeStatementTemp304;nativeStatementTemp304.push_back(normalMatrix);return nativeStatementTemp302.call(nativeStatementTemp303,nativeStatementTemp304);}());
([&]()->JSValue{auto nativeStatementTemp307=normal;const auto nativeStatementTemp308=JSValue("needsUpdate");const auto nativeStatementTemp309=JSValue(true);setReference(nativeStatementTemp307,nativeStatementTemp308,nativeStatementTemp309);return nativeStatementTemp309;}());

}
JSValue tangent=([&]()->JSValue{const auto nativeStatementTemp312=([&]()->JSValue{const auto nativeStatementTemp314=nativeStatementTemp271;const auto nativeStatementTemp315=JSValue("attributes");return nativeStatementTemp314.get(nativeStatementTemp315);}());const auto nativeStatementTemp313=JSValue("tangent");return nativeStatementTemp312.get(nativeStatementTemp313);}());
if(([&]()->JSValue{const auto nativeStatementTemp316=tangent;const auto nativeStatementTemp317=JSValue{};return binary("!==",nativeStatementTemp316,nativeStatementTemp317);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp321=tangent;const auto nativeStatementTemp322=JSValue("transformDirection");const auto nativeStatementTemp318=nativeStatementTemp321.get(nativeStatementTemp322);const auto nativeStatementTemp319=nativeStatementTemp321;Args nativeStatementTemp320;nativeStatementTemp320.push_back(matrix);return nativeStatementTemp318.call(nativeStatementTemp319,nativeStatementTemp320);}());
([&]()->JSValue{auto nativeStatementTemp323=tangent;const auto nativeStatementTemp324=JSValue("needsUpdate");const auto nativeStatementTemp325=JSValue(true);setReference(nativeStatementTemp323,nativeStatementTemp324,nativeStatementTemp325);return nativeStatementTemp325;}());

}
if(([&]()->JSValue{const auto nativeStatementTemp328=([&]()->JSValue{const auto nativeStatementTemp330=nativeStatementTemp271;const auto nativeStatementTemp331=JSValue("boundingBox");return nativeStatementTemp330.get(nativeStatementTemp331);}());const auto nativeStatementTemp329=JSValue(nullptr);return binary("!==",nativeStatementTemp328,nativeStatementTemp329);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp335=nativeStatementTemp271;const auto nativeStatementTemp336=JSValue("computeBoundingBox");const auto nativeStatementTemp332=nativeStatementTemp335.get(nativeStatementTemp336);const auto nativeStatementTemp333=nativeStatementTemp335;Args nativeStatementTemp334;return nativeStatementTemp332.call(nativeStatementTemp333,nativeStatementTemp334);}());

}
if(([&]()->JSValue{const auto nativeStatementTemp337=([&]()->JSValue{const auto nativeStatementTemp339=nativeStatementTemp271;const auto nativeStatementTemp340=JSValue("boundingSphere");return nativeStatementTemp339.get(nativeStatementTemp340);}());const auto nativeStatementTemp338=JSValue(nullptr);return binary("!==",nativeStatementTemp337,nativeStatementTemp338);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp344=nativeStatementTemp271;const auto nativeStatementTemp345=JSValue("computeBoundingSphere");const auto nativeStatementTemp341=nativeStatementTemp344.get(nativeStatementTemp345);const auto nativeStatementTemp342=nativeStatementTemp344;Args nativeStatementTemp343;return nativeStatementTemp341.call(nativeStatementTemp342,nativeStatementTemp343);}());

}
([&]()->JSValue{auto nativeStatementTemp346=nativeStatementTemp271;const auto nativeStatementTemp347=JSValue("_transformed");const auto nativeStatementTemp348=JSValue(true);setReference(nativeStatementTemp346,nativeStatementTemp347,nativeStatementTemp348);return nativeStatementTemp348;}());
return nativeStatementTemp271;
return JSValue{};
}

JSValue method13(const JSValue& nativeStatementTemp351,const Args& nativeStatementTemp352,const Args& nativeStatementTemp353){
auto nativeStatementTemp354=nativeStatementTemp353[0];auto nativeStatementTemp355=nativeStatementTemp351;
JSValue q=arg(nativeStatementTemp352,0);
([&]()->JSValue{auto nativeStatementTemp360=nativeStatementTemp354.get("_m1");const auto nativeStatementTemp361=JSValue("makeRotationFromQuaternion");const auto nativeStatementTemp357=nativeStatementTemp360.get(nativeStatementTemp361);const auto nativeStatementTemp358=nativeStatementTemp360;Args nativeStatementTemp359;nativeStatementTemp359.push_back(q);return nativeStatementTemp357.call(nativeStatementTemp358,nativeStatementTemp359);}());
([&]()->JSValue{auto nativeStatementTemp365=nativeStatementTemp355;const auto nativeStatementTemp366=JSValue("applyMatrix4");const auto nativeStatementTemp362=nativeStatementTemp365.get(nativeStatementTemp366);const auto nativeStatementTemp363=nativeStatementTemp365;Args nativeStatementTemp364;nativeStatementTemp364.push_back(nativeStatementTemp354.get("_m1"));return nativeStatementTemp362.call(nativeStatementTemp363,nativeStatementTemp364);}());
return nativeStatementTemp355;
return JSValue{};
}

JSValue method14(const JSValue& nativeStatementTemp367,const Args& nativeStatementTemp368,const Args& nativeStatementTemp369){
auto nativeStatementTemp370=nativeStatementTemp369[0];auto nativeStatementTemp371=nativeStatementTemp367;
JSValue angle=arg(nativeStatementTemp368,0);
([&]()->JSValue{auto nativeStatementTemp376=nativeStatementTemp370.get("_m1");const auto nativeStatementTemp377=JSValue("makeRotationX");const auto nativeStatementTemp373=nativeStatementTemp376.get(nativeStatementTemp377);const auto nativeStatementTemp374=nativeStatementTemp376;Args nativeStatementTemp375;nativeStatementTemp375.push_back(angle);return nativeStatementTemp373.call(nativeStatementTemp374,nativeStatementTemp375);}());
([&]()->JSValue{auto nativeStatementTemp381=nativeStatementTemp371;const auto nativeStatementTemp382=JSValue("applyMatrix4");const auto nativeStatementTemp378=nativeStatementTemp381.get(nativeStatementTemp382);const auto nativeStatementTemp379=nativeStatementTemp381;Args nativeStatementTemp380;nativeStatementTemp380.push_back(nativeStatementTemp370.get("_m1"));return nativeStatementTemp378.call(nativeStatementTemp379,nativeStatementTemp380);}());
return nativeStatementTemp371;
return JSValue{};
}

JSValue method15(const JSValue& nativeStatementTemp383,const Args& nativeStatementTemp384,const Args& nativeStatementTemp385){
auto nativeStatementTemp386=nativeStatementTemp385[0];auto nativeStatementTemp387=nativeStatementTemp383;
JSValue angle=arg(nativeStatementTemp384,0);
([&]()->JSValue{auto nativeStatementTemp392=nativeStatementTemp386.get("_m1");const auto nativeStatementTemp393=JSValue("makeRotationY");const auto nativeStatementTemp389=nativeStatementTemp392.get(nativeStatementTemp393);const auto nativeStatementTemp390=nativeStatementTemp392;Args nativeStatementTemp391;nativeStatementTemp391.push_back(angle);return nativeStatementTemp389.call(nativeStatementTemp390,nativeStatementTemp391);}());
([&]()->JSValue{auto nativeStatementTemp397=nativeStatementTemp387;const auto nativeStatementTemp398=JSValue("applyMatrix4");const auto nativeStatementTemp394=nativeStatementTemp397.get(nativeStatementTemp398);const auto nativeStatementTemp395=nativeStatementTemp397;Args nativeStatementTemp396;nativeStatementTemp396.push_back(nativeStatementTemp386.get("_m1"));return nativeStatementTemp394.call(nativeStatementTemp395,nativeStatementTemp396);}());
return nativeStatementTemp387;
return JSValue{};
}

JSValue method16(const JSValue& nativeStatementTemp399,const Args& nativeStatementTemp400,const Args& nativeStatementTemp401){
auto nativeStatementTemp402=nativeStatementTemp401[0];auto nativeStatementTemp403=nativeStatementTemp399;
JSValue angle=arg(nativeStatementTemp400,0);
([&]()->JSValue{auto nativeStatementTemp408=nativeStatementTemp402.get("_m1");const auto nativeStatementTemp409=JSValue("makeRotationZ");const auto nativeStatementTemp405=nativeStatementTemp408.get(nativeStatementTemp409);const auto nativeStatementTemp406=nativeStatementTemp408;Args nativeStatementTemp407;nativeStatementTemp407.push_back(angle);return nativeStatementTemp405.call(nativeStatementTemp406,nativeStatementTemp407);}());
([&]()->JSValue{auto nativeStatementTemp413=nativeStatementTemp403;const auto nativeStatementTemp414=JSValue("applyMatrix4");const auto nativeStatementTemp410=nativeStatementTemp413.get(nativeStatementTemp414);const auto nativeStatementTemp411=nativeStatementTemp413;Args nativeStatementTemp412;nativeStatementTemp412.push_back(nativeStatementTemp402.get("_m1"));return nativeStatementTemp410.call(nativeStatementTemp411,nativeStatementTemp412);}());
return nativeStatementTemp403;
return JSValue{};
}

JSValue method17(const JSValue& nativeStatementTemp415,const Args& nativeStatementTemp416,const Args& nativeStatementTemp417){
auto nativeStatementTemp418=nativeStatementTemp417[0];auto nativeStatementTemp419=nativeStatementTemp415;
JSValue x=arg(nativeStatementTemp416,0);
JSValue y=arg(nativeStatementTemp416,1);
JSValue z=arg(nativeStatementTemp416,2);
([&]()->JSValue{auto nativeStatementTemp424=nativeStatementTemp418.get("_m1");const auto nativeStatementTemp425=JSValue("makeTranslation");const auto nativeStatementTemp421=nativeStatementTemp424.get(nativeStatementTemp425);const auto nativeStatementTemp422=nativeStatementTemp424;Args nativeStatementTemp423;nativeStatementTemp423.push_back(x);nativeStatementTemp423.push_back(y);nativeStatementTemp423.push_back(z);return nativeStatementTemp421.call(nativeStatementTemp422,nativeStatementTemp423);}());
([&]()->JSValue{auto nativeStatementTemp429=nativeStatementTemp419;const auto nativeStatementTemp430=JSValue("applyMatrix4");const auto nativeStatementTemp426=nativeStatementTemp429.get(nativeStatementTemp430);const auto nativeStatementTemp427=nativeStatementTemp429;Args nativeStatementTemp428;nativeStatementTemp428.push_back(nativeStatementTemp418.get("_m1"));return nativeStatementTemp426.call(nativeStatementTemp427,nativeStatementTemp428);}());
return nativeStatementTemp419;
return JSValue{};
}

JSValue method18(const JSValue& nativeStatementTemp431,const Args& nativeStatementTemp432,const Args& nativeStatementTemp433){
auto nativeStatementTemp434=nativeStatementTemp433[0];auto nativeStatementTemp435=nativeStatementTemp431;
JSValue x=arg(nativeStatementTemp432,0);
JSValue y=arg(nativeStatementTemp432,1);
JSValue z=arg(nativeStatementTemp432,2);
([&]()->JSValue{auto nativeStatementTemp440=nativeStatementTemp434.get("_m1");const auto nativeStatementTemp441=JSValue("makeScale");const auto nativeStatementTemp437=nativeStatementTemp440.get(nativeStatementTemp441);const auto nativeStatementTemp438=nativeStatementTemp440;Args nativeStatementTemp439;nativeStatementTemp439.push_back(x);nativeStatementTemp439.push_back(y);nativeStatementTemp439.push_back(z);return nativeStatementTemp437.call(nativeStatementTemp438,nativeStatementTemp439);}());
([&]()->JSValue{auto nativeStatementTemp445=nativeStatementTemp435;const auto nativeStatementTemp446=JSValue("applyMatrix4");const auto nativeStatementTemp442=nativeStatementTemp445.get(nativeStatementTemp446);const auto nativeStatementTemp443=nativeStatementTemp445;Args nativeStatementTemp444;nativeStatementTemp444.push_back(nativeStatementTemp434.get("_m1"));return nativeStatementTemp442.call(nativeStatementTemp443,nativeStatementTemp444);}());
return nativeStatementTemp435;
return JSValue{};
}

JSValue method19(const JSValue& nativeStatementTemp447,const Args& nativeStatementTemp448,const Args& nativeStatementTemp449){
auto nativeStatementTemp450=nativeStatementTemp449[0];auto nativeStatementTemp451=nativeStatementTemp447;
JSValue vector=arg(nativeStatementTemp448,0);
([&]()->JSValue{auto nativeStatementTemp456=nativeStatementTemp450.get("_obj");const auto nativeStatementTemp457=JSValue("lookAt");const auto nativeStatementTemp453=nativeStatementTemp456.get(nativeStatementTemp457);const auto nativeStatementTemp454=nativeStatementTemp456;Args nativeStatementTemp455;nativeStatementTemp455.push_back(vector);return nativeStatementTemp453.call(nativeStatementTemp454,nativeStatementTemp455);}());
([&]()->JSValue{auto nativeStatementTemp461=nativeStatementTemp450.get("_obj");const auto nativeStatementTemp462=JSValue("updateMatrix");const auto nativeStatementTemp458=nativeStatementTemp461.get(nativeStatementTemp462);const auto nativeStatementTemp459=nativeStatementTemp461;Args nativeStatementTemp460;return nativeStatementTemp458.call(nativeStatementTemp459,nativeStatementTemp460);}());
([&]()->JSValue{auto nativeStatementTemp466=nativeStatementTemp451;const auto nativeStatementTemp467=JSValue("applyMatrix4");const auto nativeStatementTemp463=nativeStatementTemp466.get(nativeStatementTemp467);const auto nativeStatementTemp464=nativeStatementTemp466;Args nativeStatementTemp465;nativeStatementTemp465.push_back(([&]()->JSValue{const auto nativeStatementTemp468=nativeStatementTemp450.get("_obj");const auto nativeStatementTemp469=JSValue("matrix");return nativeStatementTemp468.get(nativeStatementTemp469);}()));return nativeStatementTemp463.call(nativeStatementTemp464,nativeStatementTemp465);}());
return nativeStatementTemp451;
return JSValue{};
}

JSValue method20(const JSValue& nativeStatementTemp470,const Args& nativeStatementTemp471,const Args& nativeStatementTemp472){
auto nativeStatementTemp473=nativeStatementTemp472[0];auto nativeStatementTemp474=nativeStatementTemp470;
([&]()->JSValue{auto nativeStatementTemp479=nativeStatementTemp474;const auto nativeStatementTemp480=JSValue("computeBoundingBox");const auto nativeStatementTemp476=nativeStatementTemp479.get(nativeStatementTemp480);const auto nativeStatementTemp477=nativeStatementTemp479;Args nativeStatementTemp478;return nativeStatementTemp476.call(nativeStatementTemp477,nativeStatementTemp478);}());
([&]()->JSValue{auto nativeStatementTemp484=([&]()->JSValue{auto nativeStatementTemp489=([&]()->JSValue{const auto nativeStatementTemp491=nativeStatementTemp474;const auto nativeStatementTemp492=JSValue("boundingBox");return nativeStatementTemp491.get(nativeStatementTemp492);}());const auto nativeStatementTemp490=JSValue("getCenter");const auto nativeStatementTemp486=nativeStatementTemp489.get(nativeStatementTemp490);const auto nativeStatementTemp487=nativeStatementTemp489;Args nativeStatementTemp488;nativeStatementTemp488.push_back(nativeStatementTemp473.get("_offset"));return nativeStatementTemp486.call(nativeStatementTemp487,nativeStatementTemp488);}());const auto nativeStatementTemp485=JSValue("negate");const auto nativeStatementTemp481=nativeStatementTemp484.get(nativeStatementTemp485);const auto nativeStatementTemp482=nativeStatementTemp484;Args nativeStatementTemp483;return nativeStatementTemp481.call(nativeStatementTemp482,nativeStatementTemp483);}());
([&]()->JSValue{auto nativeStatementTemp496=nativeStatementTemp474;const auto nativeStatementTemp497=JSValue("translate");const auto nativeStatementTemp493=nativeStatementTemp496.get(nativeStatementTemp497);const auto nativeStatementTemp494=nativeStatementTemp496;Args nativeStatementTemp495;nativeStatementTemp495.push_back(([&]()->JSValue{const auto nativeStatementTemp498=nativeStatementTemp473.get("_offset");const auto nativeStatementTemp499=JSValue("x");return nativeStatementTemp498.get(nativeStatementTemp499);}()));nativeStatementTemp495.push_back(([&]()->JSValue{const auto nativeStatementTemp500=nativeStatementTemp473.get("_offset");const auto nativeStatementTemp501=JSValue("y");return nativeStatementTemp500.get(nativeStatementTemp501);}()));nativeStatementTemp495.push_back(([&]()->JSValue{const auto nativeStatementTemp502=nativeStatementTemp473.get("_offset");const auto nativeStatementTemp503=JSValue("z");return nativeStatementTemp502.get(nativeStatementTemp503);}()));return nativeStatementTemp493.call(nativeStatementTemp494,nativeStatementTemp495);}());
return nativeStatementTemp474;
return JSValue{};
}

JSValue method21(const JSValue& nativeStatementTemp504,const Args& nativeStatementTemp505,const Args& nativeStatementTemp506){
auto nativeStatementTemp507=nativeStatementTemp506[0];auto nativeStatementTemp508=nativeStatementTemp504;
JSValue points=arg(nativeStatementTemp505,0);
JSValue positionAttribute=([&]()->JSValue{auto nativeStatementTemp513=nativeStatementTemp508;const auto nativeStatementTemp514=JSValue("getAttribute");const auto nativeStatementTemp510=nativeStatementTemp513.get(nativeStatementTemp514);const auto nativeStatementTemp511=nativeStatementTemp513;Args nativeStatementTemp512;nativeStatementTemp512.push_back(JSValue("position"));return nativeStatementTemp510.call(nativeStatementTemp511,nativeStatementTemp512);}());
if(([&]()->JSValue{const auto nativeStatementTemp515=positionAttribute;const auto nativeStatementTemp516=JSValue{};return binary("===",nativeStatementTemp515,nativeStatementTemp516);}()).toBoolean()){
JSValue position=([&]()->JSValue{auto nativeStatementTemp517=JSValue::array();return nativeStatementTemp517;}());
{JSValue i=JSValue(0);
JSValue l=([&]()->JSValue{const auto nativeStatementTemp518=points;const auto nativeStatementTemp519=JSValue("length");return nativeStatementTemp518.get(nativeStatementTemp519);}());
for(;([&]()->JSValue{const auto nativeStatementTemp520=i;const auto nativeStatementTemp521=l;return binary("<",nativeStatementTemp520,nativeStatementTemp521);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp522=i.toNumber();const auto nativeStatementTemp523=nativeStatementTemp522+1;i=nativeStatementTemp523;return nativeStatementTemp522;}())){
JSValue point=([&]()->JSValue{const auto nativeStatementTemp524=points;const auto nativeStatementTemp525=i;return nativeStatementTemp524.get(nativeStatementTemp525);}());
([&]()->JSValue{auto nativeStatementTemp529=position;const auto nativeStatementTemp530=JSValue("push");const auto nativeStatementTemp526=nativeStatementTemp529.get(nativeStatementTemp530);const auto nativeStatementTemp527=nativeStatementTemp529;Args nativeStatementTemp528;nativeStatementTemp528.push_back(([&]()->JSValue{const auto nativeStatementTemp531=point;const auto nativeStatementTemp532=JSValue("x");return nativeStatementTemp531.get(nativeStatementTemp532);}()));nativeStatementTemp528.push_back(([&]()->JSValue{const auto nativeStatementTemp533=point;const auto nativeStatementTemp534=JSValue("y");return nativeStatementTemp533.get(nativeStatementTemp534);}()));nativeStatementTemp528.push_back(([&]()->JSValue{const auto nativeStatementTemp535=([&]()->JSValue{const auto nativeStatementTemp536=point;const auto nativeStatementTemp537=JSValue("z");return nativeStatementTemp536.get(nativeStatementTemp537);}());if(nativeStatementTemp535.toBoolean())return nativeStatementTemp535;return JSValue(0);}()));return nativeStatementTemp526.call(nativeStatementTemp527,nativeStatementTemp528);}());

}
}
([&]()->JSValue{auto nativeStatementTemp541=nativeStatementTemp508;const auto nativeStatementTemp542=JSValue("setAttribute");const auto nativeStatementTemp538=nativeStatementTemp541.get(nativeStatementTemp542);const auto nativeStatementTemp539=nativeStatementTemp541;Args nativeStatementTemp540;nativeStatementTemp540.push_back(JSValue("position"));nativeStatementTemp540.push_back(([&]()->JSValue{const auto nativeStatementTemp543=nativeStatementTemp507.get("Float32BufferAttribute");Args nativeStatementTemp544;nativeStatementTemp544.push_back(position);nativeStatementTemp544.push_back(JSValue(3));return nativeStatementTemp543.construct(nativeStatementTemp544);}()));return nativeStatementTemp538.call(nativeStatementTemp539,nativeStatementTemp540);}());

}
else {
JSValue l=([&]()->JSValue{auto nativeStatementTemp548=nativeGlobalBinding("Math");const auto nativeStatementTemp549=JSValue("min");const auto nativeStatementTemp545=nativeStatementTemp548.get(nativeStatementTemp549);const auto nativeStatementTemp546=nativeStatementTemp548;Args nativeStatementTemp547;nativeStatementTemp547.push_back(([&]()->JSValue{const auto nativeStatementTemp550=points;const auto nativeStatementTemp551=JSValue("length");return nativeStatementTemp550.get(nativeStatementTemp551);}()));nativeStatementTemp547.push_back(([&]()->JSValue{const auto nativeStatementTemp552=positionAttribute;const auto nativeStatementTemp553=JSValue("count");return nativeStatementTemp552.get(nativeStatementTemp553);}()));return nativeStatementTemp545.call(nativeStatementTemp546,nativeStatementTemp547);}());
{JSValue i=JSValue(0);
for(;([&]()->JSValue{const auto nativeStatementTemp554=i;const auto nativeStatementTemp555=l;return binary("<",nativeStatementTemp554,nativeStatementTemp555);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp556=i.toNumber();const auto nativeStatementTemp557=nativeStatementTemp556+1;i=nativeStatementTemp557;return nativeStatementTemp556;}())){
JSValue point=([&]()->JSValue{const auto nativeStatementTemp558=points;const auto nativeStatementTemp559=i;return nativeStatementTemp558.get(nativeStatementTemp559);}());
([&]()->JSValue{auto nativeStatementTemp563=positionAttribute;const auto nativeStatementTemp564=JSValue("setXYZ");const auto nativeStatementTemp560=nativeStatementTemp563.get(nativeStatementTemp564);const auto nativeStatementTemp561=nativeStatementTemp563;Args nativeStatementTemp562;nativeStatementTemp562.push_back(i);nativeStatementTemp562.push_back(([&]()->JSValue{const auto nativeStatementTemp565=point;const auto nativeStatementTemp566=JSValue("x");return nativeStatementTemp565.get(nativeStatementTemp566);}()));nativeStatementTemp562.push_back(([&]()->JSValue{const auto nativeStatementTemp567=point;const auto nativeStatementTemp568=JSValue("y");return nativeStatementTemp567.get(nativeStatementTemp568);}()));nativeStatementTemp562.push_back(([&]()->JSValue{const auto nativeStatementTemp569=([&]()->JSValue{const auto nativeStatementTemp570=point;const auto nativeStatementTemp571=JSValue("z");return nativeStatementTemp570.get(nativeStatementTemp571);}());if(nativeStatementTemp569.toBoolean())return nativeStatementTemp569;return JSValue(0);}()));return nativeStatementTemp560.call(nativeStatementTemp561,nativeStatementTemp562);}());

}
}
if(([&]()->JSValue{const auto nativeStatementTemp572=([&]()->JSValue{const auto nativeStatementTemp574=points;const auto nativeStatementTemp575=JSValue("length");return nativeStatementTemp574.get(nativeStatementTemp575);}());const auto nativeStatementTemp573=([&]()->JSValue{const auto nativeStatementTemp576=positionAttribute;const auto nativeStatementTemp577=JSValue("count");return nativeStatementTemp576.get(nativeStatementTemp577);}());return binary(">",nativeStatementTemp572,nativeStatementTemp573);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp578=nativeStatementTemp507.get("warn");const JSValue nativeStatementTemp579;Args nativeStatementTemp580;nativeStatementTemp580.push_back(JSValue("BufferGeometry: Buffer size too small for points data. Use .dispose() and create a new geometry."));return nativeStatementTemp578.call(nativeStatementTemp579,nativeStatementTemp580);}());

}
([&]()->JSValue{auto nativeStatementTemp581=positionAttribute;const auto nativeStatementTemp582=JSValue("needsUpdate");const auto nativeStatementTemp583=JSValue(true);setReference(nativeStatementTemp581,nativeStatementTemp582,nativeStatementTemp583);return nativeStatementTemp583;}());

}
return nativeStatementTemp508;
return JSValue{};
}

JSValue method22(const JSValue& nativeStatementTemp586,const Args& nativeStatementTemp587,const Args& nativeStatementTemp588){
auto nativeStatementTemp589=nativeStatementTemp588[0];auto nativeStatementTemp590=nativeStatementTemp586;
if(([&]()->JSValue{const auto nativeStatementTemp592=([&]()->JSValue{const auto nativeStatementTemp594=nativeStatementTemp590;const auto nativeStatementTemp595=JSValue("boundingBox");return nativeStatementTemp594.get(nativeStatementTemp595);}());const auto nativeStatementTemp593=JSValue(nullptr);return binary("===",nativeStatementTemp592,nativeStatementTemp593);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp596=nativeStatementTemp590;const auto nativeStatementTemp597=JSValue("boundingBox");const auto nativeStatementTemp598=([&]()->JSValue{const auto nativeStatementTemp601=nativeStatementTemp589.get("Box3");Args nativeStatementTemp602;return nativeStatementTemp601.construct(nativeStatementTemp602);}());setReference(nativeStatementTemp596,nativeStatementTemp597,nativeStatementTemp598);return nativeStatementTemp598;}());

}
JSValue position=([&]()->JSValue{const auto nativeStatementTemp603=([&]()->JSValue{const auto nativeStatementTemp605=nativeStatementTemp590;const auto nativeStatementTemp606=JSValue("attributes");return nativeStatementTemp605.get(nativeStatementTemp606);}());const auto nativeStatementTemp604=JSValue("position");return nativeStatementTemp603.get(nativeStatementTemp604);}());
JSValue morphAttributesPosition=([&]()->JSValue{const auto nativeStatementTemp607=([&]()->JSValue{const auto nativeStatementTemp609=nativeStatementTemp590;const auto nativeStatementTemp610=JSValue("morphAttributes");return nativeStatementTemp609.get(nativeStatementTemp610);}());const auto nativeStatementTemp608=JSValue("position");return nativeStatementTemp607.get(nativeStatementTemp608);}());
if(([&]()->JSValue{const auto nativeStatementTemp611=position;if(!nativeStatementTemp611.toBoolean())return nativeStatementTemp611;return ([&]()->JSValue{const auto nativeStatementTemp612=position;const auto nativeStatementTemp613=JSValue("isGLBufferAttribute");return nativeStatementTemp612.get(nativeStatementTemp613);}());}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp614=nativeStatementTemp589.get("error");const JSValue nativeStatementTemp615;Args nativeStatementTemp616;nativeStatementTemp616.push_back(JSValue("BufferGeometry.computeBoundingBox(): GLBufferAttribute requires a manual bounding box."));nativeStatementTemp616.push_back(nativeStatementTemp590);return nativeStatementTemp614.call(nativeStatementTemp615,nativeStatementTemp616);}());
([&]()->JSValue{auto nativeStatementTemp620=([&]()->JSValue{const auto nativeStatementTemp622=nativeStatementTemp590;const auto nativeStatementTemp623=JSValue("boundingBox");return nativeStatementTemp622.get(nativeStatementTemp623);}());const auto nativeStatementTemp621=JSValue("set");const auto nativeStatementTemp617=nativeStatementTemp620.get(nativeStatementTemp621);const auto nativeStatementTemp618=nativeStatementTemp620;Args nativeStatementTemp619;nativeStatementTemp619.push_back(([&]()->JSValue{const auto nativeStatementTemp624=nativeStatementTemp589.get("Vector3");Args nativeStatementTemp625;nativeStatementTemp625.push_back(([&]()->JSValue{const auto nativeStatementTemp626=nativeGlobalBinding("Infinity");return -nativeStatementTemp626.toNumber();}()));nativeStatementTemp625.push_back(([&]()->JSValue{const auto nativeStatementTemp627=nativeGlobalBinding("Infinity");return -nativeStatementTemp627.toNumber();}()));nativeStatementTemp625.push_back(([&]()->JSValue{const auto nativeStatementTemp628=nativeGlobalBinding("Infinity");return -nativeStatementTemp628.toNumber();}()));return nativeStatementTemp624.construct(nativeStatementTemp625);}()));nativeStatementTemp619.push_back(([&]()->JSValue{const auto nativeStatementTemp629=nativeStatementTemp589.get("Vector3");Args nativeStatementTemp630;nativeStatementTemp630.push_back(([&]()->JSValue{const auto nativeStatementTemp631=nativeGlobalBinding("Infinity");return nativeStatementTemp631.toNumber();}()));nativeStatementTemp630.push_back(([&]()->JSValue{const auto nativeStatementTemp632=nativeGlobalBinding("Infinity");return nativeStatementTemp632.toNumber();}()));nativeStatementTemp630.push_back(([&]()->JSValue{const auto nativeStatementTemp633=nativeGlobalBinding("Infinity");return nativeStatementTemp633.toNumber();}()));return nativeStatementTemp629.construct(nativeStatementTemp630);}()));return nativeStatementTemp617.call(nativeStatementTemp618,nativeStatementTemp619);}());
return JSValue{};

}
if(([&]()->JSValue{const auto nativeStatementTemp634=position;const auto nativeStatementTemp635=JSValue{};return binary("!==",nativeStatementTemp634,nativeStatementTemp635);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp639=([&]()->JSValue{const auto nativeStatementTemp641=nativeStatementTemp590;const auto nativeStatementTemp642=JSValue("boundingBox");return nativeStatementTemp641.get(nativeStatementTemp642);}());const auto nativeStatementTemp640=JSValue("setFromBufferAttribute");const auto nativeStatementTemp636=nativeStatementTemp639.get(nativeStatementTemp640);const auto nativeStatementTemp637=nativeStatementTemp639;Args nativeStatementTemp638;nativeStatementTemp638.push_back(position);return nativeStatementTemp636.call(nativeStatementTemp637,nativeStatementTemp638);}());
if(morphAttributesPosition.toBoolean()){
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp643=morphAttributesPosition;const auto nativeStatementTemp644=JSValue("length");return nativeStatementTemp643.get(nativeStatementTemp644);}());
for(;([&]()->JSValue{const auto nativeStatementTemp645=i;const auto nativeStatementTemp646=il;return binary("<",nativeStatementTemp645,nativeStatementTemp646);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp647=i.toNumber();const auto nativeStatementTemp648=nativeStatementTemp647+1;i=nativeStatementTemp648;return nativeStatementTemp647;}())){
JSValue morphAttribute=([&]()->JSValue{const auto nativeStatementTemp649=morphAttributesPosition;const auto nativeStatementTemp650=i;return nativeStatementTemp649.get(nativeStatementTemp650);}());
([&]()->JSValue{auto nativeStatementTemp654=nativeStatementTemp589.get("_box");const auto nativeStatementTemp655=JSValue("setFromBufferAttribute");const auto nativeStatementTemp651=nativeStatementTemp654.get(nativeStatementTemp655);const auto nativeStatementTemp652=nativeStatementTemp654;Args nativeStatementTemp653;nativeStatementTemp653.push_back(morphAttribute);return nativeStatementTemp651.call(nativeStatementTemp652,nativeStatementTemp653);}());
if(([&]()->JSValue{const auto nativeStatementTemp656=nativeStatementTemp590;const auto nativeStatementTemp657=JSValue("morphTargetsRelative");return nativeStatementTemp656.get(nativeStatementTemp657);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp661=nativeStatementTemp589.get("_vector");const auto nativeStatementTemp662=JSValue("addVectors");const auto nativeStatementTemp658=nativeStatementTemp661.get(nativeStatementTemp662);const auto nativeStatementTemp659=nativeStatementTemp661;Args nativeStatementTemp660;nativeStatementTemp660.push_back(([&]()->JSValue{const auto nativeStatementTemp663=([&]()->JSValue{const auto nativeStatementTemp665=nativeStatementTemp590;const auto nativeStatementTemp666=JSValue("boundingBox");return nativeStatementTemp665.get(nativeStatementTemp666);}());const auto nativeStatementTemp664=JSValue("min");return nativeStatementTemp663.get(nativeStatementTemp664);}()));nativeStatementTemp660.push_back(([&]()->JSValue{const auto nativeStatementTemp667=nativeStatementTemp589.get("_box");const auto nativeStatementTemp668=JSValue("min");return nativeStatementTemp667.get(nativeStatementTemp668);}()));return nativeStatementTemp658.call(nativeStatementTemp659,nativeStatementTemp660);}());
([&]()->JSValue{auto nativeStatementTemp672=([&]()->JSValue{const auto nativeStatementTemp674=nativeStatementTemp590;const auto nativeStatementTemp675=JSValue("boundingBox");return nativeStatementTemp674.get(nativeStatementTemp675);}());const auto nativeStatementTemp673=JSValue("expandByPoint");const auto nativeStatementTemp669=nativeStatementTemp672.get(nativeStatementTemp673);const auto nativeStatementTemp670=nativeStatementTemp672;Args nativeStatementTemp671;nativeStatementTemp671.push_back(nativeStatementTemp589.get("_vector"));return nativeStatementTemp669.call(nativeStatementTemp670,nativeStatementTemp671);}());
([&]()->JSValue{auto nativeStatementTemp679=nativeStatementTemp589.get("_vector");const auto nativeStatementTemp680=JSValue("addVectors");const auto nativeStatementTemp676=nativeStatementTemp679.get(nativeStatementTemp680);const auto nativeStatementTemp677=nativeStatementTemp679;Args nativeStatementTemp678;nativeStatementTemp678.push_back(([&]()->JSValue{const auto nativeStatementTemp681=([&]()->JSValue{const auto nativeStatementTemp683=nativeStatementTemp590;const auto nativeStatementTemp684=JSValue("boundingBox");return nativeStatementTemp683.get(nativeStatementTemp684);}());const auto nativeStatementTemp682=JSValue("max");return nativeStatementTemp681.get(nativeStatementTemp682);}()));nativeStatementTemp678.push_back(([&]()->JSValue{const auto nativeStatementTemp685=nativeStatementTemp589.get("_box");const auto nativeStatementTemp686=JSValue("max");return nativeStatementTemp685.get(nativeStatementTemp686);}()));return nativeStatementTemp676.call(nativeStatementTemp677,nativeStatementTemp678);}());
([&]()->JSValue{auto nativeStatementTemp690=([&]()->JSValue{const auto nativeStatementTemp692=nativeStatementTemp590;const auto nativeStatementTemp693=JSValue("boundingBox");return nativeStatementTemp692.get(nativeStatementTemp693);}());const auto nativeStatementTemp691=JSValue("expandByPoint");const auto nativeStatementTemp687=nativeStatementTemp690.get(nativeStatementTemp691);const auto nativeStatementTemp688=nativeStatementTemp690;Args nativeStatementTemp689;nativeStatementTemp689.push_back(nativeStatementTemp589.get("_vector"));return nativeStatementTemp687.call(nativeStatementTemp688,nativeStatementTemp689);}());

}
else {
([&]()->JSValue{auto nativeStatementTemp697=([&]()->JSValue{const auto nativeStatementTemp699=nativeStatementTemp590;const auto nativeStatementTemp700=JSValue("boundingBox");return nativeStatementTemp699.get(nativeStatementTemp700);}());const auto nativeStatementTemp698=JSValue("expandByPoint");const auto nativeStatementTemp694=nativeStatementTemp697.get(nativeStatementTemp698);const auto nativeStatementTemp695=nativeStatementTemp697;Args nativeStatementTemp696;nativeStatementTemp696.push_back(([&]()->JSValue{const auto nativeStatementTemp701=nativeStatementTemp589.get("_box");const auto nativeStatementTemp702=JSValue("min");return nativeStatementTemp701.get(nativeStatementTemp702);}()));return nativeStatementTemp694.call(nativeStatementTemp695,nativeStatementTemp696);}());
([&]()->JSValue{auto nativeStatementTemp706=([&]()->JSValue{const auto nativeStatementTemp708=nativeStatementTemp590;const auto nativeStatementTemp709=JSValue("boundingBox");return nativeStatementTemp708.get(nativeStatementTemp709);}());const auto nativeStatementTemp707=JSValue("expandByPoint");const auto nativeStatementTemp703=nativeStatementTemp706.get(nativeStatementTemp707);const auto nativeStatementTemp704=nativeStatementTemp706;Args nativeStatementTemp705;nativeStatementTemp705.push_back(([&]()->JSValue{const auto nativeStatementTemp710=nativeStatementTemp589.get("_box");const auto nativeStatementTemp711=JSValue("max");return nativeStatementTemp710.get(nativeStatementTemp711);}()));return nativeStatementTemp703.call(nativeStatementTemp704,nativeStatementTemp705);}());

}

}
}

}

}
else {
([&]()->JSValue{auto nativeStatementTemp715=([&]()->JSValue{const auto nativeStatementTemp717=nativeStatementTemp590;const auto nativeStatementTemp718=JSValue("boundingBox");return nativeStatementTemp717.get(nativeStatementTemp718);}());const auto nativeStatementTemp716=JSValue("makeEmpty");const auto nativeStatementTemp712=nativeStatementTemp715.get(nativeStatementTemp716);const auto nativeStatementTemp713=nativeStatementTemp715;Args nativeStatementTemp714;return nativeStatementTemp712.call(nativeStatementTemp713,nativeStatementTemp714);}());

}
if(([&]()->JSValue{const auto nativeStatementTemp719=([&]()->JSValue{const auto nativeStatementTemp720=([&]()->JSValue{const auto nativeStatementTemp721=nativeGlobalBinding("isNaN");const JSValue nativeStatementTemp722;Args nativeStatementTemp723;nativeStatementTemp723.push_back(([&]()->JSValue{const auto nativeStatementTemp724=([&]()->JSValue{const auto nativeStatementTemp726=([&]()->JSValue{const auto nativeStatementTemp728=nativeStatementTemp590;const auto nativeStatementTemp729=JSValue("boundingBox");return nativeStatementTemp728.get(nativeStatementTemp729);}());const auto nativeStatementTemp727=JSValue("min");return nativeStatementTemp726.get(nativeStatementTemp727);}());const auto nativeStatementTemp725=JSValue("x");return nativeStatementTemp724.get(nativeStatementTemp725);}()));return nativeStatementTemp721.call(nativeStatementTemp722,nativeStatementTemp723);}());if(nativeStatementTemp720.toBoolean())return nativeStatementTemp720;return ([&]()->JSValue{const auto nativeStatementTemp730=nativeGlobalBinding("isNaN");const JSValue nativeStatementTemp731;Args nativeStatementTemp732;nativeStatementTemp732.push_back(([&]()->JSValue{const auto nativeStatementTemp733=([&]()->JSValue{const auto nativeStatementTemp735=([&]()->JSValue{const auto nativeStatementTemp737=nativeStatementTemp590;const auto nativeStatementTemp738=JSValue("boundingBox");return nativeStatementTemp737.get(nativeStatementTemp738);}());const auto nativeStatementTemp736=JSValue("min");return nativeStatementTemp735.get(nativeStatementTemp736);}());const auto nativeStatementTemp734=JSValue("y");return nativeStatementTemp733.get(nativeStatementTemp734);}()));return nativeStatementTemp730.call(nativeStatementTemp731,nativeStatementTemp732);}());}());if(nativeStatementTemp719.toBoolean())return nativeStatementTemp719;return ([&]()->JSValue{const auto nativeStatementTemp739=nativeGlobalBinding("isNaN");const JSValue nativeStatementTemp740;Args nativeStatementTemp741;nativeStatementTemp741.push_back(([&]()->JSValue{const auto nativeStatementTemp742=([&]()->JSValue{const auto nativeStatementTemp744=([&]()->JSValue{const auto nativeStatementTemp746=nativeStatementTemp590;const auto nativeStatementTemp747=JSValue("boundingBox");return nativeStatementTemp746.get(nativeStatementTemp747);}());const auto nativeStatementTemp745=JSValue("min");return nativeStatementTemp744.get(nativeStatementTemp745);}());const auto nativeStatementTemp743=JSValue("z");return nativeStatementTemp742.get(nativeStatementTemp743);}()));return nativeStatementTemp739.call(nativeStatementTemp740,nativeStatementTemp741);}());}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp748=nativeStatementTemp589.get("error");const JSValue nativeStatementTemp749;Args nativeStatementTemp750;nativeStatementTemp750.push_back(JSValue("BufferGeometry.computeBoundingBox(): Computed min/max have NaN values. The \"position\" attribute is likely to have NaN values."));nativeStatementTemp750.push_back(nativeStatementTemp590);return nativeStatementTemp748.call(nativeStatementTemp749,nativeStatementTemp750);}());

}
return JSValue{};
}

JSValue method23(const JSValue& nativeStatementTemp751,const Args& nativeStatementTemp752,const Args& nativeStatementTemp753){
auto nativeStatementTemp754=nativeStatementTemp753[0];auto nativeStatementTemp755=nativeStatementTemp751;
if(([&]()->JSValue{const auto nativeStatementTemp757=([&]()->JSValue{const auto nativeStatementTemp759=nativeStatementTemp755;const auto nativeStatementTemp760=JSValue("boundingSphere");return nativeStatementTemp759.get(nativeStatementTemp760);}());const auto nativeStatementTemp758=JSValue(nullptr);return binary("===",nativeStatementTemp757,nativeStatementTemp758);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp761=nativeStatementTemp755;const auto nativeStatementTemp762=JSValue("boundingSphere");const auto nativeStatementTemp763=([&]()->JSValue{const auto nativeStatementTemp766=nativeStatementTemp754.get("Sphere");Args nativeStatementTemp767;return nativeStatementTemp766.construct(nativeStatementTemp767);}());setReference(nativeStatementTemp761,nativeStatementTemp762,nativeStatementTemp763);return nativeStatementTemp763;}());

}
JSValue position=([&]()->JSValue{const auto nativeStatementTemp768=([&]()->JSValue{const auto nativeStatementTemp770=nativeStatementTemp755;const auto nativeStatementTemp771=JSValue("attributes");return nativeStatementTemp770.get(nativeStatementTemp771);}());const auto nativeStatementTemp769=JSValue("position");return nativeStatementTemp768.get(nativeStatementTemp769);}());
JSValue morphAttributesPosition=([&]()->JSValue{const auto nativeStatementTemp772=([&]()->JSValue{const auto nativeStatementTemp774=nativeStatementTemp755;const auto nativeStatementTemp775=JSValue("morphAttributes");return nativeStatementTemp774.get(nativeStatementTemp775);}());const auto nativeStatementTemp773=JSValue("position");return nativeStatementTemp772.get(nativeStatementTemp773);}());
if(([&]()->JSValue{const auto nativeStatementTemp776=position;if(!nativeStatementTemp776.toBoolean())return nativeStatementTemp776;return ([&]()->JSValue{const auto nativeStatementTemp777=position;const auto nativeStatementTemp778=JSValue("isGLBufferAttribute");return nativeStatementTemp777.get(nativeStatementTemp778);}());}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp779=nativeStatementTemp754.get("error");const JSValue nativeStatementTemp780;Args nativeStatementTemp781;nativeStatementTemp781.push_back(JSValue("BufferGeometry.computeBoundingSphere(): GLBufferAttribute requires a manual bounding sphere."));nativeStatementTemp781.push_back(nativeStatementTemp755);return nativeStatementTemp779.call(nativeStatementTemp780,nativeStatementTemp781);}());
([&]()->JSValue{auto nativeStatementTemp785=([&]()->JSValue{const auto nativeStatementTemp787=nativeStatementTemp755;const auto nativeStatementTemp788=JSValue("boundingSphere");return nativeStatementTemp787.get(nativeStatementTemp788);}());const auto nativeStatementTemp786=JSValue("set");const auto nativeStatementTemp782=nativeStatementTemp785.get(nativeStatementTemp786);const auto nativeStatementTemp783=nativeStatementTemp785;Args nativeStatementTemp784;nativeStatementTemp784.push_back(([&]()->JSValue{const auto nativeStatementTemp789=nativeStatementTemp754.get("Vector3");Args nativeStatementTemp790;return nativeStatementTemp789.construct(nativeStatementTemp790);}()));nativeStatementTemp784.push_back(nativeGlobalBinding("Infinity"));return nativeStatementTemp782.call(nativeStatementTemp783,nativeStatementTemp784);}());
return JSValue{};

}
if(position.toBoolean()){
JSValue center=([&]()->JSValue{const auto nativeStatementTemp791=([&]()->JSValue{const auto nativeStatementTemp793=nativeStatementTemp755;const auto nativeStatementTemp794=JSValue("boundingSphere");return nativeStatementTemp793.get(nativeStatementTemp794);}());const auto nativeStatementTemp792=JSValue("center");return nativeStatementTemp791.get(nativeStatementTemp792);}());
([&]()->JSValue{auto nativeStatementTemp798=nativeStatementTemp754.get("_box");const auto nativeStatementTemp799=JSValue("setFromBufferAttribute");const auto nativeStatementTemp795=nativeStatementTemp798.get(nativeStatementTemp799);const auto nativeStatementTemp796=nativeStatementTemp798;Args nativeStatementTemp797;nativeStatementTemp797.push_back(position);return nativeStatementTemp795.call(nativeStatementTemp796,nativeStatementTemp797);}());
if(morphAttributesPosition.toBoolean()){
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp800=morphAttributesPosition;const auto nativeStatementTemp801=JSValue("length");return nativeStatementTemp800.get(nativeStatementTemp801);}());
for(;([&]()->JSValue{const auto nativeStatementTemp802=i;const auto nativeStatementTemp803=il;return binary("<",nativeStatementTemp802,nativeStatementTemp803);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp804=i.toNumber();const auto nativeStatementTemp805=nativeStatementTemp804+1;i=nativeStatementTemp805;return nativeStatementTemp804;}())){
JSValue morphAttribute=([&]()->JSValue{const auto nativeStatementTemp806=morphAttributesPosition;const auto nativeStatementTemp807=i;return nativeStatementTemp806.get(nativeStatementTemp807);}());
([&]()->JSValue{auto nativeStatementTemp811=nativeStatementTemp754.get("_boxMorphTargets");const auto nativeStatementTemp812=JSValue("setFromBufferAttribute");const auto nativeStatementTemp808=nativeStatementTemp811.get(nativeStatementTemp812);const auto nativeStatementTemp809=nativeStatementTemp811;Args nativeStatementTemp810;nativeStatementTemp810.push_back(morphAttribute);return nativeStatementTemp808.call(nativeStatementTemp809,nativeStatementTemp810);}());
if(([&]()->JSValue{const auto nativeStatementTemp813=nativeStatementTemp755;const auto nativeStatementTemp814=JSValue("morphTargetsRelative");return nativeStatementTemp813.get(nativeStatementTemp814);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp818=nativeStatementTemp754.get("_vector");const auto nativeStatementTemp819=JSValue("addVectors");const auto nativeStatementTemp815=nativeStatementTemp818.get(nativeStatementTemp819);const auto nativeStatementTemp816=nativeStatementTemp818;Args nativeStatementTemp817;nativeStatementTemp817.push_back(([&]()->JSValue{const auto nativeStatementTemp820=nativeStatementTemp754.get("_box");const auto nativeStatementTemp821=JSValue("min");return nativeStatementTemp820.get(nativeStatementTemp821);}()));nativeStatementTemp817.push_back(([&]()->JSValue{const auto nativeStatementTemp822=nativeStatementTemp754.get("_boxMorphTargets");const auto nativeStatementTemp823=JSValue("min");return nativeStatementTemp822.get(nativeStatementTemp823);}()));return nativeStatementTemp815.call(nativeStatementTemp816,nativeStatementTemp817);}());
([&]()->JSValue{auto nativeStatementTemp827=nativeStatementTemp754.get("_box");const auto nativeStatementTemp828=JSValue("expandByPoint");const auto nativeStatementTemp824=nativeStatementTemp827.get(nativeStatementTemp828);const auto nativeStatementTemp825=nativeStatementTemp827;Args nativeStatementTemp826;nativeStatementTemp826.push_back(nativeStatementTemp754.get("_vector"));return nativeStatementTemp824.call(nativeStatementTemp825,nativeStatementTemp826);}());
([&]()->JSValue{auto nativeStatementTemp832=nativeStatementTemp754.get("_vector");const auto nativeStatementTemp833=JSValue("addVectors");const auto nativeStatementTemp829=nativeStatementTemp832.get(nativeStatementTemp833);const auto nativeStatementTemp830=nativeStatementTemp832;Args nativeStatementTemp831;nativeStatementTemp831.push_back(([&]()->JSValue{const auto nativeStatementTemp834=nativeStatementTemp754.get("_box");const auto nativeStatementTemp835=JSValue("max");return nativeStatementTemp834.get(nativeStatementTemp835);}()));nativeStatementTemp831.push_back(([&]()->JSValue{const auto nativeStatementTemp836=nativeStatementTemp754.get("_boxMorphTargets");const auto nativeStatementTemp837=JSValue("max");return nativeStatementTemp836.get(nativeStatementTemp837);}()));return nativeStatementTemp829.call(nativeStatementTemp830,nativeStatementTemp831);}());
([&]()->JSValue{auto nativeStatementTemp841=nativeStatementTemp754.get("_box");const auto nativeStatementTemp842=JSValue("expandByPoint");const auto nativeStatementTemp838=nativeStatementTemp841.get(nativeStatementTemp842);const auto nativeStatementTemp839=nativeStatementTemp841;Args nativeStatementTemp840;nativeStatementTemp840.push_back(nativeStatementTemp754.get("_vector"));return nativeStatementTemp838.call(nativeStatementTemp839,nativeStatementTemp840);}());

}
else {
([&]()->JSValue{auto nativeStatementTemp846=nativeStatementTemp754.get("_box");const auto nativeStatementTemp847=JSValue("expandByPoint");const auto nativeStatementTemp843=nativeStatementTemp846.get(nativeStatementTemp847);const auto nativeStatementTemp844=nativeStatementTemp846;Args nativeStatementTemp845;nativeStatementTemp845.push_back(([&]()->JSValue{const auto nativeStatementTemp848=nativeStatementTemp754.get("_boxMorphTargets");const auto nativeStatementTemp849=JSValue("min");return nativeStatementTemp848.get(nativeStatementTemp849);}()));return nativeStatementTemp843.call(nativeStatementTemp844,nativeStatementTemp845);}());
([&]()->JSValue{auto nativeStatementTemp853=nativeStatementTemp754.get("_box");const auto nativeStatementTemp854=JSValue("expandByPoint");const auto nativeStatementTemp850=nativeStatementTemp853.get(nativeStatementTemp854);const auto nativeStatementTemp851=nativeStatementTemp853;Args nativeStatementTemp852;nativeStatementTemp852.push_back(([&]()->JSValue{const auto nativeStatementTemp855=nativeStatementTemp754.get("_boxMorphTargets");const auto nativeStatementTemp856=JSValue("max");return nativeStatementTemp855.get(nativeStatementTemp856);}()));return nativeStatementTemp850.call(nativeStatementTemp851,nativeStatementTemp852);}());

}

}
}

}
([&]()->JSValue{auto nativeStatementTemp860=nativeStatementTemp754.get("_box");const auto nativeStatementTemp861=JSValue("getCenter");const auto nativeStatementTemp857=nativeStatementTemp860.get(nativeStatementTemp861);const auto nativeStatementTemp858=nativeStatementTemp860;Args nativeStatementTemp859;nativeStatementTemp859.push_back(center);return nativeStatementTemp857.call(nativeStatementTemp858,nativeStatementTemp859);}());
JSValue maxRadiusSq=JSValue(0);
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp862=position;const auto nativeStatementTemp863=JSValue("count");return nativeStatementTemp862.get(nativeStatementTemp863);}());
for(;([&]()->JSValue{const auto nativeStatementTemp864=i;const auto nativeStatementTemp865=il;return binary("<",nativeStatementTemp864,nativeStatementTemp865);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp866=i.toNumber();const auto nativeStatementTemp867=nativeStatementTemp866+1;i=nativeStatementTemp867;return nativeStatementTemp866;}())){
([&]()->JSValue{auto nativeStatementTemp871=nativeStatementTemp754.get("_vector");const auto nativeStatementTemp872=JSValue("fromBufferAttribute");const auto nativeStatementTemp868=nativeStatementTemp871.get(nativeStatementTemp872);const auto nativeStatementTemp869=nativeStatementTemp871;Args nativeStatementTemp870;nativeStatementTemp870.push_back(position);nativeStatementTemp870.push_back(i);return nativeStatementTemp868.call(nativeStatementTemp869,nativeStatementTemp870);}());
([&]()->JSValue{const auto nativeStatementTemp873=([&]()->JSValue{auto nativeStatementTemp879=nativeGlobalBinding("Math");const auto nativeStatementTemp880=JSValue("max");const auto nativeStatementTemp876=nativeStatementTemp879.get(nativeStatementTemp880);const auto nativeStatementTemp877=nativeStatementTemp879;Args nativeStatementTemp878;nativeStatementTemp878.push_back(maxRadiusSq);nativeStatementTemp878.push_back(([&]()->JSValue{auto nativeStatementTemp884=center;const auto nativeStatementTemp885=JSValue("distanceToSquared");const auto nativeStatementTemp881=nativeStatementTemp884.get(nativeStatementTemp885);const auto nativeStatementTemp882=nativeStatementTemp884;Args nativeStatementTemp883;nativeStatementTemp883.push_back(nativeStatementTemp754.get("_vector"));return nativeStatementTemp881.call(nativeStatementTemp882,nativeStatementTemp883);}()));return nativeStatementTemp876.call(nativeStatementTemp877,nativeStatementTemp878);}());maxRadiusSq=nativeStatementTemp873;return nativeStatementTemp873;}());

}
}
if(morphAttributesPosition.toBoolean()){
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp886=morphAttributesPosition;const auto nativeStatementTemp887=JSValue("length");return nativeStatementTemp886.get(nativeStatementTemp887);}());
for(;([&]()->JSValue{const auto nativeStatementTemp888=i;const auto nativeStatementTemp889=il;return binary("<",nativeStatementTemp888,nativeStatementTemp889);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp890=i.toNumber();const auto nativeStatementTemp891=nativeStatementTemp890+1;i=nativeStatementTemp891;return nativeStatementTemp890;}())){
JSValue morphAttribute=([&]()->JSValue{const auto nativeStatementTemp892=morphAttributesPosition;const auto nativeStatementTemp893=i;return nativeStatementTemp892.get(nativeStatementTemp893);}());
JSValue morphTargetsRelative=([&]()->JSValue{const auto nativeStatementTemp894=nativeStatementTemp755;const auto nativeStatementTemp895=JSValue("morphTargetsRelative");return nativeStatementTemp894.get(nativeStatementTemp895);}());
{JSValue j=JSValue(0);
JSValue jl=([&]()->JSValue{const auto nativeStatementTemp896=morphAttribute;const auto nativeStatementTemp897=JSValue("count");return nativeStatementTemp896.get(nativeStatementTemp897);}());
for(;([&]()->JSValue{const auto nativeStatementTemp898=j;const auto nativeStatementTemp899=jl;return binary("<",nativeStatementTemp898,nativeStatementTemp899);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp900=j.toNumber();const auto nativeStatementTemp901=nativeStatementTemp900+1;j=nativeStatementTemp901;return nativeStatementTemp900;}())){
([&]()->JSValue{auto nativeStatementTemp905=nativeStatementTemp754.get("_vector");const auto nativeStatementTemp906=JSValue("fromBufferAttribute");const auto nativeStatementTemp902=nativeStatementTemp905.get(nativeStatementTemp906);const auto nativeStatementTemp903=nativeStatementTemp905;Args nativeStatementTemp904;nativeStatementTemp904.push_back(morphAttribute);nativeStatementTemp904.push_back(j);return nativeStatementTemp902.call(nativeStatementTemp903,nativeStatementTemp904);}());
if(morphTargetsRelative.toBoolean()){
([&]()->JSValue{auto nativeStatementTemp910=nativeStatementTemp754.get("_offset");const auto nativeStatementTemp911=JSValue("fromBufferAttribute");const auto nativeStatementTemp907=nativeStatementTemp910.get(nativeStatementTemp911);const auto nativeStatementTemp908=nativeStatementTemp910;Args nativeStatementTemp909;nativeStatementTemp909.push_back(position);nativeStatementTemp909.push_back(j);return nativeStatementTemp907.call(nativeStatementTemp908,nativeStatementTemp909);}());
([&]()->JSValue{auto nativeStatementTemp915=nativeStatementTemp754.get("_vector");const auto nativeStatementTemp916=JSValue("add");const auto nativeStatementTemp912=nativeStatementTemp915.get(nativeStatementTemp916);const auto nativeStatementTemp913=nativeStatementTemp915;Args nativeStatementTemp914;nativeStatementTemp914.push_back(nativeStatementTemp754.get("_offset"));return nativeStatementTemp912.call(nativeStatementTemp913,nativeStatementTemp914);}());

}
([&]()->JSValue{const auto nativeStatementTemp917=([&]()->JSValue{auto nativeStatementTemp923=nativeGlobalBinding("Math");const auto nativeStatementTemp924=JSValue("max");const auto nativeStatementTemp920=nativeStatementTemp923.get(nativeStatementTemp924);const auto nativeStatementTemp921=nativeStatementTemp923;Args nativeStatementTemp922;nativeStatementTemp922.push_back(maxRadiusSq);nativeStatementTemp922.push_back(([&]()->JSValue{auto nativeStatementTemp928=center;const auto nativeStatementTemp929=JSValue("distanceToSquared");const auto nativeStatementTemp925=nativeStatementTemp928.get(nativeStatementTemp929);const auto nativeStatementTemp926=nativeStatementTemp928;Args nativeStatementTemp927;nativeStatementTemp927.push_back(nativeStatementTemp754.get("_vector"));return nativeStatementTemp925.call(nativeStatementTemp926,nativeStatementTemp927);}()));return nativeStatementTemp920.call(nativeStatementTemp921,nativeStatementTemp922);}());maxRadiusSq=nativeStatementTemp917;return nativeStatementTemp917;}());

}
}

}
}

}
([&]()->JSValue{auto nativeStatementTemp930=([&]()->JSValue{const auto nativeStatementTemp932=nativeStatementTemp755;const auto nativeStatementTemp933=JSValue("boundingSphere");return nativeStatementTemp932.get(nativeStatementTemp933);}());const auto nativeStatementTemp931=JSValue("radius");const auto nativeStatementTemp934=([&]()->JSValue{auto nativeStatementTemp940=nativeGlobalBinding("Math");const auto nativeStatementTemp941=JSValue("sqrt");const auto nativeStatementTemp937=nativeStatementTemp940.get(nativeStatementTemp941);const auto nativeStatementTemp938=nativeStatementTemp940;Args nativeStatementTemp939;nativeStatementTemp939.push_back(maxRadiusSq);return nativeStatementTemp937.call(nativeStatementTemp938,nativeStatementTemp939);}());setReference(nativeStatementTemp930,nativeStatementTemp931,nativeStatementTemp934);return nativeStatementTemp934;}());
if(([&]()->JSValue{const auto nativeStatementTemp942=nativeGlobalBinding("isNaN");const JSValue nativeStatementTemp943;Args nativeStatementTemp944;nativeStatementTemp944.push_back(([&]()->JSValue{const auto nativeStatementTemp945=([&]()->JSValue{const auto nativeStatementTemp947=nativeStatementTemp755;const auto nativeStatementTemp948=JSValue("boundingSphere");return nativeStatementTemp947.get(nativeStatementTemp948);}());const auto nativeStatementTemp946=JSValue("radius");return nativeStatementTemp945.get(nativeStatementTemp946);}()));return nativeStatementTemp942.call(nativeStatementTemp943,nativeStatementTemp944);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp949=nativeStatementTemp754.get("error");const JSValue nativeStatementTemp950;Args nativeStatementTemp951;nativeStatementTemp951.push_back(JSValue("BufferGeometry.computeBoundingSphere(): Computed radius is NaN. The \"position\" attribute is likely to have NaN values."));nativeStatementTemp951.push_back(nativeStatementTemp755);return nativeStatementTemp949.call(nativeStatementTemp950,nativeStatementTemp951);}());

}

}
return JSValue{};
}

JSValue method24(const JSValue& nativeStatementTemp952,const Args& nativeStatementTemp953,const Args& nativeStatementTemp954){
auto nativeStatementTemp955=nativeStatementTemp954[0];auto nativeStatementTemp956=nativeStatementTemp952;
JSValue index=([&]()->JSValue{const auto nativeStatementTemp958=nativeStatementTemp956;const auto nativeStatementTemp959=JSValue("index");return nativeStatementTemp958.get(nativeStatementTemp959);}());
JSValue attributes=([&]()->JSValue{const auto nativeStatementTemp960=nativeStatementTemp956;const auto nativeStatementTemp961=JSValue("attributes");return nativeStatementTemp960.get(nativeStatementTemp961);}());
if(([&]()->JSValue{const auto nativeStatementTemp962=([&]()->JSValue{const auto nativeStatementTemp963=([&]()->JSValue{const auto nativeStatementTemp964=([&]()->JSValue{const auto nativeStatementTemp965=index;const auto nativeStatementTemp966=JSValue(nullptr);return binary("===",nativeStatementTemp965,nativeStatementTemp966);}());if(nativeStatementTemp964.toBoolean())return nativeStatementTemp964;return ([&]()->JSValue{const auto nativeStatementTemp967=([&]()->JSValue{const auto nativeStatementTemp969=attributes;const auto nativeStatementTemp970=JSValue("position");return nativeStatementTemp969.get(nativeStatementTemp970);}());const auto nativeStatementTemp968=JSValue{};return binary("===",nativeStatementTemp967,nativeStatementTemp968);}());}());if(nativeStatementTemp963.toBoolean())return nativeStatementTemp963;return ([&]()->JSValue{const auto nativeStatementTemp971=([&]()->JSValue{const auto nativeStatementTemp973=attributes;const auto nativeStatementTemp974=JSValue("normal");return nativeStatementTemp973.get(nativeStatementTemp974);}());const auto nativeStatementTemp972=JSValue{};return binary("===",nativeStatementTemp971,nativeStatementTemp972);}());}());if(nativeStatementTemp962.toBoolean())return nativeStatementTemp962;return ([&]()->JSValue{const auto nativeStatementTemp975=([&]()->JSValue{const auto nativeStatementTemp977=attributes;const auto nativeStatementTemp978=JSValue("uv");return nativeStatementTemp977.get(nativeStatementTemp978);}());const auto nativeStatementTemp976=JSValue{};return binary("===",nativeStatementTemp975,nativeStatementTemp976);}());}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp979=nativeStatementTemp955.get("error");const JSValue nativeStatementTemp980;Args nativeStatementTemp981;nativeStatementTemp981.push_back(JSValue("BufferGeometry: .computeTangents() failed. Missing required attributes (index, position, normal or uv)"));return nativeStatementTemp979.call(nativeStatementTemp980,nativeStatementTemp981);}());
return JSValue{};

}
JSValue positionAttribute=([&]()->JSValue{const auto nativeStatementTemp982=attributes;const auto nativeStatementTemp983=JSValue("position");return nativeStatementTemp982.get(nativeStatementTemp983);}());
JSValue normalAttribute=([&]()->JSValue{const auto nativeStatementTemp984=attributes;const auto nativeStatementTemp985=JSValue("normal");return nativeStatementTemp984.get(nativeStatementTemp985);}());
JSValue uvAttribute=([&]()->JSValue{const auto nativeStatementTemp986=attributes;const auto nativeStatementTemp987=JSValue("uv");return nativeStatementTemp986.get(nativeStatementTemp987);}());
JSValue tangentAttribute=([&]()->JSValue{auto nativeStatementTemp991=nativeStatementTemp956;const auto nativeStatementTemp992=JSValue("getAttribute");const auto nativeStatementTemp988=nativeStatementTemp991.get(nativeStatementTemp992);const auto nativeStatementTemp989=nativeStatementTemp991;Args nativeStatementTemp990;nativeStatementTemp990.push_back(JSValue("tangent"));return nativeStatementTemp988.call(nativeStatementTemp989,nativeStatementTemp990);}());
if(([&]()->JSValue{const auto nativeStatementTemp993=([&]()->JSValue{const auto nativeStatementTemp994=tangentAttribute;const auto nativeStatementTemp995=JSValue{};return binary("===",nativeStatementTemp994,nativeStatementTemp995);}());if(nativeStatementTemp993.toBoolean())return nativeStatementTemp993;return ([&]()->JSValue{const auto nativeStatementTemp996=([&]()->JSValue{const auto nativeStatementTemp998=tangentAttribute;const auto nativeStatementTemp999=JSValue("count");return nativeStatementTemp998.get(nativeStatementTemp999);}());const auto nativeStatementTemp997=([&]()->JSValue{const auto nativeStatementTemp1000=positionAttribute;const auto nativeStatementTemp1001=JSValue("count");return nativeStatementTemp1000.get(nativeStatementTemp1001);}());return binary("!==",nativeStatementTemp996,nativeStatementTemp997);}());}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp1002=([&]()->JSValue{const auto nativeStatementTemp1005=nativeStatementTemp955.get("BufferAttribute");Args nativeStatementTemp1006;nativeStatementTemp1006.push_back(([&]()->JSValue{const auto nativeStatementTemp1007=nativeGlobalBinding("Float32Array");Args nativeStatementTemp1008;nativeStatementTemp1008.push_back(([&]()->JSValue{const auto nativeStatementTemp1009=JSValue(4);const auto nativeStatementTemp1010=([&]()->JSValue{const auto nativeStatementTemp1011=positionAttribute;const auto nativeStatementTemp1012=JSValue("count");return nativeStatementTemp1011.get(nativeStatementTemp1012);}());return binary("*",nativeStatementTemp1009,nativeStatementTemp1010);}()));return nativeStatementTemp1007.construct(nativeStatementTemp1008);}()));nativeStatementTemp1006.push_back(JSValue(4));return nativeStatementTemp1005.construct(nativeStatementTemp1006);}());tangentAttribute=nativeStatementTemp1002;return nativeStatementTemp1002;}());
([&]()->JSValue{auto nativeStatementTemp1016=nativeStatementTemp956;const auto nativeStatementTemp1017=JSValue("setAttribute");const auto nativeStatementTemp1013=nativeStatementTemp1016.get(nativeStatementTemp1017);const auto nativeStatementTemp1014=nativeStatementTemp1016;Args nativeStatementTemp1015;nativeStatementTemp1015.push_back(JSValue("tangent"));nativeStatementTemp1015.push_back(tangentAttribute);return nativeStatementTemp1013.call(nativeStatementTemp1014,nativeStatementTemp1015);}());

}
JSValue tan1=([&]()->JSValue{auto nativeStatementTemp1018=JSValue::array();return nativeStatementTemp1018;}());
JSValue tan2=([&]()->JSValue{auto nativeStatementTemp1019=JSValue::array();return nativeStatementTemp1019;}());
JSValue used=([&]()->JSValue{const auto nativeStatementTemp1020=nativeGlobalBinding("Uint8Array");Args nativeStatementTemp1021;nativeStatementTemp1021.push_back(([&]()->JSValue{const auto nativeStatementTemp1022=positionAttribute;const auto nativeStatementTemp1023=JSValue("count");return nativeStatementTemp1022.get(nativeStatementTemp1023);}()));return nativeStatementTemp1020.construct(nativeStatementTemp1021);}());
{JSValue i=JSValue(0);
for(;([&]()->JSValue{const auto nativeStatementTemp1024=i;const auto nativeStatementTemp1025=([&]()->JSValue{const auto nativeStatementTemp1026=positionAttribute;const auto nativeStatementTemp1027=JSValue("count");return nativeStatementTemp1026.get(nativeStatementTemp1027);}());return binary("<",nativeStatementTemp1024,nativeStatementTemp1025);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1028=i.toNumber();const auto nativeStatementTemp1029=nativeStatementTemp1028+1;i=nativeStatementTemp1029;return nativeStatementTemp1028;}())){
([&]()->JSValue{auto nativeStatementTemp1030=tan1;const auto nativeStatementTemp1031=i;const auto nativeStatementTemp1032=([&]()->JSValue{const auto nativeStatementTemp1035=nativeStatementTemp955.get("Vector3");Args nativeStatementTemp1036;return nativeStatementTemp1035.construct(nativeStatementTemp1036);}());setReference(nativeStatementTemp1030,nativeStatementTemp1031,nativeStatementTemp1032);return nativeStatementTemp1032;}());
([&]()->JSValue{auto nativeStatementTemp1037=tan2;const auto nativeStatementTemp1038=i;const auto nativeStatementTemp1039=([&]()->JSValue{const auto nativeStatementTemp1042=nativeStatementTemp955.get("Vector3");Args nativeStatementTemp1043;return nativeStatementTemp1042.construct(nativeStatementTemp1043);}());setReference(nativeStatementTemp1037,nativeStatementTemp1038,nativeStatementTemp1039);return nativeStatementTemp1039;}());

}
}
JSValue vA=([&]()->JSValue{const auto nativeStatementTemp1044=nativeStatementTemp955.get("Vector3");Args nativeStatementTemp1045;return nativeStatementTemp1044.construct(nativeStatementTemp1045);}());
JSValue vB=([&]()->JSValue{const auto nativeStatementTemp1046=nativeStatementTemp955.get("Vector3");Args nativeStatementTemp1047;return nativeStatementTemp1046.construct(nativeStatementTemp1047);}());
JSValue vC=([&]()->JSValue{const auto nativeStatementTemp1048=nativeStatementTemp955.get("Vector3");Args nativeStatementTemp1049;return nativeStatementTemp1048.construct(nativeStatementTemp1049);}());
JSValue uvA=([&]()->JSValue{const auto nativeStatementTemp1050=nativeStatementTemp955.get("Vector2");Args nativeStatementTemp1051;return nativeStatementTemp1050.construct(nativeStatementTemp1051);}());
JSValue uvB=([&]()->JSValue{const auto nativeStatementTemp1052=nativeStatementTemp955.get("Vector2");Args nativeStatementTemp1053;return nativeStatementTemp1052.construct(nativeStatementTemp1053);}());
JSValue uvC=([&]()->JSValue{const auto nativeStatementTemp1054=nativeStatementTemp955.get("Vector2");Args nativeStatementTemp1055;return nativeStatementTemp1054.construct(nativeStatementTemp1055);}());
JSValue sdir=([&]()->JSValue{const auto nativeStatementTemp1056=nativeStatementTemp955.get("Vector3");Args nativeStatementTemp1057;return nativeStatementTemp1056.construct(nativeStatementTemp1057);}());
JSValue tdir=([&]()->JSValue{const auto nativeStatementTemp1058=nativeStatementTemp955.get("Vector3");Args nativeStatementTemp1059;return nativeStatementTemp1058.construct(nativeStatementTemp1059);}());
JSValue handleTriangle=([&]()->JSValue{auto nativeStatementTemp1243=JSValue::constructor(nested0,{nativeStatementTemp955,used,vA,positionAttribute,vB,vC,uvA,uvAttribute,uvB,uvC,sdir,tdir,tan1,tan2},"handleTriangle",3,JSValue::ConstructorKind::Callable);nativeStatementTemp1243.setFunctionSourceText("function handleTriangle( a, b, c ) {\r\n\r\n\t\t\tused[ a ] = used[ b ] = used[ c ] = 1;\r\n\r\n\t\t\tvA.fromBufferAttribute( positionAttribute, a );\r\n\t\t\tvB.fromBufferAttribute( positionAttribute, b );\r\n\t\t\tvC.fromBufferAttribute( positionAttribute, c );\r\n\r\n\t\t\tuvA.fromBufferAttribute( uvAttribute, a );\r\n\t\t\tuvB.fromBufferAttribute( uvAttribute, b );\r\n\t\t\tuvC.fromBufferAttribute( uvAttribute, c );\r\n\r\n\t\t\tvB.sub( vA );\r\n\t\t\tvC.sub( vA );\r\n\r\n\t\t\tuvB.sub( uvA );\r\n\t\t\tuvC.sub( uvA );\r\n\r\n\t\t\tconst r = 1.0 / ( uvB.x * uvC.y - uvC.x * uvB.y );\r\n\r\n\t\t\t// silently ignore degenerate uv triangles having coincident or colinear vertices\r\n\r\n\t\t\tif ( ! isFinite( r ) ) return;\r\n\r\n\t\t\tsdir.copy( vB ).multiplyScalar( uvC.y ).addScaledVector( vC, - uvB.y ).multiplyScalar( r );\r\n\t\t\ttdir.copy( vC ).multiplyScalar( uvB.x ).addScaledVector( vB, - uvC.x ).multiplyScalar( r );\r\n\r\n\t\t\ttan1[ a ].add( sdir );\r\n\t\t\ttan1[ b ].add( sdir );\r\n\t\t\ttan1[ c ].add( sdir );\r\n\r\n\t\t\ttan2[ a ].add( tdir );\r\n\t\t\ttan2[ b ].add( tdir );\r\n\t\t\ttan2[ c ].add( tdir );\r\n\r\n\t\t}");return nativeStatementTemp1243;}());
JSValue groups=([&]()->JSValue{const auto nativeStatementTemp1244=nativeStatementTemp956;const auto nativeStatementTemp1245=JSValue("groups");return nativeStatementTemp1244.get(nativeStatementTemp1245);}());
if(([&]()->JSValue{const auto nativeStatementTemp1246=([&]()->JSValue{const auto nativeStatementTemp1248=groups;const auto nativeStatementTemp1249=JSValue("length");return nativeStatementTemp1248.get(nativeStatementTemp1249);}());const auto nativeStatementTemp1247=JSValue(0);return binary("===",nativeStatementTemp1246,nativeStatementTemp1247);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp1250=([&]()->JSValue{auto nativeStatementTemp1253=JSValue::array();own(nativeStatementTemp1253,JSValue(0),([&]()->JSValue{auto nativeStatementTemp1254=JSValue::object();{const auto nativeStatementTemp1255=JSValue("start");const auto nativeStatementTemp1256=JSValue(0);own(nativeStatementTemp1254,nativeStatementTemp1255,nativeStatementTemp1256);}{const auto nativeStatementTemp1257=JSValue("count");const auto nativeStatementTemp1258=([&]()->JSValue{const auto nativeStatementTemp1259=index;const auto nativeStatementTemp1260=JSValue("count");return nativeStatementTemp1259.get(nativeStatementTemp1260);}());own(nativeStatementTemp1254,nativeStatementTemp1257,nativeStatementTemp1258);}return nativeStatementTemp1254;}()));return nativeStatementTemp1253;}());groups=nativeStatementTemp1250;return nativeStatementTemp1250;}());

}
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp1261=groups;const auto nativeStatementTemp1262=JSValue("length");return nativeStatementTemp1261.get(nativeStatementTemp1262);}());
for(;([&]()->JSValue{const auto nativeStatementTemp1263=i;const auto nativeStatementTemp1264=il;return binary("<",nativeStatementTemp1263,nativeStatementTemp1264);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1265=i.toNumber();const auto nativeStatementTemp1266=nativeStatementTemp1265+1;i=nativeStatementTemp1266;return nativeStatementTemp1266;}())){
JSValue group=([&]()->JSValue{const auto nativeStatementTemp1267=groups;const auto nativeStatementTemp1268=i;return nativeStatementTemp1267.get(nativeStatementTemp1268);}());
JSValue start=([&]()->JSValue{const auto nativeStatementTemp1269=group;const auto nativeStatementTemp1270=JSValue("start");return nativeStatementTemp1269.get(nativeStatementTemp1270);}());
JSValue count=([&]()->JSValue{const auto nativeStatementTemp1271=group;const auto nativeStatementTemp1272=JSValue("count");return nativeStatementTemp1271.get(nativeStatementTemp1272);}());
{JSValue j=start;
JSValue jl=([&]()->JSValue{const auto nativeStatementTemp1273=start;const auto nativeStatementTemp1274=count;return binary("+",nativeStatementTemp1273,nativeStatementTemp1274);}());
for(;([&]()->JSValue{const auto nativeStatementTemp1275=j;const auto nativeStatementTemp1276=jl;return binary("<",nativeStatementTemp1275,nativeStatementTemp1276);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1278=j;const auto nativeStatementTemp1279=JSValue(3);const auto nativeStatementTemp1277=binary("+",nativeStatementTemp1278,nativeStatementTemp1279);j=nativeStatementTemp1277;return nativeStatementTemp1277;}())){
([&]()->JSValue{const auto nativeStatementTemp1280=handleTriangle;const JSValue nativeStatementTemp1281;Args nativeStatementTemp1282;nativeStatementTemp1282.push_back(([&]()->JSValue{auto nativeStatementTemp1286=index;const auto nativeStatementTemp1287=JSValue("getX");const auto nativeStatementTemp1283=nativeStatementTemp1286.get(nativeStatementTemp1287);const auto nativeStatementTemp1284=nativeStatementTemp1286;Args nativeStatementTemp1285;nativeStatementTemp1285.push_back(([&]()->JSValue{const auto nativeStatementTemp1288=j;const auto nativeStatementTemp1289=JSValue(0);return binary("+",nativeStatementTemp1288,nativeStatementTemp1289);}()));return nativeStatementTemp1283.call(nativeStatementTemp1284,nativeStatementTemp1285);}()));nativeStatementTemp1282.push_back(([&]()->JSValue{auto nativeStatementTemp1293=index;const auto nativeStatementTemp1294=JSValue("getX");const auto nativeStatementTemp1290=nativeStatementTemp1293.get(nativeStatementTemp1294);const auto nativeStatementTemp1291=nativeStatementTemp1293;Args nativeStatementTemp1292;nativeStatementTemp1292.push_back(([&]()->JSValue{const auto nativeStatementTemp1295=j;const auto nativeStatementTemp1296=JSValue(1);return binary("+",nativeStatementTemp1295,nativeStatementTemp1296);}()));return nativeStatementTemp1290.call(nativeStatementTemp1291,nativeStatementTemp1292);}()));nativeStatementTemp1282.push_back(([&]()->JSValue{auto nativeStatementTemp1300=index;const auto nativeStatementTemp1301=JSValue("getX");const auto nativeStatementTemp1297=nativeStatementTemp1300.get(nativeStatementTemp1301);const auto nativeStatementTemp1298=nativeStatementTemp1300;Args nativeStatementTemp1299;nativeStatementTemp1299.push_back(([&]()->JSValue{const auto nativeStatementTemp1302=j;const auto nativeStatementTemp1303=JSValue(2);return binary("+",nativeStatementTemp1302,nativeStatementTemp1303);}()));return nativeStatementTemp1297.call(nativeStatementTemp1298,nativeStatementTemp1299);}()));return nativeStatementTemp1280.call(nativeStatementTemp1281,nativeStatementTemp1282);}());

}
}

}
}
JSValue tmp=([&]()->JSValue{const auto nativeStatementTemp1304=nativeStatementTemp955.get("Vector3");Args nativeStatementTemp1305;return nativeStatementTemp1304.construct(nativeStatementTemp1305);}());
JSValue tmp2=([&]()->JSValue{const auto nativeStatementTemp1306=nativeStatementTemp955.get("Vector3");Args nativeStatementTemp1307;return nativeStatementTemp1306.construct(nativeStatementTemp1307);}());
JSValue n=([&]()->JSValue{const auto nativeStatementTemp1308=nativeStatementTemp955.get("Vector3");Args nativeStatementTemp1309;return nativeStatementTemp1308.construct(nativeStatementTemp1309);}());
JSValue n2=([&]()->JSValue{const auto nativeStatementTemp1310=nativeStatementTemp955.get("Vector3");Args nativeStatementTemp1311;return nativeStatementTemp1310.construct(nativeStatementTemp1311);}());
JSValue handleVertex=([&]()->JSValue{auto nativeStatementTemp1381=JSValue::constructor(nested1,{nativeStatementTemp955,n,normalAttribute,n2,tan1,tmp,tmp2,tan2,tangentAttribute},"handleVertex",1,JSValue::ConstructorKind::Callable);nativeStatementTemp1381.setFunctionSourceText("function handleVertex( v ) {\r\n\r\n\t\t\tn.fromBufferAttribute( normalAttribute, v );\r\n\t\t\tn2.copy( n );\r\n\r\n\t\t\tconst t = tan1[ v ];\r\n\r\n\t\t\t// Gram-Schmidt orthogonalize\r\n\r\n\t\t\ttmp.copy( t );\r\n\t\t\ttmp.sub( n.multiplyScalar( n.dot( t ) ) ).normalize();\r\n\r\n\t\t\t// Calculate handedness\r\n\r\n\t\t\ttmp2.crossVectors( n2, t );\r\n\t\t\tconst test = tmp2.dot( tan2[ v ] );\r\n\t\t\tconst w = ( test < 0.0 ) ? - 1.0 : 1.0;\r\n\r\n\t\t\ttangentAttribute.setXYZW( v, tmp.x, tmp.y, tmp.z, w );\r\n\r\n\t\t}");return nativeStatementTemp1381;}());
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp1382=positionAttribute;const auto nativeStatementTemp1383=JSValue("count");return nativeStatementTemp1382.get(nativeStatementTemp1383);}());
for(;([&]()->JSValue{const auto nativeStatementTemp1384=i;const auto nativeStatementTemp1385=il;return binary("<",nativeStatementTemp1384,nativeStatementTemp1385);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1386=i.toNumber();const auto nativeStatementTemp1387=nativeStatementTemp1386+1;i=nativeStatementTemp1387;return nativeStatementTemp1387;}())){
if(([&]()->JSValue{const auto nativeStatementTemp1388=used;const auto nativeStatementTemp1389=i;return nativeStatementTemp1388.get(nativeStatementTemp1389);}()).toBoolean())([&]()->JSValue{const auto nativeStatementTemp1390=handleVertex;const JSValue nativeStatementTemp1391;Args nativeStatementTemp1392;nativeStatementTemp1392.push_back(i);return nativeStatementTemp1390.call(nativeStatementTemp1391,nativeStatementTemp1392);}());

}
}
([&]()->JSValue{auto nativeStatementTemp1393=nativeStatementTemp956;const auto nativeStatementTemp1394=JSValue("_transformed");const auto nativeStatementTemp1395=JSValue(true);setReference(nativeStatementTemp1393,nativeStatementTemp1394,nativeStatementTemp1395);return nativeStatementTemp1395;}());
return JSValue{};
}

JSValue method25(const JSValue& nativeStatementTemp1398,const Args& nativeStatementTemp1399,const Args& nativeStatementTemp1400){
auto nativeStatementTemp1401=nativeStatementTemp1400[0];auto nativeStatementTemp1402=nativeStatementTemp1398;
JSValue index=([&]()->JSValue{const auto nativeStatementTemp1404=nativeStatementTemp1402;const auto nativeStatementTemp1405=JSValue("index");return nativeStatementTemp1404.get(nativeStatementTemp1405);}());
JSValue positionAttribute=([&]()->JSValue{auto nativeStatementTemp1409=nativeStatementTemp1402;const auto nativeStatementTemp1410=JSValue("getAttribute");const auto nativeStatementTemp1406=nativeStatementTemp1409.get(nativeStatementTemp1410);const auto nativeStatementTemp1407=nativeStatementTemp1409;Args nativeStatementTemp1408;nativeStatementTemp1408.push_back(JSValue("position"));return nativeStatementTemp1406.call(nativeStatementTemp1407,nativeStatementTemp1408);}());
if(([&]()->JSValue{const auto nativeStatementTemp1411=positionAttribute;const auto nativeStatementTemp1412=JSValue{};return binary("!==",nativeStatementTemp1411,nativeStatementTemp1412);}()).toBoolean()){
JSValue normalAttribute=([&]()->JSValue{auto nativeStatementTemp1416=nativeStatementTemp1402;const auto nativeStatementTemp1417=JSValue("getAttribute");const auto nativeStatementTemp1413=nativeStatementTemp1416.get(nativeStatementTemp1417);const auto nativeStatementTemp1414=nativeStatementTemp1416;Args nativeStatementTemp1415;nativeStatementTemp1415.push_back(JSValue("normal"));return nativeStatementTemp1413.call(nativeStatementTemp1414,nativeStatementTemp1415);}());
if(([&]()->JSValue{const auto nativeStatementTemp1418=([&]()->JSValue{const auto nativeStatementTemp1419=normalAttribute;const auto nativeStatementTemp1420=JSValue{};return binary("===",nativeStatementTemp1419,nativeStatementTemp1420);}());if(nativeStatementTemp1418.toBoolean())return nativeStatementTemp1418;return ([&]()->JSValue{const auto nativeStatementTemp1421=([&]()->JSValue{const auto nativeStatementTemp1423=normalAttribute;const auto nativeStatementTemp1424=JSValue("count");return nativeStatementTemp1423.get(nativeStatementTemp1424);}());const auto nativeStatementTemp1422=([&]()->JSValue{const auto nativeStatementTemp1425=positionAttribute;const auto nativeStatementTemp1426=JSValue("count");return nativeStatementTemp1425.get(nativeStatementTemp1426);}());return binary("!==",nativeStatementTemp1421,nativeStatementTemp1422);}());}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp1427=([&]()->JSValue{const auto nativeStatementTemp1430=nativeStatementTemp1401.get("BufferAttribute");Args nativeStatementTemp1431;nativeStatementTemp1431.push_back(([&]()->JSValue{const auto nativeStatementTemp1432=nativeGlobalBinding("Float32Array");Args nativeStatementTemp1433;nativeStatementTemp1433.push_back(([&]()->JSValue{const auto nativeStatementTemp1434=([&]()->JSValue{const auto nativeStatementTemp1436=positionAttribute;const auto nativeStatementTemp1437=JSValue("count");return nativeStatementTemp1436.get(nativeStatementTemp1437);}());const auto nativeStatementTemp1435=JSValue(3);return binary("*",nativeStatementTemp1434,nativeStatementTemp1435);}()));return nativeStatementTemp1432.construct(nativeStatementTemp1433);}()));nativeStatementTemp1431.push_back(JSValue(3));return nativeStatementTemp1430.construct(nativeStatementTemp1431);}());normalAttribute=nativeStatementTemp1427;return nativeStatementTemp1427;}());
([&]()->JSValue{auto nativeStatementTemp1441=nativeStatementTemp1402;const auto nativeStatementTemp1442=JSValue("setAttribute");const auto nativeStatementTemp1438=nativeStatementTemp1441.get(nativeStatementTemp1442);const auto nativeStatementTemp1439=nativeStatementTemp1441;Args nativeStatementTemp1440;nativeStatementTemp1440.push_back(JSValue("normal"));nativeStatementTemp1440.push_back(normalAttribute);return nativeStatementTemp1438.call(nativeStatementTemp1439,nativeStatementTemp1440);}());

}
else {
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp1443=normalAttribute;const auto nativeStatementTemp1444=JSValue("count");return nativeStatementTemp1443.get(nativeStatementTemp1444);}());
for(;([&]()->JSValue{const auto nativeStatementTemp1445=i;const auto nativeStatementTemp1446=il;return binary("<",nativeStatementTemp1445,nativeStatementTemp1446);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1447=i.toNumber();const auto nativeStatementTemp1448=nativeStatementTemp1447+1;i=nativeStatementTemp1448;return nativeStatementTemp1447;}())){
([&]()->JSValue{auto nativeStatementTemp1452=normalAttribute;const auto nativeStatementTemp1453=JSValue("setXYZ");const auto nativeStatementTemp1449=nativeStatementTemp1452.get(nativeStatementTemp1453);const auto nativeStatementTemp1450=nativeStatementTemp1452;Args nativeStatementTemp1451;nativeStatementTemp1451.push_back(i);nativeStatementTemp1451.push_back(JSValue(0));nativeStatementTemp1451.push_back(JSValue(0));nativeStatementTemp1451.push_back(JSValue(0));return nativeStatementTemp1449.call(nativeStatementTemp1450,nativeStatementTemp1451);}());

}
}

}
JSValue pA=([&]()->JSValue{const auto nativeStatementTemp1454=nativeStatementTemp1401.get("Vector3");Args nativeStatementTemp1455;return nativeStatementTemp1454.construct(nativeStatementTemp1455);}());
JSValue pB=([&]()->JSValue{const auto nativeStatementTemp1456=nativeStatementTemp1401.get("Vector3");Args nativeStatementTemp1457;return nativeStatementTemp1456.construct(nativeStatementTemp1457);}());
JSValue pC=([&]()->JSValue{const auto nativeStatementTemp1458=nativeStatementTemp1401.get("Vector3");Args nativeStatementTemp1459;return nativeStatementTemp1458.construct(nativeStatementTemp1459);}());
JSValue nA=([&]()->JSValue{const auto nativeStatementTemp1460=nativeStatementTemp1401.get("Vector3");Args nativeStatementTemp1461;return nativeStatementTemp1460.construct(nativeStatementTemp1461);}());
JSValue nB=([&]()->JSValue{const auto nativeStatementTemp1462=nativeStatementTemp1401.get("Vector3");Args nativeStatementTemp1463;return nativeStatementTemp1462.construct(nativeStatementTemp1463);}());
JSValue nC=([&]()->JSValue{const auto nativeStatementTemp1464=nativeStatementTemp1401.get("Vector3");Args nativeStatementTemp1465;return nativeStatementTemp1464.construct(nativeStatementTemp1465);}());
JSValue cb=([&]()->JSValue{const auto nativeStatementTemp1466=nativeStatementTemp1401.get("Vector3");Args nativeStatementTemp1467;return nativeStatementTemp1466.construct(nativeStatementTemp1467);}());
JSValue ab=([&]()->JSValue{const auto nativeStatementTemp1468=nativeStatementTemp1401.get("Vector3");Args nativeStatementTemp1469;return nativeStatementTemp1468.construct(nativeStatementTemp1469);}());
if(index.toBoolean()){
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp1470=index;const auto nativeStatementTemp1471=JSValue("count");return nativeStatementTemp1470.get(nativeStatementTemp1471);}());
for(;([&]()->JSValue{const auto nativeStatementTemp1472=i;const auto nativeStatementTemp1473=il;return binary("<",nativeStatementTemp1472,nativeStatementTemp1473);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1475=i;const auto nativeStatementTemp1476=JSValue(3);const auto nativeStatementTemp1474=binary("+",nativeStatementTemp1475,nativeStatementTemp1476);i=nativeStatementTemp1474;return nativeStatementTemp1474;}())){
JSValue vA=([&]()->JSValue{auto nativeStatementTemp1480=index;const auto nativeStatementTemp1481=JSValue("getX");const auto nativeStatementTemp1477=nativeStatementTemp1480.get(nativeStatementTemp1481);const auto nativeStatementTemp1478=nativeStatementTemp1480;Args nativeStatementTemp1479;nativeStatementTemp1479.push_back(([&]()->JSValue{const auto nativeStatementTemp1482=i;const auto nativeStatementTemp1483=JSValue(0);return binary("+",nativeStatementTemp1482,nativeStatementTemp1483);}()));return nativeStatementTemp1477.call(nativeStatementTemp1478,nativeStatementTemp1479);}());
JSValue vB=([&]()->JSValue{auto nativeStatementTemp1487=index;const auto nativeStatementTemp1488=JSValue("getX");const auto nativeStatementTemp1484=nativeStatementTemp1487.get(nativeStatementTemp1488);const auto nativeStatementTemp1485=nativeStatementTemp1487;Args nativeStatementTemp1486;nativeStatementTemp1486.push_back(([&]()->JSValue{const auto nativeStatementTemp1489=i;const auto nativeStatementTemp1490=JSValue(1);return binary("+",nativeStatementTemp1489,nativeStatementTemp1490);}()));return nativeStatementTemp1484.call(nativeStatementTemp1485,nativeStatementTemp1486);}());
JSValue vC=([&]()->JSValue{auto nativeStatementTemp1494=index;const auto nativeStatementTemp1495=JSValue("getX");const auto nativeStatementTemp1491=nativeStatementTemp1494.get(nativeStatementTemp1495);const auto nativeStatementTemp1492=nativeStatementTemp1494;Args nativeStatementTemp1493;nativeStatementTemp1493.push_back(([&]()->JSValue{const auto nativeStatementTemp1496=i;const auto nativeStatementTemp1497=JSValue(2);return binary("+",nativeStatementTemp1496,nativeStatementTemp1497);}()));return nativeStatementTemp1491.call(nativeStatementTemp1492,nativeStatementTemp1493);}());
([&]()->JSValue{auto nativeStatementTemp1501=pA;const auto nativeStatementTemp1502=JSValue("fromBufferAttribute");const auto nativeStatementTemp1498=nativeStatementTemp1501.get(nativeStatementTemp1502);const auto nativeStatementTemp1499=nativeStatementTemp1501;Args nativeStatementTemp1500;nativeStatementTemp1500.push_back(positionAttribute);nativeStatementTemp1500.push_back(vA);return nativeStatementTemp1498.call(nativeStatementTemp1499,nativeStatementTemp1500);}());
([&]()->JSValue{auto nativeStatementTemp1506=pB;const auto nativeStatementTemp1507=JSValue("fromBufferAttribute");const auto nativeStatementTemp1503=nativeStatementTemp1506.get(nativeStatementTemp1507);const auto nativeStatementTemp1504=nativeStatementTemp1506;Args nativeStatementTemp1505;nativeStatementTemp1505.push_back(positionAttribute);nativeStatementTemp1505.push_back(vB);return nativeStatementTemp1503.call(nativeStatementTemp1504,nativeStatementTemp1505);}());
([&]()->JSValue{auto nativeStatementTemp1511=pC;const auto nativeStatementTemp1512=JSValue("fromBufferAttribute");const auto nativeStatementTemp1508=nativeStatementTemp1511.get(nativeStatementTemp1512);const auto nativeStatementTemp1509=nativeStatementTemp1511;Args nativeStatementTemp1510;nativeStatementTemp1510.push_back(positionAttribute);nativeStatementTemp1510.push_back(vC);return nativeStatementTemp1508.call(nativeStatementTemp1509,nativeStatementTemp1510);}());
([&]()->JSValue{auto nativeStatementTemp1516=cb;const auto nativeStatementTemp1517=JSValue("subVectors");const auto nativeStatementTemp1513=nativeStatementTemp1516.get(nativeStatementTemp1517);const auto nativeStatementTemp1514=nativeStatementTemp1516;Args nativeStatementTemp1515;nativeStatementTemp1515.push_back(pC);nativeStatementTemp1515.push_back(pB);return nativeStatementTemp1513.call(nativeStatementTemp1514,nativeStatementTemp1515);}());
([&]()->JSValue{auto nativeStatementTemp1521=ab;const auto nativeStatementTemp1522=JSValue("subVectors");const auto nativeStatementTemp1518=nativeStatementTemp1521.get(nativeStatementTemp1522);const auto nativeStatementTemp1519=nativeStatementTemp1521;Args nativeStatementTemp1520;nativeStatementTemp1520.push_back(pA);nativeStatementTemp1520.push_back(pB);return nativeStatementTemp1518.call(nativeStatementTemp1519,nativeStatementTemp1520);}());
([&]()->JSValue{auto nativeStatementTemp1526=cb;const auto nativeStatementTemp1527=JSValue("cross");const auto nativeStatementTemp1523=nativeStatementTemp1526.get(nativeStatementTemp1527);const auto nativeStatementTemp1524=nativeStatementTemp1526;Args nativeStatementTemp1525;nativeStatementTemp1525.push_back(ab);return nativeStatementTemp1523.call(nativeStatementTemp1524,nativeStatementTemp1525);}());
([&]()->JSValue{auto nativeStatementTemp1531=nA;const auto nativeStatementTemp1532=JSValue("fromBufferAttribute");const auto nativeStatementTemp1528=nativeStatementTemp1531.get(nativeStatementTemp1532);const auto nativeStatementTemp1529=nativeStatementTemp1531;Args nativeStatementTemp1530;nativeStatementTemp1530.push_back(normalAttribute);nativeStatementTemp1530.push_back(vA);return nativeStatementTemp1528.call(nativeStatementTemp1529,nativeStatementTemp1530);}());
([&]()->JSValue{auto nativeStatementTemp1536=nB;const auto nativeStatementTemp1537=JSValue("fromBufferAttribute");const auto nativeStatementTemp1533=nativeStatementTemp1536.get(nativeStatementTemp1537);const auto nativeStatementTemp1534=nativeStatementTemp1536;Args nativeStatementTemp1535;nativeStatementTemp1535.push_back(normalAttribute);nativeStatementTemp1535.push_back(vB);return nativeStatementTemp1533.call(nativeStatementTemp1534,nativeStatementTemp1535);}());
([&]()->JSValue{auto nativeStatementTemp1541=nC;const auto nativeStatementTemp1542=JSValue("fromBufferAttribute");const auto nativeStatementTemp1538=nativeStatementTemp1541.get(nativeStatementTemp1542);const auto nativeStatementTemp1539=nativeStatementTemp1541;Args nativeStatementTemp1540;nativeStatementTemp1540.push_back(normalAttribute);nativeStatementTemp1540.push_back(vC);return nativeStatementTemp1538.call(nativeStatementTemp1539,nativeStatementTemp1540);}());
([&]()->JSValue{auto nativeStatementTemp1546=nA;const auto nativeStatementTemp1547=JSValue("add");const auto nativeStatementTemp1543=nativeStatementTemp1546.get(nativeStatementTemp1547);const auto nativeStatementTemp1544=nativeStatementTemp1546;Args nativeStatementTemp1545;nativeStatementTemp1545.push_back(cb);return nativeStatementTemp1543.call(nativeStatementTemp1544,nativeStatementTemp1545);}());
([&]()->JSValue{auto nativeStatementTemp1551=nB;const auto nativeStatementTemp1552=JSValue("add");const auto nativeStatementTemp1548=nativeStatementTemp1551.get(nativeStatementTemp1552);const auto nativeStatementTemp1549=nativeStatementTemp1551;Args nativeStatementTemp1550;nativeStatementTemp1550.push_back(cb);return nativeStatementTemp1548.call(nativeStatementTemp1549,nativeStatementTemp1550);}());
([&]()->JSValue{auto nativeStatementTemp1556=nC;const auto nativeStatementTemp1557=JSValue("add");const auto nativeStatementTemp1553=nativeStatementTemp1556.get(nativeStatementTemp1557);const auto nativeStatementTemp1554=nativeStatementTemp1556;Args nativeStatementTemp1555;nativeStatementTemp1555.push_back(cb);return nativeStatementTemp1553.call(nativeStatementTemp1554,nativeStatementTemp1555);}());
([&]()->JSValue{auto nativeStatementTemp1561=normalAttribute;const auto nativeStatementTemp1562=JSValue("setXYZ");const auto nativeStatementTemp1558=nativeStatementTemp1561.get(nativeStatementTemp1562);const auto nativeStatementTemp1559=nativeStatementTemp1561;Args nativeStatementTemp1560;nativeStatementTemp1560.push_back(vA);nativeStatementTemp1560.push_back(([&]()->JSValue{const auto nativeStatementTemp1563=nA;const auto nativeStatementTemp1564=JSValue("x");return nativeStatementTemp1563.get(nativeStatementTemp1564);}()));nativeStatementTemp1560.push_back(([&]()->JSValue{const auto nativeStatementTemp1565=nA;const auto nativeStatementTemp1566=JSValue("y");return nativeStatementTemp1565.get(nativeStatementTemp1566);}()));nativeStatementTemp1560.push_back(([&]()->JSValue{const auto nativeStatementTemp1567=nA;const auto nativeStatementTemp1568=JSValue("z");return nativeStatementTemp1567.get(nativeStatementTemp1568);}()));return nativeStatementTemp1558.call(nativeStatementTemp1559,nativeStatementTemp1560);}());
([&]()->JSValue{auto nativeStatementTemp1572=normalAttribute;const auto nativeStatementTemp1573=JSValue("setXYZ");const auto nativeStatementTemp1569=nativeStatementTemp1572.get(nativeStatementTemp1573);const auto nativeStatementTemp1570=nativeStatementTemp1572;Args nativeStatementTemp1571;nativeStatementTemp1571.push_back(vB);nativeStatementTemp1571.push_back(([&]()->JSValue{const auto nativeStatementTemp1574=nB;const auto nativeStatementTemp1575=JSValue("x");return nativeStatementTemp1574.get(nativeStatementTemp1575);}()));nativeStatementTemp1571.push_back(([&]()->JSValue{const auto nativeStatementTemp1576=nB;const auto nativeStatementTemp1577=JSValue("y");return nativeStatementTemp1576.get(nativeStatementTemp1577);}()));nativeStatementTemp1571.push_back(([&]()->JSValue{const auto nativeStatementTemp1578=nB;const auto nativeStatementTemp1579=JSValue("z");return nativeStatementTemp1578.get(nativeStatementTemp1579);}()));return nativeStatementTemp1569.call(nativeStatementTemp1570,nativeStatementTemp1571);}());
([&]()->JSValue{auto nativeStatementTemp1583=normalAttribute;const auto nativeStatementTemp1584=JSValue("setXYZ");const auto nativeStatementTemp1580=nativeStatementTemp1583.get(nativeStatementTemp1584);const auto nativeStatementTemp1581=nativeStatementTemp1583;Args nativeStatementTemp1582;nativeStatementTemp1582.push_back(vC);nativeStatementTemp1582.push_back(([&]()->JSValue{const auto nativeStatementTemp1585=nC;const auto nativeStatementTemp1586=JSValue("x");return nativeStatementTemp1585.get(nativeStatementTemp1586);}()));nativeStatementTemp1582.push_back(([&]()->JSValue{const auto nativeStatementTemp1587=nC;const auto nativeStatementTemp1588=JSValue("y");return nativeStatementTemp1587.get(nativeStatementTemp1588);}()));nativeStatementTemp1582.push_back(([&]()->JSValue{const auto nativeStatementTemp1589=nC;const auto nativeStatementTemp1590=JSValue("z");return nativeStatementTemp1589.get(nativeStatementTemp1590);}()));return nativeStatementTemp1580.call(nativeStatementTemp1581,nativeStatementTemp1582);}());

}
}

}
else {
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp1591=positionAttribute;const auto nativeStatementTemp1592=JSValue("count");return nativeStatementTemp1591.get(nativeStatementTemp1592);}());
for(;([&]()->JSValue{const auto nativeStatementTemp1593=i;const auto nativeStatementTemp1594=il;return binary("<",nativeStatementTemp1593,nativeStatementTemp1594);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1596=i;const auto nativeStatementTemp1597=JSValue(3);const auto nativeStatementTemp1595=binary("+",nativeStatementTemp1596,nativeStatementTemp1597);i=nativeStatementTemp1595;return nativeStatementTemp1595;}())){
([&]()->JSValue{auto nativeStatementTemp1601=pA;const auto nativeStatementTemp1602=JSValue("fromBufferAttribute");const auto nativeStatementTemp1598=nativeStatementTemp1601.get(nativeStatementTemp1602);const auto nativeStatementTemp1599=nativeStatementTemp1601;Args nativeStatementTemp1600;nativeStatementTemp1600.push_back(positionAttribute);nativeStatementTemp1600.push_back(([&]()->JSValue{const auto nativeStatementTemp1603=i;const auto nativeStatementTemp1604=JSValue(0);return binary("+",nativeStatementTemp1603,nativeStatementTemp1604);}()));return nativeStatementTemp1598.call(nativeStatementTemp1599,nativeStatementTemp1600);}());
([&]()->JSValue{auto nativeStatementTemp1608=pB;const auto nativeStatementTemp1609=JSValue("fromBufferAttribute");const auto nativeStatementTemp1605=nativeStatementTemp1608.get(nativeStatementTemp1609);const auto nativeStatementTemp1606=nativeStatementTemp1608;Args nativeStatementTemp1607;nativeStatementTemp1607.push_back(positionAttribute);nativeStatementTemp1607.push_back(([&]()->JSValue{const auto nativeStatementTemp1610=i;const auto nativeStatementTemp1611=JSValue(1);return binary("+",nativeStatementTemp1610,nativeStatementTemp1611);}()));return nativeStatementTemp1605.call(nativeStatementTemp1606,nativeStatementTemp1607);}());
([&]()->JSValue{auto nativeStatementTemp1615=pC;const auto nativeStatementTemp1616=JSValue("fromBufferAttribute");const auto nativeStatementTemp1612=nativeStatementTemp1615.get(nativeStatementTemp1616);const auto nativeStatementTemp1613=nativeStatementTemp1615;Args nativeStatementTemp1614;nativeStatementTemp1614.push_back(positionAttribute);nativeStatementTemp1614.push_back(([&]()->JSValue{const auto nativeStatementTemp1617=i;const auto nativeStatementTemp1618=JSValue(2);return binary("+",nativeStatementTemp1617,nativeStatementTemp1618);}()));return nativeStatementTemp1612.call(nativeStatementTemp1613,nativeStatementTemp1614);}());
([&]()->JSValue{auto nativeStatementTemp1622=cb;const auto nativeStatementTemp1623=JSValue("subVectors");const auto nativeStatementTemp1619=nativeStatementTemp1622.get(nativeStatementTemp1623);const auto nativeStatementTemp1620=nativeStatementTemp1622;Args nativeStatementTemp1621;nativeStatementTemp1621.push_back(pC);nativeStatementTemp1621.push_back(pB);return nativeStatementTemp1619.call(nativeStatementTemp1620,nativeStatementTemp1621);}());
([&]()->JSValue{auto nativeStatementTemp1627=ab;const auto nativeStatementTemp1628=JSValue("subVectors");const auto nativeStatementTemp1624=nativeStatementTemp1627.get(nativeStatementTemp1628);const auto nativeStatementTemp1625=nativeStatementTemp1627;Args nativeStatementTemp1626;nativeStatementTemp1626.push_back(pA);nativeStatementTemp1626.push_back(pB);return nativeStatementTemp1624.call(nativeStatementTemp1625,nativeStatementTemp1626);}());
([&]()->JSValue{auto nativeStatementTemp1632=cb;const auto nativeStatementTemp1633=JSValue("cross");const auto nativeStatementTemp1629=nativeStatementTemp1632.get(nativeStatementTemp1633);const auto nativeStatementTemp1630=nativeStatementTemp1632;Args nativeStatementTemp1631;nativeStatementTemp1631.push_back(ab);return nativeStatementTemp1629.call(nativeStatementTemp1630,nativeStatementTemp1631);}());
([&]()->JSValue{auto nativeStatementTemp1637=normalAttribute;const auto nativeStatementTemp1638=JSValue("setXYZ");const auto nativeStatementTemp1634=nativeStatementTemp1637.get(nativeStatementTemp1638);const auto nativeStatementTemp1635=nativeStatementTemp1637;Args nativeStatementTemp1636;nativeStatementTemp1636.push_back(([&]()->JSValue{const auto nativeStatementTemp1639=i;const auto nativeStatementTemp1640=JSValue(0);return binary("+",nativeStatementTemp1639,nativeStatementTemp1640);}()));nativeStatementTemp1636.push_back(([&]()->JSValue{const auto nativeStatementTemp1641=cb;const auto nativeStatementTemp1642=JSValue("x");return nativeStatementTemp1641.get(nativeStatementTemp1642);}()));nativeStatementTemp1636.push_back(([&]()->JSValue{const auto nativeStatementTemp1643=cb;const auto nativeStatementTemp1644=JSValue("y");return nativeStatementTemp1643.get(nativeStatementTemp1644);}()));nativeStatementTemp1636.push_back(([&]()->JSValue{const auto nativeStatementTemp1645=cb;const auto nativeStatementTemp1646=JSValue("z");return nativeStatementTemp1645.get(nativeStatementTemp1646);}()));return nativeStatementTemp1634.call(nativeStatementTemp1635,nativeStatementTemp1636);}());
([&]()->JSValue{auto nativeStatementTemp1650=normalAttribute;const auto nativeStatementTemp1651=JSValue("setXYZ");const auto nativeStatementTemp1647=nativeStatementTemp1650.get(nativeStatementTemp1651);const auto nativeStatementTemp1648=nativeStatementTemp1650;Args nativeStatementTemp1649;nativeStatementTemp1649.push_back(([&]()->JSValue{const auto nativeStatementTemp1652=i;const auto nativeStatementTemp1653=JSValue(1);return binary("+",nativeStatementTemp1652,nativeStatementTemp1653);}()));nativeStatementTemp1649.push_back(([&]()->JSValue{const auto nativeStatementTemp1654=cb;const auto nativeStatementTemp1655=JSValue("x");return nativeStatementTemp1654.get(nativeStatementTemp1655);}()));nativeStatementTemp1649.push_back(([&]()->JSValue{const auto nativeStatementTemp1656=cb;const auto nativeStatementTemp1657=JSValue("y");return nativeStatementTemp1656.get(nativeStatementTemp1657);}()));nativeStatementTemp1649.push_back(([&]()->JSValue{const auto nativeStatementTemp1658=cb;const auto nativeStatementTemp1659=JSValue("z");return nativeStatementTemp1658.get(nativeStatementTemp1659);}()));return nativeStatementTemp1647.call(nativeStatementTemp1648,nativeStatementTemp1649);}());
([&]()->JSValue{auto nativeStatementTemp1663=normalAttribute;const auto nativeStatementTemp1664=JSValue("setXYZ");const auto nativeStatementTemp1660=nativeStatementTemp1663.get(nativeStatementTemp1664);const auto nativeStatementTemp1661=nativeStatementTemp1663;Args nativeStatementTemp1662;nativeStatementTemp1662.push_back(([&]()->JSValue{const auto nativeStatementTemp1665=i;const auto nativeStatementTemp1666=JSValue(2);return binary("+",nativeStatementTemp1665,nativeStatementTemp1666);}()));nativeStatementTemp1662.push_back(([&]()->JSValue{const auto nativeStatementTemp1667=cb;const auto nativeStatementTemp1668=JSValue("x");return nativeStatementTemp1667.get(nativeStatementTemp1668);}()));nativeStatementTemp1662.push_back(([&]()->JSValue{const auto nativeStatementTemp1669=cb;const auto nativeStatementTemp1670=JSValue("y");return nativeStatementTemp1669.get(nativeStatementTemp1670);}()));nativeStatementTemp1662.push_back(([&]()->JSValue{const auto nativeStatementTemp1671=cb;const auto nativeStatementTemp1672=JSValue("z");return nativeStatementTemp1671.get(nativeStatementTemp1672);}()));return nativeStatementTemp1660.call(nativeStatementTemp1661,nativeStatementTemp1662);}());

}
}

}
([&]()->JSValue{auto nativeStatementTemp1676=nativeStatementTemp1402;const auto nativeStatementTemp1677=JSValue("normalizeNormals");const auto nativeStatementTemp1673=nativeStatementTemp1676.get(nativeStatementTemp1677);const auto nativeStatementTemp1674=nativeStatementTemp1676;Args nativeStatementTemp1675;return nativeStatementTemp1673.call(nativeStatementTemp1674,nativeStatementTemp1675);}());
([&]()->JSValue{auto nativeStatementTemp1678=normalAttribute;const auto nativeStatementTemp1679=JSValue("needsUpdate");const auto nativeStatementTemp1680=JSValue(true);setReference(nativeStatementTemp1678,nativeStatementTemp1679,nativeStatementTemp1680);return nativeStatementTemp1680;}());

}
return JSValue{};
}

JSValue method26(const JSValue& nativeStatementTemp1683,const Args& nativeStatementTemp1684,const Args& nativeStatementTemp1685){
auto nativeStatementTemp1686=nativeStatementTemp1685[0];auto nativeStatementTemp1687=nativeStatementTemp1683;
JSValue normals=([&]()->JSValue{const auto nativeStatementTemp1689=([&]()->JSValue{const auto nativeStatementTemp1691=nativeStatementTemp1687;const auto nativeStatementTemp1692=JSValue("attributes");return nativeStatementTemp1691.get(nativeStatementTemp1692);}());const auto nativeStatementTemp1690=JSValue("normal");return nativeStatementTemp1689.get(nativeStatementTemp1690);}());
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp1693=normals;const auto nativeStatementTemp1694=JSValue("count");return nativeStatementTemp1693.get(nativeStatementTemp1694);}());
for(;([&]()->JSValue{const auto nativeStatementTemp1695=i;const auto nativeStatementTemp1696=il;return binary("<",nativeStatementTemp1695,nativeStatementTemp1696);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1697=i.toNumber();const auto nativeStatementTemp1698=nativeStatementTemp1697+1;i=nativeStatementTemp1698;return nativeStatementTemp1697;}())){
([&]()->JSValue{auto nativeStatementTemp1702=nativeStatementTemp1686.get("_vector");const auto nativeStatementTemp1703=JSValue("fromBufferAttribute");const auto nativeStatementTemp1699=nativeStatementTemp1702.get(nativeStatementTemp1703);const auto nativeStatementTemp1700=nativeStatementTemp1702;Args nativeStatementTemp1701;nativeStatementTemp1701.push_back(normals);nativeStatementTemp1701.push_back(i);return nativeStatementTemp1699.call(nativeStatementTemp1700,nativeStatementTemp1701);}());
([&]()->JSValue{auto nativeStatementTemp1707=nativeStatementTemp1686.get("_vector");const auto nativeStatementTemp1708=JSValue("normalize");const auto nativeStatementTemp1704=nativeStatementTemp1707.get(nativeStatementTemp1708);const auto nativeStatementTemp1705=nativeStatementTemp1707;Args nativeStatementTemp1706;return nativeStatementTemp1704.call(nativeStatementTemp1705,nativeStatementTemp1706);}());
([&]()->JSValue{auto nativeStatementTemp1712=normals;const auto nativeStatementTemp1713=JSValue("setXYZ");const auto nativeStatementTemp1709=nativeStatementTemp1712.get(nativeStatementTemp1713);const auto nativeStatementTemp1710=nativeStatementTemp1712;Args nativeStatementTemp1711;nativeStatementTemp1711.push_back(i);nativeStatementTemp1711.push_back(([&]()->JSValue{const auto nativeStatementTemp1714=nativeStatementTemp1686.get("_vector");const auto nativeStatementTemp1715=JSValue("x");return nativeStatementTemp1714.get(nativeStatementTemp1715);}()));nativeStatementTemp1711.push_back(([&]()->JSValue{const auto nativeStatementTemp1716=nativeStatementTemp1686.get("_vector");const auto nativeStatementTemp1717=JSValue("y");return nativeStatementTemp1716.get(nativeStatementTemp1717);}()));nativeStatementTemp1711.push_back(([&]()->JSValue{const auto nativeStatementTemp1718=nativeStatementTemp1686.get("_vector");const auto nativeStatementTemp1719=JSValue("z");return nativeStatementTemp1718.get(nativeStatementTemp1719);}()));return nativeStatementTemp1709.call(nativeStatementTemp1710,nativeStatementTemp1711);}());

}
}
return JSValue{};
}

JSValue method27(const JSValue& nativeStatementTemp1720,const Args& nativeStatementTemp1721,const Args& nativeStatementTemp1722){
auto nativeStatementTemp1723=nativeStatementTemp1722[0];auto nativeStatementTemp1724=nativeStatementTemp1720;
JSValue convertBufferAttribute=([&]()->JSValue{auto nativeStatementTemp1793=JSValue::constructor(nested2,{nativeStatementTemp1723},"convertBufferAttribute",2,JSValue::ConstructorKind::Callable);nativeStatementTemp1793.setFunctionSourceText("function convertBufferAttribute( attribute, indices ) {\r\n\r\n\t\t\tconst array = attribute.array;\r\n\t\t\tconst itemSize = attribute.itemSize;\r\n\t\t\tconst normalized = attribute.normalized;\r\n\r\n\t\t\tconst array2 = new array.constructor( indices.length * itemSize );\r\n\r\n\t\t\tlet index = 0, index2 = 0;\r\n\r\n\t\t\tfor ( let i = 0, l = indices.length; i < l; i ++ ) {\r\n\r\n\t\t\t\tif ( attribute.isInterleavedBufferAttribute ) {\r\n\r\n\t\t\t\t\tindex = indices[ i ] * attribute.data.stride + attribute.offset;\r\n\r\n\t\t\t\t} else {\r\n\r\n\t\t\t\t\tindex = indices[ i ] * itemSize;\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t\tfor ( let j = 0; j < itemSize; j ++ ) {\r\n\r\n\t\t\t\t\tarray2[ index2 ++ ] = array[ index ++ ];\r\n\r\n\t\t\t\t}\r\n\r\n\t\t\t}\r\n\r\n\t\t\treturn new BufferAttribute( array2, itemSize, normalized );\r\n\r\n\t\t}");return nativeStatementTemp1793;}());
if(([&]()->JSValue{const auto nativeStatementTemp1794=([&]()->JSValue{const auto nativeStatementTemp1796=nativeStatementTemp1724;const auto nativeStatementTemp1797=JSValue("index");return nativeStatementTemp1796.get(nativeStatementTemp1797);}());const auto nativeStatementTemp1795=JSValue(nullptr);return binary("===",nativeStatementTemp1794,nativeStatementTemp1795);}()).toBoolean()){
([&]()->JSValue{const auto nativeStatementTemp1798=nativeStatementTemp1723.get("warn");const JSValue nativeStatementTemp1799;Args nativeStatementTemp1800;nativeStatementTemp1800.push_back(JSValue("BufferGeometry.toNonIndexed(): BufferGeometry is already non-indexed."));return nativeStatementTemp1798.call(nativeStatementTemp1799,nativeStatementTemp1800);}());
return nativeStatementTemp1724;

}
JSValue geometry2=([&]()->JSValue{const auto nativeStatementTemp1801=nativeStatementTemp1723.get("BufferGeometry");Args nativeStatementTemp1802;return nativeStatementTemp1801.construct(nativeStatementTemp1802);}());
JSValue indices=([&]()->JSValue{const auto nativeStatementTemp1803=([&]()->JSValue{const auto nativeStatementTemp1805=nativeStatementTemp1724;const auto nativeStatementTemp1806=JSValue("index");return nativeStatementTemp1805.get(nativeStatementTemp1806);}());const auto nativeStatementTemp1804=JSValue("array");return nativeStatementTemp1803.get(nativeStatementTemp1804);}());
JSValue attributes=([&]()->JSValue{const auto nativeStatementTemp1807=nativeStatementTemp1724;const auto nativeStatementTemp1808=JSValue("attributes");return nativeStatementTemp1807.get(nativeStatementTemp1808);}());
{ForInCursor nativeStatementTemp1809(attributes);while(const auto nativeStatementTemp1810=nativeStatementTemp1809.next()){const auto name=*nativeStatementTemp1810;{
JSValue attribute=([&]()->JSValue{const auto nativeStatementTemp1811=attributes;const auto nativeStatementTemp1812=name;return nativeStatementTemp1811.get(nativeStatementTemp1812);}());
JSValue newAttribute=([&]()->JSValue{const auto nativeStatementTemp1813=convertBufferAttribute;const JSValue nativeStatementTemp1814;Args nativeStatementTemp1815;nativeStatementTemp1815.push_back(attribute);nativeStatementTemp1815.push_back(indices);return nativeStatementTemp1813.call(nativeStatementTemp1814,nativeStatementTemp1815);}());
([&]()->JSValue{auto nativeStatementTemp1819=geometry2;const auto nativeStatementTemp1820=JSValue("setAttribute");const auto nativeStatementTemp1816=nativeStatementTemp1819.get(nativeStatementTemp1820);const auto nativeStatementTemp1817=nativeStatementTemp1819;Args nativeStatementTemp1818;nativeStatementTemp1818.push_back(name);nativeStatementTemp1818.push_back(newAttribute);return nativeStatementTemp1816.call(nativeStatementTemp1817,nativeStatementTemp1818);}());

}
}}
JSValue morphAttributes=([&]()->JSValue{const auto nativeStatementTemp1821=nativeStatementTemp1724;const auto nativeStatementTemp1822=JSValue("morphAttributes");return nativeStatementTemp1821.get(nativeStatementTemp1822);}());
{ForInCursor nativeStatementTemp1823(morphAttributes);while(const auto nativeStatementTemp1824=nativeStatementTemp1823.next()){const auto name=*nativeStatementTemp1824;{
JSValue morphArray=([&]()->JSValue{auto nativeStatementTemp1825=JSValue::array();return nativeStatementTemp1825;}());
JSValue morphAttribute=([&]()->JSValue{const auto nativeStatementTemp1826=morphAttributes;const auto nativeStatementTemp1827=name;return nativeStatementTemp1826.get(nativeStatementTemp1827);}());
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp1828=morphAttribute;const auto nativeStatementTemp1829=JSValue("length");return nativeStatementTemp1828.get(nativeStatementTemp1829);}());
for(;([&]()->JSValue{const auto nativeStatementTemp1830=i;const auto nativeStatementTemp1831=il;return binary("<",nativeStatementTemp1830,nativeStatementTemp1831);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1832=i.toNumber();const auto nativeStatementTemp1833=nativeStatementTemp1832+1;i=nativeStatementTemp1833;return nativeStatementTemp1832;}())){
JSValue attribute=([&]()->JSValue{const auto nativeStatementTemp1834=morphAttribute;const auto nativeStatementTemp1835=i;return nativeStatementTemp1834.get(nativeStatementTemp1835);}());
JSValue newAttribute=([&]()->JSValue{const auto nativeStatementTemp1836=convertBufferAttribute;const JSValue nativeStatementTemp1837;Args nativeStatementTemp1838;nativeStatementTemp1838.push_back(attribute);nativeStatementTemp1838.push_back(indices);return nativeStatementTemp1836.call(nativeStatementTemp1837,nativeStatementTemp1838);}());
([&]()->JSValue{auto nativeStatementTemp1842=morphArray;const auto nativeStatementTemp1843=JSValue("push");const auto nativeStatementTemp1839=nativeStatementTemp1842.get(nativeStatementTemp1843);const auto nativeStatementTemp1840=nativeStatementTemp1842;Args nativeStatementTemp1841;nativeStatementTemp1841.push_back(newAttribute);return nativeStatementTemp1839.call(nativeStatementTemp1840,nativeStatementTemp1841);}());

}
}
([&]()->JSValue{auto nativeStatementTemp1844=([&]()->JSValue{const auto nativeStatementTemp1846=geometry2;const auto nativeStatementTemp1847=JSValue("morphAttributes");return nativeStatementTemp1846.get(nativeStatementTemp1847);}());const auto nativeStatementTemp1845=name;const auto nativeStatementTemp1848=morphArray;setReference(nativeStatementTemp1844,nativeStatementTemp1845,nativeStatementTemp1848);return nativeStatementTemp1848;}());

}
}}
([&]()->JSValue{auto nativeStatementTemp1851=geometry2;const auto nativeStatementTemp1852=JSValue("morphTargetsRelative");const auto nativeStatementTemp1853=([&]()->JSValue{const auto nativeStatementTemp1856=nativeStatementTemp1724;const auto nativeStatementTemp1857=JSValue("morphTargetsRelative");return nativeStatementTemp1856.get(nativeStatementTemp1857);}());setReference(nativeStatementTemp1851,nativeStatementTemp1852,nativeStatementTemp1853);return nativeStatementTemp1853;}());
JSValue groups=([&]()->JSValue{const auto nativeStatementTemp1858=nativeStatementTemp1724;const auto nativeStatementTemp1859=JSValue("groups");return nativeStatementTemp1858.get(nativeStatementTemp1859);}());
{JSValue i=JSValue(0);
JSValue l=([&]()->JSValue{const auto nativeStatementTemp1860=groups;const auto nativeStatementTemp1861=JSValue("length");return nativeStatementTemp1860.get(nativeStatementTemp1861);}());
for(;([&]()->JSValue{const auto nativeStatementTemp1862=i;const auto nativeStatementTemp1863=l;return binary("<",nativeStatementTemp1862,nativeStatementTemp1863);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp1864=i.toNumber();const auto nativeStatementTemp1865=nativeStatementTemp1864+1;i=nativeStatementTemp1865;return nativeStatementTemp1864;}())){
JSValue group=([&]()->JSValue{const auto nativeStatementTemp1866=groups;const auto nativeStatementTemp1867=i;return nativeStatementTemp1866.get(nativeStatementTemp1867);}());
([&]()->JSValue{auto nativeStatementTemp1871=geometry2;const auto nativeStatementTemp1872=JSValue("addGroup");const auto nativeStatementTemp1868=nativeStatementTemp1871.get(nativeStatementTemp1872);const auto nativeStatementTemp1869=nativeStatementTemp1871;Args nativeStatementTemp1870;nativeStatementTemp1870.push_back(([&]()->JSValue{const auto nativeStatementTemp1873=group;const auto nativeStatementTemp1874=JSValue("start");return nativeStatementTemp1873.get(nativeStatementTemp1874);}()));nativeStatementTemp1870.push_back(([&]()->JSValue{const auto nativeStatementTemp1875=group;const auto nativeStatementTemp1876=JSValue("count");return nativeStatementTemp1875.get(nativeStatementTemp1876);}()));nativeStatementTemp1870.push_back(([&]()->JSValue{const auto nativeStatementTemp1877=group;const auto nativeStatementTemp1878=JSValue("materialIndex");return nativeStatementTemp1877.get(nativeStatementTemp1878);}()));return nativeStatementTemp1868.call(nativeStatementTemp1869,nativeStatementTemp1870);}());

}
}
return geometry2;
return JSValue{};
}

JSValue method28(const JSValue& nativeStatementTemp1879,const Args& nativeStatementTemp1880,const Args& nativeStatementTemp1881){
auto nativeStatementTemp1882=nativeStatementTemp1881[0];auto nativeStatementTemp1883=nativeStatementTemp1879;
JSValue data=([&]()->JSValue{auto nativeStatementTemp1885=JSValue::object();{const auto nativeStatementTemp1886=JSValue("metadata");const auto nativeStatementTemp1887=([&]()->JSValue{auto nativeStatementTemp1888=JSValue::object();{const auto nativeStatementTemp1889=JSValue("version");const auto nativeStatementTemp1890=JSValue(4.7);own(nativeStatementTemp1888,nativeStatementTemp1889,nativeStatementTemp1890);}{const auto nativeStatementTemp1891=JSValue("type");const auto nativeStatementTemp1892=JSValue("BufferGeometry");own(nativeStatementTemp1888,nativeStatementTemp1891,nativeStatementTemp1892);}{const auto nativeStatementTemp1893=JSValue("generator");const auto nativeStatementTemp1894=JSValue("BufferGeometry.toJSON");own(nativeStatementTemp1888,nativeStatementTemp1893,nativeStatementTemp1894);}return nativeStatementTemp1888;}());own(nativeStatementTemp1885,nativeStatementTemp1886,nativeStatementTemp1887);}return nativeStatementTemp1885;}());
([&]()->JSValue{auto nativeStatementTemp1895=data;const auto nativeStatementTemp1896=JSValue("uuid");const auto nativeStatementTemp1897=([&]()->JSValue{const auto nativeStatementTemp1900=nativeStatementTemp1883;const auto nativeStatementTemp1901=JSValue("uuid");return nativeStatementTemp1900.get(nativeStatementTemp1901);}());setReference(nativeStatementTemp1895,nativeStatementTemp1896,nativeStatementTemp1897);return nativeStatementTemp1897;}());
([&]()->JSValue{auto nativeStatementTemp1902=data;const auto nativeStatementTemp1903=JSValue("type");const auto nativeStatementTemp1904=([&]()->JSValue{if(([&]()->JSValue{const auto nativeStatementTemp1907=([&]()->JSValue{const auto nativeStatementTemp1908=([&]()->JSValue{const auto nativeStatementTemp1910=nativeStatementTemp1883;const auto nativeStatementTemp1911=JSValue("parameters");return nativeStatementTemp1910.get(nativeStatementTemp1911);}());const auto nativeStatementTemp1909=JSValue{};return binary("!==",nativeStatementTemp1908,nativeStatementTemp1909);}());if(!nativeStatementTemp1907.toBoolean())return nativeStatementTemp1907;return ([&]()->JSValue{const auto nativeStatementTemp1912=([&]()->JSValue{const auto nativeStatementTemp1914=nativeStatementTemp1883;const auto nativeStatementTemp1915=JSValue("_transformed");return nativeStatementTemp1914.get(nativeStatementTemp1915);}());const auto nativeStatementTemp1913=JSValue(true);return binary("===",nativeStatementTemp1912,nativeStatementTemp1913);}());}()).toBoolean())return JSValue("BufferGeometry");return ([&]()->JSValue{const auto nativeStatementTemp1916=nativeStatementTemp1883;const auto nativeStatementTemp1917=JSValue("type");return nativeStatementTemp1916.get(nativeStatementTemp1917);}());}());setReference(nativeStatementTemp1902,nativeStatementTemp1903,nativeStatementTemp1904);return nativeStatementTemp1904;}());
([&]()->JSValue{auto nativeStatementTemp1918=data;const auto nativeStatementTemp1919=JSValue("name");const auto nativeStatementTemp1920=([&]()->JSValue{const auto nativeStatementTemp1923=nativeStatementTemp1883;const auto nativeStatementTemp1924=JSValue("name");return nativeStatementTemp1923.get(nativeStatementTemp1924);}());setReference(nativeStatementTemp1918,nativeStatementTemp1919,nativeStatementTemp1920);return nativeStatementTemp1920;}());
if(([&]()->JSValue{const auto nativeStatementTemp1925=([&]()->JSValue{const auto nativeStatementTemp1927=([&]()->JSValue{auto nativeStatementTemp1932=nativeGlobalBinding("Object");const auto nativeStatementTemp1933=JSValue("keys");const auto nativeStatementTemp1929=nativeStatementTemp1932.get(nativeStatementTemp1933);const auto nativeStatementTemp1930=nativeStatementTemp1932;Args nativeStatementTemp1931;nativeStatementTemp1931.push_back(([&]()->JSValue{const auto nativeStatementTemp1934=nativeStatementTemp1883;const auto nativeStatementTemp1935=JSValue("userData");return nativeStatementTemp1934.get(nativeStatementTemp1935);}()));return nativeStatementTemp1929.call(nativeStatementTemp1930,nativeStatementTemp1931);}());const auto nativeStatementTemp1928=JSValue("length");return nativeStatementTemp1927.get(nativeStatementTemp1928);}());const auto nativeStatementTemp1926=JSValue(0);return binary(">",nativeStatementTemp1925,nativeStatementTemp1926);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp1936=data;const auto nativeStatementTemp1937=JSValue("userData");const auto nativeStatementTemp1938=([&]()->JSValue{const auto nativeStatementTemp1941=nativeStatementTemp1883;const auto nativeStatementTemp1942=JSValue("userData");return nativeStatementTemp1941.get(nativeStatementTemp1942);}());setReference(nativeStatementTemp1936,nativeStatementTemp1937,nativeStatementTemp1938);return nativeStatementTemp1938;}());
if(([&]()->JSValue{const auto nativeStatementTemp1943=([&]()->JSValue{const auto nativeStatementTemp1944=([&]()->JSValue{const auto nativeStatementTemp1946=nativeStatementTemp1883;const auto nativeStatementTemp1947=JSValue("parameters");return nativeStatementTemp1946.get(nativeStatementTemp1947);}());const auto nativeStatementTemp1945=JSValue{};return binary("!==",nativeStatementTemp1944,nativeStatementTemp1945);}());if(!nativeStatementTemp1943.toBoolean())return nativeStatementTemp1943;return ([&]()->JSValue{const auto nativeStatementTemp1948=([&]()->JSValue{const auto nativeStatementTemp1950=nativeStatementTemp1883;const auto nativeStatementTemp1951=JSValue("_transformed");return nativeStatementTemp1950.get(nativeStatementTemp1951);}());const auto nativeStatementTemp1949=JSValue(true);return binary("!==",nativeStatementTemp1948,nativeStatementTemp1949);}());}()).toBoolean()){
JSValue parameters=([&]()->JSValue{const auto nativeStatementTemp1952=nativeStatementTemp1883;const auto nativeStatementTemp1953=JSValue("parameters");return nativeStatementTemp1952.get(nativeStatementTemp1953);}());
{ForInCursor nativeStatementTemp1954(parameters);while(const auto nativeStatementTemp1955=nativeStatementTemp1954.next()){const auto key=*nativeStatementTemp1955;{
if(([&]()->JSValue{const auto nativeStatementTemp1956=([&]()->JSValue{const auto nativeStatementTemp1958=parameters;const auto nativeStatementTemp1959=key;return nativeStatementTemp1958.get(nativeStatementTemp1959);}());const auto nativeStatementTemp1957=JSValue{};return binary("!==",nativeStatementTemp1956,nativeStatementTemp1957);}()).toBoolean())([&]()->JSValue{auto nativeStatementTemp1960=data;const auto nativeStatementTemp1961=key;const auto nativeStatementTemp1962=([&]()->JSValue{const auto nativeStatementTemp1965=parameters;const auto nativeStatementTemp1966=key;return nativeStatementTemp1965.get(nativeStatementTemp1966);}());setReference(nativeStatementTemp1960,nativeStatementTemp1961,nativeStatementTemp1962);return nativeStatementTemp1962;}());

}
}}
return data;

}
([&]()->JSValue{auto nativeStatementTemp1967=data;const auto nativeStatementTemp1968=JSValue("data");const auto nativeStatementTemp1969=([&]()->JSValue{auto nativeStatementTemp1972=JSValue::object();{const auto nativeStatementTemp1973=JSValue("attributes");const auto nativeStatementTemp1974=([&]()->JSValue{auto nativeStatementTemp1975=JSValue::object();return nativeStatementTemp1975;}());own(nativeStatementTemp1972,nativeStatementTemp1973,nativeStatementTemp1974);}return nativeStatementTemp1972;}());setReference(nativeStatementTemp1967,nativeStatementTemp1968,nativeStatementTemp1969);return nativeStatementTemp1969;}());
JSValue index=([&]()->JSValue{const auto nativeStatementTemp1976=nativeStatementTemp1883;const auto nativeStatementTemp1977=JSValue("index");return nativeStatementTemp1976.get(nativeStatementTemp1977);}());
if(([&]()->JSValue{const auto nativeStatementTemp1978=index;const auto nativeStatementTemp1979=JSValue(nullptr);return binary("!==",nativeStatementTemp1978,nativeStatementTemp1979);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp1980=([&]()->JSValue{const auto nativeStatementTemp1982=data;const auto nativeStatementTemp1983=JSValue("data");return nativeStatementTemp1982.get(nativeStatementTemp1983);}());const auto nativeStatementTemp1981=JSValue("index");const auto nativeStatementTemp1984=([&]()->JSValue{auto nativeStatementTemp1987=JSValue::object();{const auto nativeStatementTemp1988=JSValue("type");const auto nativeStatementTemp1989=([&]()->JSValue{const auto nativeStatementTemp1990=([&]()->JSValue{const auto nativeStatementTemp1992=([&]()->JSValue{const auto nativeStatementTemp1994=index;const auto nativeStatementTemp1995=JSValue("array");return nativeStatementTemp1994.get(nativeStatementTemp1995);}());const auto nativeStatementTemp1993=JSValue("constructor");return nativeStatementTemp1992.get(nativeStatementTemp1993);}());const auto nativeStatementTemp1991=JSValue("name");return nativeStatementTemp1990.get(nativeStatementTemp1991);}());own(nativeStatementTemp1987,nativeStatementTemp1988,nativeStatementTemp1989);}{const auto nativeStatementTemp1996=JSValue("array");const auto nativeStatementTemp1997=([&]()->JSValue{auto nativeStatementTemp2001=([&]()->JSValue{const auto nativeStatementTemp2003=([&]()->JSValue{const auto nativeStatementTemp2005=nativeGlobalBinding("Array");const auto nativeStatementTemp2006=JSValue("prototype");return nativeStatementTemp2005.get(nativeStatementTemp2006);}());const auto nativeStatementTemp2004=JSValue("slice");return nativeStatementTemp2003.get(nativeStatementTemp2004);}());const auto nativeStatementTemp2002=JSValue("call");const auto nativeStatementTemp1998=nativeStatementTemp2001.get(nativeStatementTemp2002);const auto nativeStatementTemp1999=nativeStatementTemp2001;Args nativeStatementTemp2000;nativeStatementTemp2000.push_back(([&]()->JSValue{const auto nativeStatementTemp2007=index;const auto nativeStatementTemp2008=JSValue("array");return nativeStatementTemp2007.get(nativeStatementTemp2008);}()));return nativeStatementTemp1998.call(nativeStatementTemp1999,nativeStatementTemp2000);}());own(nativeStatementTemp1987,nativeStatementTemp1996,nativeStatementTemp1997);}return nativeStatementTemp1987;}());setReference(nativeStatementTemp1980,nativeStatementTemp1981,nativeStatementTemp1984);return nativeStatementTemp1984;}());

}
JSValue attributes=([&]()->JSValue{const auto nativeStatementTemp2009=nativeStatementTemp1883;const auto nativeStatementTemp2010=JSValue("attributes");return nativeStatementTemp2009.get(nativeStatementTemp2010);}());
{ForInCursor nativeStatementTemp2011(attributes);while(const auto nativeStatementTemp2012=nativeStatementTemp2011.next()){const auto key=*nativeStatementTemp2012;{
JSValue attribute=([&]()->JSValue{const auto nativeStatementTemp2013=attributes;const auto nativeStatementTemp2014=key;return nativeStatementTemp2013.get(nativeStatementTemp2014);}());
([&]()->JSValue{auto nativeStatementTemp2015=([&]()->JSValue{const auto nativeStatementTemp2017=([&]()->JSValue{const auto nativeStatementTemp2019=data;const auto nativeStatementTemp2020=JSValue("data");return nativeStatementTemp2019.get(nativeStatementTemp2020);}());const auto nativeStatementTemp2018=JSValue("attributes");return nativeStatementTemp2017.get(nativeStatementTemp2018);}());const auto nativeStatementTemp2016=key;const auto nativeStatementTemp2021=([&]()->JSValue{auto nativeStatementTemp2027=attribute;const auto nativeStatementTemp2028=JSValue("toJSON");const auto nativeStatementTemp2024=nativeStatementTemp2027.get(nativeStatementTemp2028);const auto nativeStatementTemp2025=nativeStatementTemp2027;Args nativeStatementTemp2026;nativeStatementTemp2026.push_back(([&]()->JSValue{const auto nativeStatementTemp2029=data;const auto nativeStatementTemp2030=JSValue("data");return nativeStatementTemp2029.get(nativeStatementTemp2030);}()));return nativeStatementTemp2024.call(nativeStatementTemp2025,nativeStatementTemp2026);}());setReference(nativeStatementTemp2015,nativeStatementTemp2016,nativeStatementTemp2021);return nativeStatementTemp2021;}());

}
}}
JSValue morphAttributes=([&]()->JSValue{auto nativeStatementTemp2031=JSValue::object();return nativeStatementTemp2031;}());
JSValue hasMorphAttributes=JSValue(false);
{ForInCursor nativeStatementTemp2032(([&]()->JSValue{const auto nativeStatementTemp2034=nativeStatementTemp1883;const auto nativeStatementTemp2035=JSValue("morphAttributes");return nativeStatementTemp2034.get(nativeStatementTemp2035);}()));while(const auto nativeStatementTemp2033=nativeStatementTemp2032.next()){const auto key=*nativeStatementTemp2033;{
JSValue attributeArray=([&]()->JSValue{const auto nativeStatementTemp2036=([&]()->JSValue{const auto nativeStatementTemp2038=nativeStatementTemp1883;const auto nativeStatementTemp2039=JSValue("morphAttributes");return nativeStatementTemp2038.get(nativeStatementTemp2039);}());const auto nativeStatementTemp2037=key;return nativeStatementTemp2036.get(nativeStatementTemp2037);}());
JSValue array=([&]()->JSValue{auto nativeStatementTemp2040=JSValue::array();return nativeStatementTemp2040;}());
{JSValue i=JSValue(0);
JSValue il=([&]()->JSValue{const auto nativeStatementTemp2041=attributeArray;const auto nativeStatementTemp2042=JSValue("length");return nativeStatementTemp2041.get(nativeStatementTemp2042);}());
for(;([&]()->JSValue{const auto nativeStatementTemp2043=i;const auto nativeStatementTemp2044=il;return binary("<",nativeStatementTemp2043,nativeStatementTemp2044);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp2045=i.toNumber();const auto nativeStatementTemp2046=nativeStatementTemp2045+1;i=nativeStatementTemp2046;return nativeStatementTemp2045;}())){
JSValue attribute=([&]()->JSValue{const auto nativeStatementTemp2047=attributeArray;const auto nativeStatementTemp2048=i;return nativeStatementTemp2047.get(nativeStatementTemp2048);}());
([&]()->JSValue{auto nativeStatementTemp2052=array;const auto nativeStatementTemp2053=JSValue("push");const auto nativeStatementTemp2049=nativeStatementTemp2052.get(nativeStatementTemp2053);const auto nativeStatementTemp2050=nativeStatementTemp2052;Args nativeStatementTemp2051;nativeStatementTemp2051.push_back(([&]()->JSValue{auto nativeStatementTemp2057=attribute;const auto nativeStatementTemp2058=JSValue("toJSON");const auto nativeStatementTemp2054=nativeStatementTemp2057.get(nativeStatementTemp2058);const auto nativeStatementTemp2055=nativeStatementTemp2057;Args nativeStatementTemp2056;nativeStatementTemp2056.push_back(([&]()->JSValue{const auto nativeStatementTemp2059=data;const auto nativeStatementTemp2060=JSValue("data");return nativeStatementTemp2059.get(nativeStatementTemp2060);}()));return nativeStatementTemp2054.call(nativeStatementTemp2055,nativeStatementTemp2056);}()));return nativeStatementTemp2049.call(nativeStatementTemp2050,nativeStatementTemp2051);}());

}
}
if(([&]()->JSValue{const auto nativeStatementTemp2061=([&]()->JSValue{const auto nativeStatementTemp2063=array;const auto nativeStatementTemp2064=JSValue("length");return nativeStatementTemp2063.get(nativeStatementTemp2064);}());const auto nativeStatementTemp2062=JSValue(0);return binary(">",nativeStatementTemp2061,nativeStatementTemp2062);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp2065=morphAttributes;const auto nativeStatementTemp2066=key;const auto nativeStatementTemp2067=array;setReference(nativeStatementTemp2065,nativeStatementTemp2066,nativeStatementTemp2067);return nativeStatementTemp2067;}());
([&]()->JSValue{const auto nativeStatementTemp2070=JSValue(true);hasMorphAttributes=nativeStatementTemp2070;return nativeStatementTemp2070;}());

}

}
}}
if(hasMorphAttributes.toBoolean()){
([&]()->JSValue{auto nativeStatementTemp2073=([&]()->JSValue{const auto nativeStatementTemp2075=data;const auto nativeStatementTemp2076=JSValue("data");return nativeStatementTemp2075.get(nativeStatementTemp2076);}());const auto nativeStatementTemp2074=JSValue("morphAttributes");const auto nativeStatementTemp2077=morphAttributes;setReference(nativeStatementTemp2073,nativeStatementTemp2074,nativeStatementTemp2077);return nativeStatementTemp2077;}());
([&]()->JSValue{auto nativeStatementTemp2080=([&]()->JSValue{const auto nativeStatementTemp2082=data;const auto nativeStatementTemp2083=JSValue("data");return nativeStatementTemp2082.get(nativeStatementTemp2083);}());const auto nativeStatementTemp2081=JSValue("morphTargetsRelative");const auto nativeStatementTemp2084=([&]()->JSValue{const auto nativeStatementTemp2087=nativeStatementTemp1883;const auto nativeStatementTemp2088=JSValue("morphTargetsRelative");return nativeStatementTemp2087.get(nativeStatementTemp2088);}());setReference(nativeStatementTemp2080,nativeStatementTemp2081,nativeStatementTemp2084);return nativeStatementTemp2084;}());

}
JSValue groups=([&]()->JSValue{const auto nativeStatementTemp2089=nativeStatementTemp1883;const auto nativeStatementTemp2090=JSValue("groups");return nativeStatementTemp2089.get(nativeStatementTemp2090);}());
if(([&]()->JSValue{const auto nativeStatementTemp2091=([&]()->JSValue{const auto nativeStatementTemp2093=groups;const auto nativeStatementTemp2094=JSValue("length");return nativeStatementTemp2093.get(nativeStatementTemp2094);}());const auto nativeStatementTemp2092=JSValue(0);return binary(">",nativeStatementTemp2091,nativeStatementTemp2092);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp2095=([&]()->JSValue{const auto nativeStatementTemp2097=data;const auto nativeStatementTemp2098=JSValue("data");return nativeStatementTemp2097.get(nativeStatementTemp2098);}());const auto nativeStatementTemp2096=JSValue("groups");const auto nativeStatementTemp2099=([&]()->JSValue{auto nativeStatementTemp2105=nativeGlobalBinding("JSON");const auto nativeStatementTemp2106=JSValue("parse");const auto nativeStatementTemp2102=nativeStatementTemp2105.get(nativeStatementTemp2106);const auto nativeStatementTemp2103=nativeStatementTemp2105;Args nativeStatementTemp2104;nativeStatementTemp2104.push_back(([&]()->JSValue{auto nativeStatementTemp2110=nativeGlobalBinding("JSON");const auto nativeStatementTemp2111=JSValue("stringify");const auto nativeStatementTemp2107=nativeStatementTemp2110.get(nativeStatementTemp2111);const auto nativeStatementTemp2108=nativeStatementTemp2110;Args nativeStatementTemp2109;nativeStatementTemp2109.push_back(groups);return nativeStatementTemp2107.call(nativeStatementTemp2108,nativeStatementTemp2109);}()));return nativeStatementTemp2102.call(nativeStatementTemp2103,nativeStatementTemp2104);}());setReference(nativeStatementTemp2095,nativeStatementTemp2096,nativeStatementTemp2099);return nativeStatementTemp2099;}());

}
JSValue boundingSphere=([&]()->JSValue{const auto nativeStatementTemp2112=nativeStatementTemp1883;const auto nativeStatementTemp2113=JSValue("boundingSphere");return nativeStatementTemp2112.get(nativeStatementTemp2113);}());
if(([&]()->JSValue{const auto nativeStatementTemp2114=boundingSphere;const auto nativeStatementTemp2115=JSValue(nullptr);return binary("!==",nativeStatementTemp2114,nativeStatementTemp2115);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp2116=([&]()->JSValue{const auto nativeStatementTemp2118=data;const auto nativeStatementTemp2119=JSValue("data");return nativeStatementTemp2118.get(nativeStatementTemp2119);}());const auto nativeStatementTemp2117=JSValue("boundingSphere");const auto nativeStatementTemp2120=([&]()->JSValue{auto nativeStatementTemp2126=boundingSphere;const auto nativeStatementTemp2127=JSValue("toJSON");const auto nativeStatementTemp2123=nativeStatementTemp2126.get(nativeStatementTemp2127);const auto nativeStatementTemp2124=nativeStatementTemp2126;Args nativeStatementTemp2125;return nativeStatementTemp2123.call(nativeStatementTemp2124,nativeStatementTemp2125);}());setReference(nativeStatementTemp2116,nativeStatementTemp2117,nativeStatementTemp2120);return nativeStatementTemp2120;}());

}
return data;
return JSValue{};
}

JSValue method29(const JSValue& nativeStatementTemp2128,const Args& nativeStatementTemp2129,const Args& nativeStatementTemp2130){
auto nativeStatementTemp2131=nativeStatementTemp2130[0];auto nativeStatementTemp2132=nativeStatementTemp2128;
return ([&]()->JSValue{auto nativeStatementTemp2137=([&]()->JSValue{const auto nativeStatementTemp2139=([&]()->JSValue{const auto nativeStatementTemp2141=nativeStatementTemp2132;const auto nativeStatementTemp2142=JSValue("constructor");return nativeStatementTemp2141.get(nativeStatementTemp2142);}());Args nativeStatementTemp2140;return nativeStatementTemp2139.construct(nativeStatementTemp2140);}());const auto nativeStatementTemp2138=JSValue("copy");const auto nativeStatementTemp2134=nativeStatementTemp2137.get(nativeStatementTemp2138);const auto nativeStatementTemp2135=nativeStatementTemp2137;Args nativeStatementTemp2136;nativeStatementTemp2136.push_back(nativeStatementTemp2132);return nativeStatementTemp2134.call(nativeStatementTemp2135,nativeStatementTemp2136);}());
return JSValue{};
}

JSValue method30(const JSValue& nativeStatementTemp2143,const Args& nativeStatementTemp2144,const Args& nativeStatementTemp2145){
auto nativeStatementTemp2146=nativeStatementTemp2145[0];auto nativeStatementTemp2147=nativeStatementTemp2143;
JSValue source=arg(nativeStatementTemp2144,0);
([&]()->JSValue{auto nativeStatementTemp2149=nativeStatementTemp2147;const auto nativeStatementTemp2150=JSValue("index");const auto nativeStatementTemp2151=JSValue(nullptr);setReference(nativeStatementTemp2149,nativeStatementTemp2150,nativeStatementTemp2151);return nativeStatementTemp2151;}());
([&]()->JSValue{auto nativeStatementTemp2154=nativeStatementTemp2147;const auto nativeStatementTemp2155=JSValue("attributes");const auto nativeStatementTemp2156=([&]()->JSValue{auto nativeStatementTemp2159=JSValue::object();return nativeStatementTemp2159;}());setReference(nativeStatementTemp2154,nativeStatementTemp2155,nativeStatementTemp2156);return nativeStatementTemp2156;}());
([&]()->JSValue{auto nativeStatementTemp2160=nativeStatementTemp2147;const auto nativeStatementTemp2161=JSValue("morphAttributes");const auto nativeStatementTemp2162=([&]()->JSValue{auto nativeStatementTemp2165=JSValue::object();return nativeStatementTemp2165;}());setReference(nativeStatementTemp2160,nativeStatementTemp2161,nativeStatementTemp2162);return nativeStatementTemp2162;}());
([&]()->JSValue{auto nativeStatementTemp2166=nativeStatementTemp2147;const auto nativeStatementTemp2167=JSValue("groups");const auto nativeStatementTemp2168=([&]()->JSValue{auto nativeStatementTemp2171=JSValue::array();return nativeStatementTemp2171;}());setReference(nativeStatementTemp2166,nativeStatementTemp2167,nativeStatementTemp2168);return nativeStatementTemp2168;}());
([&]()->JSValue{auto nativeStatementTemp2172=nativeStatementTemp2147;const auto nativeStatementTemp2173=JSValue("boundingBox");const auto nativeStatementTemp2174=JSValue(nullptr);setReference(nativeStatementTemp2172,nativeStatementTemp2173,nativeStatementTemp2174);return nativeStatementTemp2174;}());
([&]()->JSValue{auto nativeStatementTemp2177=nativeStatementTemp2147;const auto nativeStatementTemp2178=JSValue("boundingSphere");const auto nativeStatementTemp2179=JSValue(nullptr);setReference(nativeStatementTemp2177,nativeStatementTemp2178,nativeStatementTemp2179);return nativeStatementTemp2179;}());
JSValue data=([&]()->JSValue{auto nativeStatementTemp2182=JSValue::object();return nativeStatementTemp2182;}());
([&]()->JSValue{auto nativeStatementTemp2183=nativeStatementTemp2147;const auto nativeStatementTemp2184=JSValue("name");const auto nativeStatementTemp2185=([&]()->JSValue{const auto nativeStatementTemp2188=source;const auto nativeStatementTemp2189=JSValue("name");return nativeStatementTemp2188.get(nativeStatementTemp2189);}());setReference(nativeStatementTemp2183,nativeStatementTemp2184,nativeStatementTemp2185);return nativeStatementTemp2185;}());
JSValue index=([&]()->JSValue{const auto nativeStatementTemp2190=source;const auto nativeStatementTemp2191=JSValue("index");return nativeStatementTemp2190.get(nativeStatementTemp2191);}());
if(([&]()->JSValue{const auto nativeStatementTemp2192=index;const auto nativeStatementTemp2193=JSValue(nullptr);return binary("!==",nativeStatementTemp2192,nativeStatementTemp2193);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp2197=nativeStatementTemp2147;const auto nativeStatementTemp2198=JSValue("setIndex");const auto nativeStatementTemp2194=nativeStatementTemp2197.get(nativeStatementTemp2198);const auto nativeStatementTemp2195=nativeStatementTemp2197;Args nativeStatementTemp2196;nativeStatementTemp2196.push_back(([&]()->JSValue{auto nativeStatementTemp2202=index;const auto nativeStatementTemp2203=JSValue("clone");const auto nativeStatementTemp2199=nativeStatementTemp2202.get(nativeStatementTemp2203);const auto nativeStatementTemp2200=nativeStatementTemp2202;Args nativeStatementTemp2201;return nativeStatementTemp2199.call(nativeStatementTemp2200,nativeStatementTemp2201);}()));return nativeStatementTemp2194.call(nativeStatementTemp2195,nativeStatementTemp2196);}());

}
JSValue attributes=([&]()->JSValue{const auto nativeStatementTemp2204=source;const auto nativeStatementTemp2205=JSValue("attributes");return nativeStatementTemp2204.get(nativeStatementTemp2205);}());
{ForInCursor nativeStatementTemp2206(attributes);while(const auto nativeStatementTemp2207=nativeStatementTemp2206.next()){const auto name=*nativeStatementTemp2207;{
JSValue attribute=([&]()->JSValue{const auto nativeStatementTemp2208=attributes;const auto nativeStatementTemp2209=name;return nativeStatementTemp2208.get(nativeStatementTemp2209);}());
([&]()->JSValue{auto nativeStatementTemp2213=nativeStatementTemp2147;const auto nativeStatementTemp2214=JSValue("setAttribute");const auto nativeStatementTemp2210=nativeStatementTemp2213.get(nativeStatementTemp2214);const auto nativeStatementTemp2211=nativeStatementTemp2213;Args nativeStatementTemp2212;nativeStatementTemp2212.push_back(name);nativeStatementTemp2212.push_back(([&]()->JSValue{auto nativeStatementTemp2218=attribute;const auto nativeStatementTemp2219=JSValue("clone");const auto nativeStatementTemp2215=nativeStatementTemp2218.get(nativeStatementTemp2219);const auto nativeStatementTemp2216=nativeStatementTemp2218;Args nativeStatementTemp2217;nativeStatementTemp2217.push_back(data);return nativeStatementTemp2215.call(nativeStatementTemp2216,nativeStatementTemp2217);}()));return nativeStatementTemp2210.call(nativeStatementTemp2211,nativeStatementTemp2212);}());

}
}}
JSValue morphAttributes=([&]()->JSValue{const auto nativeStatementTemp2220=source;const auto nativeStatementTemp2221=JSValue("morphAttributes");return nativeStatementTemp2220.get(nativeStatementTemp2221);}());
{ForInCursor nativeStatementTemp2222(morphAttributes);while(const auto nativeStatementTemp2223=nativeStatementTemp2222.next()){const auto name=*nativeStatementTemp2223;{
JSValue array=([&]()->JSValue{auto nativeStatementTemp2224=JSValue::array();return nativeStatementTemp2224;}());
JSValue morphAttribute=([&]()->JSValue{const auto nativeStatementTemp2225=morphAttributes;const auto nativeStatementTemp2226=name;return nativeStatementTemp2225.get(nativeStatementTemp2226);}());
{JSValue i=JSValue(0);
JSValue l=([&]()->JSValue{const auto nativeStatementTemp2227=morphAttribute;const auto nativeStatementTemp2228=JSValue("length");return nativeStatementTemp2227.get(nativeStatementTemp2228);}());
for(;([&]()->JSValue{const auto nativeStatementTemp2229=i;const auto nativeStatementTemp2230=l;return binary("<",nativeStatementTemp2229,nativeStatementTemp2230);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp2231=i.toNumber();const auto nativeStatementTemp2232=nativeStatementTemp2231+1;i=nativeStatementTemp2232;return nativeStatementTemp2231;}())){
([&]()->JSValue{auto nativeStatementTemp2236=array;const auto nativeStatementTemp2237=JSValue("push");const auto nativeStatementTemp2233=nativeStatementTemp2236.get(nativeStatementTemp2237);const auto nativeStatementTemp2234=nativeStatementTemp2236;Args nativeStatementTemp2235;nativeStatementTemp2235.push_back(([&]()->JSValue{auto nativeStatementTemp2241=([&]()->JSValue{const auto nativeStatementTemp2243=morphAttribute;const auto nativeStatementTemp2244=i;return nativeStatementTemp2243.get(nativeStatementTemp2244);}());const auto nativeStatementTemp2242=JSValue("clone");const auto nativeStatementTemp2238=nativeStatementTemp2241.get(nativeStatementTemp2242);const auto nativeStatementTemp2239=nativeStatementTemp2241;Args nativeStatementTemp2240;nativeStatementTemp2240.push_back(data);return nativeStatementTemp2238.call(nativeStatementTemp2239,nativeStatementTemp2240);}()));return nativeStatementTemp2233.call(nativeStatementTemp2234,nativeStatementTemp2235);}());

}
}
([&]()->JSValue{auto nativeStatementTemp2245=([&]()->JSValue{const auto nativeStatementTemp2247=nativeStatementTemp2147;const auto nativeStatementTemp2248=JSValue("morphAttributes");return nativeStatementTemp2247.get(nativeStatementTemp2248);}());const auto nativeStatementTemp2246=name;const auto nativeStatementTemp2249=array;setReference(nativeStatementTemp2245,nativeStatementTemp2246,nativeStatementTemp2249);return nativeStatementTemp2249;}());

}
}}
([&]()->JSValue{auto nativeStatementTemp2252=nativeStatementTemp2147;const auto nativeStatementTemp2253=JSValue("morphTargetsRelative");const auto nativeStatementTemp2254=([&]()->JSValue{const auto nativeStatementTemp2257=source;const auto nativeStatementTemp2258=JSValue("morphTargetsRelative");return nativeStatementTemp2257.get(nativeStatementTemp2258);}());setReference(nativeStatementTemp2252,nativeStatementTemp2253,nativeStatementTemp2254);return nativeStatementTemp2254;}());
JSValue groups=([&]()->JSValue{const auto nativeStatementTemp2259=source;const auto nativeStatementTemp2260=JSValue("groups");return nativeStatementTemp2259.get(nativeStatementTemp2260);}());
{JSValue i=JSValue(0);
JSValue l=([&]()->JSValue{const auto nativeStatementTemp2261=groups;const auto nativeStatementTemp2262=JSValue("length");return nativeStatementTemp2261.get(nativeStatementTemp2262);}());
for(;([&]()->JSValue{const auto nativeStatementTemp2263=i;const auto nativeStatementTemp2264=l;return binary("<",nativeStatementTemp2263,nativeStatementTemp2264);}()).toBoolean();([&]()->JSValue{const auto nativeStatementTemp2265=i.toNumber();const auto nativeStatementTemp2266=nativeStatementTemp2265+1;i=nativeStatementTemp2266;return nativeStatementTemp2265;}())){
JSValue group=([&]()->JSValue{const auto nativeStatementTemp2267=groups;const auto nativeStatementTemp2268=i;return nativeStatementTemp2267.get(nativeStatementTemp2268);}());
([&]()->JSValue{auto nativeStatementTemp2272=nativeStatementTemp2147;const auto nativeStatementTemp2273=JSValue("addGroup");const auto nativeStatementTemp2269=nativeStatementTemp2272.get(nativeStatementTemp2273);const auto nativeStatementTemp2270=nativeStatementTemp2272;Args nativeStatementTemp2271;nativeStatementTemp2271.push_back(([&]()->JSValue{const auto nativeStatementTemp2274=group;const auto nativeStatementTemp2275=JSValue("start");return nativeStatementTemp2274.get(nativeStatementTemp2275);}()));nativeStatementTemp2271.push_back(([&]()->JSValue{const auto nativeStatementTemp2276=group;const auto nativeStatementTemp2277=JSValue("count");return nativeStatementTemp2276.get(nativeStatementTemp2277);}()));nativeStatementTemp2271.push_back(([&]()->JSValue{const auto nativeStatementTemp2278=group;const auto nativeStatementTemp2279=JSValue("materialIndex");return nativeStatementTemp2278.get(nativeStatementTemp2279);}()));return nativeStatementTemp2269.call(nativeStatementTemp2270,nativeStatementTemp2271);}());

}
}
JSValue boundingBox=([&]()->JSValue{const auto nativeStatementTemp2280=source;const auto nativeStatementTemp2281=JSValue("boundingBox");return nativeStatementTemp2280.get(nativeStatementTemp2281);}());
if(([&]()->JSValue{const auto nativeStatementTemp2282=boundingBox;const auto nativeStatementTemp2283=JSValue(nullptr);return binary("!==",nativeStatementTemp2282,nativeStatementTemp2283);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp2284=nativeStatementTemp2147;const auto nativeStatementTemp2285=JSValue("boundingBox");const auto nativeStatementTemp2286=([&]()->JSValue{auto nativeStatementTemp2292=boundingBox;const auto nativeStatementTemp2293=JSValue("clone");const auto nativeStatementTemp2289=nativeStatementTemp2292.get(nativeStatementTemp2293);const auto nativeStatementTemp2290=nativeStatementTemp2292;Args nativeStatementTemp2291;return nativeStatementTemp2289.call(nativeStatementTemp2290,nativeStatementTemp2291);}());setReference(nativeStatementTemp2284,nativeStatementTemp2285,nativeStatementTemp2286);return nativeStatementTemp2286;}());

}
JSValue boundingSphere=([&]()->JSValue{const auto nativeStatementTemp2294=source;const auto nativeStatementTemp2295=JSValue("boundingSphere");return nativeStatementTemp2294.get(nativeStatementTemp2295);}());
if(([&]()->JSValue{const auto nativeStatementTemp2296=boundingSphere;const auto nativeStatementTemp2297=JSValue(nullptr);return binary("!==",nativeStatementTemp2296,nativeStatementTemp2297);}()).toBoolean()){
([&]()->JSValue{auto nativeStatementTemp2298=nativeStatementTemp2147;const auto nativeStatementTemp2299=JSValue("boundingSphere");const auto nativeStatementTemp2300=([&]()->JSValue{auto nativeStatementTemp2306=boundingSphere;const auto nativeStatementTemp2307=JSValue("clone");const auto nativeStatementTemp2303=nativeStatementTemp2306.get(nativeStatementTemp2307);const auto nativeStatementTemp2304=nativeStatementTemp2306;Args nativeStatementTemp2305;return nativeStatementTemp2303.call(nativeStatementTemp2304,nativeStatementTemp2305);}());setReference(nativeStatementTemp2298,nativeStatementTemp2299,nativeStatementTemp2300);return nativeStatementTemp2300;}());

}
([&]()->JSValue{auto nativeStatementTemp2308=([&]()->JSValue{const auto nativeStatementTemp2310=nativeStatementTemp2147;const auto nativeStatementTemp2311=JSValue("drawRange");return nativeStatementTemp2310.get(nativeStatementTemp2311);}());const auto nativeStatementTemp2309=JSValue("start");const auto nativeStatementTemp2312=([&]()->JSValue{const auto nativeStatementTemp2315=([&]()->JSValue{const auto nativeStatementTemp2317=source;const auto nativeStatementTemp2318=JSValue("drawRange");return nativeStatementTemp2317.get(nativeStatementTemp2318);}());const auto nativeStatementTemp2316=JSValue("start");return nativeStatementTemp2315.get(nativeStatementTemp2316);}());setReference(nativeStatementTemp2308,nativeStatementTemp2309,nativeStatementTemp2312);return nativeStatementTemp2312;}());
([&]()->JSValue{auto nativeStatementTemp2319=([&]()->JSValue{const auto nativeStatementTemp2321=nativeStatementTemp2147;const auto nativeStatementTemp2322=JSValue("drawRange");return nativeStatementTemp2321.get(nativeStatementTemp2322);}());const auto nativeStatementTemp2320=JSValue("count");const auto nativeStatementTemp2323=([&]()->JSValue{const auto nativeStatementTemp2326=([&]()->JSValue{const auto nativeStatementTemp2328=source;const auto nativeStatementTemp2329=JSValue("drawRange");return nativeStatementTemp2328.get(nativeStatementTemp2329);}());const auto nativeStatementTemp2327=JSValue("count");return nativeStatementTemp2326.get(nativeStatementTemp2327);}());setReference(nativeStatementTemp2319,nativeStatementTemp2320,nativeStatementTemp2323);return nativeStatementTemp2323;}());
([&]()->JSValue{auto nativeStatementTemp2330=nativeStatementTemp2147;const auto nativeStatementTemp2331=JSValue("userData");const auto nativeStatementTemp2332=([&]()->JSValue{const auto nativeStatementTemp2335=source;const auto nativeStatementTemp2336=JSValue("userData");return nativeStatementTemp2335.get(nativeStatementTemp2336);}());setReference(nativeStatementTemp2330,nativeStatementTemp2331,nativeStatementTemp2332);return nativeStatementTemp2332;}());
([&]()->JSValue{auto nativeStatementTemp2337=nativeStatementTemp2147;const auto nativeStatementTemp2338=JSValue("_transformed");const auto nativeStatementTemp2339=([&]()->JSValue{const auto nativeStatementTemp2342=source;const auto nativeStatementTemp2343=JSValue("_transformed");return nativeStatementTemp2342.get(nativeStatementTemp2343);}());setReference(nativeStatementTemp2337,nativeStatementTemp2338,nativeStatementTemp2339);return nativeStatementTemp2339;}());
return nativeStatementTemp2147;
return JSValue{};
}

JSValue method31(const JSValue& nativeStatementTemp2344,const Args& nativeStatementTemp2345,const Args& nativeStatementTemp2346){
auto nativeStatementTemp2347=nativeStatementTemp2346[0];auto nativeStatementTemp2348=nativeStatementTemp2344;
([&]()->JSValue{auto nativeStatementTemp2353=nativeStatementTemp2348;const auto nativeStatementTemp2354=JSValue("dispatchEvent");const auto nativeStatementTemp2350=nativeStatementTemp2353.get(nativeStatementTemp2354);const auto nativeStatementTemp2351=nativeStatementTemp2353;Args nativeStatementTemp2352;nativeStatementTemp2352.push_back(([&]()->JSValue{auto nativeStatementTemp2355=JSValue::object();{const auto nativeStatementTemp2356=JSValue("type");const auto nativeStatementTemp2357=JSValue("dispose");own(nativeStatementTemp2355,nativeStatementTemp2356,nativeStatementTemp2357);}return nativeStatementTemp2355;}()));return nativeStatementTemp2350.call(nativeStatementTemp2351,nativeStatementTemp2352);}());
return JSValue{};
}
}
JSValue nativeBufferGeometryModule(){
using namespace BufferGeometryManaged;static thread_local JSWeakValue weak;if(const auto cached=weak.lock();cached&&cached->isObject())return *cached;static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"BufferGeometry module");auto module=JSValue::object(nullptr);
own(module,"Vector3",nativeVector3Constructor());
own(module,"Vector2",nativeVector2Constructor());
own(module,"Box3",nativeBox3Constructor());
own(module,"EventDispatcher",EventDispatcher::runtimeConstructor());
own(module,"BufferAttribute",nativeBufferAttributeConstructor());
own(module,"Float32BufferAttribute",nativeFloat32BufferAttributeConstructor());
own(module,"Uint16BufferAttribute",nativeUint16BufferAttributeConstructor());
own(module,"Uint32BufferAttribute",nativeUint32BufferAttributeConstructor());
own(module,"Sphere",nativeSphereConstructor());
own(module,"Object3D",nativeObject3DConstructor());
own(module,"Matrix4",nativeMatrix4Constructor());
own(module,"Matrix3",nativeMatrix3Constructor());
own(module,"generateUUID",nativeMathUtilsGenerateUUID());
own(module,"arrayNeedsUint32",ThreeUtils::nativeArrayNeedsUint32());
own(module,"warn",ThreeUtils::nativeWarnFunction());
own(module,"error",ThreeUtils::nativeErrorFunction());
own(module,"_id",JSValue(0));
own(module,"_m1",([&]()->JSValue{const auto nativeStatementTemp2358=module.get("Matrix4");Args nativeStatementTemp2359;return nativeStatementTemp2358.construct(nativeStatementTemp2359);}()));
own(module,"_obj",([&]()->JSValue{const auto nativeStatementTemp2360=module.get("Object3D");Args nativeStatementTemp2361;return nativeStatementTemp2360.construct(nativeStatementTemp2361);}()));
own(module,"_offset",([&]()->JSValue{const auto nativeStatementTemp2362=module.get("Vector3");Args nativeStatementTemp2363;return nativeStatementTemp2362.construct(nativeStatementTemp2363);}()));
own(module,"_box",([&]()->JSValue{const auto nativeStatementTemp2364=module.get("Box3");Args nativeStatementTemp2365;return nativeStatementTemp2364.construct(nativeStatementTemp2365);}()));
own(module,"_boxMorphTargets",([&]()->JSValue{const auto nativeStatementTemp2366=module.get("Box3");Args nativeStatementTemp2367;return nativeStatementTemp2366.construct(nativeStatementTemp2367);}()));
own(module,"_vector",([&]()->JSValue{const auto nativeStatementTemp2368=module.get("Vector3");Args nativeStatementTemp2369;return nativeStatementTemp2368.construct(nativeStatementTemp2369);}()));
auto ctor=JSValue::builtinConstructor(method0,{module},"BufferGeometry",0);ctor.setFunctionSourceText(BufferGeometryRuntimeSource::classSource);ctor.setPrototype(module.get("EventDispatcher"));auto prototype=ctor.get("prototype");prototype.setPrototype(module.get("EventDispatcher").get("prototype"));own(module,"BufferGeometry",ctor);
{auto function=JSValue::function(method1,{module},"getIndex",0);function.setFunctionSourceText(BufferGeometryRuntimeSource::method1);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("getIndex",d);}
{auto function=JSValue::function(method2,{module},"setIndex",1);function.setFunctionSourceText(BufferGeometryRuntimeSource::method2);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("setIndex",d);}
{auto function=JSValue::function(method3,{module},"setIndirect",1);function.setFunctionSourceText(BufferGeometryRuntimeSource::method3);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("setIndirect",d);}
{auto function=JSValue::function(method4,{module},"getIndirect",0);function.setFunctionSourceText(BufferGeometryRuntimeSource::method4);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("getIndirect",d);}
{auto function=JSValue::function(method5,{module},"getAttribute",1);function.setFunctionSourceText(BufferGeometryRuntimeSource::method5);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("getAttribute",d);}
{auto function=JSValue::function(method6,{module},"setAttribute",2);function.setFunctionSourceText(BufferGeometryRuntimeSource::method6);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("setAttribute",d);}
{auto function=JSValue::function(method7,{module},"deleteAttribute",1);function.setFunctionSourceText(BufferGeometryRuntimeSource::method7);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("deleteAttribute",d);}
{auto function=JSValue::function(method8,{module},"hasAttribute",1);function.setFunctionSourceText(BufferGeometryRuntimeSource::method8);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("hasAttribute",d);}
{auto function=JSValue::function(method9,{module},"addGroup",2);function.setFunctionSourceText(BufferGeometryRuntimeSource::method9);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("addGroup",d);}
{auto function=JSValue::function(method10,{module},"clearGroups",0);function.setFunctionSourceText(BufferGeometryRuntimeSource::method10);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("clearGroups",d);}
{auto function=JSValue::function(method11,{module},"setDrawRange",2);function.setFunctionSourceText(BufferGeometryRuntimeSource::method11);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("setDrawRange",d);}
{auto function=JSValue::function(method12,{module},"applyMatrix4",1);function.setFunctionSourceText(BufferGeometryRuntimeSource::method12);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("applyMatrix4",d);}
{auto function=JSValue::function(method13,{module},"applyQuaternion",1);function.setFunctionSourceText(BufferGeometryRuntimeSource::method13);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("applyQuaternion",d);}
{auto function=JSValue::function(method14,{module},"rotateX",1);function.setFunctionSourceText(BufferGeometryRuntimeSource::method14);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("rotateX",d);}
{auto function=JSValue::function(method15,{module},"rotateY",1);function.setFunctionSourceText(BufferGeometryRuntimeSource::method15);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("rotateY",d);}
{auto function=JSValue::function(method16,{module},"rotateZ",1);function.setFunctionSourceText(BufferGeometryRuntimeSource::method16);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("rotateZ",d);}
{auto function=JSValue::function(method17,{module},"translate",3);function.setFunctionSourceText(BufferGeometryRuntimeSource::method17);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("translate",d);}
{auto function=JSValue::function(method18,{module},"scale",3);function.setFunctionSourceText(BufferGeometryRuntimeSource::method18);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("scale",d);}
{auto function=JSValue::function(method19,{module},"lookAt",1);function.setFunctionSourceText(BufferGeometryRuntimeSource::method19);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("lookAt",d);}
{auto function=JSValue::function(method20,{module},"center",0);function.setFunctionSourceText(BufferGeometryRuntimeSource::method20);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("center",d);}
{auto function=JSValue::function(method21,{module},"setFromPoints",1);function.setFunctionSourceText(BufferGeometryRuntimeSource::method21);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("setFromPoints",d);}
{auto function=JSValue::function(method22,{module},"computeBoundingBox",0);function.setFunctionSourceText(BufferGeometryRuntimeSource::method22);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("computeBoundingBox",d);}
{auto function=JSValue::function(method23,{module},"computeBoundingSphere",0);function.setFunctionSourceText(BufferGeometryRuntimeSource::method23);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("computeBoundingSphere",d);}
{auto function=JSValue::function(method24,{module},"computeTangents",0);function.setFunctionSourceText(BufferGeometryRuntimeSource::method24);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("computeTangents",d);}
{auto function=JSValue::function(method25,{module},"computeVertexNormals",0);function.setFunctionSourceText(BufferGeometryRuntimeSource::method25);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("computeVertexNormals",d);}
{auto function=JSValue::function(method26,{module},"normalizeNormals",0);function.setFunctionSourceText(BufferGeometryRuntimeSource::method26);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("normalizeNormals",d);}
{auto function=JSValue::function(method27,{module},"toNonIndexed",0);function.setFunctionSourceText(BufferGeometryRuntimeSource::method27);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("toNonIndexed",d);}
{auto function=JSValue::function(method28,{module},"toJSON",0);function.setFunctionSourceText(BufferGeometryRuntimeSource::method28);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("toJSON",d);}
{auto function=JSValue::function(method29,{module},"clone",0);function.setFunctionSourceText(BufferGeometryRuntimeSource::method29);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("clone",d);}
{auto function=JSValue::function(method30,{module},"copy",1);function.setFunctionSourceText(BufferGeometryRuntimeSource::method30);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("copy",d);}
{auto function=JSValue::function(method31,{module},"dispose",0);function.setFunctionSourceText(BufferGeometryRuntimeSource::method31);JSPropertyDescriptor d;d.value=function;d.writable=d.configurable=true;prototype.defineProperty("dispose",d);}
module.retainForHeapLifetime();lexical()=JSWeakValue(module);auto facade=JSValue::object();own(facade,"BufferGeometry",module.get("BufferGeometry"));facade.retainForHeapLifetime();weak=JSWeakValue(facade);return facade;
}
JSValue nativeBufferGeometryBinding(const std::string& name){const auto keeper=nativeBufferGeometryModule();const auto state=BufferGeometryManaged::lexical().lock();return name=="BufferGeometry"?state->get(name):JSValue{};}
JSValue nativeBufferGeometryConstructor(){return nativeBufferGeometryBinding("BufferGeometry");}
