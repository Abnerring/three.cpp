#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeUserDataNodeConstructor();
JSValue nativeUserDataNodeModule();
JSValue nativeUserDataNodeBinding(const std::string& name);

class UserDataNode {
    JSValue object_;

  public:
    static constexpr bool isUserDataNode = true;
    static constexpr const char* kTypeName = "UserDataNode";

    explicit UserDataNode(const JSValue& property = {}, const JSValue& inputType = {},
                          const JSValue& userData = nullptr)
        : object_(constructor().construct({property, inputType, userData})) {}

    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeUserDataNodeBinding("default"); }
    static JSValue prototype() { return constructor().get("prototype"); }
};
