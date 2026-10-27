#pragma once
#include "core/JSValue.h"
JSValue nativeFlipNodeConstructor();

#include <string>
#include <vector>

// Port of the component matching in src/nodes/utils/FlipNode.js at the pinned revision. Each
// component flips with one less itself, which is simple; which components flip is decided by a
// walk that compares one name at a time, and that walk has a consequence the name does not tell
// you about.
class FlipNode {
    JSValue object_;
  public:
    explicit FlipNode(const JSValue& sourceNode = {}, const JSValue& components = {}) : object_(nativeFlipNodeConstructor().construct({sourceNode, components})) {}
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeFlipNodeConstructor(); }
    static JSValue prototype() { return constructor().get("prototype"); }

    static constexpr const char* kTypeName = "FlipNode";

    // The names the components go by, in the order the source walks them.
    static const std::vector<std::string>& componentNames();

    // Which of a type's components are flipped. The names are compared in step with the walk, so
    // a list must be in the same order as the components it names: asking for the second then
    // the first matches neither, because at the first step the walk is looking for the first and
    // at the second step it is looking for the second. Nothing is flipped at all in that case,
    // and no error is raised.
    static std::vector<bool> flipped(int length, const std::string& components);

    // One component, after flipping if it was named. A flip is one less the value, which takes
    // nought to one and one to nought.
    static double flip(double value);

    // Whether a list of names is in the order the walk needs, which a caller may use to avoid
    // the silent case.
    static bool isInWalkOrder(const std::string& components);
};
