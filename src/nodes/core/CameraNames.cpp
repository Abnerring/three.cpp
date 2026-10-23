#include "nodes/core/CameraNames.h"

bool CameraNames::indexHasPrefix() {
    // The index is the one name with a prefix; the near and far values go without.
    return std::string(kIndexName).rfind("u_", 0) == 0;
}

bool CameraNames::usesMatrixArray(bool isArrayCamera, std::size_t subCameraCount) {
    // Both conditions: an array camera that holds at least one of its own.
    return isArrayCamera && subCameraCount > 0;
}
