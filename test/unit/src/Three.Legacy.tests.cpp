#include "Three.Legacy.h"

#include "catch.hpp"

TEST_CASE("Three.Legacy is an empty module at the pinned revision") {
    const auto module = nativeThreeLegacyModule();
    REQUIRE(module.isObject());
    // The upstream file has no statements at all, so the module answers no names.
    REQUIRE(module.ownPropertyNames().empty());
    REQUIRE(module.get("Object").isUndefined());
    REQUIRE(module.get("BufferGeometry").isUndefined());
}
