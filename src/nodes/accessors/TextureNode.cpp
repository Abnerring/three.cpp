// Native port of three.js 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "nodes/accessors/TextureNode.h"
#include "nodes/core/UniformNode.h"
#include "nodes/accessors/UV.h"
#include "nodes/accessors/TextureSizeNode.h"
#include "nodes/display/ColorSpaceNode.h"
#include "nodes/code/ExpressionNode.h"
#include "nodes/utils/MaxMipLevelNode.h"
#include "nodes/tsl/TSLBase.h"
#include "nodes/math/MathNode.h"
#include "nodes/core/NodeUtils.h"
#include "nodes/core/NodeError.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "textures/Texture.h"
#include "core/JSGlobal.h"
#include "core/JSPromise.h"
#include "core/JSRegExp.h"
#include "constants.h"
#include "utils.h"
#include <algorithm>

namespace {
using namespace UtilityNodeRuntime;
#include "TextureNodeSource.inc"
JSWeakValue& lexical() { static thread_local JSWeakValue weak; return weak; }
JSValue raw(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue array(const Args& values) {
    auto result = JSValue::array();
    for (std::size_t i = 0; i < values.size(); ++i) own(result, std::to_string(i).c_str(), values[i]);
    return result;
}
JSValue uniformUpdate(const JSValue&, const Args&, const Args& captures) {
    const auto base = captures[1], texture = base.get("value");
    if (captures[0].strictEquals("matrix")) {
        if (texture.get("matrixAutoUpdate").strictEquals(true)) invoke(texture, "updateMatrix")();
        return texture.get("matrix");
    }
    const auto image = texture.get("image");
    const auto bitmap = nativeGlobalBinding("ImageBitmap");
    return (image.instanceOf(bitmap) && texture.get("flipY").strictEquals(true)) ||
        texture.get("isRenderTargetTexture").strictEquals(true) ||
        texture.get("isFramebufferTexture").strictEquals(true) || texture.get("isDepthTexture").strictEquals(true);
}
JSValue uvBody(const JSValue&, const Args&, const Args& captures) {
    const auto self = captures[0], builder = captures[1]; auto uv = self.get("uvNode");
    if ((uv.isNull() || builder.get("context").get("forceUVContext").strictEquals(true)) &&
        builder.get("context").get("getUV").toBoolean()) uv = invoke(builder.get("context"), "getUV")({self, builder});
    if (!uv.toBoolean()) uv = invoke(self, "getDefaultUV")();
    if (self.get("updateMatrix").strictEquals(true)) uv = invoke(self, "getTransformedUV")({uv});
    return invoke(self, "setupUV")({builder, uv});
}
JSValue accessor(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto field = captures[0].string();
    if (captures[1].toBoolean()) {
        if (self.get("referenceNode").toBoolean()) self.get("referenceNode").set(field, raw(args, 0));
        else self.set("_" + field, raw(args, 0));
        return {};
    }
    return self.get("referenceNode").toBoolean() ? self.get("referenceNode").get(field) : self.get("_" + field);
}
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string(); const auto imports = captures[2];
    auto builder = raw(args, 0);
    if (name == "getUniformHash") {
        if (self.get("referenceNode").toBoolean() && !self.get("referenceNode").get("mergeable").toBoolean())
            return invoke(self.get("referenceNode"), "getHash")({builder});
        if (!self.get("_mergeable").toBoolean()) return invoke(self, "getHash")({builder});
        return self.get("value").get("uuid");
    }
    if (name == "customCacheKey") {
        const auto value = self.get("value"), type = value.get("type"), min = value.get("minFilter"), mag = value.get("magFilter");
        return imports.get("hash").call({}, {self.get("id"), type, min, mag});
    }
    if (name == "generateNodeType") {
        if (!self.get("gatherNode").isNull()) return "vec4";
        return imports.get("getTextureType").call({}, {self.get("value")});
    }
    if (name == "getInputType") return "texture";
    if (name == "getDefaultUV") return imports.get("uv").call({}, {self.get("value").get("channel")});
    if (name == "updateReference") return self.get("value");
    if (name == "getTransformedUV") {
        auto base = invoke(self, "getBase")();
        if (base.get("_matrixUniform").isNull()) {
            const auto uniform = imports.get("uniform").call({}, {base.get("value").get("matrix")});
            const auto onUpdate = invoke(uniform, "onObjectUpdate");
            auto callback = JSValue::function(uniformUpdate, {"matrix", base}, "", 0); callback.setFunctionSourceText(TextureNodeSource::matrixUpdate);
            base.set("_matrixUniform", onUpdate({callback}));
        }
        const auto multiply = invoke(base.get("_matrixUniform"), "mul");
        const auto uv = imports.get("vec3").call({}, {builder, 1});
        return multiply({uv}).get("xy");
    }
    if (name == "setUpdateMatrix") { self.set("updateMatrix", builder); return self; }
    if (name == "setupUV") {
        auto uv = raw(args, 1);
        if (invoke(builder, "isFlipY")().toBoolean()) {
            auto base = invoke(self, "getBase")();
            if (base.get("_flipYUniform").isNull()) {
                const auto uniform = imports.get("uniform").call({}, {false});
                const auto onUpdate = invoke(uniform, "onObjectUpdate");
                auto callback = JSValue::function(uniformUpdate, {"flip", base}, "", 0); callback.setFunctionSourceText(TextureNodeSource::flipUpdate);
                base.set("_flipYUniform", onUpdate({callback}));
            }
            uv = invoke(uv, "toVar")();
            if (self.get("sampler").toBoolean()) {
                const auto select = invoke(base.get("_flipYUniform"), "select");
                const auto flipped = invoke(uv, "flipY")(); uv = select({flipped, uv});
            } else {
                const auto select = invoke(base.get("_flipYUniform"), "select"), setY = invoke(uv, "setY");
                const auto size = imports.get("textureSize").call({}, {self, self.get("levelNode")}).get("y");
                const auto first = imports.get("int").call({}, {size}); const auto sub = invoke(first, "sub");
                const auto second = imports.get("int").call({}, {uv.get("y")});
                const auto last = invoke(sub({second}), "sub")({1}); uv = select({setY({last}), uv});
            }
        }
        return uv;
    }
    if (name == "setup") {
        auto properties = invoke(builder, "getNodeProperties")({self}); properties.set("referenceNode", self.get("referenceNode"));
        const auto texture = self.get("value");
        if (!texture.toBoolean() || !texture.get("isTexture").strictEquals(true))
            throw JSThrownValue(imports.get("NodeError").construct({"THREE.TSL: `texture( value )` function expects a valid instance of THREE.Texture().", self.get("stackTrace")}));
        auto callback = JSValue::function(uvBody, {self, builder}, "", 0); callback.setFunctionSourceText(TextureNodeSource::uvBody);
        const auto shader = imports.get("Fn").call({}, {callback});
        const auto uv = shader.call(); auto level = self.get("levelNode");
        if (level.isNull() && builder.get("context").get("getTextureLevel").toBoolean())
            level = invoke(builder.get("context"), "getTextureLevel")({self});
        JSValue compare = nullptr, compareStep = nullptr;
        if (!self.get("compareNode").isNull()) {
            const auto compatibility = invoke(builder.get("renderer"), "hasCompatibility");
            if (compatibility({imports.get("Compatibility").get("TEXTURE_COMPARE")}).toBoolean()) compare = self.get("compareNode");
            else {
                const auto function = texture.get("compareFunction");
                if (function.isNull() || function.strictEquals(imports.get("LessCompare")) || function.strictEquals(imports.get("LessEqualCompare")) ||
                    function.strictEquals(imports.get("GreaterCompare")) || function.strictEquals(imports.get("GreaterEqualCompare"))) compareStep = self.get("compareNode");
                else { compare = self.get("compareNode"); ThreeUtils::warnOnce({"TSL: Only \"LessCompare\", \"LessEqualCompare\", \"GreaterCompare\" and \"GreaterEqualCompare\" are supported for depth texture comparison fallback."}); }
            }
        }
        properties.set("uvNode", uv); properties.set("levelNode", level); properties.set("biasNode", self.get("biasNode"));
        properties.set("compareNode", compare); properties.set("compareStepNode", compareStep);
        for (const auto* key : {"gradNode", "gatherNode", "depthNode", "offsetNode"}) properties.set(key, self.get(key));
        return {};
    }
    if (name == "generateUV") { const auto build = invoke(raw(args, 1), "build"); return build({builder, self.get("sampler").strictEquals(true) ? "vec2" : "ivec2"}); }
    if (name == "generateOffset") return invoke(raw(args, 1), "build")({builder, "ivec2"});
    if (name == "generateSnippet") {
        const auto texture = self.get("value"), property = raw(args, 1), uv = raw(args, 2), level = raw(args, 3), bias = raw(args, 4), depth = raw(args, 5),
            compare = raw(args, 6), grad = raw(args, 7), gather = raw(args, 8), offset = raw(args, 9), flip = raw(args, 10);
        if (bias.toBoolean()) return invoke(builder, "generateTextureBias")({texture, property, uv, bias, depth, offset});
        if (grad.toBoolean()) return invoke(builder, "generateTextureGrad")({texture, property, uv, grad, depth, offset});
        if (gather.toBoolean()) {
            if (compare.toBoolean()) return invoke(builder, "generateTextureGatherCompare")({texture, property, uv, compare, depth, offset, flip});
            return invoke(builder, "generateTextureGather")({texture, property, uv, gather, depth, offset, flip});
        }
        if (compare.toBoolean()) return invoke(builder, "generateTextureCompare")({texture, property, uv, compare, depth, offset});
        if (self.get("sampler").strictEquals(false)) return invoke(builder, "generateTextureLoad")({texture, property, uv, level, depth, offset});
        if (level.toBoolean()) return invoke(builder, "generateTextureLevel")({texture, property, uv, level, depth, offset});
        return invoke(builder, "generateTexture")({texture, property, uv, depth, offset});
    }
    if (name == "generate") {
        const auto output = raw(args, 1), texture = self.get("value"), properties = invoke(builder, "getNodeProperties")({self});
        const auto property = captures[1].getPrototype().get("generate", self).call(self, {builder, "property"});
        if (output.strictEquals("samplerComparison") || !properties.get("compareNode").isNull()) invoke(self, "getSharedNode")({builder}).set("_samplerComparison", true);
        const auto re = createNativeRegExp("^sampler");
        if (invoke(re, "test")({output}).toBoolean()) return add(property, "_sampler");
        if (invoke(builder, "isReference")({output}).toBoolean()) return property;
        auto data = invoke(builder, "getDataFromNode")({self}); const auto type = invoke(self, "getNodeType")({builder});
        auto propertyName = data.get("propertyName");
        if (propertyName.isUndefined()) {
            const auto uv = properties.get("uvNode"), level = properties.get("levelNode"), bias = properties.get("biasNode"), compare = properties.get("compareNode"),
                compareStep = properties.get("compareStepNode"), depth = properties.get("depthNode"), grad = properties.get("gradNode"), gather = properties.get("gatherNode"), offset = properties.get("offsetNode");
            const auto uvSnippet = invoke(self, "generateUV")({builder, uv});
            const auto levelSnippet = level.toBoolean() ? invoke(level, "build")({builder, "float"}) : JSValue(nullptr);
            const auto biasSnippet = bias.toBoolean() ? invoke(bias, "build")({builder, "float"}) : JSValue(nullptr);
            const auto depthSnippet = depth.toBoolean() ? invoke(depth, "build")({builder, "int"}) : JSValue(nullptr);
            const auto compareSnippet = compare.toBoolean() ? invoke(compare, "build")({builder, "float"}) : JSValue(nullptr);
            const auto compareStepSnippet = compareStep.toBoolean() ? invoke(compareStep, "build")({builder, "float"}) : JSValue(nullptr);
            JSValue gradSnippet = nullptr;
            if (grad.toBoolean()) { const auto x = invoke(grad.get("0"), "build")({builder, "vec2"}); const auto y = invoke(grad.get("1"), "build")({builder, "vec2"}); gradSnippet = array({x, y}); }
            const auto gatherSnippet = gather.toBoolean() ? invoke(gather, "build")({builder, "int"}) : JSValue(nullptr);
            const auto offsetSnippet = offset.toBoolean() ? invoke(self, "generateOffset")({builder, offset}) : JSValue(nullptr);
            const auto flipUniform = invoke(self, "getBase")().get("_flipYUniform");
            const auto flipSnippet = flipUniform.toBoolean() ? invoke(flipUniform, "build")({builder, "bool"}) : JSValue(nullptr);
            auto finalDepth = depthSnippet;
            if (finalDepth.isNull() && texture.get("isArrayTexture").toBoolean() && !self.get("isTexture3DNode").strictEquals(true)) finalDepth = "0";
            const auto variable = invoke(builder, "getVarFromNode")({self}); propertyName = invoke(builder, "getPropertyName")({variable});
            auto snippet = invoke(self, "generateSnippet")({builder, property, uvSnippet, levelSnippet, biasSnippet, finalDepth, compareSnippet, gradSnippet, gatherSnippet, offsetSnippet, flipSnippet});
            JSValue snippetType;
            if (texture.get("isDepthTexture").strictEquals(true) && gatherSnippet.isNull()) snippetType = "float";
            else snippetType = texture.get("type").strictEquals(imports.get("UnsignedIntType")) ? "uvec4" : (texture.get("type").strictEquals(imports.get("IntType")) ? "ivec4" : "vec4");
            snippet = invoke(builder, "format")({snippet, snippetType, type});
            if (!compareStepSnippet.isNull()) {
                const auto function = texture.get("compareFunction");
                JSValue first, second;
                if (function.strictEquals(imports.get("GreaterCompare")) || function.strictEquals(imports.get("GreaterEqualCompare"))) {
                    first = imports.get("expression").call({}, {snippet, type}); second = imports.get("expression").call({}, {compareStepSnippet, "float"});
                } else { first = imports.get("expression").call({}, {compareStepSnippet, "float"}); second = imports.get("expression").call({}, {snippet, type}); }
                snippet = invoke(imports.get("step").call({}, {first, second}), "build")({builder, type});
            }
            const auto addLine = invoke(builder, "addLineFlowCode"); const auto left = propertyName.toString(); const auto right = snippet.toString();
            addLine({left + " = " + right, self}); data.set("snippet", snippet); data.set("propertyName", propertyName);
        }
        auto snippet = propertyName;
        if (invoke(builder, "needsToWorkingColorSpace")({texture}).toBoolean()) {
            const auto expr = imports.get("expression").call({}, {snippet, type});
            const auto converted = imports.get("colorSpaceToWorking").call({}, {expr, texture.get("colorSpace")});
            snippet = invoke(invoke(converted, "setup")({builder}), "build")({builder, type});
        }
        return invoke(builder, "format")({snippet, type, output});
    }
    if (name == "setSampler") { self.set("sampler", builder); return self; }
    if (name == "getSampler") return self.get("sampler");
    if (name == "getBase") return self.get("referenceNode").toBoolean() ? invoke(self.get("referenceNode"), "getBase")() : self;
    if (name == "load") return invoke(invoke(self, "sample")({builder}), "setSampler")({false});
    if (name == "size") return imports.get("textureSize").call({}, {self, builder});
    if (name == "isSampleCompare") return !self.get("compareNode").isNull() || self.get("_samplerComparison").strictEquals(true);
    if (name == "clone") {
        const auto ctor = self.get("constructor"); const auto value = self.get("value"), uv = self.get("uvNode"), level = self.get("levelNode"), bias = self.get("biasNode");
        auto node = ctor.construct({value, uv, level, bias});
        for (const auto* key : {"sampler", "depthNode", "compareNode", "gradNode", "gatherNode", "offsetNode"}) node.set(key, self.get(key));
        return node;
    }
    if (name == "serialize") {
        captures[1].getPrototype().get("serialize", self).call(self, {builder});
        const auto toJSON = invoke(self.get("value"), "toJSON"); builder.set("value", toJSON({builder.get("meta")}).get("uuid"));
        builder.set("sampler", self.get("sampler")); builder.set("updateMatrix", self.get("updateMatrix")); return {};
    }
    if (name == "deserialize") {
        captures[1].getPrototype().get("deserialize", self).call(self, {builder});
        const auto textures = builder.get("meta").get("textures"), key = builder.get("value"); self.set("value", textures.get(key));
        self.set("sampler", builder.get("sampler")); self.set("updateMatrix", builder.get("updateMatrix")); return {};
    }
    // These methods all create a true TextureNode clone before evaluating their
    // node arguments and finally resolve the original reference dynamically.
    if (name == "sample" || name == "level" || name == "bias" || name == "compare" || name == "grad" || name == "gather" || name == "depth" || name == "offset" || name == "blur") {
        auto node = invoke(self, "clone")();
        if (name == "grad") { const auto x = imports.get("nodeObject").call({}, {builder}); const auto y = imports.get("nodeObject").call({}, {raw(args, 1)}); node.set("gradNode", array({x, y})); }
        else if (name == "blur") {
            const auto multiply = invoke(imports.get("nodeObject").call({}, {builder}), "mul");
            node.set("biasNode", multiply({imports.get("maxMipLevel").call({}, {node})}));
        } else {
            const char* field = name == "sample" ? "uvNode" : name == "level" ? "levelNode" : name == "bias" ? "biasNode" : name == "compare" ? "compareNode" : name == "gather" ? "gatherNode" : name == "depth" ? "depthNode" : "offsetNode";
            node.set(field, imports.get("nodeObject").call({}, {name == "gather" ? arg(args, 0, 0) : builder}));
        }
        node.set("referenceNode", invoke(self, "getBase")());
        if (name == "blur") {
            const auto map = node.get("value");
            if (node.get("generateMipmaps").strictEquals(false) && ((map.toBoolean() && map.get("generateMipmaps").strictEquals(false)) || map.get("minFilter").strictEquals(imports.get("NearestFilter")) || map.get("magFilter").strictEquals(imports.get("NearestFilter")))) {
                ThreeUtils::warn({"TSL: texture().blur() requires mipmaps and sampling. Use .generateMipmaps=true and .minFilter/.magFilter=THREE.LinearFilter in the Texture."}); node.set("biasNode", nullptr);
            }
        }
        return node;
    }
    throw std::logic_error("Unknown TextureNode method");
}
JSValue factory(const JSValue&, const Args& args, const Args& captures) {
    const auto name = captures[0], imports = captures[1], bindings = captures[2];
    if (name.strictEquals("texture")) {
        const auto value = arg(args, 0, imports.get("EmptyTexture")), uv = arg(args, 1, nullptr), level = arg(args, 2, nullptr), bias = arg(args, 3, nullptr);
        if (value.toBoolean() && value.get("isTextureNode").strictEquals(true)) {
            auto node = invoke(value, "clone")(); node.set("referenceNode", invoke(value, "getBase")());
            if (!uv.isNull()) node.set("uvNode", imports.get("nodeObject").call({}, {uv}));
            if (!level.isNull()) node.set("levelNode", imports.get("nodeObject").call({}, {level}));
            if (!bias.isNull()) node.set("biasNode", imports.get("nodeObject").call({}, {bias})); return node;
        }
        return bindings.get("textureBase").call({}, {value, uv, level, bias});
    }
    if (name.strictEquals("uniformTexture")) { auto node = bindings.get("texture").call({}, {arg(args, 0, imports.get("EmptyTexture"))}); node.set("mergeable", false); return node; }
    if (name.strictEquals("textureLoad")) return invoke(bindings.get("texture").call({}, args), "setSampler")({false});
    if (name.strictEquals("textureLevel")) return invoke(bindings.get("texture").call({}, {raw(args, 0), raw(args, 1)}), "level")({raw(args, 2)});
    const auto value = raw(args, 0); const auto node = value.get("isNode").strictEquals(true) ? value : bindings.get("texture").call({}, {value});
    return invoke(node, "convert")({name.strictEquals("samplerComparison") ? "samplerComparison" : "sampler"});
}
}

