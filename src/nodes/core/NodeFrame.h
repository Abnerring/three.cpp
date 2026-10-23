#pragma once
#include <cstdint>

// Port of the counters and the update rules in src/nodes/core/NodeFrame.js at the pinned
// revision. What lives here is the bookkeeping the frame uses to decide whether a node's update
// should run now or has already run for this frame or render; the node methods themselves belong
// to the node.
class NodeFrame {
  public:
    // The three kinds of update a node may ask for.
    enum class UpdateType { None, Frame, Render, Object };

    // How a node's update is gated. A node that has not run for the current frame has a
    // different stored id from the frame's own, so the difference is what lets it run.
    struct Gate {
        std::uint64_t renderId = 0;
        std::uint64_t frameId = 0;
    };

    // Whether a gated update should run now, and what the gate becomes afterwards. The three
    // methods in the source differ in one thing: which of the two they do first. An update
    // before a frame records the id first and takes it back if the node says it did not run,
    // while an update and an update after call the node first and record only on success. The
    // two orders therefore differ when a node declines: one has already claimed the frame and
    // must undo, the other has not claimed yet. This function reports both the decision and the
    // gate, leaving the undo to the caller so the difference is visible rather than hidden.
    enum class GateDecision {
        // The update has already run for this frame or render.
        Skip,
        // Run, and record the id first; take it back if the node declines.
        RunRecordingFirst,
        // Run, and record the id only if the node does not decline.
        RunRecordingLast,
        // Run with no record at all.
        RunUngated
    };

    static GateDecision decide(UpdateType type, const Gate& gate, std::uint64_t frameId,
                               std::uint64_t renderId, bool recordsFirst);

    // The counters the frame keeps and how they move on.
    std::uint64_t time() const { return time_; }
    double seconds() const { return seconds_; }
    std::uint64_t frameId() const { return frameId_; }
    std::uint64_t renderId() const { return renderId_; }

    // Advances the frame: the frame counter goes up by one and the elapsed seconds grow by the
    // step given. The source reads its clock twice, once to measure the step and once to store
    // for the next call, so the two readings are not the same instant; the port takes the two
    // readings as arguments rather than calling a clock, which keeps that difference explicit
    // and the class free of any dependency on one.
    void advance(double stepSeconds);

  private:
    std::uint64_t time_ = 0;
    double seconds_ = 0;
    std::uint64_t frameId_ = 0;
    std::uint64_t renderId_ = 0;
};
