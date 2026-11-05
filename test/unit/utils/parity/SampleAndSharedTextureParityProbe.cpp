#include "nodes/display/ViewportSharedTextureNode.h"
#include "nodes/utils/SampleNode.h"

#include "core/JSValue.h"
#include "nodes/tsl/TSLBase.h"

#include <iostream>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue countingCallback(const JSValue& state) {
    return JSValue::function(
        +[](const JSValue&, const ArgList& args, const ArgList& captures) -> JSValue {
            auto holder = captures[0];
            holder.set("count", JSValue(holder.get("count").toNumber() + 1));
            holder.set("lastArg", args.size() > 0 ? args[0] : JSValue{});
            return captures[1];
        },
        {state, nativeTSLBaseBinding("float").call({}, {JSValue(2.0)})}, "callback", 1);
}
}  // namespace

int main() {
    try {
        const auto sampleCtor = nativeSampleNodeConstructor();
        const auto sampleFactory = nativeSampleFunction();

        auto state = JSValue::object(nullptr);
        state.set("count", 0);
        const auto sampleNode = sampleCtor.construct({countingCallback(state)});
        const auto uvArgument = nativeTSLBaseBinding("float").call({}, {JSValue(0.5)});
        const auto sampled = sampleNode.get("sample").call(sampleNode, {uvArgument});
        const auto argumentWasTheUv = state.get("lastArg").sameValue(uvArgument);
        const auto afterFirstSample = state.get("count");
        const auto built = sampleNode.get("setup").call(sampleNode, {});
        const auto setupArgumentIsNode = state.get("lastArg").get("isNode");

        auto factoryState = JSValue::object(nullptr);
        factoryState.set("count", 0);
        const auto factoryCallback = countingCallback(factoryState);
        const auto factoryNode = sampleFactory.call({}, {factoryCallback});

        const auto sharedModule = nativeViewportSharedTextureNodeModule();
        const auto sharedCtor = nativeViewportSharedTextureNodeConstructor();
        const auto first = sharedCtor.construct();
        const auto second = sharedCtor.construct();
        const auto proxy = nativeViewportSharedTextureNodeBinding("viewportSharedTexture");
        const auto produced = proxy.call({}, {});

        auto output = JSValue::object(nullptr);
        output.set("sampleName", sampleCtor.get("name"));
        output.set("sampleLength", sampleCtor.get("length"));
        output.set("sampleStaticType", sampleCtor.get("type"));
        output.set("sampleType", sampleNode.get("type"));
        output.set("sampleIsSampleNode", sampleNode.get("isSampleNode"));
        output.set("sampleUvNodeIsNull", sampleNode.get("uvNode").isNull());
        output.set("sampleIsCacheable", sampleNode.get("isCacheable").call(sampleNode, {}));
        output.set("sampleCallsAfterFirst", afterFirstSample);
        output.set("sampleArgumentWasTheUv", argumentWasTheUv);
        output.set("sampleResultIsNode", sampled.get("isNode"));
        output.set("sampleCallsAfterSetup", state.get("count"));
        output.set("sampleSetupArgumentIsNode", setupArgumentIsNode);
        output.set("sampleSetupResultIsNode", built.get("isNode"));
        output.set("factoryName", sampleFactory.get("name"));
        output.set("factoryLength", sampleFactory.get("length"));
        output.set("factoryIsSampleNode", factoryNode.get("isSampleNode"));
        output.set("factoryCallbackSame", factoryNode.get("callback").sameValue(factoryCallback));
        output.set("factoryUvNodeIsNull", factoryNode.get("uvNode").isNull());

        output.set("sharedName", sharedCtor.get("name"));
        output.set("sharedLength", sharedCtor.get("length"));
        output.set("sharedStaticType", sharedCtor.get("type"));
        output.set("sharedType", first.get("type"));
        output.set("sharedTextureSame",
                   first.get("getTextureForReference").call(first, {}).sameValue(
                       second.get("getTextureForReference").call(second, {})));
        output.set("updateReferenceReturnsSelf",
                   first.get("updateReference").call(first, {}).sameValue(first));
        output.set("proxyIsCallable", proxy.isCallable());
        output.set("proxyLength", proxy.get("length"));
        output.set("proxyProducesCallable",
                   produced.get("getTextureForReference").isCallable());
        output.set("proxyTextureIsNotNull",
                   !produced.get("getTextureForReference").call(produced, {}).isNull());
        output.set("sampleSource", sampleCtor.get("toString").call(sampleCtor));
        output.set("sharedSource", sharedCtor.get("toString").call(sharedCtor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
