#include "loaders/ColladaLoader.h"

ColladaLoader::Outcome ColladaLoader::outcomeFor(bool textIsEmpty, bool parserFailed,
                                                  bool composerRan) {
    // An empty text short circuits to an empty scene.
    if (textIsEmpty) return Outcome::EmptyScene;
    // A parser that answered nothing stops the whole thing with nothing.
    if (parserFailed) return Outcome::Nothing;
    if (composerRan) return Outcome::Composed;
    return Outcome::Nothing;
}

bool ColladaLoader::emptyTextReachesTheParser() {
    // The length is checked first, so the parser never sees an empty text.
    return false;
}

std::string ColladaLoader::basePath(const std::string& loaderPath,
                                    const std::string& urlDirectory) {
    // The loader's own path unless it is the empty string.
    if (loaderPath.empty()) return urlDirectory;
    return loaderPath;
}

bool ColladaLoader::tgaGuardIsAlwaysTrue() {
    return true;
}
