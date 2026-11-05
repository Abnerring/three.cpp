#include "misc/Gyroscope.h"

#include "math/Quaternion.h"
#include "math/Vector3.h"

namespace {
// The pinned module keeps the six decomposition scratch values at module scope.
Vector3& translationObject() { static thread_local Vector3 value; return value; }
Quaternion& quaternionObject() { static thread_local Quaternion value; return value; }
Vector3& scaleObject() { static thread_local Vector3 value; return value; }
Vector3& translationWorld() { static thread_local Vector3 value; return value; }
Quaternion& quaternionWorld() { static thread_local Quaternion value; return value; }
Vector3& scaleWorld() { static thread_local Vector3 value; return value; }
}  // namespace

Gyroscope::Gyroscope() = default;

void Gyroscope::updateMatrixWorld(bool force) {
    if (matrixAutoUpdate) updateMatrix();
    if (matrixWorldNeedsUpdate || force) {
        if (parent != nullptr) {
            // World translation and scale from the parent chain, rotation from the local matrix.
            matrixWorld.multiplyMatrices(parent->matrixWorld, matrix);
            matrixWorld.decompose(translationWorld(), quaternionWorld(), scaleWorld());
            matrix.decompose(translationObject(), quaternionObject(), scaleObject());
            matrixWorld.compose(translationWorld(), quaternionObject(), scaleWorld());
        } else {
            matrixWorld.copy(matrix);
        }
        matrixWorldNeedsUpdate = false;
        force = true;
    }
    for (const auto& child : children) {
        if (child) child->updateMatrixWorld(force);
    }
}
