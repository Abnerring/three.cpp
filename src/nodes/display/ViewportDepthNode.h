#pragma once
#include "core/JSValue.h"
JSValue nativeViewportDepthNodeConstructor();
JSValue nativeViewportDepthNodeModule();
JSValue nativeViewportDepthNodeBinding(const std::string& name);
#include <string>
#include <vector>

// Port of the rules in src/nodes/display/ViewportDepthNode.js at the pinned revision. The node
// answers one of three questions about depth depending on its scope, and three of its rules are
// worth writing down.
class ViewportDepthNode {
    JSValue object_;
  public:
    explicit ViewportDepthNode(const JSValue& scope={},const JSValue& valueNode={});
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeViewportDepthNodeBinding("default");}
  public:
    static constexpr bool isViewportDepthNode = true;
    static constexpr const char* kTypeName = "ViewportDepthNode";

    // The type the node reports. Depth is a single number whatever the scope.
    static constexpr const char* kNodeType = "float";

    // The three scopes, as the class exposes them.
    static constexpr const char* kDepthBase = "depthBase";
    static constexpr const char* kDepth = "depth";
    static constexpr const char* kLinearDepth = "linearDepth";

    static std::vector<std::string> scopes();

    // What generating the node produces. Only the base scope answers with the fragment's own
    // depth; the other two have nothing of their own and fall through to the ordinary path.
    static bool answersWithFragDepth(const std::string& scope);

    // What setting the node up produces, which may be nothing at all. The base scope with no value
    // answers nothing rather than the fragment depth; the depth scope answers a conversion of the
    // view position and never uses the value it was given, which its own documentation says; and
    // the linear scope has three cases, of which two use the value and one does not.
    enum class Result { Nothing, AssignedBase, PerspectiveDepth, OrthographicDepth, LinearFromValue, ValueAsItStands, LinearFromPosition };

    static Result setupResult(const std::string& scope, bool hasValue, bool isPerspectiveCamera);

    // How many conversions the linear scope with a value applies. A perspective camera has to undo
    // the projection and then convert again, so it takes two; an orthographic one is already
    // linear, so the value is passed through with none. That is the sharpest difference between
    // the two camera kinds in this node.
    static int conversionsForLinearValue(bool isPerspectiveCamera);

    // Whether the value node matters at all in a scope, which it does not for the depth one.
    static bool valueIsUsed(const std::string& scope);

    static bool isCacheable();
};
