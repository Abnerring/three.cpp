#include "core/BufferGeometry.h"
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
    morphAttributes.clear();
    groups.clear();boundingBox.reset();boundingSphere.reset();
    auto metadata=JSValue::object();name=source.name;
    index=source.index?source.index->clone():nullptr;
    for (const auto& [name, a] : source.attributes)
        attributes[name] = a->cloneAttribute(metadata);
    for (const auto& [name, as] : source.morphAttributes)
        for (const auto& a : as)
            morphAttributes[name].push_back(a->cloneAttribute(metadata));
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
