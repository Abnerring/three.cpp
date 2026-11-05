#pragma once

#include "core/Object3D.h"
#include "math/Vector3.h"

#include <functional>
#include <memory>
#include <vector>

class BufferGeometry;
class Group;
class InstancedMesh;
class Material;

// Native port of examples/jsm/utils/SceneUtils.js from the pinned three.js revision
// (see upstream.json).
//
// Ported helpers:
//   createMeshesFromInstancedMesh  - expand an InstancedMesh into a Group of Meshes
//   createMultiMaterialObject      - one Mesh per material sharing the geometry
//   reduceVertices                 - fold every visible vertex through a callback
//   traverse / traverseVisible / traverseAncestors equivalents already exist on Object3D
//
// Not ported yet (documented instead of approximated):
//   createMeshesFromMultiMaterialMesh - needs mergeGroups from BufferGeometryUtils
//   traverseGenerator and the two sibling generators - C++ uses the Object3D traversal methods,
//   which have the same visit order, so generators are not offered as a separate API.
namespace SceneUtils {
std::shared_ptr<Group> createMeshesFromInstancedMesh(const std::shared_ptr<InstancedMesh>& mesh);
std::shared_ptr<Group> createMultiMaterialObject(
    const std::shared_ptr<BufferGeometry>& geometry,
    const std::vector<std::shared_ptr<Material>>& materials);
double reduceVertices(const std::shared_ptr<Object3D>& object,
                      const std::function<double(double, const Vector3&)>& callback,
                      double initialValue);
// Sorts the instance payloads by the given index comparator; the comparator receives two instance
// indices like the pinned Array.prototype.sort callback.
void sortInstancedMesh(const std::shared_ptr<InstancedMesh>& mesh,
                       const std::function<bool(double, double)>& compareFn);
}  // namespace SceneUtils
