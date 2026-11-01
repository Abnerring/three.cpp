#include "loaders/FBXLoader.h"

bool FBXLoader::versionIsSupported(Encoding encoding, double version) {
    // Each encoding has its own scale.
    if (encoding == Encoding::Ascii) return version >= kMinimumAsciiVersion;
    return version >= static_cast<double>(kMinimumBinaryVersion);
}

int FBXLoader::moduleLevelStateNames() {
    // The tree, the connections and the graph, all written outside the class.
    return 3;
}

bool FBXLoader::isReentrant() {
    // A second load would overwrite the first one's state.
    return false;
}

bool FBXLoader::compressionLibraryIsBundled() {
    // A relative path, so the library ships with the code.
    return true;
}
