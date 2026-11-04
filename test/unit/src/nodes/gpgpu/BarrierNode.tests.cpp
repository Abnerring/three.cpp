#include "nodes/gpgpu/BarrierNode.h"

#include "catch.hpp"

TEST_CASE("BarrierNode exports the three synchronization scopes") {
    nativeBarrierNodeModule();
    const auto barrier = nativeBarrierNodeConstructor();
    for (const auto* scope : {"workgroup", "storage", "texture"}) {
        const auto node = barrier.construct({scope});
        REQUIRE(node.get("type").strictEquals("Node"));
        REQUIRE(node.get("scope").strictEquals(scope));
        REQUIRE(node.get("isBarrierNode").strictEquals(true));
        REQUIRE(node.get("isCacheable").call(node).strictEquals(false));
    }
}

TEST_CASE("BarrierNode convenience functions return stack nodes") {
    nativeBarrierNodeModule();
    for (const auto* name : {"workgroupBarrier", "storageBarrier", "textureBarrier"}) {
        const auto stack = nativeBarrierNodeBinding(name).call();
        REQUIRE(stack.get("type").strictEquals("Node"));
        REQUIRE(stack.get("isNode").strictEquals(true));
    }
}

TEST_CASE("BarrierNode setup disables early returns and globals") {
    const auto node = nativeBarrierNodeConstructor().construct({"storage"});
    auto builder = JSValue::object();
    node.get("setup").call(node, {builder});
    REQUIRE(builder.get("allowEarlyReturns").strictEquals(false));
    REQUIRE(builder.get("allowGlobalVariables").strictEquals(false));
}

