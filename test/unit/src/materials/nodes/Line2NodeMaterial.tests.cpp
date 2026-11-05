#include "materials/nodes/Line2NodeMaterial.h"
#include "../../../utils/parity/Line2NodeMaterialFixtures.h"
#include "catch.hpp"

namespace {
namespace F = Line2NodeMaterialFixtures;
JSValue field(const JSValue& graph, const char* name) {
    const auto root = graph.get("root").get("1");
    const auto props = graph.get("objects").get(root).get("properties");
    for (std::size_t i = 0; i < props.length(); ++i) {
        const auto pair = props.get(JSValue(i));
        if (pair.get("0").strictEquals(name)) return pair.get("1");
    }
    throw std::logic_error("Missing graph field");
}
std::size_t arrayLength(const JSValue& graph, const char* name) {
    const auto ref = field(graph, name);
    return static_cast<std::size_t>(graph.get("objects").get(ref.get("1")).get("length").toNumber());
}
}

TEST_CASE("Line2NodeMaterial preserves defaults and its complete native export") {
    const auto ctor = nativeLine2NodeMaterialConstructor(), module = nativeLine2NodeMaterialModule();
    REQUIRE(module.get("default").sameValue(ctor));
    REQUIRE(nativeLine2NodeMaterialBinding("mvpLine").isUndefined());
    REQUIRE(ctor.get("length").strictEquals(0));
    const auto material = ctor.construct();
    REQUIRE(material.get("isLine2NodeMaterial").strictEquals(true));
    REQUIRE(material.get("vertexColors").isUndefined());
    REQUIRE(material.get("dashed").isUndefined());
    REQUIRE(material.get("worldUnits").strictEquals(false));
    REQUIRE(material.get("alphaToCoverage").strictEquals(true));
    REQUIRE(material.get("blending").strictEquals(0));
    REQUIRE(material.get("dashOffset").strictEquals(0));
    REQUIRE(material.get("dashSize").strictEquals(3));
    REQUIRE(material.get("gapSize").strictEquals(1));
    for (const char* key : {"offsetNode", "dashScaleNode", "dashSizeNode", "gapSizeNode"}) REQUIRE(material.get(key).isNull());
    REQUIRE_THROWS_AS(ctor.construct({nullptr}), JSTypeError);
    const auto meta = F::metadata();
    REQUIRE(meta.get("base").strictEquals(true));
    REQUIRE(meta.get("prototypeBase").strictEquals(true));
    REQUIRE(meta.get("descriptors").length() == 6);
    REQUIRE(meta.get("helpers").get("trimSegmentAlpha").get("nodeType").strictEquals("float"));
    REQUIRE(meta.get("helpers").get("closestLineToLine").get("layout").length() == 4);
}

TEST_CASE("Line2 feature setters preserve raw identity and invalidate only on strict changes") {
    auto material = nativeLine2NodeMaterialConstructor().construct();
    for (const char* key : {"worldUnits", "dashed", "alphaToCoverage"}) {
        const auto original = material.get(key);
        const auto version = material.get("version").toNumber();
        material.set(key, original);
        REQUIRE(material.get("version").toNumber() == version);
        auto raw = JSValue::object();
        material.set(key, raw);
        REQUIRE(material.get(key).sameValue(raw));
        REQUIRE(material.get("version").toNumber() == version + 1);
        material.set(key, raw);
        REQUIRE(material.get("version").toNumber() == version + 1);
        material.set(key, std::numeric_limits<double>::quiet_NaN());
        material.set(key, std::numeric_limits<double>::quiet_NaN());
        REQUIRE(material.get("version").toNumber() == version + 3);
    }
    const auto stopped = F::accessor(F::record({{"key", "worldUnits"}, {"initial", false}, {"next", true}, {"throwAt", "_useWorldUnits"}}));
    REQUIRE(arrayLength(stopped, "events") == 3);
    REQUIRE(field(stopped, "after").strictEquals(false));
}

TEST_CASE("Line2 MVP constructs every perspective trimming and endcap branch") {
    for (const bool dashed : {false, true}) for (const bool world : {false, true}) {
        const auto graph = F::graphCase(F::record({{"helper", "mvpLine"}, {"_useDash", dashed}, {"_useWorldUnits", world},
            {"dashScaleNode", "@node"}, {"offsetNode", "@node"}}));
        REQUIRE(arrayLength(graph, "branches") == (world ? (dashed ? 3 : 4) : 5));
        REQUIRE(graph.get("objects").length() > 190);
        const auto json = graph.toJSON()->dump();
        REQUIRE(json.find("trimSegmentAlpha") != std::string::npos);
        REQUIRE(json.find("start.z.lessThan") != std::string::npos);
        if (dashed) REQUIRE(json.find("distanceEnd.assign") != std::string::npos);
    }
}

TEST_CASE("Line2 alpha graph executes orthographic perspective coverage and discard paths") {
    for (const bool dashed : {false, true}) for (const bool world : {false, true})
        for (const bool coverage : {false, true}) for (const int samples : {0, 4}) {
            const auto graph = F::graphCase(F::record({{"helper", "alphaLine"}, {"_useDash", dashed}, {"_useWorldUnits", world},
                {"_useAlphaToCoverage", coverage}, {"samples", samples}, {"dashSizeNode", "@node"}, {"gapSizeNode", "@node"}}));
            REQUIRE(arrayLength(graph, "branches") == (world ? 2 : 1));
            REQUIRE(graph.get("objects").length() > 30);
            const auto json = graph.toJSON()->dump();
            REQUIRE(json.find("nativeError") == std::string::npos);
            if (world) REQUIRE(json.find("closestLineToLine") != std::string::npos);
        }
}

TEST_CASE("Line2 setup preserves previous-position writes and inherited exceptions") {
    const auto previous = F::setupCase(F::record({{"method", "setupPosition"}, {"previous", true}}));
    const auto current = F::setupCase(F::record({{"method", "setupPosition"}, {"previous", false}}));
    REQUIRE(arrayLength(previous, "nodes") > arrayLength(current, "nodes"));
    REQUIRE(arrayLength(previous, "events") == 2);
    const auto failed = F::setupCase(F::record({{"method", "setupPosition"}, {"previous", true}, {"throwAt", "needsPreviousData"}}));
    REQUIRE(arrayLength(failed, "events") == 1);
    const auto diffuse = F::setupCase(F::record({{"method", "setupDiffuseColor"}, {"vertexColors", true}, {"attribute", true}, {"transparent", true}}));
    REQUIRE(arrayLength(diffuse, "nodes") > 2);
    REQUIRE(arrayLength(diffuse, "events") == 2);
    const auto stopped = F::setupCase(F::record({{"method", "setupDiffuseColor"}, {"vertexColors", true}, {"attribute", true}, {"throwAt", "super"}}));
    REQUIRE(arrayLength(stopped, "nodes") == 0);
}
