#pragma once
#include <string>

// Port of the names and the conditions in src/nodes/accessors/Camera.js at the pinned revision.
// The module builds a family of uniforms read from the camera in use; what is portable here is
// how they are named and when the array form is chosen.
class CameraNames {
  public:
    // The uniforms the module names. Most carry the plain name of what they read, while the
    // index is the one that takes a prefix, which is the kind of difference worth writing down
    // rather than smoothing over.
    static constexpr const char* kIndexName = "u_cameraIndex";
    static constexpr const char* kNearName = "cameraNear";
    static constexpr const char* kFarName = "cameraFar";
    static constexpr const char* kProjectionMatrixArrayName = "cameraProjectionMatrices";

    // Whether the index name carries the prefix the others do not.
    static bool indexHasPrefix();

    // Whether a camera needs the array form of its matrices. It does when it is an array camera
    // and holds at least one of its own, so an array camera holding none falls back to the
    // single form rather than to an empty array.
    static bool usesMatrixArray(bool isArrayCamera, std::size_t subCameraCount);

    // The type the index uniform is made with, which is an unsigned integer rather than the
    // float most of the family uses.
    static constexpr const char* kIndexType = "uint";
};
