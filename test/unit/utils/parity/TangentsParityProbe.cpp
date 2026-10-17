// Differential probe for the native BufferGeometry.computeTangents port.
//
// Each stdin line is a geometry document in the same shape BufferGeometryLoader reads. The document is
// parsed, the tangents are computed, and the resulting attribute is printed as float32 bit patterns -
// bits rather than numbers, because a tangent that differs in the last place is still a difference.
//
// Documents that omit the index, the positions, the normals or the uvs are part of the run: upstream
// reports the missing attributes and returns without touching the geometry, which is what should be seen.
#include "animation/AnimationUtils.h"
#include "core/BufferAttribute.h"
#include "core/JSValue.h"
#include "loaders/BufferGeometryLoader.h"
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

int main() {
    const auto realm = JSValue::object();
    static_cast<void>(realm);
    std::string line;
    while (std::getline(std::cin, line)) {
        try {
            BufferGeometryLoader loader;
            const auto geometry = loader.parseGeometry(Animation_detail::parseJSON(line));
            geometry->computeTangents();
            const auto tangent = std::dynamic_pointer_cast<BufferAttribute>(geometry->getAttribute("tangent"));
            if (!tangent) {
                std::cout << JsonValue(JsonValue::Object{{"present", false}}).dump() << '\n';
                continue;
            }
            std::string bits;
            const auto vertices = static_cast<int>(tangent->count);
            for (int vertex = 0; vertex < vertices; ++vertex) {
                const double components[4] = {tangent->getX(vertex), tangent->getY(vertex),
                                              tangent->getZ(vertex), tangent->getW(vertex)};
                for (int component = 0; component < 4; ++component) {
                    const float value = static_cast<float>(components[component]);
                    std::uint32_t word = 0;
                    std::memcpy(&word, &value, sizeof(word));
                    char hex[16];
                    std::snprintf(hex, sizeof(hex), "%08x", word);
                    bits += hex;
                    if (vertex + 1 < vertices || component < 3) bits += ' ';
                }
            }
            std::cout << JsonValue(JsonValue::Object{{"present", true}, {"count", tangent->count},
                {"itemSize", tangent->itemSize}, {"bits", bits}}).dump() << '\n';
        } catch (const std::exception& error) {
            std::cout << JsonValue(JsonValue::Object{{"probeError", error.what()}}).dump() << '\n';
        }
    }
    return 0;
}