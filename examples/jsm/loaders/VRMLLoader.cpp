#include "loaders/VRMLLoader.h"

bool VRMLLoader::parserLibraryIsBundled() {
    // A relative path inside the addons set, so the library ships with the code.
    return true;
}

int VRMLLoader::managerCallsOnFailure() {
    // The failure alone, not the end.
    return 1;
}

std::string VRMLLoader::basePath(const std::string& loaderPath,
                                 const std::string& urlDirectory) {
    if (loaderPath.empty()) return urlDirectory;
    return loaderPath;
}