JSValue nativeTextureNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak; if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "TextureNode module");
    auto imports = JSValue::object(nullptr);
    const auto base = nativeUniformNodeConstructor(); own(imports, "uniform", nativeUniformFactory());
    const auto uv = nativeUVModule(); own(imports, "uv", nativeUVBinding("uv"));
    const auto size = nativeTextureSizeNodeModule(); own(imports, "textureSize", nativeTextureSizeNodeBinding("textureSize"));
    const auto color = nativeColorSpaceNodeModule(); own(imports, "colorSpaceToWorking", nativeColorSpaceNodeBinding("colorSpaceToWorking"));
    const auto expression = nativeExpressionNodeModule(); own(imports, "expression", nativeExpressionNodeBinding("expression"));
    const auto mip = nativeMaxMipLevelNodeModule(); own(imports, "maxMipLevel", nativeMaxMipLevelNodeBinding("maxMipLevel"));
    const auto tsl = nativeTSLBase(); for (const auto* key : {"nodeProxy", "vec3", "nodeObject", "int", "Fn"}) own(imports, key, nativeTSLBaseBinding(key));
    const auto math = nativeMathNodeModule(); own(imports, "step", nativeMathNodeBinding("step"));
    const auto utils = nativeNodeUtils(); for (const auto* key : {"getTextureType", "hash"}) own(imports, key, nativeNodeUtilsBinding(key));
    const auto constants = nativeConstantsModule();
    for (const auto* key : {"Compatibility", "GreaterCompare", "GreaterEqualCompare", "IntType", "LessCompare", "LessEqualCompare", "NearestFilter", "UnsignedIntType"}) own(imports, key, nativeConstantsBinding(key));
    const auto texture = nativeTextureConstructor(); const auto error = nativeNodeErrorConstructor(); own(imports, "NodeError", error);
    own(imports, "EmptyTexture", texture.construct());
    auto holder = JSValue::object(nullptr), bindings = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor(+[](const JSValue& target, const Args& args, const Args& captures)->JSValue {
        const auto value = arg(args, 0, captures[1].get("EmptyTexture")), uv = arg(args, 1, nullptr), level = arg(args, 2, nullptr), bias = arg(args, 3, nullptr);
        auto self = superConstruct(target, captures, {value});
        self.set("isTextureNode", true); self.set("uvNode", uv); self.set("levelNode", level); self.set("biasNode", bias);
        for (const auto* key : {"compareNode", "depthNode", "gradNode", "gatherNode", "offsetNode"}) self.set(key, nullptr);
        self.set("sampler", true); self.set("updateMatrix", false); self.set("referenceNode", nullptr); self.set("_mergeable", true); self.set("_value", value);
        self.set("_matrixUniform", nullptr); self.set("_flipYUniform", nullptr); self.set("_samplerComparison", false);
        invoke(self, "setUpdateMatrix")({uv.isNull()}); return self;
    }, {holder, imports}, "TextureNode", 0);
    holder.set("constructor", ctor); ctor.setPrototype(base); auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype")); ctor.setFunctionSourceText(TextureNodeSource::TextureNode);
    JSPropertyDescriptor type; type.configurable = true; auto typeGetter = JSValue::function(+[](const JSValue&, const Args&, const Args&)->JSValue { return "TextureNode"; }, {}, "get type", 0); typeGetter.setFunctionSourceText(TextureNodeSource::get_type); type.getter = typeGetter; ctor.defineProperty("type", type);
    for (const auto* key : {"mergeable", "value"}) {
        JSPropertyDescriptor d; d.configurable = true; auto getter = JSValue::function(accessor, {key, false}, std::string("get ") + key, 0);
        auto setter = JSValue::function(accessor, {key, true}, std::string("set ") + key, 1); getter.setFunctionSourceText(TextureNodeSource::get(std::string("get_") + key)); setter.setFunctionSourceText(TextureNodeSource::get(std::string("set_") + key)); d.getter = getter; d.setter = setter; prototype.defineProperty(key, d);
    }
    for (const auto& [key, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
        {"getUniformHash",1},{"customCacheKey",0},{"generateNodeType",0},{"getInputType",0},{"getDefaultUV",0},{"updateReference",0},
        {"getTransformedUV",1},{"setUpdateMatrix",1},{"setupUV",2},{"setup",1},{"generateUV",2},{"generateOffset",2},{"generateSnippet",11},{"generate",2},
        {"setSampler",1},{"getSampler",0},{"sample",1},{"load",1},{"blur",1},{"level",1},{"size",1},{"bias",1},{"getBase",0},{"compare",1},{"grad",2},
        {"gather",0},{"isSampleCompare",0},{"depth",1},{"offset",1},{"serialize",1},{"deserialize",1},{"clone",0}}) {
        JSPropertyDescriptor d; auto function = JSValue::function(method, {key, prototype, imports}, key, arity); function.setFunctionSourceText(TextureNodeSource::get(key)); d.value = function; d.writable = d.configurable = true; prototype.defineProperty(key, d);
    }
    own(bindings, "default", ctor);
    const auto proxy = imports.get("nodeProxy").call({}, {ctor}); const auto limited = invoke(proxy, "setParameterLength")({1, 4});
    own(bindings, "textureBase", invoke(limited, "setName")({"texture"}));
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"texture",0},{"uniformTexture",0},{"textureLoad",0},{"textureLevel",3},{"sampler",1},{"samplerComparison",1}}) {
        auto function = JSValue::function(factory, {name, imports, bindings}, name, arity); function.setFunctionSourceText(TextureNodeSource::get(name)); own(bindings, name, function);
    }
    auto module = JSValue::object(); auto names = bindings.ownPropertyNames(); std::sort(names.begin(), names.end());
    for (const auto& name : names) if (name != "textureBase") own(module, name.c_str(), bindings.get(name));
    bindings.retainForHeapLifetime(); lexical() = JSWeakValue(bindings); module.retainForHeapLifetime(); weak = JSWeakValue(module); return module;
}
JSValue nativeTextureNodeBinding(const std::string& name) { const auto keeper = nativeTextureNodeModule(); return lexical().lock()->get(name); }
JSValue nativeTextureNodeConstructor() { return nativeTextureNodeBinding("default"); }
TextureNode::TextureNode(const JSValue& value, const JSValue& uv, const JSValue& level, const JSValue& bias)
    : object_(constructor().construct({value, uv, level, bias})) {}
