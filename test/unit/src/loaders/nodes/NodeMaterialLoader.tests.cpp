#include "loaders/nodes/NodeMaterialLoader.h"

#include "nodes/core/Node.h"

#include "catch.hpp"

namespace {
JSValue materialStub() {
    return JSValue::constructor(
        +[](const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue {
            auto material = JSValue::object(nullptr);
            material.set("type", JSValue("StubNodeMaterial"));
            material.set("isMaterial", true);
            material.set("isNodeMaterial", true);
            return material;
        },
        {}, "StubNodeMaterial", 0);
}

JSValue nodeStub() {
    auto node = JSValue::object(nullptr);
    node.set("isNode", true);
    node.set("type", JSValue("StubNode"));
    return node;
}

bool throwsFor(const JSValue& loader, const char* type) {
    try {
        loader.get("createMaterialFromType").call(loader, {JSValue(type)});
        return false;
    } catch (...) {
        return true;
    }
}
}  // namespace

TEST_CASE("NodeMaterialLoader keeps the node and material registries") {
    const auto module = nativeNodeMaterialLoaderModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("NodeMaterialLoader"));
    REQUIRE(ctor.get("length").strictEquals(1));

    const auto loader = ctor.construct();
    REQUIRE(loader.get("nodes").isObject());
    REQUIRE(loader.get("nodeMaterials").isObject());
    REQUIRE(loader.get("parse").get("length").strictEquals(1));
    REQUIRE(loader.get("setNodes").get("length").strictEquals(1));
    REQUIRE(loader.get("setNodeMaterials").get("length").strictEquals(1));
    REQUIRE(loader.get("createMaterialFromType").get("length").strictEquals(1));

    // Both setters answer the loader and store the registry.
    auto nodes = JSValue::object(nullptr);
    REQUIRE(loader.get("setNodes").call(loader, {nodes}).sameValue(loader));
    REQUIRE(loader.get("nodes").sameValue(nodes));
    auto materials = JSValue::object(nullptr);
    REQUIRE(loader.get("setNodeMaterials").call(loader, {materials}).sameValue(loader));
    REQUIRE(loader.get("nodeMaterials").sameValue(materials));
}

TEST_CASE("NodeMaterialLoader prefers the node material registry") {
    const auto loader = nativeNodeMaterialLoaderConstructor().construct();
    auto registry = JSValue::object(nullptr);
    registry.set("StubNodeMaterial", materialStub());
    loader.get("setNodeMaterials").call(loader, {registry});

    const auto created = loader.get("createMaterialFromType").call(loader, {JSValue("StubNodeMaterial")});
    REQUIRE(created.get("type").strictEquals("StubNodeMaterial"));
    REQUIRE(created.get("isNodeMaterial").strictEquals(true));

    // An unregistered type is delegated to the base material loader, which reports an error and answers a
    // plain Material instead of raising, exactly like the pinned loader.
    JSValue fallback;
    REQUIRE_NOTHROW(fallback = loader.get("createMaterialFromType").call(loader, {JSValue("DefinitelyNotAMaterialType")}));
    REQUIRE(fallback.get("isMaterial").strictEquals(true));
    REQUIRE(fallback.get("type").strictEquals("Material"));
}

TEST_CASE("NodeMaterialLoader parse assigns the input nodes to the material") {
    const auto loader = nativeNodeMaterialLoaderConstructor().construct();
    auto childNode = JSValue::object(nullptr);
    childNode.set("isNode", true);
    childNode.set("type", JSValue("StubNode"));
    auto nodes = JSValue::object(nullptr);
    nodes.set("node-uuid", childNode);
    loader.get("setNodes").call(loader, {nodes});

    auto inputNodes = JSValue::object(nullptr);
    inputNodes.set("colorNode", JSValue("node-uuid"));
    auto json = JSValue::object(nullptr);
    json.set("type", JSValue("MeshBasicMaterial"));
    json.set("uuid", JSValue("material-uuid"));
    json.set("inputNodes", inputNodes);

    JSValue material;
    REQUIRE_NOTHROW(material = loader.get("parse").call(loader, {json}));
    REQUIRE(material.get("isMaterial").strictEquals(true));
    // The input node was resolved through the registry by uuid.
    REQUIRE(material.get("colorNode").sameValue(childNode));
}

#include "../../../utils/parity/LoaderSemanticsFixtures.h"

TEST_CASE("NodeMaterialLoader resolves inherited inputs while own hidden names shadow prototypes", "[loaders][node-material-loader]") {
    namespace F=LoaderSemanticsFixtures;
    const auto result=F::run(F::request("forIn"));REQUIRE(result.get("result").strictEquals(true));REQUIRE(result.get("assigned").get("own").strictEquals("node:a"));REQUIRE(result.get("assigned").get("inherited").strictEquals("node:b"));REQUIRE_FALSE(result.get("assigned").hasOwn("hidden"));REQUIRE(result.get("assigned").keys().size()==2);
    const auto mutated=F::run(F::request("forIn","mutate"));REQUIRE(mutated.get("assigned").get("deleted").strictEquals("node:c"));REQUIRE_FALSE(mutated.get("assigned").hasOwn("added"));
    const auto string=F::run(F::request("forIn","string"));REQUIRE(string.get("assigned").get("0").strictEquals("node:a"));REQUIRE(string.get("assigned").get("1").strictEquals("node:b"));
    for(const auto* variant:{"null","undefined"})REQUIRE(F::run(F::request("forIn",variant)).get("assigned").keys().empty());
    const auto thrown=F::run(F::request("forIn","throw"));REQUIRE(thrown.get("result").strictEquals("reason"));REQUIRE(thrown.get("assigned").keys().empty());
}
TEST_CASE("NodeMaterialLoader uses dynamic home super and actual managed node materials", "[loaders][node-material-loader]") {
    namespace F=LoaderSemanticsFixtures;
    const auto super=F::run(F::request("super"));REQUIRE(super.get("parseSame").toBoolean());REQUIRE(super.get("createSame").toBoolean());REQUIRE(super.get("trace").toJSON()->dump()==R"([["get:super",true],["call:super",true,"json"],["get:create",true],["call:create",true,"missing"]])");
    const auto real=F::run(F::request("real"));REQUIRE(real.get("isNodeMaterial").strictEquals(true));REQUIRE(real.get("uuid").strictEquals("material-1"));REQUIRE(real.get("opacity").strictEquals(.75));REQUIRE(real.get("sameNode").toBoolean());REQUIRE(real.get("value").strictEquals(.375));
}
