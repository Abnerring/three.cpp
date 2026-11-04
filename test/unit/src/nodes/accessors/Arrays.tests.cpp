#include "nodes/accessors/Arrays.h"

#include "catch.hpp"

TEST_CASE("Arrays exports the storage array factories") {
    const auto module = nativeArraysModule();
    REQUIRE(module.get("attributeArray").isCallable());
    REQUIRE(module.get("instancedArray").isCallable());
}

TEST_CASE("Arrays creates storage buffers with the requested dimensions") {
    const auto module = nativeArraysModule();
    const auto attribute = module.get("attributeArray").call({}, {5, "vec3"});
    REQUIRE(attribute.get("type").strictEquals("StorageBufferNode"));
    REQUIRE(attribute.get("isStorageBufferNode").strictEquals(true));
    REQUIRE(attribute.get("getInputType").call(attribute).strictEquals("storageBuffer"));
    REQUIRE(attribute.get("bufferCount").strictEquals(5));
    REQUIRE(attribute.get("nodeType").strictEquals("vec3"));
    REQUIRE(attribute.get("value").get("isStorageBufferAttribute").strictEquals(true));
    REQUIRE(attribute.get("value").get("itemSize").strictEquals(3));
    REQUIRE(attribute.get("value").get("count").strictEquals(5));
    REQUIRE(attribute.get("value").get("array").get("length").strictEquals(15));

    const auto instanced = module.get("instancedArray").call({}, {2, "vec4"});
    REQUIRE(instanced.get("value").get("isStorageInstancedBufferAttribute").strictEquals(true));
    REQUIRE(instanced.get("bufferCount").strictEquals(2));
    REQUIRE(instanced.get("value").get("itemSize").strictEquals(4));
    REQUIRE(instanced.get("value").get("array").get("length").strictEquals(8));
}

