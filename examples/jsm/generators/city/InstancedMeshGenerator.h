#pragma once

#include "core/BufferGeometry.h"
#include "core/JsonValue.h"
#include "math/Matrix4.h"
#include "objects/InstancedMesh.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

// Native port of examples/jsm/generators/city/InstancedMeshGenerator.js from the pinned three.js
// revision (see upstream.json).
//
// A small helper that keeps one InstancedMesh for a set of placements, rebuilding it only when the
// geometry/material parameters change or when the placement count outgrows the allocated instance
// capacity. The pinned constructor reads `this.constructor.defaults`; C++ passes the defaults
// explicitly (defaults first, then the parameters, like Object.assign).
//
// createGeometry / createMaterial are injected as std::function callbacks, matching the pinned
// factory arguments, and placements are Matrix4 values side by side like the pinned array.
class InstancedMeshGenerator {
  public:
    JsonValue parameters;
    std::shared_ptr<BufferGeometry> geometry = nullptr;
    std::shared_ptr<Material> material = nullptr;
    std::shared_ptr<InstancedMesh> mesh = nullptr;
    std::function<std::shared_ptr<BufferGeometry>(const JsonValue&)> createGeometry;
    std::function<std::shared_ptr<Material>()> createMaterial;
    std::string name;
    // The pinned constructor stores this as _receiveShadow and the builder copies it to the mesh.
    bool _receiveShadow = true;

    InstancedMeshGenerator(JsonValue parameters,
                           std::function<std::shared_ptr<BufferGeometry>(const JsonValue&)> createGeometry,
                           std::function<std::shared_ptr<Material>()> createMaterial, std::string name,
                           bool receiveShadow = true, JsonValue defaults = JsonValue());

    std::shared_ptr<InstancedMesh> build(const std::vector<Matrix4>& placements);
    void dispose();

  private:
    std::string parametersKey_;
};

// function createInstances( geometry, material, count, name )
std::shared_ptr<InstancedMesh> createInstances(const std::shared_ptr<BufferGeometry>& geometry,
                                               const std::shared_ptr<Material>& material,
                                               std::size_t count, const std::string& name);

// function updateInstances( mesh, placements )
void updateInstances(const std::shared_ptr<InstancedMesh>& mesh,
                     const std::vector<Matrix4>& placements);