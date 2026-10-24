#pragma once
#include "core/JSValue.h"
#include <memory>

// NodeCache.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
// The ordinary runtime object is authoritative. Its nodesData field contains a
// real ephemeron WeakMap; neither cache entries nor their keys are native roots.
class NodeCache {
  public:
    NodeCache();
    explicit NodeCache(const JSValue& parent);
    explicit NodeCache(const std::shared_ptr<NodeCache>& parent);

    JSValue object() const;
    static JSValue constructor();
    static JSValue prototype();
    JSValue id() const;
    JSValue parent() const;
    JSValue getData(const JSValue& node) const;
    JSValue setData(const JSValue& node, const JSValue& data);

  private:
    JSValue object_;
};

JSValue nativeNodeCacheConstructor();
