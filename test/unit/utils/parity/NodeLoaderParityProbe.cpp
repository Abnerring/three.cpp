#include "loaders/nodes/NodeLoader.h"

#include "core/JSValue.h"
#include "loaders/LoadingManager.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue makeNodeClass(JSValue& log) {
    return JSValue::constructor(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
            auto node = JSValue::object(nullptr);
            node.set("uuid", JSValue(""));
            node.set("deserialize", JSValue::function(
                +[](const JSValue& self, const ArgList& args, const ArgList& inner) -> JSValue {
                    const auto data = args.empty() ? JSValue(nullptr) : args[0];
                    auto log = inner[0];
                    auto entry = JSValue::object(nullptr);
                    entry.set("uuid", self.get("uuid"));
                    entry.set("type", data.get("type"));
                    entry.set("hadMeta", !data.get("meta").isUndefined());
                    log.set(log.length(), entry);
                    return {};
                },
                {captures[0]}, "deserialize", 1));
            return node;
        },
        {log}, "TestNode", 0);
}}  // namespace

int main() {
    try {
        const auto module = nativeNodeLoaderModule();
        const auto ctor = module.get("default");
        const auto loader = ctor.construct();
        const bool managerIsDefault =
            loader.get("manager").sameValue(nativeDefaultLoadingManager());
        const auto crossOrigin = loader.get("crossOrigin");
        const auto withCredentials = loader.get("withCredentials");
        const auto pathValue = loader.get("path");

        auto textures = JSValue::object(nullptr);
        const bool setTexturesSame = loader.get("setTextures").call(loader, {textures}).sameValue(loader);
        const bool texturesStored = loader.get("textures").sameValue(textures);

        auto log = JSValue::array();
        auto registry = JSValue::object(nullptr);
        registry.set("TestNode", makeNodeClass(log));
        loader.get("setNodes").call(loader, {registry});

        // Unknown type falls back to a float node.
        const auto fallback = loader.get("createNodeFromType").call(loader, {JSValue("MissingNode")});
        const bool fallbackIsNode = fallback.get("isNode").strictEquals(true);
        const auto fallbackType = fallback.get("nodeType");

        auto childJSON = JSValue::object(nullptr);
        childJSON.set("uuid", JSValue("child-uuid"));
        childJSON.set("type", JSValue("TestNode"));
        auto childList = JSValue::array();
        childList.set(JSValue(0.0), childJSON);

        auto json = JSValue::object(nullptr);
        json.set("uuid", JSValue("root-uuid"));
        json.set("type", JSValue("TestNode"));
        json.set("nodes", childList);

        const auto node = loader.get("parse").call(loader, {json});
        const auto parsedUuid = node.get("uuid");
        const auto logLength = log.length();
        const bool childHadMeta = log.length() > 0 &&
            log.get(JSValue(0.0)).get("hadMeta").strictEquals(true);
        const bool rootHadMeta = log.length() > 1 &&
            log.get(JSValue(1.0)).get("hadMeta").strictEquals(true);
        const bool jsonMetaRemoved = json.get("meta").isUndefined();
        const bool childMetaRemoved = childJSON.get("meta").isUndefined();

        auto first = JSValue::object(nullptr);
        first.set("uuid", JSValue("a"));
        first.set("type", JSValue("TestNode"));
        auto second = JSValue::object(nullptr);
        second.set("uuid", JSValue("b"));
        second.set("type", JSValue("TestNode"));
        auto list = JSValue::array();
        list.set(JSValue(0.0), first);
        list.set(JSValue(1.0), second);
        const auto nodes = loader.get("parseNodes").call(loader, {list});
        const auto firstUuid = nodes.get("a").get("uuid");
        const auto secondUuid = nodes.get("b").get("uuid");

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));


        output.set("managerIsDefault", managerIsDefault);
        output.set("crossOrigin", crossOrigin);
        output.set("withCredentials", withCredentials);
        output.set("path", pathValue);
        output.set("texturesIsObject", loader.get("textures").isObject());
        output.set("nodesIsObject", loader.get("nodes").isObject());
        output.set("setTexturesSame", setTexturesSame);
        output.set("texturesStored", texturesStored);
        output.set("fallbackIsNode", fallbackIsNode);
        output.set("fallbackType", fallbackType);
        output.set("parsedUuid", parsedUuid);
        output.set("logLength", logLength);
        output.set("childHadMeta", childHadMeta);
        output.set("rootHadMeta", rootHadMeta);
        output.set("jsonMetaRemoved", jsonMetaRemoved);
        output.set("childMetaRemoved", childMetaRemoved);
        output.set("firstUuid", firstUuid);
        output.set("secondUuid", secondUuid);
        output.set("arityTable", [&]() {
            auto table = JSValue::object(nullptr);
            for (const auto* name : {"load", "parseNodes", "parse", "setTextures", "setNodes",
                                     "createNodeFromType"}) {
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
