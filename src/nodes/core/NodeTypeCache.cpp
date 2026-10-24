#include "nodes/core/NodeTypeCache.h"

NodeTypeCache::CacheSlot NodeTypeCache::slotFor(bool outputWasNamed) {
    // A named output gets a map keyed by it; no output gets a single slot.
    if (outputWasNamed) return CacheSlot::PerOutput;
    return CacheSlot::Single;
}

NodeTypeCache::Lookup NodeTypeCache::lookup(const std::optional<std::string>& remembered,
                                            const std::string& computedOnMiss) {
    Lookup out;
    if (remembered.has_value()) {
        // An entry that exists is used as it stands, even when it is the empty string.
        out.wasRemembered = true;
        out.type = remembered.value();
        return out;
    }
    out.wasRemembered = false;
    out.type = computedOnMiss;
    return out;
}

bool NodeTypeCache::emptyTypeIsRemembered() {
    return true;
}

NodeTypeCache::Source NodeTypeCache::sourceFor(bool hasOutputNode) {
    // The wrapper's own output node answers when there is one, the declared type otherwise.
    if (hasOutputNode) return Source::DelegatedToOutputNode;
    return Source::OwnDeclaredType;
}

bool NodeTypeCache::outputIsPassedThrough() {
    return true;
}
