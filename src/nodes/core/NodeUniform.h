#pragma once
#include "core/JSValue.h"
#include <cstdint>
#include <string>

// Port of src/nodes/core/NodeUniform.js at the pinned revision. A node uniform is what the
// builder hands to the shader: a name, a type and a reference to the node the value comes from.
// It holds no value of its own, so everything it answers is the node's answer, which is why the
// three things it forwards differ: the value may be written as well as read, while the id and
// the group are only read.
class NodeUniform {
  public:
    static constexpr bool isNodeUniform = true;

    NodeUniform(std::string name, std::string type, std::int64_t id = 0,
                std::string groupName = {})
        : name_(std::move(name)),
          type_(std::move(type)),
          id_(id),
          groupName_(std::move(groupName)) {}

    const std::string& name() const { return name_; }
    const std::string& type() const { return type_; }

    // The id and the group come from the node and cannot be written here, since the source
    // gives them a getter and nothing else.
    std::int64_t id() const { return id_; }
    const std::string& groupName() const { return groupName_; }

    // The value may be read and written, both of which go to the node's own value.
    double value() const { return value_; }
    void setValue(double value) { value_ = value; }

  private:
    std::string name_;
    std::string type_;
    std::int64_t id_ = 0;
    std::string groupName_;
    double value_ = 0;
};

JSValue nativeNodeUniformConstructor();
