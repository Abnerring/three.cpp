#pragma once
#include "core/JSValue.h"
#include <cstdint>
#include <string>

// Port of the rules in src/nodes/core/UniformNode.js at the pinned revision. The node holds a
// value that the shader reads, and four of its rules are worth writing down, one of them being
// that two nodes can turn out to be one.
JSValue nativeUniformNodeConstructor();
JSValue nativeUniformFactory();
class UniformNode {
    JSValue object_;
  public:
    explicit UniformNode(const JSValue& value = {}, const JSValue& nodeType = nullptr);
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeUniformNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    static constexpr bool isUniformNode = true;
    static constexpr const char* kTypeName = "UniformNode";

    // The name a fresh uniform carries, which is the empty string rather than nothing.
    static constexpr const char* kDefaultName = "";

    // The group a fresh uniform belongs to. It is the per-object one rather than a shared one, so
    // a uniform made without saying follows its object; a caller that wanted it updated once per
    // frame or per render has to ask.
    static constexpr const char* kDefaultGroup = "object";

    // The hash a uniform is known by, which is the ordinary node hash unless a derived class
    // answers otherwise.
    static std::string uniformHash(const std::string& ordinaryHash);

    // Which node a uniform actually refers to. Uniforms with the same hash share one, and the one
    // they share is the first registered for that hash; so two nodes made separately and hashing
    // alike turn out to be one node, which is a merge rather than a copy and is what the
    // mechanism is for.
    struct Shared {
        bool wasRegistered = false;
        std::uint64_t nodeId = 0;
    };

    static Shared sharedNode(bool hashAlreadyKnown, std::uint64_t registeredId,
                             std::uint64_t ownId);

    // Whether two node identities that hash alike end up as one node, which they do.
    static bool hashesMergeNodes();

    // The sentence the deprecated setter prints.
    static std::string deprecationWarning();
};
