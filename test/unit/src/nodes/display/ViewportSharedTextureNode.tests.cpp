#include "nodes/display/ViewportSharedTextureNode.h"

#include "nodes/core/Node.h"

#include "catch.hpp"

TEST_CASE("ViewportSharedTextureNode shares one framebuffer texture") {
    const auto module = nativeViewportSharedTextureNodeModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("ViewportSharedTextureNode"));
    // constructor( uvNode = screenUV, levelNode = null ) has both parameters defaulted.
    REQUIRE(ctor.get("length").strictEquals(0));
    REQUIRE(ctor.get("type").strictEquals("ViewportSharedTextureNode"));

    const auto first = ctor.construct();
    const auto second = ctor.construct();
    REQUIRE(first.get("type").strictEquals("ViewportSharedTextureNode"));
    // One module level framebuffer is shared by every instance.
    REQUIRE(first.get("getTextureForReference").call(first, {}).sameValue(
        second.get("getTextureForReference").call(second, {})));
    // updateReference answers the node itself.
    REQUIRE(first.get("updateReference").call(first, {}).sameValue(first));
}

TEST_CASE("ViewportSharedTextureNode exports the proxy factory") {
    const auto proxy = nativeViewportSharedTextureNodeBinding("viewportSharedTexture");
    REQUIRE(proxy.isCallable());
    // The factory takes no required parameter, so its length is zero.
    REQUIRE(proxy.get("length").strictEquals(0));
    const auto produced = proxy.call({}, {});
    REQUIRE(produced.get("getTextureForReference").isCallable());
    REQUIRE(produced.get("getTextureForReference").call(produced, {}).isNull() == false);
}
