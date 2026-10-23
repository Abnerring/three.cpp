#include "nodes/core/UniformNode.h"

std::string UniformNode::uniformHash(const std::string& ordinaryHash) {
    // The ordinary hash, unless a derived class says otherwise.
    return ordinaryHash;
}

UniformNode::Shared UniformNode::sharedNode(bool hashAlreadyKnown, std::uint64_t registeredId,
                                            std::uint64_t ownId) {
    Shared out;
    if (hashAlreadyKnown) {
        // The first one registered is the one they all share.
        out.wasRegistered = false;
        out.nodeId = registeredId;
    } else {
        // Nothing is registered yet, so this node registers itself and becomes the shared one.
        out.wasRegistered = true;
        out.nodeId = ownId;
    }
    return out;
}

bool UniformNode::hashesMergeNodes() {
    return true;
}

std::string UniformNode::deprecationWarning() {
    return "TSL: \"label()\" has been deprecated. Use \"setName()\" instead.";
}
