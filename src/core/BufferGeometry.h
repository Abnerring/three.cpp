#pragma once
#include "core/JsonValue.h"
#include "core/OrderedMap.h"
#include "core/SharedVector.h"
#include "core/BufferAttribute.h"
#include "math/Box3.h"
#include "math/Quaternion.h"
#include "math/Sphere.h"
#include <atomic>
#include <limits>
#include <map>
#include <optional>

class AnimationClip;
class BufferGeometry : public EventDispatcher, public std::enable_shared_from_this<BufferGeometry> {
  public:
    struct Group {
        double start, count, materialIndex;
    };
    struct DrawRange {
        double start = 0, count = std::numeric_limits<double>::infinity();
    };
    const bool isBufferGeometry = true;
    bool isInstancedBufferGeometry = false;
    const std::uint64_t id = nextId++;
    std::string uuid, name, type = "BufferGeometry";
    std::shared_ptr<BufferAttribute> index;
    OrderedMap<std::shared_ptr<VertexAttribute>> attributes;
    OrderedMap<std::vector<std::shared_ptr<VertexAttribute>>> morphAttributes;
    bool morphTargetsRelative = false;
    SharedVector<Group> groups;
    std::optional<Box3> boundingBox;
    std::optional<Sphere> boundingSphere;
    DrawRange drawRange;
    JsonValue userData = JsonValue::Object{};
    // Optional legacy field read by AnimationClip.findByName; absent by default.
    std::shared_ptr<std::vector<std::shared_ptr<AnimationClip>>> animations;
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
    // Upstream keeps the indirect attribute and its offset as plain fields, sets both in setIndirect and
    // hands the attribute back from getIndirect, with it null until something is set.
    std::shared_ptr<BufferAttribute> indirect;
    double indirectOffset = 0;
    BufferGeometry& setIndirect(std::shared_ptr<BufferAttribute> value, double offset = 0) {
        indirect = std::move(value);
        indirectOffset = offset;
        return *this;
    }
    std::shared_ptr<BufferAttribute> getIndirect() const { return indirect; }
    std::shared_ptr<VertexAttribute> getAttribute(const std::string& name) const;
    BufferGeometry& setAttribute(const std::string& name,
                                 std::shared_ptr<VertexAttribute> attribute);
    BufferGeometry& deleteAttribute(const std::string& name);
    bool hasAttribute(const std::string& name) const;
    void addGroup(double start, double count, double materialIndex = 0);
    void clearGroups();
    void setDrawRange(double start, double count);
    BufferGeometry& applyMatrix4(const Matrix4& matrix);
    // Upstream routes this through a scratch Object3D: lookAt orients it, updateMatrix composes, and the
    // geometry is transformed by that matrix.
    BufferGeometry& lookAt(const Vector3& vector);
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
    // Upstream logs and returns when the index, positions, normals or uvs are missing, so this returns
    // without changing anything rather than throwing. Degenerate uv triangles are skipped the same way.
    void computeTangents();
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
