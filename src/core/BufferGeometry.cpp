#include "core/BufferGeometry.h"
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
std::shared_ptr<BufferAttribute> BufferGeometry::getAttribute(const std::string& name) const {
    const auto it = attributes.find(name);
    return it == attributes.end() ? nullptr : it->second;
}
BufferGeometry& BufferGeometry::setAttribute(const std::string& name,
                                             std::shared_ptr<BufferAttribute> a) {
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
void BufferGeometry::addGroup(std::size_t start, std::size_t count, std::size_t materialIndex) {
    groups.push_back({start, count, materialIndex});
}
void BufferGeometry::clearGroups() {
    groups.clear();
}
void BufferGeometry::setDrawRange(std::size_t start, std::size_t count) {
    drawRange = {start, count};
}
BufferGeometry& BufferGeometry::applyMatrix4(const Matrix4& m) {
    if (auto a = getAttribute("position")) {
        a->applyMatrix4(m);
        a->needsUpdate = true;
    }
    if (auto a = getAttribute("normal")) {
        Matrix3 n;
        n.getNormalMatrix(m);
        a->applyNormalMatrix(n);
        a->needsUpdate = true;
    }
    if (auto a = getAttribute("tangent")) {
        a->transformDirection(m);
        a->needsUpdate = true;
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
    return translate(v.x(), v.y(), v.z());
}
BufferGeometry& BufferGeometry::setFromPoints(const std::vector<Vector3>& points) {
    auto a = getAttribute("position");
    if (!a) {
        std::vector<float> values;
        values.reserve(points.size() * 3);
        for (const auto& p : points) {
            values.push_back(static_cast<float>(p.x()));
            values.push_back(static_cast<float>(p.y()));
            values.push_back(static_cast<float>(p.z() == 0 || std::isnan(p.z()) ? 0 : p.z()));
        }
        setAttribute("position", std::make_shared<Float32BufferAttribute>(std::move(values), 3));
    } else {
        for (std::size_t i = 0; i < std::min(a->count, points.size()); ++i)
            a->setXYZ(i, points[i].x(), points[i].y(),
                      points[i].z() == 0 || std::isnan(points[i].z()) ? 0 : points[i].z());
        a->needsUpdate = true;
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
    for (std::size_t i = 0; i < p->count; ++i) {
        Vector3 v(p->getX(i), p->getY(i), p->getZ(i));
        radiusSq = MathUtils::max(radiusSq, v.distanceToSquared(boundingSphere->center));
    }
    if (it != morphAttributes.end())
        for (const auto& a : it->second)
            for (std::size_t i = 0; i < a->count; ++i) {
                Vector3 v(a->getX(i), a->getY(i), a->getZ(i));
                if (morphTargetsRelative) {
                    Vector3 base(p->getX(i), p->getY(i), p->getZ(i));
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
    if (!n || n->count != p->count) {
        n = std::make_shared<Float32BufferAttribute>(std::vector<float>(p->count * 3, 0), 3);
        setAttribute("normal", n);
    } else
        for (std::size_t i = 0; i < n->count; ++i)
            n->setXYZ(i, 0, 0, 0);
    const auto length = index ? index->count : p->count;
    for (std::size_t i = 0; i + 2 < length; i += 3) {
        const std::size_t a = index ? static_cast<std::size_t>(index->getX(i)) : i,
                          b = index ? static_cast<std::size_t>(index->getX(i + 1)) : i + 1,
                          c = index ? static_cast<std::size_t>(index->getX(i + 2)) : i + 2;
        Vector3 pa(p->getX(a), p->getY(a), p->getZ(a)), pb(p->getX(b), p->getY(b), p->getZ(b)),
            pc(p->getX(c), p->getY(c), p->getZ(c)), cb, ab;
        cb.subVectors(pc, pb);
        ab.subVectors(pa, pb);
        cb.cross(ab);
        for (auto v : {a, b, c}) {
            if (index)
                n->setXYZ(v, n->getX(v) + cb.x(), n->getY(v) + cb.y(), n->getZ(v) + cb.z());
            else
                n->setXYZ(v, cb.x(), cb.y(), cb.z());
        }
    }
    normalizeNormals();
    n->needsUpdate = true;
}
void BufferGeometry::normalizeNormals() {
    auto n = getAttribute("normal");
    if (!n)
        return;
    for (std::size_t i = 0; i < n->count; ++i) {
        Vector3 v(n->getX(i), n->getY(i), n->getZ(i));
        v.normalize();
        n->setXYZ(i, v.x(), v.y(), v.z());
    }
}
std::shared_ptr<BufferGeometry> BufferGeometry::toNonIndexed() {
    if (!index)
        return shared_from_this();
    auto result = std::make_shared<BufferGeometry>();
    const auto convert = [&](const std::shared_ptr<BufferAttribute>& a) {
        auto copy = std::visit(
            [&](const auto& storage) -> std::shared_ptr<BufferAttribute> {
                using T = typename std::decay_t<decltype(storage)>::value_type;
                std::vector<T> values;
                values.reserve(index->count * a->itemSize);
                for (std::size_t i = 0; i < index->count; ++i) {
                    const auto rawIndex = std::visit(
                        [&](const auto& values) { return static_cast<std::size_t>(values.at(i)); },
                        index->array);
                    const auto offset = rawIndex * a->itemSize;
                    for (std::size_t c = 0; c < a->itemSize; ++c)
                        values.push_back(storage.at(offset + c));
                }
                return std::make_shared<BufferAttribute>(std::move(values), a->itemSize,
                                                         a->normalized);
            },
            a->array);
        return copy;
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
    if (this == &source)
        return *this;
    index = source.index ? source.index->clone() : nullptr;
    attributes.clear();
    morphAttributes.clear();
    for (const auto& [name, a] : source.attributes)
        attributes[name] = a->clone();
    for (const auto& [name, as] : source.morphAttributes)
        for (const auto& a : as)
            morphAttributes[name].push_back(a->clone());
    name = source.name;
    morphTargetsRelative = source.morphTargetsRelative;
    groups.clear();
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
        result["userData"] = userData;
    JsonValue data = JsonValue::Object{{"attributes", JsonValue::Object{}}};
    if (index) {
        auto serialized = index->toJSON();
        data["index"] =
            JsonValue::Object{{"type", serialized.at("type")}, {"array", serialized.at("array")}};
    }
    for (const auto& [key, a] : attributes)
        data["attributes"][key] = a->toJSON();
    JsonValue morphs = JsonValue::Object{};
    for (const auto& [key, as] : morphAttributes) {
        if (as.empty())
            continue;
        JsonValue::Array serialized;
        for (const auto& a : as)
            serialized.push_back(a->toJSON());
        morphs[key] = std::move(serialized);
    }
    if (morphs.size()) {
        data["morphAttributes"] = morphs;
        data["morphTargetsRelative"] = morphTargetsRelative;
    }
    if (!groups.empty()) {
        JsonValue::Array serialized;
        for (const auto& g : groups)
            serialized.push_back(JsonValue::Object{
                {"start", g.start}, {"count", g.count}, {"materialIndex", g.materialIndex}});
        data["groups"] = std::move(serialized);
    }
    if (boundingSphere)
        data["boundingSphere"] = JsonValue::Object{
            {"center", JsonValue::Array{boundingSphere->center.x(), boundingSphere->center.y(),
                                        boundingSphere->center.z()}},
            {"radius", boundingSphere->radius}};
    result["data"] = std::move(data);
    return result;
}
