#include "loaders/nodes/NodeMaterialLoader.h"

#include "core/JSValue.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue materialStub() {
    return JSValue::constructor(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue {
            auto material = JSValue::object(nullptr);
            material.set("type", JSValue("StubNodeMaterial"));
            material.set("isMaterial", true);
            material.set("isNodeMaterial", true);
            return material;
        },
        {}, "StubNodeMaterial", 0);
}
}  // namespace

int main() {
    try {
        const auto module = nativeNodeMaterialLoaderModule();
        const auto ctor = module.get("default");
        auto manager = JSValue::object(nullptr);
        const auto loader = ctor.construct({manager});

        auto nodes = JSValue::object(nullptr);
        const bool setNodesSame = loader.get("setNodes").call(loader, {nodes}).sameValue(loader);
        const bool nodesStored = loader.get("nodes").sameValue(nodes);

        auto registry = JSValue::object(nullptr);
        registry.set("StubNodeMaterial", materialStub());
        const bool setMaterialsSame = loader.get("setNodeMaterials").call(loader, {registry}).sameValue(loader);
        const bool materialsStored = loader.get("nodeMaterials").sameValue(registry);

        const auto created = loader.get("createMaterialFromType").call(loader, {JSValue("StubNodeMaterial")});
        const auto createdType = created.get("type");
        const auto createdIsNodeMaterial = created.get("isNodeMaterial");

        // An unregistered type falls back to the base loader, which answers a plain material and only logs.
        bool unknownThrows = false;
        JSValue unknownResult;
        try {
            unknownResult = loader.get("createMaterialFromType").call(loader, {JSValue("DefinitelyNotAMaterialType")});
        } catch (...) {
            unknownThrows = true;
        }
        const bool unknownIsMaterial = !unknownThrows && unknownResult.get("isMaterial").strictEquals(true);
        const auto unknownType = unknownThrows ? JSValue("") : unknownResult.get("type");

        // Parsing a minimal node material JSON with one input node.
        auto childNode = JSValue::object(nullptr);
        childNode.set("isNode", true);
        childNode.set("type", JSValue("StubNode"));
        auto nodeRegistry = JSValue::object(nullptr);
        nodeRegistry.set("node-uuid", childNode);
        loader.get("setNodes").call(loader, {nodeRegistry});

        auto inputNodes = JSValue::object(nullptr);
        inputNodes.set("colorNode", JSValue("node-uuid"));
        auto json = JSValue::object(nullptr);
        json.set("type", JSValue("MeshBasicMaterial"));
        json.set("uuid", JSValue("material-uuid"));
        json.set("inputNodes", inputNodes);

        bool parseBuilt = true;
        JSValue material;
        try {
            material = loader.get("parse").call(loader, {json});
        } catch (...) {
            parseBuilt = false;
        }
        const bool parsedIsMaterial = parseBuilt && material.get("isMaterial").strictEquals(true);
        const bool parsedInputSame = parseBuilt && material.get("colorNode").sameValue(childNode);

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("nodesIsObject", loader.get("nodes").isObject());
        output.set("nodeMaterialsIsObject", loader.get("nodeMaterials").isObject());
        output.set("setNodesSame", setNodesSame);
        output.set("nodesStored", nodesStored);
        output.set("setMaterialsSame", setMaterialsSame);
        output.set("materialsStored", materialsStored);
        output.set("createdType", createdType);
        output.set("createdIsNodeMaterial", createdIsNodeMaterial);
        output.set("unknownThrows", unknownThrows);
        output.set("unknownIsMaterial", unknownIsMaterial);
        output.set("unknownType", unknownType);
        output.set("parseBuilt", parseBuilt);
        output.set("parsedIsMaterial", parsedIsMaterial);
        output.set("parsedInputSame", parsedInputSame);
        output.set("arityTable", [&]() {
            auto table = JSValue::object(nullptr);
            for (const auto* name : {"parse", "setNodes", "setNodeMaterials", "createMaterialFromType"}) {
                table.set(name, loader.get(name).get("length"));
            }
            return table;
        }());
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
