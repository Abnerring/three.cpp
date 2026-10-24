#include "nodes/core/NodeSetup.h"

bool NodeSetup::defaultReferenceIsTheNode() {
    return true;
}

bool NodeSetup::replacesTheMethodOnTheInstance() {
    return true;
}

bool NodeSetup::globalAnswerIsTheFlag() {
    return true;
}

bool NodeSetup::marksBeforeTheWork() {
    return true;
}

NodeSetup::OutputSource NodeSetup::outputSource(bool setupAnswered, bool hadOneBefore) {
    // The first of the three that is a value.
    if (setupAnswered) return OutputSource::FromSetup;
    if (hadOneBefore) return OutputSource::FromBefore;
    return OutputSource::None;
}

bool NodeSetup::registersAsParent(bool parentsFlagIsExactlyTrue) {
    return parentsFlagIsExactlyTrue;
}
