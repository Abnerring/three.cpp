#pragma once
#include "math/Box3.h"
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
class BatchedMesh : public Mesh {
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
};