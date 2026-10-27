#pragma once
#include "core/JSValue.h"

JSValue nativeTextureNodeConstructor();
JSValue nativeTextureNodeModule();
JSValue nativeTextureNodeBinding(const std::string& name);

class TextureNode {
    JSValue object_;
    JSValue call(const char* name, const std::vector<JSValue>& args = {}) const {
        return object_.get(name).call(object_, args);
    }
public:
    TextureNode(const JSValue& value = {}, const JSValue& uvNode = nullptr,
                const JSValue& levelNode = nullptr, const JSValue& biasNode = nullptr);
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeTextureNodeBinding("default"); }
    static JSValue prototype() { return constructor().get("prototype"); }
    JSValue getUniformHash(const JSValue& builder = {}) const { return call("getUniformHash", {builder}); }
    JSValue customCacheKey() const { return call("customCacheKey"); }
    JSValue generateNodeType() const { return call("generateNodeType"); }
    JSValue getInputType() const { return call("getInputType"); }
    JSValue getDefaultUV() const { return call("getDefaultUV"); }
    JSValue updateReference() const { return call("updateReference"); }
    JSValue getTransformedUV(const JSValue& uv) const { return call("getTransformedUV", {uv}); }
    JSValue setUpdateMatrix(const JSValue& value) const { return call("setUpdateMatrix", {value}); }
    JSValue setupUV(const JSValue& builder, const JSValue& uv) const { return call("setupUV", {builder, uv}); }
    JSValue setup(const JSValue& builder) const { return call("setup", {builder}); }
    JSValue generateUV(const JSValue& builder, const JSValue& uv) const { return call("generateUV", {builder, uv}); }
    JSValue generateOffset(const JSValue& builder, const JSValue& offset) const { return call("generateOffset", {builder, offset}); }
    JSValue generateSnippet(const JSValue& builder, const JSValue& texture, const JSValue& uv,
        const JSValue& level, const JSValue& bias, const JSValue& depth, const JSValue& compare,
        const JSValue& grad, const JSValue& gather, const JSValue& offset, const JSValue& flip) const {
        return call("generateSnippet", {builder, texture, uv, level, bias, depth, compare, grad, gather, offset, flip});
    }
    JSValue generate(const JSValue& builder, const JSValue& output = {}) const { return call("generate", {builder, output}); }
    JSValue setSampler(const JSValue& value) const { return call("setSampler", {value}); }
    JSValue getSampler() const { return call("getSampler"); }
    JSValue sample(const JSValue& uv) const { return call("sample", {uv}); }
    JSValue load(const JSValue& uv) const { return call("load", {uv}); }
    JSValue blur(const JSValue& amount) const { return call("blur", {amount}); }
    JSValue level(const JSValue& value) const { return call("level", {value}); }
    JSValue size(const JSValue& value = {}) const { return call("size", {value}); }
    JSValue bias(const JSValue& value) const { return call("bias", {value}); }
    JSValue getBase() const { return call("getBase"); }
    JSValue compare(const JSValue& value) const { return call("compare", {value}); }
    JSValue grad(const JSValue& x, const JSValue& y) const { return call("grad", {x, y}); }
    JSValue gather(const JSValue& value = {}) const { return call("gather", {value}); }
    JSValue isSampleCompare() const { return call("isSampleCompare"); }
    JSValue depth(const JSValue& value) const { return call("depth", {value}); }
    JSValue offset(const JSValue& value) const { return call("offset", {value}); }
    JSValue serialize(const JSValue& data) const { return call("serialize", {data}); }
    JSValue deserialize(const JSValue& data) const { return call("deserialize", {data}); }
    JSValue clone() const { return call("clone"); }
};
