#pragma once
#include "nodes/core/NodeConstants.h"

#include <cstdint>
#include <optional>
#include <string>

// Port of src/nodes/core/Node.js at the pinned revision, partial: the identity, cache key
// and type accessors that are plain logic. The methods that need a NodeBuilder (setup,
// analyze, generate, build) are not implemented and report that by name instead.
//
// Upstream registers every node on construction and hands out an increasing id; the same
// function local counter is used here.
class Node {
  public:
    static constexpr bool isNode = true;

    // Upstream defaults this to false and captures a JavaScript stack when it is true.
    // There is no engine stack to capture here, so the trace stays empty unless a caller
    // supplies one.
    static inline bool captureStackTrace = false;

    explicit Node(std::optional<std::string> nodeType = {});
    virtual ~Node() = default;

    std::uint64_t id() const { return _id; }
    std::uint64_t version() const { return _version; }
    std::string_view name() const { return _name; }
    void setName(std::string name) { _name = std::move(name); }
    bool global() const { return _global; }
    bool parents() const { return _parents; }
    const std::optional<std::string>& nodeType() const { return _nodeType; }

    // Whether caching the result in a temporary variable is allowed.
    virtual bool isCacheable() const { return true; }
    // Upstream returns this.id; derived nodes may override.
    virtual double customCacheKey() const { return static_cast<double>(_id); }
    // Returns this node, which is the default scope.
    virtual Node* getScope() { return this; }
    // The identifier used when hashing the node; upstream returns String( this.id ).
    virtual std::string getHash() const { return std::to_string(_id); }

    virtual std::string_view getUpdateType() const { return _updateType; }
    virtual std::string_view getUpdateBeforeType() const { return _updateBeforeType; }
    virtual std::string_view getUpdateAfterType() const { return _updateAfterType; }

  protected:
    std::optional<std::string> _nodeType;
    std::string_view _updateType = NodeUpdateType::NONE;
    std::string_view _updateBeforeType = NodeUpdateType::NONE;
    std::string_view _updateAfterType = NodeUpdateType::NONE;
    std::uint64_t _version = 0;
    std::string _name;
    bool _global = false;
    bool _parents = false;

  private:
    std::uint64_t _id;
};
