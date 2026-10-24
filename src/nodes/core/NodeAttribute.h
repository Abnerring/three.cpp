#pragma once
#include "core/JSValue.h"
#include <cstdint>
#include <string>

// Port of src/nodes/core/NodeAttribute.js at the pinned revision. The class holds one attribute's
// name and type together with an optional reference to the node that asked for it; it has no
// behaviour of its own.
class NodeAttribute {
  public:
    static constexpr bool isNodeAttribute = true;

    NodeAttribute(std::string name, std::string type, bool hasNode = false,
                  std::uint64_t nodeId = 0)
        : name_(std::move(name)),
          type_(std::move(type)),
          hasNode_(hasNode),
          nodeId_(nodeId) {}

    const std::string& name() const { return name_; }
    const std::string& type() const { return type_; }

    // The node is optional and starts absent, so an attribute may be declared without one.
    bool hasNode() const { return hasNode_; }
    std::uint64_t nodeId() const { return nodeId_; }

  private:
    std::string name_;
    std::string type_;
    bool hasNode_ = false;
    std::uint64_t nodeId_ = 0;
};

JSValue nativeNodeAttributeConstructor();
