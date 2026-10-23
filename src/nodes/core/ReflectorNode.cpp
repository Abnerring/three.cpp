#include "nodes/core/ReflectorNode.h"

bool ReflectorNode::buildsFor(bool objectIsQuadMesh) {
    // A quad mesh is skipped.
    return !objectIsQuadMesh;
}

bool ReflectorNode::mayAskForDepth(bool depthOption) {
    // Only exactly true counts.
    return depthOption;
}

std::string ReflectorNode::depthNotEnabledMessage() {
    // The trailing space before the closing quote is in the source.
    return "THREE.ReflectorNode: Depth node can only be requested when the reflector is created "
           "with { depth: true }. ";
}

bool ReflectorNode::updatesMatrix() {
    // Turned off when the node is made.
    return false;
}

bool ReflectorNode::skipsWhileOneIsBeingRendered(bool alreadyRendering) {
    // A second reflector asked for during a render is declined, so a mirror cannot show itself.
    return alreadyRendering;
}
