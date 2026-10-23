#include "nodes/core/SampleNode.h"

SampleNode::CoordinateSource SampleNode::sourceUsedInSetup() {
    // The setup makes its own default coordinate rather than reading the stored one.
    return CoordinateSource::Default;
}

bool SampleNode::isCacheable() {
    return false;
}

double SampleNode::sample(double callbackAnswer) {
    // The node has nothing of its own to add.
    return callbackAnswer;
}
