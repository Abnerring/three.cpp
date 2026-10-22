#pragma once
#include "core/JSValue.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

// Port of src/nodes/core/NodeCache.js: the builder's per node data cache.
//
// Upstream keys a WeakMap by Node identity. The C++ version keys by pointer, which is the
// same identity relation, and stores a JSValue so callers can put whatever the node code
// needs there. A cache may point at a parent and look there when it has nothing itself.
class NodeCache {
  public:
    explicit NodeCache(std::shared_ptr<NodeCache> parent = {});

    std::uint64_t id() const { return _id; }
    const std::shared_ptr<NodeCache>& parent() const { return _parent; }

    // Returns undefined when neither this cache nor any parent holds data for the key.
    JSValue getData(const void* node) const;
    void setData(const void* node, JSValue data);

  private:
    std::uint64_t _id;
    std::unordered_map<const void*, JSValue> _nodesData;
    std::shared_ptr<NodeCache> _parent;
};
