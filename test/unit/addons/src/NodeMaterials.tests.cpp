#include "materials/nodes/NodeMaterials.h"

#include "catch.hpp"

// The aggregator is a re-export map, so the cases check the map contents rather than behaviour.
TEST_CASE("NodeMaterials re-exports every ported node material") {
    const auto module = nativeNodeMaterialsModule();
    for (const char* name : {"NodeMaterialObserver", "NodeMaterial", "ProxyNodeMaterial",
                             "LineBasicNodeMaterial", "LineDashedNodeMaterial",
                             "MeshNormalNodeMaterial", "MeshBasicNodeMaterial",
                             "MeshStandardNodeMaterial", "MeshPhysicalNodeMaterial", "MeshSSSNodeMaterial",
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
    REQUIRE_FALSE(nativeNodeMaterialsBinding("MeshStandardNodeMaterial").isUndefined());
    REQUIRE_FALSE(nativeNodeMaterialsBinding("MeshPhysicalNodeMaterial").isUndefined());
    REQUIRE_FALSE(nativeNodeMaterialsBinding("MeshSSSNodeMaterial").isUndefined());
    REQUIRE(nativeNodeMaterialsBinding("notANodeMaterial").isUndefined());
}
