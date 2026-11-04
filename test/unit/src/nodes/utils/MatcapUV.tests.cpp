#include "nodes/utils/MatcapUV.h"

#include "catch.hpp"

TEST_CASE("matcapUV exports the pinned node") {
    const auto module = nativeMatcapUVModule();
    const auto node = nativeMatcapUV();
    REQUIRE(module.get("matcapUV").sameValue(node));
    REQUIRE(node.get("type").strictEquals("VarNode"));
    REQUIRE(node.get("node").get("type").strictEquals("VarNode"));
}
