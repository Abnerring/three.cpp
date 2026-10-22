#pragma once
#include <optional>
#include <string>

// Port of src/nodes/core/NodeFunctionInput.js: describes one input of a NodeFunction.
class NodeFunctionInput {
  public:
    static constexpr bool isNodeFunctionInput = true;

    NodeFunctionInput(std::string type, std::string name, std::optional<double> count = {},
                      std::string qualifier = {}, bool isConst = false)
        : type(std::move(type)),
          name(std::move(name)),
          count(count),
          qualifier(std::move(qualifier)),
          isConst(isConst) {}

    std::string type;
    std::string name;
    // If the input is an array, the length; upstream uses null, which is an empty optional.
    std::optional<double> count;
    // "in", "out" or "inout"; only relevant for GLSL.
    std::string qualifier;
    // Whether the parameter takes a const qualifier; only relevant for GLSL.
    bool isConst = false;
};
