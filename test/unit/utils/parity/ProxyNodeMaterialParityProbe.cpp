#include "materials/nodes/ProxyNodeMaterial.h"

#include "core/JSValue.h"

#include <iostream>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue makeNodeMaterial(const JSValue& userData, const JSValue& counters) {
    auto material = JSValue::object(nullptr);
    material.set("isNodeMaterial", true);
    material.set("uuid", JSValue("node-uuid"));
    material.set("version", 3);
    material.set("userData", userData);
    material.set("colorNode", JSValue::object(nullptr));
    material.set("someValue", 42);
    material.set("build", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
            auto state = captures[0];
            state.set("builds", JSValue(state.get("builds").toNumber() + 1));
            return {};
        },
        {counters}, "build", 1));
    return material;
}
}  // namespace

int main() {
    try {
        const auto ctor = nativeProxyNodeMaterialConstructor();

        bool rejectedPlain = false;
        try {
            ctor.construct({JSValue::object(nullptr)});
        } catch (...) {
            rejectedPlain = true;
        }

        auto counters = JSValue::object(nullptr);
        counters.set("builds", 0);
        auto cloneCounters = JSValue::object(nullptr);
        cloneCounters.set("cloned", 0);
        auto cloneable = JSValue::object(nullptr);
        cloneable.set("clone", JSValue::function(
            +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue {
                auto state = captures[0];
                state.set("cloned", JSValue(state.get("cloned").toNumber() + 1));
                auto copy = JSValue::object(nullptr);
                copy.set("copy", true);
                return copy;
            },
            {cloneCounters}, "clone", 0));
        auto texture = JSValue::object(nullptr);
        texture.set("isTexture", true);
        auto userData = JSValue::object(nullptr);
        userData.set("cloneable", cloneable);
        userData.set("texture", texture);

        const auto nodeMaterial = makeNodeMaterial(userData, counters);
        const auto proxy = ctor.construct({nodeMaterial});

        // Node suffixed properties are getters, so a later change on the wrapped material shows through.
        const auto beforeColor = proxy.get("colorNode").sameValue(nodeMaterial.get("colorNode"));
        const auto replacement = JSValue::object(nullptr);
        auto nodeRef = nodeMaterial;
        nodeRef.set("colorNode", replacement);
        const auto afterColor = proxy.get("colorNode").sameValue(replacement);

        // The version getter reads through and the setter does nothing.
        const auto versionValue = proxy.get("version");
        // JSValue is a handle, so the copy still writes the same object property.
    auto proxyHandle = proxy;
    proxyHandle.set("version", 99);
        const auto versionAfterSet = proxy.get("version");

        // build delegates.
        proxy.get("build").call(proxy, {JSValue::object(nullptr)});

        // clone() and the user data observations are taken before copy(), which replaces userData.
        const auto cloneableHasCopy = proxy.get("userData").get("cloneable").get("copy");
        const auto textureKept = proxy.get("userData").get("texture").sameValue(texture);

        // copy skips the three reserved keys.
        auto source = JSValue::object(nullptr);
        source.set("uuid", JSValue("source-uuid"));
        source.set("version", 7);
        source.set("_listeners", JSValue::object(nullptr));
        source.set("copied", 5);
        source.set("userData", JSValue::object(nullptr));
        proxy.get("copy").call(proxy, {source});
        const auto copiedValue = proxy.get("copied");
        const auto uuidSkipped = !proxy.get("uuid").strictEquals("source-uuid");

        const auto copy = proxy.get("clone").call(proxy, {});

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("typeIsUndefined", ctor.get("type").isUndefined());
        output.set("rejectedPlain", rejectedPlain);
        output.set("proxyType", proxy.get("type"));
        output.set("isProxyNodeMaterial", proxy.get("isProxyNodeMaterial"));
        output.set("wrapsSame", proxy.get("nodeMaterial").sameValue(nodeMaterial));
        output.set("someValue", proxy.get("someValue"));
        output.set("colorBeforeChangeShows", beforeColor);
        output.set("colorAfterChangeShows", afterColor);
        output.set("version", versionValue);
        output.set("versionAfterSet", versionAfterSet);
        output.set("cacheKey", proxy.get("customProgramCacheKey").call(proxy, {JSValue::object(nullptr)}));
        output.set("builds", counters.get("builds"));
        output.set("clonedCount", cloneCounters.get("cloned"));
        output.set("clonedHasCopy", cloneableHasCopy);
        output.set("textureKeptByIdentity", textureKept);
        output.set("copied", copiedValue);
        output.set("uuidSkipped", uuidSkipped);
        output.set("copyIsProxy", copy.get("isProxyNodeMaterial"));
        output.set("copyWrapsSame", copy.get("nodeMaterial").sameValue(nodeMaterial));
        output.set("copyIsNew", !copy.sameValue(proxy));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
