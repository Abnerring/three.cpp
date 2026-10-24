#include "loaders/MaterialXLoader.h"

std::string MaterialXLoader::resourcePath(const std::string& loaderPath, const std::string& url) {
    // A value of any kind wins, not only a non-empty string.
    if (!loaderPath.empty()) return loaderPath;
    const auto slash = url.find_last_of('/');
    if (slash == std::string::npos) return std::string();
    // The directory keeps its trailing slash.
    return url.substr(0, slash + 1);
}

MaterialXLoader::DisposeEffect MaterialXLoader::disposeEffect(bool hasDisposer) {
    // The disposer is run and then forgotten, so a second call does nothing.
    return hasDisposer ? DisposeEffect::RanAndForgot : DisposeEffect::Nothing;
}

bool MaterialXLoader::reportsStart() {
    return true;
}

int MaterialXLoader::managerCallsOnFailure() {
    // The failure and then the end.
    return 2;
}

bool MaterialXLoader::errorBeforeEnd() {
    return true;
}
