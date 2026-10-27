#pragma once
#include <string>
#include "core/JSValue.h"

JSValue nativePropertyNodeConstructor();
JSValue nativePropertyNodeModule();
JSValue nativePropertyNodeBinding(const std::string& name);

// Port of the rules in src/nodes/core/PropertyNode.js at the pinned revision. The node names a
// shader variable the engine expects to exist, and four of its rules are worth writing down.
class PropertyNode {
    JSValue object_;
  public:
    explicit PropertyNode(const JSValue& nodeType = {}, const JSValue& name = nullptr, const JSValue& varying = false, const JSValue& placeholderNode = nullptr);
    PropertyNode(const PropertyNode& other) : object_(other.object_) {}
    PropertyNode& operator=(const PropertyNode& other) { object_ = other.object_; return *this; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativePropertyNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }
    static constexpr bool isPropertyNode = true;
    static constexpr const char* kTypeName = "PropertyNode";

    // The type name that is not a type. A property declared as this one answers with whatever the
    // builder says the output type is, so the name is a request to look it up rather than a kind
    // of value.
    static constexpr const char* kOutputTypeName = "output";

    static std::string nodeType(const std::string& declaredType, const std::string& builderOutputType);

    // The cache key. It is the type, the name and whether the property is a varying, joined with
    // colons, and the flag is written as the digit one or nought rather than as a word. A
    // property with no name leaves the middle empty rather than writing the word for absence.
    static std::string cacheKeyText(const std::string& type, bool hasName, const std::string& name,
                                    bool varying);

    // Whether the property is a varying, which decides how it is made: a varying is taken from
    // the builder with the interpolation flag set unconditionally, while an ordinary property is
    // taken as a variable.
    static bool isVarying(bool varying);

    // Whether a varying's interpolation flag is set only when it is not already: it is not, since
    // the source assigns true without asking. That is different from the varying node, which asks.
    static bool interpolationSetUnconditionally();

    // Whether the placeholder is written. It is written only when there is a placeholder and the
    // property has never been written to, since a property that is written elsewhere does not
    // need a starting value. The test for a write is a strict comparison against false.
    static bool writesPlaceholder(bool hasPlaceholder, bool hasWriteUsage);

    static bool isCacheable();
};
