#pragma once
#include <string>
#include "core/JSValue.h"

JSValue nativeInspectorNodeConstructor();
JSValue nativeInspectorNodeModule();
JSValue nativeInspectorNodeBinding(const std::string& name);

// Port of the rules in src/nodes/core/InspectorNode.js at the pinned revision. The node wraps
// another so that its value can be looked at while rendering, and three of its rules are about
// what happens when the name or the condition is not what a reader assumes.
class InspectorNode {
    JSValue object_;
  public:
    InspectorNode(const JSValue& node,const JSValue& name="",const JSValue& callback=nullptr);
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeInspectorNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue getName()const{return object_.get("getName").call(object_);}
    JSValue update(const JSValue& frame)const{return object_.get("update").call(object_,{frame});}
    JSValue generateNodeType(const JSValue& builder)const{return object_.get("generateNodeType").call(object_,{builder});}
    JSValue setup(const JSValue& builder)const{return object_.get("setup").call(object_,{builder});}
    static constexpr const char* kTypeName = "InspectorNode";
    static constexpr bool isInspectorNode = true;

    // The name the node answers with. Its own is used when it is a value worth using, and the
    // wrapped node's otherwise, so a node made without a name borrows one rather than answering
    // with nothing.
    static std::string name(const std::string& ownName, const std::string& wrappedName);

    // Whether the callback runs during setup. Two conditions guard it and they are tested
    // differently: the context's flag is compared against true exactly, while the callback is
    // compared against null. So a context flag that is merely present does not run the callback,
    // and a callback that is present but not a function would be called.
    static bool runsCallback(bool contextInspectorIsTrue, bool hasCallback);

    // Whether the warning about the backend is printed. It needs a backend that is not exactly
    // the WebGPU one and an inspector that is not the base class, so both negations have to
    // hold; a default inspector on a non-WebGPU backend is silent.
    static bool warnsAboutBackend(bool isWebGpuBackend, bool inspectorIsBaseClass);

    // The sentence printed.
    static std::string backendWarning();

    // What the wrapper function answers. It inserts the inspector before the wrapped node and
    // answers that node's before-chain rather than the inspector itself, so the original node is
    // what continues in the expression.
    static bool answersTheWrappedNodesChain();

    // The kind of refresh the node asks for, which is once per frame.
    static constexpr const char* kUpdateType = "FRAME";
};
