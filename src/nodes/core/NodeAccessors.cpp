#include "nodes/core/NodeAccessors.h"

bool NodeAccessors::scopeIsTheNodeItself() {
    return true;
}

std::string NodeAccessors::hash(std::uint64_t id) {
    // The identifier as text, as the parameter node also does.
    return std::to_string(id);
}

std::uint64_t NodeAccessors::customCacheKey(std::uint64_t id) {
    // As a number here, which is the other choice the layer makes for the same identifier.
    return id;
}

std::string NodeAccessors::propertyNameFromStorageName(const std::string& storageName) {
    // Four characters off the end, which is the word for a node.
    if (storageName.size() <= 4) return std::string();
    return storageName.substr(0, storageName.size() - 4);
}

bool NodeAccessors::shouldRecompute(bool callerAsked, int version, int keyVersion) {
    // The caller's request is or-ed with the version check.
    return callerAsked || version != keyVersion;
}
