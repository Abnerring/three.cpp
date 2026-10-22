#include "nodes/core/Node.h"

namespace {
std::uint64_t nextNodeId() {
    static std::uint64_t id = 0;
    return id++;
}
}  // namespace

Node::Node(std::optional<std::string> nodeType) : _nodeType(std::move(nodeType)), _id(nextNodeId()) {
    // stackTrace stays empty: upstream fills it from a JavaScript engine stack, which has no
    // equivalent here, so it is only set when a caller supplies a trace of its own.
}
