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
