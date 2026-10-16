#pragma once
#include "cameras/PerspectiveCamera.h"
#include "math/Quaternion.h"
#include "math/Vector3.h"

// Native port of examples/jsm/utils/CameraUtils.js from the pinned three.js revision
// (see upstream.json). The module exports `frameCorners`, which places a camera so that its
// near plane passes through three corner points and builds the matching off-axis projection
// matrix.
//
// The pinned function reads `camera.near`, `camera.far`, `camera.fov` and `camera.aspect`
// through JS duck typing; in C++ those properties belong to PerspectiveCamera, so the native
// port takes the concrete type. That is the only signature adaptation.
//
// The JavaScript module holds its scratch vectors in module scope:
//
//     const _va = new Vector3(), _vb = ..., _vc = ..., _vr = ..., _vu = ..., _vn = ...,
//           _vec = new Vector3(), _quat = new Quaternion();
//
// so the native port keeps that state as one thread-local block, following the
// module-temporary rule in docs/PORTING.md. `estimateViewFrustum` defaults to false upstream.
namespace CameraUtils {
void frameCorners(PerspectiveCamera& camera, const Vector3& bottomLeftCorner, const Vector3& bottomRightCorner,
    const Vector3& topLeftCorner, bool estimateViewFrustum = false);
}