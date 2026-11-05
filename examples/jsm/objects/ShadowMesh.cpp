#include "objects/ShadowMesh.h"

#include "constants.h"
#include "math/Plane.h"
#include "math/Vector4.h"
#include "materials/MeshBasicMaterial.h"

#include <memory>

namespace {
// The pinned module keeps one scratch matrix at module scope; the port keeps it per thread.
thread_local Matrix4 shadowMatrixScratch;

std::shared_ptr<Material> buildShadowMaterial() {
    // new MeshBasicMaterial( { color: 0x000000, transparent: true, opacity: 0.6, depthWrite: false,
    //                          stencilWrite: true, stencilFunc: EqualStencilFunc, stencilRef: 0,
    //                          stencilZPass: IncrementStencilOp } )
    auto material = std::make_shared<MeshBasicMaterial>();
    material->color = Color(0x000000);
    material->transparent = true;
    material->opacity = 0.6;
    material->depthWrite = false;
    material->stencilWrite = true;
    material->stencilFunc = EqualStencilFunc;
    material->stencilRef = 0;
    material->stencilZPass = IncrementStencilOp;
    return material;
}
}  // namespace

ShadowMesh::ShadowMesh(const std::shared_ptr<Mesh>& mesh)
    : Mesh(mesh ? mesh->geometry : nullptr, buildShadowMaterial()) {
    // isShadowMesh is a class constant, already true.
    // this.meshMatrix = mesh.matrixWorld
    meshMatrix = mesh ? const_cast<Matrix4*>(&mesh->matrixWorld) : nullptr;
    frustumCulled = false;
    matrixAutoUpdate = false;
}

void ShadowMesh::update(const Plane& plane, const Vector4& lightPosition4D) {
    const double dot = plane.normal.x * lightPosition4D.x +
                       plane.normal.y * lightPosition4D.y +
                       plane.normal.z * lightPosition4D.z + -plane.constant * lightPosition4D.w;
    auto elements = shadowMatrixScratch.elements();
    elements[0] = dot - lightPosition4D.x * plane.normal.x;
    elements[4] = -lightPosition4D.x * plane.normal.y;
    elements[8] = -lightPosition4D.x * plane.normal.z;
    elements[12] = -lightPosition4D.x * -plane.constant;
    elements[1] = -lightPosition4D.y * plane.normal.x;
    elements[5] = dot - lightPosition4D.y * plane.normal.y;
    elements[9] = -lightPosition4D.y * plane.normal.z;
    elements[13] = -lightPosition4D.y * -plane.constant;
    elements[2] = -lightPosition4D.z * plane.normal.x;
    elements[6] = -lightPosition4D.z * plane.normal.y;
    elements[10] = dot - lightPosition4D.z * plane.normal.z;
    elements[14] = -lightPosition4D.z * -plane.constant;
    elements[3] = -lightPosition4D.w * plane.normal.x;
    elements[7] = -lightPosition4D.w * plane.normal.y;
    elements[11] = -lightPosition4D.w * plane.normal.z;
    elements[15] = dot - lightPosition4D.w * -plane.constant;
    // this.matrix.multiplyMatrices( _shadowMatrix, this.meshMatrix )
    if (meshMatrix != nullptr) matrix.multiplyMatrices(shadowMatrixScratch, *meshMatrix);
}
