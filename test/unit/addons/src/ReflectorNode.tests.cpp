#include "nodes/utils/ReflectorNode.h"

#include "core/JSValue.h"

#include "catch.hpp"

TEST_CASE("ReflectorBaseNode applies the pinned options") {
    auto textureNode = JSValue::object(nullptr);
    const auto base = nativeReflectorBaseNodeConstructor().construct({textureNode});
    REQUIRE(base.get("textureNode").strictEquals(textureNode));
    REQUIRE(base.get("resolutionScale").toNumber() == Approx(1));
    REQUIRE_FALSE(base.get("generateMipmaps").toBoolean());
    REQUIRE(base.get("bounces").toBoolean());
    REQUIRE_FALSE(base.get("depth").toBoolean());
    REQUIRE(base.get("samples").toNumber() == Approx(0));
    REQUIRE(base.get("target").toBoolean());
    REQUIRE(base.get("virtualCameras").toBoolean());
    REQUIRE(base.get("renderTargets").toBoolean());
    REQUIRE_FALSE(base.get("isCacheable").call(base, {}).toBoolean());
}