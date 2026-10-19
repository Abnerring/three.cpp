// Differential probe for the primitive geometries: one scenario per stdin line, one JSON object per line
// back. Values are emitted as float32 bit patterns so the comparison is exact and cannot drift through
// decimal printing.
#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "geometries/BoxGeometry.h"
#include "geometries/PlaneGeometry.h"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace {
std::uint32_t bits(float value) {
    std::uint32_t raw = 0;
    std::memcpy(&raw, &value, sizeof(raw));
    return raw;
}

void emitAttribute(const std::shared_ptr<BufferGeometry>& geometry, const char* name, int itemSize) {
    const auto attribute = std::dynamic_pointer_cast<BufferAttribute>(geometry->getAttribute(name));
    if (!attribute) {
        std::cout << "\"" << name << "\":null";
        return;
    }
    const auto count = static_cast<int>(attribute->count);
    std::cout << "\"" << name << "\":{\"count\":" << count << ",\"itemSize\":" << itemSize << ",\"bits\":[";
    const int limit = count < 6 ? count : 6;
    for (int i = 0; i < limit; i += 1) {
        for (int c = 0; c < itemSize; c += 1) {
            if (i > 0 || c > 0) std::cout << ",";
            std::cout << bits(static_cast<float>(attribute->getComponent(i, c)));
        }
    }
    std::cout << "]}";
}
}

int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string kind;
        if (!(in >> kind)) continue;
        std::shared_ptr<BufferGeometry> geometry;
        if (kind == "box") {
            double width = 0, height = 0, depth = 0, ws = 0, hs = 0, ds = 0;
            in >> width >> height >> depth >> ws >> hs >> ds;
            geometry = std::make_shared<BoxGeometry>(width, height, depth, ws, hs, ds);
        } else if (kind == "plane") {
            double width = 0, height = 0, ws = 0, hs = 0;
            in >> width >> height >> ws >> hs;
            geometry = std::make_shared<PlaneGeometry>(width, height, ws, hs);
        } else {
            std::cout << "{\"error\":\"unknown kind\"}" << std::endl;
            continue;
        }
        std::cout << "{\"kind\":\"" << kind << "\",";
        emitAttribute(geometry, "position", 3);
        std::cout << ",";
        emitAttribute(geometry, "normal", 3);
        std::cout << ",";
        emitAttribute(geometry, "uv", 2);
        std::cout << ",";
        const auto index = geometry->getIndex();
        if (index) {
            std::cout << "\"index\":{\"count\":" << static_cast<int>(index->count) << ",\"first\":[";
            const int indexCount = static_cast<int>(index->count);
            const int limit = indexCount < 8 ? indexCount : 8;
            for (int i = 0; i < limit; i += 1) {
                if (i > 0) std::cout << ",";
                std::cout << static_cast<int>(index->getX(i));
            }
            std::cout << "]}";
        } else {
            std::cout << "\"index\":null";
        }
        std::cout << ",\"groups\":" << static_cast<int>(geometry->groups.size());
        std::cout << "}" << std::endl;
    }
    return 0;
}