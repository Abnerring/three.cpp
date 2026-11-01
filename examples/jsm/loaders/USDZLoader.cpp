#include "loaders/USDZLoader.h"

std::string USDZLoader::deprecationWarning() {
    return "USDZLoader has been deprecated. Please use USDLoader instead.";
}

bool USDZLoader::warnsBeforeParentInitialises() {
    // The message is printed before the parent is called.
    return true;
}

bool USDZLoader::parsesAnythingItself() {
    return false;
}
