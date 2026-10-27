#pragma once
#include "core/JSValue.h"
#include <string>
#include <vector>
JSValue nativeLoopNodeConstructor();
JSValue nativeLoopNodeModule();
JSValue nativeLoopNodeBinding(const std::string& name);

// Port of the rules in src/nodes/utils/LoopNode.js at the pinned revision. The module builds loops
// out of a list of parameters whose last entry is the body, and four of its rules are worth
// writing down.
class LoopNode {
    JSValue object_;
  public:
    explicit LoopNode(const JSValue& params=JSValue::array()):object_(constructor().construct({params})){}
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeLoopNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    JSValue getVarName(const JSValue& index)const{return object_.get("getVarName").call(object_,{index});}
    JSValue getProperties(const JSValue& builder)const{return object_.get("getProperties").call(object_,{builder});}
    JSValue setup(const JSValue& builder)const{return object_.get("setup").call(object_,{builder});}
    JSValue generate(const JSValue& builder)const{return object_.get("generate").call(object_,{builder});}
    static constexpr const char* kTypeName = "LoopNode";

    // The type the node reports. A loop is a statement rather than a value, so it is the empty
    // type.
    static constexpr const char* kNodeType = "void";

    // The name of the loop variable at a position. It is the letter i and the character that many
    // places after it, so the first three are i, j and k as a reader would expect, and the fourth
    // is l rather than anything more exotic; the point is that there is no table of names, only
    // an offset from i.
    static std::string varName(int index);

    // What a bare callback means. When the first parameter is a function there is no bound to
    // count towards, so a constant true is put in front of it and the loop becomes an unbounded
    // one: the callback is the body and the condition never fails.
    static std::vector<std::string> internalParams(bool firstIsFunction);

    // Which parameters describe the loop and which is the body. Every parameter but the last
    // describes it, and the last is always the body whatever it holds, so a list with one entry
    // has no descriptors at all and the loop runs on the constant true in front of it.
    static int descriptorCount(int paramCount);

    // The default type and name a descriptor that does not carry its own gets. The type defaults
    // to an integer, and the name to the positional one, so a descriptor may be a plain number
    // and still produce a usable variable.
    static constexpr const char* kDefaultType = "int";

    static std::string paramName(bool paramIsNode, bool hasName, const std::string& name, int index);
    static std::string paramType(bool paramIsNode, bool hasType, const std::string& type);

    // Whether the body is invoked once and its result kept, which it is: a record is checked
    // before the work and written after it, so a second question finds the record and does no
    // work. That matters because the body is a callable the caller supplied, and invoking it
    // twice would run its side effects twice.
    static bool bodyIsInvokedOnce();

    static bool isCacheable();
};
