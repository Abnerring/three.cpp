#include "loaders/AMFLoader.h"

#include <cctype>

bool AMFLoader::isZip(const std::vector<std::uint8_t>& buffer) {
    // The archive signature is the two characters PK.
    return buffer.size() >= 2 && buffer[0] == 0x50 && buffer[1] == 0x4b;
}

double AMFLoader::unitScale(const std::string& unit) {
    std::string lowered = unit;
    for (auto& c : lowered) c = char(std::tolower(static_cast<unsigned char>(c)));

    // Every scale is expressed in millimetres, which is the unit the coordinates use.
    if (lowered == "millimeter") return 1.0;
    if (lowered == "inch") return 25.4;
    if (lowered == "feet") return 304.8;
    if (lowered == "meter") return 1000.0;
    if (lowered == "micron") return 0.001;
    // An unknown unit leaves the coordinates as they are.
    return 1.0;
}
