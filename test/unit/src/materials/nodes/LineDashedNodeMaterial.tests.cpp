#include "materials/nodes/LineDashedNodeMaterial.h"

#include "materials/nodes/NodeMaterial.h"
#include "nodes/core/Node.h"

#include "catch.hpp"

TEST_CASE("LineDashedNodeMaterial applies the dashed defaults and its own fields") {
    const auto module = nativeLineDashedNodeMaterialModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("LineDashedNodeMaterial"));
    REQUIRE(ctor.get("length").strictEquals(1));
    REQUIRE(ctor.get("type").strictEquals("LineDashedNodeMaterial"));

    const auto material = ctor.construct();
    REQUIRE(material.get("type").strictEquals("LineDashedNodeMaterial"));
    REQUIRE(material.get("isLineDashedNodeMaterial").strictEquals(true));
    REQUIRE(material.get("isNodeMaterial").strictEquals(true));
    // The constructor writes the dash offset and the four node overrides after the defaults are applied.
    REQUIRE(material.get("dashOffset").toNumber() == Approx(0));
    for (const char* field : {"offsetNode", "dashScaleNode", "dashSizeNode", "gapSizeNode"}) {
        REQUIRE(material.get(field).isNull());
    }
    // The line dashed material defaults dashSize to three and gapSize to one.
    REQUIRE(material.get("dashSize").toNumber() == Approx(3));
    REQUIRE(material.get("gapSize").toNumber() == Approx(1));
    // The builder parameter is commented out in the source, so the length is zero.
    REQUIRE(material.get("setupVariants").get("length").strictEquals(0));
}

TEST_CASE("LineDashedNodeMaterial setupVariants builds the dashed discard graph") {
    const auto material = nativeLineDashedNodeMaterialConstructor().construct();
    auto builder = JSValue::object(nullptr);
    // The graph uses the material accessors when no override node is set, so an empty builder is enough
    // for it to build.
    REQUIRE_NOTHROW(material.get("setupVariants").call(material, {builder}));
}
