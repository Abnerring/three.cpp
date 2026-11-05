#pragma once

#include "core/Object3D.h"
#include "math/Vector3.h"

#include <functional>
#include <memory>
#include <vector>

class BufferGeometry;
class Group;
class Mesh;
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

//   traverseGenerator and the two sibling generators - C++ uses the Object3D traversal methods,
//   which have the same visit order, so generators are not offered as a separate API.
namespace SceneUtils {
std::shared_ptr<Group> createMeshesFromInstancedMesh(const std::shared_ptr<InstancedMesh>& mesh);
std::shared_ptr<Group> createMultiMaterialObject(
    const std::shared_ptr<BufferGeometry>& geometry,
    const std::vector<std::shared_ptr<Material>>& materials);
// The pinned helper returns the input mesh unchanged when it has a single material, so the result is
// an Object3D that is either the input mesh or a new Group.
std::shared_ptr<Object3D> createMeshesFromMultiMaterialMesh(const std::shared_ptr<Mesh>& mesh);
double reduceVertices(const std::shared_ptr<Object3D>& object,
                      const std::function<double(double, const Vector3&)>& callback,
                      double initialValue);
// Sorts the instance payloads by the given index comparator. Like the pinned Array.prototype.sort
// callback the comparator receives two instance indices and answers a negative value when the first
// sorts before the second, zero for a tie and a positive value otherwise.
void sortInstancedMesh(const std::shared_ptr<InstancedMesh>& mesh,
                       const std::function<double(double, double)>& compareFn);
}  // namespace SceneUtils
