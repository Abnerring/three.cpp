#include "materials/nodes/NodeMaterials.h"

#include "catch.hpp"

// The aggregator is a re-export map, so the cases check the map contents rather than behaviour.
TEST_CASE("NodeMaterials re-exports every ported node material") {
    const auto module = nativeNodeMaterialsModule();
    for (const char* name : {"NodeMaterialObserver", "NodeMaterial", "ProxyNodeMaterial",
                             "LineBasicNodeMaterial", "LineDashedNodeMaterial",
                             "MeshNormalNodeMaterial", "MeshBasicNodeMaterial",
                             "MeshLambertNodeMaterial", "MeshPhongNodeMaterial",
                             "MeshToonNodeMaterial", "MeshMatcapNodeMaterial", "PointsNodeMaterial",
                             "SpriteNodeMaterial", "ShadowNodeMaterial", "VolumeNodeMaterial"}) {
        REQUIRE_FALSE(module.get(name).isUndefined());
    }
}

TEST_CASE("NodeMaterials binding resolves the exported constructors") {
    const auto basic = nativeNodeMaterialsBinding("MeshBasicNodeMaterial");
    REQUIRE_FALSE(basic.isUndefined());
    REQUIRE(basic.strictEquals(nativeNodeMaterialsModule().get("MeshBasicNodeMaterial")));
    // A class that is still missing from the map answers undefined instead of pretending to exist.
    REQUIRE(nativeNodeMaterialsBinding("MeshStandardNodeMaterial").isUndefined());
    REQUIRE(nativeNodeMaterialsBinding("notANodeMaterial").isUndefined());
}
