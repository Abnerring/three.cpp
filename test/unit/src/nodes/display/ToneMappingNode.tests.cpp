#include "nodes/display/ToneMappingNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ToneMapping names itself and the no mapping case") {
    REQUIRE(std::string(ToneMapping::kTypeName) == "ToneMappingNode");
    REQUIRE(std::string(ToneMapping::kNodeType) == "vec3");
    REQUIRE(std::string(ToneMapping::kNoToneMappingName) == "NoToneMapping");
}

TEST_CASE("ToneMapping's cache key changes with the mapping") {
    // Two different mappings must not share a key, since they produce different shaders.
    REQUIRE_FALSE(ToneMapping::keysCollide(0, 1));
    REQUIRE_FALSE(ToneMapping::keysCollide(1, 2));
    REQUIRE_FALSE(ToneMapping::keysCollide(0, 5));
    // The same mapping gives the same key, so the cache still works for it.
    REQUIRE(ToneMapping::customCacheKey(3) == ToneMapping::customCacheKey(3));
    // And a sweep over the low numbers shows no pair collides.
    for (int a = 0; a < 12; ++a) {
        for (int b = a + 1; b < 12; ++b) {
            REQUIRE_FALSE(ToneMapping::keysCollide(a, b));
        }
    }
}

TEST_CASE("ToneMapping's hash does not keep neighbours together") {
    // Successive mappings give keys that are not successive, which is what the mixing is for.
    const auto first = ToneMapping::customCacheKey(0);
    const auto second = ToneMapping::customCacheKey(1);
    REQUIRE(second != first + 1);
    REQUIRE(second != first);
    // The keys are spread rather than following the input one for one.
    REQUIRE(ToneMapping::customCacheKey(2) != first + 2);
}

TEST_CASE("ToneMapping falls back for a falsy colour as well as an absent one") {
    // A colour that is there and truthy is used.
    REQUIRE(ToneMapping::usesGivenColour(true, true));
    // Absent, and there but falsy, both fall back; the short-circuiting or cannot tell them
    // apart, which is the thing worth pinning.
    REQUIRE_FALSE(ToneMapping::usesGivenColour(false, false));
    REQUIRE_FALSE(ToneMapping::usesGivenColour(true, false));
    REQUIRE(ToneMapping::usesGivenColour(true, true) != ToneMapping::usesGivenColour(true, false));
}