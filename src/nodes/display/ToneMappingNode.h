#pragma once
#include "core/JSValue.h"
#include <cstdint>
#include <string>

JSValue nativeToneMappingNodeConstructor();
JSValue nativeToneMappingNodeModule();
JSValue nativeToneMappingNodeBinding(const std::string& name);

class ToneMappingNode {
    JSValue object_;
  public:
    explicit ToneMappingNode(const JSValue& toneMapping = {}, const JSValue& exposureNode = {}, const JSValue& colorNode = nullptr)
        : object_(constructor().construct({toneMapping,exposureNode,colorNode})) {}
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeToneMappingNodeBinding("default"); }
    static JSValue prototype() { return constructor().get("prototype"); }
};

// Port of the rules in src/nodes/display/ToneMappingNode.js at the pinned revision. The node
// applies a tone mapping to a colour; what is portable is its cache key, which has to tell the
// mappings apart, and where it takes its colour from when it was not given one.
class ToneMapping {
  public:
    static constexpr const char* kTypeName = "ToneMappingNode";
    static constexpr const char* kNodeType = "vec3";

    // The name the no-mapping case goes by, which the setup step compares against.
    static constexpr const char* kNoToneMappingName = "NoToneMapping";

    // The key the node caches under. Since the mapping is part of what the node does, two nodes
    // that differ only in their mapping must not share a cache entry, so the number is folded
    // into the key rather than the key being fixed.
    static double customCacheKey(int toneMapping);

    // Whether the same key comes out for two mappings, which must be false for any pair a
    // caller is likely to use. The node's own defaults occupy the low numbers.
    static bool keysCollide(int first, int second);

    // Which colour the node works on. It uses the one it was given, and falls back to the
    // builder's colour when that is falsy rather than only when it is absent, since the source
    // uses the short-circuiting or.
    static bool usesGivenColour(bool hasColourNode, bool colourNodeIsTruthy);
};
