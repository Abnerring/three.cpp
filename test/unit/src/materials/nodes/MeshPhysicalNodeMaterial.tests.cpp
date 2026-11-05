#include "../../../utils/parity/MeshPhysicalNodeMaterialFixtures.h"
#include "catch.hpp"

using namespace MeshStandardNodeMaterialFixtures;
TEST_CASE("MeshPhysicalNodeMaterial preserves feature truthiness and strict null checks") {
    const auto result = MeshPhysicalNodeMaterialFixtures::features(record({{"value", 0}, {"node", "@undefined"}}));
    REQUIRE(result.length() == 8);
    for (std::size_t i = 0; i < result.length(); ++i) {
        REQUIRE(result.get(JSValue(i)).get("result").strictEquals(true));
        REQUIRE(result.get(JSValue(i)).get("error").isNull());
    }
    const auto positive = MeshPhysicalNodeMaterialFixtures::features(record({{"value", 0.5}, {"node", nullptr}}));
    for (std::size_t i = 0; i < positive.length(); ++i)
        REQUIRE(positive.get(JSValue(i)).get("events").length() == 1);
}
TEST_CASE("MeshPhysicalNodeMaterial builds all physical variant assignments and deferred branches") {
    auto values = record({{"diffuseRoughness", 0.2}, {"clearcoat", 0.3}, {"sheen", 0.4},
        {"retroreflectivity", 0.5}, {"iridescence", 0.6}, {"anisotropy", 0.7},
        {"transmission", 0.8}, {"dispersion", 0.9}});
    const auto result = MeshPhysicalNodeMaterialFixtures::variants(record({{"values", values}, {"branches", true}, {"observe", true}}));
    REQUIRE(result.get("error").isNull());
    REQUIRE(result.get("returnsUndefined").strictEquals(true));
    REQUIRE(result.get("graph").get("objects").length() > 50);
    REQUIRE(result.get("events").length() > 20);
}
TEST_CASE("MeshPhysicalNodeMaterial installs a lexical clearcoat normal context before super setup") {
    const auto result = MeshPhysicalNodeMaterialFixtures::setupCase(record({{"superThrows", true}}));
    REQUIRE(result.get("error").strictEquals("TypeError"));
    REQUIRE(result.get("callbackError").isNull());
    REQUIRE(result.get("callbackLength").strictEquals(0));
    REQUIRE(result.get("events").length() == 2);
}
