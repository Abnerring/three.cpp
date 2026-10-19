#pragma once
#include "math/Box3.h"
#include "math/Color.h"
#include "math/Matrix4.h"
#include "math/Sphere.h"
#include "objects/Mesh.h"
#include <memory>
#include <vector>

// Port of src/objects/BatchedMesh.js, data core only.
//
// Upstream draws a batch through two data textures and a multi draw call list, and those are the parts this
// port cannot carry without a renderer: the matrices texture holds one matrix per instance, the indirect
// texture maps an instance to its geometry range. What is portable is the bookkeeping around them - the
// per geometry ranges, the per instance records, the geometry initialization and validation - and that is
// what this first slice covers. Anything that would silently pretend to draw is left out.
class ObjectLoader;

class BatchedMesh : public Mesh {
    // The loader restores the bookkeeping straight from a document, as upstream does when it assigns to the
    // underscore fields. A friend declaration keeps that out of the public surface without adding anything
    // to the class layout.
    friend class ObjectLoader;

  public:
    struct GeometryInfo {
        double vertexStart = -1, vertexCount = -1, reservedVertexCount = -1;
        double indexStart = -1, indexCount = -1, reservedIndexCount = -1;
        double start = -1, count = -1;
        bool active = true;
        // Upstream clones whatever bounds the source geometry carried; a null one stays null.
        std::optional<Box3> boundingBox;
        std::optional<Sphere> boundingSphere;
    };

    const bool isBatchedMesh = true;
    bool perObjectFrustumCulled = true, sortObjects = true;

    BatchedMesh(double maxInstanceCount, double maxVertexCount, double maxIndexCount = -1, std::shared_ptr<Material> material = nullptr);

    double getMaxInstanceCount() const { return _maxInstanceCount; }
    double getInstanceCount() const { return static_cast<double>(_instanceInfo.size()) - static_cast<double>(_availableInstanceIds.size()); }
    double getUnusedVertexCount() const { return _maxVertexCount - _nextVertexStart; }
    double getUnusedIndexCount() const { return _maxIndexCount - _nextIndexStart; }

    // Upstream keeps these private and calls them from addGeometry; they are public here so a test can drive
    // them without going through a geometry copy that this slice does not carry yet.
    void initializeGeometry(const std::shared_ptr<BufferGeometry>& reference);
    void validateGeometry(const std::shared_ptr<BufferGeometry>& geometry) const;
    void validateInstanceId(double instanceId) const;
    void validateGeometryId(double geometryId) const;
    double addInstance(double geometryId);
    BatchedMesh& setVisibleAt(double instanceId, bool visible);
    bool getVisibleAt(double instanceId) const;
    BatchedMesh& setGeometryIdAt(double instanceId, double geometryId);
    double getGeometryIdAt(double instanceId) const;
    GeometryInfo getGeometryRangeAt(double geometryId) const;
    BatchedMesh& setMatrixAt(double instanceId, const Matrix4& matrix);
    Matrix4 getMatrixAt(double instanceId) const;
    BatchedMesh& setColorAt(double instanceId, const Color& color);
    Color getColorAt(double instanceId) const;
    BatchedMesh& deleteInstance(double instanceId);
    BatchedMesh& deleteGeometry(double geometryId);
    // Upstream returns null for an id past the geometry count, which an empty optional models here.
    std::optional<Box3> getBoundingBoxAt(double geometryId);
    std::optional<Sphere> getBoundingSphereAt(double geometryId);
    void computeBoundingBox();
    void computeBoundingSphere();
    // Upstream copies the whole bookkeeping and clones its two instance textures; the vectors stand in for
    // those textures here, so they are copied rather than cloned and the rest maps field for field.
    BatchedMesh& copy(const Object3D& source, bool recursive = true) override;
    bool raycast(const Raycaster& raycaster, std::vector<Intersection>& intersects) override;
    BatchedMesh& setInstanceCount(double maxInstanceCount);
    BatchedMesh& setGeometrySize(double maxVertexCount, double maxIndexCount);
    std::optional<Box3> boundingBox;
    std::optional<Sphere> boundingSphere;
    double addGeometry(const std::shared_ptr<BufferGeometry>& geometry, double reservedVertexCount = -1, double reservedIndexCount = -1);
    double setGeometryAt(double geometryId, const std::shared_ptr<BufferGeometry>& geometry);

  private:
    // One record per instance: its geometry id and whether it is active.
    struct InstanceInfo {
        double geometryId = -1;
        bool active = false;
        bool visible = true;
    };
    std::vector<InstanceInfo> _instanceInfo;
    std::vector<GeometryInfo> _geometryInfo;
    std::vector<double> _availableInstanceIds, _availableGeometryIds;
    double _nextIndexStart = 0, _nextVertexStart = 0, _geometryCount = 0;
    bool _geometryInitialized = false;
    double _maxInstanceCount = 0, _maxVertexCount = 0, _maxIndexCount = 0;
    // Upstream holds one matrix and one colour per instance in data textures; without a renderer the same
    // values live in these vectors, which is the only difference and keeps the accessors exact.
    std::vector<Matrix4> _instanceMatrices;
    std::vector<Color> _instanceColors;
};