#include "nodes/core/InspectorNode.h"

std::string InspectorNode::name(const std::string& ownName, const std::string& wrappedName) {
    // The own name when it is a value worth using, the wrapped one otherwise.
    if (!ownName.empty()) return ownName;
    return wrappedName;
}

bool InspectorNode::runsCallback(bool contextInspectorIsTrue, bool hasCallback) {
    // The flag exactly true, the callback merely present.
    return contextInspectorIsTrue && hasCallback;
}

bool InspectorNode::warnsAboutBackend(bool isWebGpuBackend, bool inspectorIsBaseClass) {
    // Both negations: not the WebGPU backend and not the base class.
    return !isWebGpuBackend && !inspectorIsBaseClass;
}

std::string InspectorNode::backendWarning() {
    return "TSL: \".toInspector()\" is only available with WebGPU.";
}

bool InspectorNode::answersTheWrappedNodesChain() {
    // The expression continues from the wrapped node, with the inspector inserted before it.
    return true;
}
