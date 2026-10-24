#pragma once
#include "core/JSValue.h"
#include <string>

// Port of src/nodes/core/NodeCode.js: user supplied native shader code that the builder
// injects. Upstream marks instances with a non-enumerable isNodeCode property.
class NodeCode {
  public:
    static constexpr bool isNodeCode = true;

    NodeCode(std::string name, std::string type, std::string code = {})
        : name(std::move(name)), type(std::move(type)), code(std::move(code)) {}

    std::string name;
    std::string type;
    std::string code;
};

JSValue nativeNodeCodeConstructor();
