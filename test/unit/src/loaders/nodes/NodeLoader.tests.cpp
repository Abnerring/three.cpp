#include "loaders/nodes/NodeLoader.h"

#include "loaders/LoadingManager.h"
#include "nodes/core/Node.h"

#include "catch.hpp"

#include <string>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

// A stub node class whose deserialize records the payload it received, including whether meta was present.
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

TEST_CASE("NodeLoader keeps its textures and node registries") {
    const auto module = nativeNodeLoaderModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("NodeLoader"));
    REQUIRE(ctor.get("length").strictEquals(1));
    // The class declares no static type getter.
    REQUIRE(ctor.get("type").isUndefined());

    // Without an argument the base loader falls back to the DefaultLoadingManager singleton.
    const auto loader = ctor.construct();
    REQUIRE(loader.get("manager").sameValue(nativeDefaultLoadingManager()));
    // The base loader defaults match the pinned ones.
    REQUIRE(loader.get("crossOrigin").strictEquals("anonymous"));
    REQUIRE(loader.get("withCredentials").strictEquals(false));
    REQUIRE(loader.get("path").strictEquals(""));
    REQUIRE(loader.get("resourcePath").strictEquals(""));
    // An explicit manager is used as given.
    auto manager = JSValue::object(nullptr);
    const auto explicitLoader = ctor.construct({manager});
    REQUIRE(explicitLoader.get("manager").sameValue(manager));
    REQUIRE(loader.get("textures").isObject());
    REQUIRE(loader.get("nodes").isObject());
    REQUIRE(loader.get("load").get("length").strictEquals(4));
    REQUIRE(loader.get("parseNodes").get("length").strictEquals(1));
    REQUIRE(loader.get("parse").get("length").strictEquals(1));
    REQUIRE(loader.get("setTextures").get("length").strictEquals(1));
    REQUIRE(loader.get("setNodes").get("length").strictEquals(1));
    REQUIRE(loader.get("createNodeFromType").get("length").strictEquals(1));

    // Both setters answer the loader.
    auto textures = JSValue::object(nullptr);
    REQUIRE(loader.get("setTextures").call(loader, {textures}).sameValue(loader));
    REQUIRE(loader.get("textures").sameValue(textures));
    auto nodes = JSValue::object(nullptr);
    REQUIRE(loader.get("setNodes").call(loader, {nodes}).sameValue(loader));
    REQUIRE(loader.get("nodes").sameValue(nodes));
}

TEST_CASE("NodeLoader falls back to a float node for an unknown type") {
    const auto loader = nativeNodeLoaderConstructor().construct();
    // Nothing is registered yet, so the fallback answers a float node instead of raising.
    const auto fallback = loader.get("createNodeFromType").call(loader, {JSValue("MissingNode")});
    REQUIRE(fallback.get("isNode").strictEquals(true));
    // The fallback is a plain float conversion node, whose node type stays null until generated.
    REQUIRE(fallback.get("nodeType").isNull());
}

TEST_CASE("NodeLoader parses a node with its child nodes") {
    auto log = JSValue::array();
    auto registry = JSValue::object(nullptr);
    registry.set("TestNode", makeNodeClass(log));
    const auto loader = nativeNodeLoaderConstructor().construct();
    loader.get("setNodes").call(loader, {registry});

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
    REQUIRE(node.get("uuid").strictEquals("root-uuid"));
    // Deserialize ran for the child and for the root, and both payloads carried the meta table.
    REQUIRE(log.length() == Approx(2));
    REQUIRE(log.get(JSValue(0.0)).get("hadMeta").strictEquals(true));
    REQUIRE(log.get(JSValue(1.0)).get("hadMeta").strictEquals(true));
    // The meta tables are removed again after deserializing.
    REQUIRE(json.get("meta").isUndefined());
    REQUIRE(childJSON.get("meta").isUndefined());
}

TEST_CASE("NodeLoader parseNodes answers the nodes keyed by uuid") {
    auto log = JSValue::array();
    auto registry = JSValue::object(nullptr);
    registry.set("TestNode", makeNodeClass(log));
    const auto loader = nativeNodeLoaderConstructor().construct();
    loader.get("setNodes").call(loader, {registry});

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
    REQUIRE(nodes.get("a").get("uuid").strictEquals("a"));
    REQUIRE(nodes.get("b").get("uuid").strictEquals("b"));
    REQUIRE(log.length() == Approx(2));
}

#include "../../../utils/parity/LoaderSemanticsFixtures.h"

TEST_CASE("NodeLoader observes iterable passes closes abrupt bodies and retains failed meta", "[loaders][node-loader][iterator]") {
    namespace F=LoaderSemanticsFixtures;
    const auto good=F::run(F::request("iterator"));REQUIRE(good.get("result").strictEquals("u"));REQUIRE_FALSE(good.get("metaRetained").toBoolean());
    REQUIRE(good.get("trace").toJSON()->dump()==R"([["iterator",1,true],["next",1,0,true],"get:uuid","get:type",["create","T"],["next",1,1,true],["iterator",2,true],["next",2,0,true],"get:uuid",["deserialize","u",true,true],["next",2,1,true]])");
    for(const auto* variant:{"uuidThrow","typeThrow","createThrow","deserializeThrow","nextThrow","secondNextThrow","returnThrow"}){
        CAPTURE(variant);const auto result=F::run(F::request("iterator",variant));REQUIRE(result.get("result").strictEquals("reason"));
        REQUIRE(result.get("metaRetained").toBoolean()==(std::string(variant)=="deserializeThrow"));
        const auto trace=result.get("trace").toJSON()->dump();
        if(std::string(variant)=="nextThrow"||std::string(variant)=="secondNextThrow")REQUIRE(trace.find("return")==std::string::npos);
        else REQUIRE(trace.find("get:return")!=std::string::npos);
    }
    const auto closeThrow=F::run(F::request("iterator","returnThrow"));REQUIRE(closeThrow.get("trace").get(std::to_string(closeThrow.get("trace").length()-1)).get("0").strictEquals("get:return"));
}
TEST_CASE("NodeLoader preserves computed keys repeated registry reads and changing arrays", "[loaders][node-loader]") {
    namespace F=LoaderSemanticsFixtures;
    const auto registry=F::run(F::request("registry"));REQUIRE(registry.get("tag").strictEquals("second"));REQUIRE(registry.get("trace").toJSON()->dump()==R"(["get:nodes","first:type","get:nodes","construct"])");
    REQUIRE(F::run(F::request("keys")).toJSON()->dump()==R"(["get:create","create","key","key","set:uuid","key","deserialize"])");
    const auto grow=F::run(F::request("array","grow"));REQUIRE(grow.get("count").strictEquals(2));REQUIRE(grow.get("keys").toJSON()->dump()==R"(["a","b"])");
    const auto duplicate=F::run(F::request("array","duplicate"));REQUIRE(duplicate.get("keys").toJSON()->dump()==R"(["same"])");REQUIRE(duplicate.get("trace").toJSON()->dump()==R"([["create",1],["create",2],["deserialize",2,1],["deserialize",2,2]])");
    const auto nullish=F::run(F::request("nullish"));REQUIRE(nullish.get("undefined").toBoolean());REQUIRE(nullish.get("null").strictEquals("TypeError"));REQUIRE(nullish.get("number").strictEquals("TypeError"));REQUIRE(nullish.get("texturesUndefined").toBoolean());REQUIRE(nullish.get("nodesUndefined").toBoolean());
    // This pinned source attempts to assign Node.uuid, which is getter-only.
    REQUIRE(F::run(F::request("realNode")).strictEquals("TypeError"));
}
