#include "nodes/core/NodeFrame.h"

NodeFrame::GateDecision NodeFrame::decide(UpdateType type, const Gate& gate, std::uint64_t frameId,
                                          std::uint64_t renderId, bool recordsFirst) {
    // An object update is never gated, so it always runs.
    if (type == UpdateType::Object) return GateDecision::RunUngated;
    if (type == UpdateType::Frame) {
        // A node that has already run for this frame is skipped.
        if (gate.frameId == frameId) return GateDecision::Skip;
        return recordsFirst ? GateDecision::RunRecordingFirst : GateDecision::RunRecordingLast;
    }
    if (type == UpdateType::Render) {
        if (gate.renderId == renderId) return GateDecision::Skip;
        return recordsFirst ? GateDecision::RunRecordingFirst : GateDecision::RunRecordingLast;
    }
    // A node that names no kind of update is not run at all, which the source reaches by
    // comparing against none of the three.
    return GateDecision::Skip;
}

void NodeFrame::advance(double stepSeconds) {
    // The frame counter goes up, and the elapsed seconds grow by the step; the caller has
    // already worked the step out from its own two clock readings.
    frameId_ += 1;
    seconds_ += stepSeconds;
    time_ += 1;
}
