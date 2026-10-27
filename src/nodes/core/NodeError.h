#pragma once
#include "nodes/core/StackTrace.h"

#include <optional>
#include <stdexcept>
#include <string>

JSValue nativeNodeErrorConstructor();

// Port of src/nodes/core/NodeError.js: an error carrying a node stack trace.
class NodeError : public std::runtime_error {
  public:
    explicit NodeError(const std::string& message, std::optional<StackTrace> stackTrace = {})
        : std::runtime_error(message), name("NodeError"), stackTrace(std::move(stackTrace)) {}

    // Upstream sets this.name; the message is what std::runtime_error already carries.
    std::string name;
    std::optional<StackTrace> stackTrace;
};
