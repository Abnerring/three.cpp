#include "loaders/ThreeDMLoader.h"

bool ThreeDMLoader::libraryIsBundled() {
    // The example names a content delivery address, so the library is fetched when the page runs.
    return false;
}

bool ThreeDMLoader::subdivisionIsOff(int level) {
    // Nought subdivisions leaves the surface as it is.
    return level <= 0;
}

bool ThreeDMLoader::geometryLoaderIsShared() {
    return true;
}

bool ThreeDMLoader::libraryPathStartsEmpty() {
    return true;
}
