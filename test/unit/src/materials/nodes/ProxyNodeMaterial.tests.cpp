#include "materials/nodes/ProxyNodeMaterial.h"

#include "core/JSValue.h"
#include "materials/Material.h"

#include "catch.hpp"

#include <stdexcept>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

// A node material stand-in: it only has to mark itself, carry a uuid and version, provide some node
// properties, and record build calls.
JSValue makeNodeMaterial(const JSValue& userData) {
    auto material = JSValue::object(nullptr);
    material.set("isNodeMaterial", true);
    material.set("uuid", JSValue("node-uuid"));
    material.set("version", 3);
    material.set("userData", userData);
    material.set("colorNode", JSValue::object(nullptr));
    material.set("opacityNode", JSValue::object(nullptr));
    material.set("someValue", 42);
    material.set("build", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
            auto state = captures[0];
            state.set("builds", JSValue(state.get("builds").toNumber() + 1));
            return {};
        },
        {userData}, "build", 1));
    return material;
}
}  // namespace

TEST_CASE("ProxyNodeMaterial requires a node material and proxies its properties") {
    const auto module = nativeProxyNodeMaterialModule();
    const auto ctor = module.get("default");
    REQUIRE(ctor.get("name").strictEquals("ProxyNodeMaterial"));
    REQUIRE(ctor.get("length").strictEquals(1));
    // The class declares no static type getter, so neither side has one.
    REQUIRE(ctor.get("type").isUndefined());

    // A material that is not a node material is rejected.
    auto plain = JSValue::object(nullptr);
    REQUIRE_THROWS(ctor.construct({plain}));

    const auto nodeMaterial = makeNodeMaterial(JSValue::object(nullptr));
    const auto proxy = ctor.construct({nodeMaterial});
    REQUIRE(proxy.get("type").strictEquals("ProxyNodeMaterial"));
    REQUIRE(proxy.get("isProxyNodeMaterial").strictEquals(true));
    REQUIRE(proxy.get("nodeMaterial").sameValue(nodeMaterial));
    // Ordinary properties are copied by value.
    REQUIRE(proxy.get("someValue").toNumber() == Approx(42));
    // Node suffixed properties become getters that read through to the node material.
    REQUIRE(proxy.get("colorNode").sameValue(nodeMaterial.get("colorNode")));
    const auto replacement = JSValue::object(nullptr);
    auto nodeRef = nodeMaterial;
    nodeRef.set("colorNode", replacement);
    REQUIRE(proxy.get("colorNode").sameValue(replacement));
    // The version accessor reads through, and its setter deliberately does nothing.
    REQUIRE(proxy.get("version").toNumber() == Approx(3));
    // JSValue is a handle, so the copy still writes the same object property.
    auto proxyHandle = proxy;
    proxyHandle.set("version", 99);
    REQUIRE(proxy.get("version").toNumber() == Approx(3));
}

TEST_CASE("ProxyNodeMaterial builds its cache key from the wrapped uuid and version") {
    const auto nodeMaterial = makeNodeMaterial(JSValue::object(nullptr));
    const auto proxy = nativeProxyNodeMaterialConstructor().construct({nodeMaterial});
    REQUIRE(proxy.get("customProgramCacheKey").call(proxy, {}).strictEquals("node-uuid,3"));
    // build delegates to the wrapped material.
    auto state = JSValue::object(nullptr);
    state.set("builds", 0);
    auto delegated = makeNodeMaterial(state);
    const auto delegatingProxy = nativeProxyNodeMaterialConstructor().construct({delegated});
    delegatingProxy.get("build").call(delegatingProxy, {JSValue::object(nullptr)});
    REQUIRE(state.get("builds").toNumber() == Approx(1));
}

TEST_CASE("ProxyNodeMaterial clones its user data with the texture exception") {
    auto cloneCount = JSValue::object(nullptr);
    cloneCount.set("cloned", 0);
    auto cloneable = JSValue::object(nullptr);
    cloneable.set("clone", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
            auto state = captures[0];
            state.set("cloned", JSValue(state.get("cloned").toNumber() + 1));
            auto copy = JSValue::object(nullptr);
            copy.set("copy", true);
            return copy;
        },
        {cloneCount}, "clone", 0));
    auto texture = JSValue::object(nullptr);
    texture.set("isTexture", true);
    auto userData = JSValue::object(nullptr);
    userData.set("cloneable", cloneable);
    userData.set("texture", texture);

    const auto proxy =
        nativeProxyNodeMaterialConstructor().construct({makeNodeMaterial(userData)});
    // The cloneable is cloned once; the texture is kept by identity rather than cloned.
    REQUIRE(cloneCount.get("cloned").toNumber() == Approx(1));
    REQUIRE(proxy.get("userData").get("cloneable").get("copy").strictEquals(true));
    REQUIRE(proxy.get("userData").get("texture").sameValue(texture));
}

TEST_CASE("ProxyNodeMaterial skips uuid, version and listeners while copying") {
    auto source = JSValue::object(nullptr);
    source.set("uuid", JSValue("source-uuid"));
    source.set("version", 7);
    source.set("_listeners", JSValue::object(nullptr));
    source.set("copied", 5);
    source.set("userData", JSValue::object(nullptr));
    const auto proxy = nativeProxyNodeMaterialConstructor().construct(
        {makeNodeMaterial(JSValue::object(nullptr))});
    proxy.get("copy").call(proxy, {source});
    REQUIRE(proxy.get("copied").toNumber() == Approx(5));
    // The proxy keeps its own uuid; the source's is skipped.
    REQUIRE_FALSE(proxy.get("uuid").strictEquals("source-uuid"));
}

TEST_CASE("ProxyNodeMaterial clone wraps the same node material") {
    const auto nodeMaterial = makeNodeMaterial(JSValue::object(nullptr));
    const auto proxy = nativeProxyNodeMaterialConstructor().construct({nodeMaterial});
    const auto copy = proxy.get("clone").call(proxy, {});
    REQUIRE_FALSE(copy.sameValue(proxy));
    REQUIRE(copy.get("nodeMaterial").sameValue(nodeMaterial));
    REQUIRE(copy.get("isProxyNodeMaterial").strictEquals(true));
}
