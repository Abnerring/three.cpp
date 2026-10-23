#include "loaders/TTFLoader.h"

bool TTFLoader::dependencyIsBundled() {
    // The import names an address, so the library is fetched when the module runs.
    return false;
}

bool TTFLoader::orderComesFromTheFile() {
    return true;
}

bool TTFLoader::reportsToManagerAlways(bool hasErrorHandler) {
    // The manager is told in both branches; only the caller's handler depends on the caller.
    (void)hasErrorHandler;
    return true;
}

bool TTFLoader::keepsTheBuffer() {
    return false;
}
