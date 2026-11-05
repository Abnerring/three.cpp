#include "nodes/utils/RTTNode.h"

#include "core/JSValue.h"
#include "constants.h"
#include "nodes/core/NodeConstants.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
using ArgList = std::vector<JSValue>;

JSValue sourceNode() {
    auto source = JSValue::object(nullptr);
    source.set("isNode", true);
    return source;
}
}  // namespace

int main() {
    try {
        const auto module = nativeRTTNodeModule();
        const auto ctor = module.get("default");
        const auto source = sourceNode();
        const auto node = ctor.construct({source});

        const bool widthNull = node.get("width").isNull();
        const bool heightNull = node.get("height").isNull();
        const bool valueIsTexture = node.get("value").sameValue(node.get("renderTarget").get("texture"));
        const bool autoResizeDefault = node.get("autoResize").strictEquals(true);
        const bool updateBeforeTypeMatches = node.get("updateBeforeType").strictEquals("frame");
        const bool quadMaterialIsNodeMaterial =
            node.get("_quadMesh").get("material").get("isNodeMaterial").strictEquals(true);

        const auto sizedNode = ctor.construct({source, JSValue(4), JSValue(4)});
        const bool autoResizeExplicit = sizedNode.get("autoResize").strictEquals(false);
        const auto explicitWidth = sizedNode.get("renderTarget").get("width");

        auto options = JSValue::object(nullptr);
        options.set("resolutionScale", 0.5);
        const auto scaled = ctor.construct({source, JSValue(10), JSValue(5), options});
        scaled.get("setSize").call(scaled, {JSValue(10), JSValue(5)});
        const auto scaledWidth = scaled.get("renderTarget").get("width");
        const auto scaledHeight = scaled.get("renderTarget").get("height");

        const auto autoNode = ctor.construct({source});
        const bool scaleReturnsSelf =
            autoNode.get("setResolutionScale").call(autoNode, {JSValue(0.25)}).sameValue(autoNode);
        const auto storedScale = autoNode.get("getResolutionScale").call(autoNode, {});
        const auto explicitScaled = ctor.construct({source, JSValue(8), JSValue(8)});
        explicitScaled.get("setResolutionScale").call(explicitScaled, {JSValue(0.5)});
        const auto explicitScaledWidth = explicitScaled.get("renderTarget").get("width");

        const auto copy = node.get("clone").call(node, {});
        const bool cloneIsTextureNode = copy.get("isTextureNode").strictEquals(true);
        const bool cloneRefersBack = copy.get("referenceNode").sameValue(node);
        const bool cloneSamplerSame = copy.get("sampler").sameValue(node.get("sampler"));
        bool disposeRan = true;
        try {
            node.get("dispose").call(node, {});
        } catch (...) {
            disposeRan = false;
        }

        auto sample = JSValue::object(nullptr);
        sample.set("isSampleNode", true);
        const bool samplePassThrough =
            module.get("convertToTexture").call({}, {sample}).sameValue(sample);
        auto texture = JSValue::object(nullptr);
        texture.set("isTextureNode", true);
        const bool texturePassThrough =
            module.get("convertToTexture").call({}, {texture}).sameValue(texture);
        auto produced = JSValue::object(nullptr);
        produced.set("isTextureNode", true);
        auto pass = JSValue::object(nullptr);
        pass.set("isPassNode", true);
        pass.set("getTextureNode", JSValue::function(
            +[](const JSValue&, const ArgList&, const ArgList& captures) -> JSValue { return captures[0]; },
            {produced}, "getTextureNode", 0));
        const bool passUsesGetter =
            module.get("convertToTexture").call({}, {pass}).sameValue(produced);
        auto plain = JSValue::object(nullptr);
        plain.set("isNode", true);
        const auto wrapped = module.get("convertToTexture").call({}, {plain});
        const bool wrappedIsRTT = wrapped.get("isRTTNode").strictEquals(true);
        const bool wrappedKeepsNode = wrapped.get("node").sameValue(plain);

        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("type", node.get("type"));
        output.set("isRTTNode", node.get("isRTTNode"));
        output.set("widthNull", widthNull);
        output.set("heightNull", heightNull);
        output.set("valueIsTexture", valueIsTexture);
        output.set("textureNeedsUpdate", node.get("textureNeedsUpdate"));
        output.set("autoUpdate", node.get("autoUpdate"));
        output.set("resolutionScale", node.get("_resolutionScale"));
        output.set("autoResizeDefault", autoResizeDefault);
        output.set("autoResizeExplicit", autoResizeExplicit);
        output.set("explicitWidth", explicitWidth);
        output.set("updateBeforeTypeMatches", updateBeforeTypeMatches);
        output.set("quadMaterialIsNodeMaterial", quadMaterialIsNodeMaterial);
        output.set("scaledWidth", scaledWidth);
        output.set("scaledHeight", scaledHeight);
        output.set("scaleReturnsSelf", scaleReturnsSelf);
        output.set("storedScale", storedScale);
        output.set("explicitScaledWidth", explicitScaledWidth);
        output.set("cloneIsTextureNode", cloneIsTextureNode);
        output.set("cloneRefersBack", cloneRefersBack);
        output.set("cloneSamplerSame", cloneSamplerSame);
        output.set("disposeRan", disposeRan);
        output.set("samplePassThrough", samplePassThrough);
        output.set("texturePassThrough", texturePassThrough);
        output.set("passUsesGetter", passUsesGetter);
        output.set("wrappedIsRTT", wrappedIsRTT);
        output.set("wrappedKeepsNode", wrappedKeepsNode);
        output.set("rttLength", module.get("rtt").get("length"));
        output.set("convertLength", module.get("convertToTexture").get("length"));
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
