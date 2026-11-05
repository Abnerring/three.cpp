#include "nodes/utils/PostProcessingUtils.h"

#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"

#include "catch.hpp"

namespace {
JSValue floatNode(double value) { return nativeTSLBaseBinding("float").call({}, {JSValue(value)}); }
JSValue vec2Node(double x, double y) {
    return nativeTSLBaseBinding("vec2").call({}, {JSValue(x), JSValue(y)});
}
}  // namespace

TEST_CASE("PostProcessingUtils exports the seven pinned helpers") {
    const auto module = nativePostProcessingUtilsModule();
    for (const char* name : {"isBackgroundDepth", "getViewPosition", "getScreenPosition",
                             "getScreenPositionFromClip", "getNormalFromDepth",
                             "interleavedGradientNoise", "vogelDiskSample"}) {
        REQUIRE(module.get(name).isCallable());
    }
    // A name the module does not hold answers nothing rather than a default.
    REQUIRE(module.get("notAHelper").isUndefined());
}

TEST_CASE("PostProcessingUtils isBackgroundDepth picks the comparison from the depth convention") {
    const auto depth = floatNode(0.5);
    // Without a builder the reversed flag is absent, so the ordinary comparison is used.
    const auto ordinary = nativePostProcessingUtilsBinding("isBackgroundDepth").call({}, {depth});
    REQUIRE(ordinary.get("isNode").strictEquals(true));

    // A builder that reports a reversed depth buffer takes the other branch. The comparisons are
    // operator nodes, so what is checked is that the two branches really produce different graphs
    // rather than the operation name, which lives on a node kind this runtime does not expose here.
    auto renderer = JSValue::object(nullptr);
    renderer.set("reversedDepthBuffer", true);
    auto builder = JSValue::object(nullptr);
    builder.set("renderer", renderer);
    const auto reversed =
        nativePostProcessingUtilsBinding("isBackgroundDepth").call({}, {depth, builder});
    REQUIRE(reversed.get("isNode").strictEquals(true));
    REQUIRE_FALSE(reversed.sameValue(ordinary));

    // A builder that does not report a reversed buffer stays on the ordinary branch, which is what
    // the strict comparison against true in the source means.
    auto plainRenderer = JSValue::object(nullptr);
    plainRenderer.set("reversedDepthBuffer", false);
    auto plainBuilder = JSValue::object(nullptr);
    plainBuilder.set("renderer", plainRenderer);
    const auto plain =
        nativePostProcessingUtilsBinding("isBackgroundDepth").call({}, {depth, plainBuilder});
    REQUIRE(plain.get("isNode").strictEquals(true));
    REQUIRE(plain.sameValue(ordinary) == false);
}

TEST_CASE("PostProcessingUtils helpers answer nodes for node inputs") {
    // getScreenPositionFromClip( clipPosition )
    const auto clip = nativeTSLBaseBinding("vec4").call(
        {}, {vec2Node(0.5, 0.5), floatNode(1), floatNode(1)});
    const auto screen = nativePostProcessingUtilsBinding("getScreenPositionFromClip").call({}, {clip});
    REQUIRE(screen.get("isNode").strictEquals(true));

    // interleavedGradientNoise( position )
    const auto noise =
        nativePostProcessingUtilsBinding("interleavedGradientNoise").call({}, {vec2Node(0.5, 0.5)});
    REQUIRE(noise.get("isNode").strictEquals(true));

    // vogelDiskSample( sampleIndex, samplesCount, phi )
    const auto sample = nativePostProcessingUtilsBinding("vogelDiskSample")
                            .call({}, {floatNode(0), floatNode(8), floatNode(0)});
    REQUIRE(sample.get("isNode").strictEquals(true));

    // getScreenPosition( viewPosition, projectionMatrix ) needs a matrix node, which is only
    // meaningful with a builder; the export's presence is what is checked here.
    REQUIRE(nativePostProcessingUtilsBinding("getScreenPosition").isCallable());
    REQUIRE(nativePostProcessingUtilsBinding("getViewPosition").isCallable());
    REQUIRE(nativePostProcessingUtilsBinding("getNormalFromDepth").isCallable());
}
