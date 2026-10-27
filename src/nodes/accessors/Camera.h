#pragma once
#include "core/JSValue.h"
#include <string>

JSValue nativeCameraModule();
JSValue nativeCameraBinding(const std::string& name);

// Compatibility helpers for names and selection rules in Camera.js.
class CameraNames {
  public:
    static constexpr const char* kIndexName = "u_cameraIndex";
    static constexpr const char* kNearName = "cameraNear";
    static constexpr const char* kFarName = "cameraFar";
    static constexpr const char* kProjectionMatrixArrayName = "cameraProjectionMatrices";
    static constexpr const char* kIndexType = "uint";
    static bool indexHasPrefix();
    static bool usesMatrixArray(bool isArrayCamera, std::size_t subCameraCount);
};
