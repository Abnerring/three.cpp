#include "loaders/ThreeMFLoader.h"

std::string ThreeMFLoader::unitOf(const std::string& declared) {
    // An absent attribute reads as the empty string, which the format fills in with millimeters.
    if (declared.empty()) return kDefaultUnit;
    return declared;
}
