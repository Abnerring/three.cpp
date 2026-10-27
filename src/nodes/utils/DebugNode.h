#pragma once
#include <string>
#include "core/JSValue.h"
JSValue nativeDebugNodeConstructor();
JSValue nativeDebugNodeModule();
JSValue nativeDebugNodeBinding(const std::string& name);

// Port of the text the debug node assembles in src/nodes/utils/DebugNode.js at the pinned
// revision, when it is given no callback of its own. What is worth writing down is how the
// border is sized and what the tidying of the shader text leaves alone.
class DebugNode {
    JSValue object_;
  public:
    DebugNode(const JSValue& node,const JSValue& callback=nullptr);
    JSValue object()const{return object_;}
    operator JSValue()const{return object_;}
    static JSValue constructor(){return nativeDebugNodeBinding("default");}
    static JSValue prototype(){return constructor().get("prototype");}
    static constexpr const char* kTypeName = "DebugNode";

    // The title a stage is given. The two dashes at each end are part of it, so the border that
    // is made from its length covers them too.
    static std::string title(const std::string& shaderStage);

    // A run of dashes as long as the title. Since the title already ends in dashes, the border
    // is exactly as wide as the line it closes rather than two wider.
    static std::string border(const std::string& title);

    // The shader text with the tabs at the start of each line removed. Only tabs are removed and
    // only when they begin a line, so a tab inside a line and any spaces are left as they are.
    static std::string stripLeadingTabs(const std::string& code);

    // The whole block, in the order the node builds it.
    static std::string block(const std::string& shaderStage, const std::string& code,
                             const std::string& snippet);
};
