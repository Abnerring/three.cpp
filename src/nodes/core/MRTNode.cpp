#include "nodes/core/MRTNode.h"

int MRTNode::textureIndex(const std::vector<std::string>& textureNames, const std::string& name) {
    for (std::size_t i = 0; i < textureNames.size(); ++i) {
        if (textureNames[i] == name) return static_cast<int>(i);
    }
    // The array idiom for a miss.
    return -1;
}

std::string MRTNode::blendMode(bool outputWasGiven, bool nameWasSet,
                               const std::string& givenMode) {
    // A name that was set uses its own mode.
    if (nameWasSet) return givenMode;
    // The output name has a default of its own; every other name falls back to no blending.
    if (outputWasGiven) return kMaterialBlending;
    return kNoBlending;
}

bool MRTNode::skipsUnusedOutput() {
    return true;
}

bool MRTNode::membersArePlacedByIndex() {
    return true;
}

bool MRTNode::leavesGaps(bool declaresSecondOnly) {
    // Declaring only the second output places it at index one, so index nought is absent.
    return declaresSecondOnly;
}

bool MRTNode::argumentWinsOnConflict() {
    return true;
}

bool MRTNode::isCacheable() {
    return false;
}
