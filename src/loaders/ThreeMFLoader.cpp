#include "loaders/ThreeMFLoader.h"

std::string ThreeMFLoader::unitOf(const std::string& declared) {
    // An absent attribute reads as the empty string, which the format fills in with millimeters.
    if (declared.empty()) return kDefaultUnit;
    return declared;
}

bool ThreeMFLoader::unitIsApplied() {
    // The attribute is recorded and never read, so nothing in the loader scales by it.
    return false;
}