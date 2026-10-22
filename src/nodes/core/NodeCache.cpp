#include "nodes/core/NodeCache.h"

namespace {
std::uint64_t nextCacheId() {
    static std::uint64_t id = 0;
    return id++;
}
}  // namespace

NodeCache::NodeCache(std::shared_ptr<NodeCache> parent)
    : _id(nextCacheId()), _parent(std::move(parent)) {}

JSValue NodeCache::getData(const void* node) const {
    const auto found = _nodesData.find(node);
    if (found != _nodesData.end()) return found->second;
    // Upstream falls back to the parent only when the WeakMap has no entry at all.
    return _parent ? _parent->getData(node) : JSValue();
}

void NodeCache::setData(const void* node, JSValue data) {
    _nodesData[node] = std::move(data);
}
