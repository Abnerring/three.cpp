#pragma once
#include "common/JsonValue.h"
#include "common/OrderedMap.h"
#include "common/SharedVector.h"
#include "core/BufferAttribute.h"
#include "math/Box3.h"
#include "math/Quaternion.h"
#include "math/Sphere.h"
#include <atomic>
#include <limits>
#include <map>
#include <optional>

class BufferGeometry : public EventDispatcher, public std::enable_shared_from_this<BufferGeometry> {
  public:
    struct Group {
        std::size_t start, count, materialIndex;
    };
    struct DrawRange {
        std::size_t start = 0, count = std::numeric_limits<std::size_t>::max();
    };
    const bool isBufferGeometry = true;
    const std::uint64_t id = nextId++;
    std::string uuid, name, type = "BufferGeometry";
    std::shared_ptr<BufferAttribute> index;
    OrderedMap<std::shared_ptr<BufferAttribute>> attributes;
    OrderedMap<std::vector<std::shared_ptr<BufferAttribute>>> morphAttributes;
    bool morphTargetsRelative = false;
    SharedVector<Group> groups;
    std::optional<Box3> boundingBox;
    std::optional<Sphere> boundingSphere;
    DrawRange drawRange;
    JsonValue userData = JsonValue::Object{};
    bool _transformed = false;

    BufferGeometry();
    virtual ~BufferGeometry() = default;
    BufferGeometry(const BufferGeometry&) = delete;
    BufferGeometry& operator=(const BufferGeometry&) = delete;
    std::shared_ptr<BufferAttribute> getIndex() const { return index; }
    BufferGeometry& setIndex(std::shared_ptr<BufferAttribute> value) {
        index = std::move(value);
        return *this;
    }
    BufferGeometry& setIndex(const std::vector<std::uint32_t>& values);
    std::shared_ptr<BufferAttribute> getAttribute(const std::string& name) const;
    BufferGeometry& setAttribute(const std::string& name,
                                 std::shared_ptr<BufferAttribute> attribute);
    BufferGeometry& deleteAttribute(const std::string& name);
    bool hasAttribute(const std::string& name) const;
    void addGroup(std::size_t start, std::size_t count, std::size_t materialIndex = 0);
    void clearGroups();
    void setDrawRange(std::size_t start, std::size_t count);
    BufferGeometry& applyMatrix4(const Matrix4& matrix);
    BufferGeometry& applyQuaternion(const Quaternion& quaternion);
    BufferGeometry& rotateX(double angle);
    BufferGeometry& rotateY(double angle);
    BufferGeometry& rotateZ(double angle);
    BufferGeometry& translate(double x, double y, double z);
    BufferGeometry& scale(double x, double y, double z);
    BufferGeometry& center();
    BufferGeometry& setFromPoints(const std::vector<Vector3>& points);
    void computeBoundingBox();
    void computeBoundingSphere();
    void computeVertexNormals();
    void normalizeNormals();
    std::shared_ptr<BufferGeometry> toNonIndexed();
    virtual std::shared_ptr<BufferGeometry> clone() const;
    virtual BufferGeometry& copy(const BufferGeometry& source);
    virtual JsonValue toJSON() const;
    void dispose();

  private:
    inline static std::atomic<std::uint64_t> nextId{0};
};
