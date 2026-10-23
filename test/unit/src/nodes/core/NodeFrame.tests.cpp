#include "nodes/core/NodeFrame.h"

#include "catch.hpp"

TEST_CASE("NodeFrame skips an update that has already run") {
    // A node whose stored frame id matches the frame's own has run already.
    const NodeFrame::Gate gate{0, 7};
    REQUIRE(NodeFrame::decide(NodeFrame::UpdateType::Frame, gate, 7, 0, true) ==
            NodeFrame::GateDecision::Skip);
    REQUIRE(NodeFrame::decide(NodeFrame::UpdateType::Frame, gate, 7, 0, false) ==
            NodeFrame::GateDecision::Skip);
    // A different id means it has not.
    REQUIRE(NodeFrame::decide(NodeFrame::UpdateType::Frame, gate, 8, 0, true) ==
            NodeFrame::GateDecision::RunRecordingFirst);
    // The render side is gated the same way, by its own id.
    const NodeFrame::Gate renderGate{3, 0};
    REQUIRE(NodeFrame::decide(NodeFrame::UpdateType::Render, renderGate, 0, 3, false) ==
            NodeFrame::GateDecision::Skip);
    REQUIRE(NodeFrame::decide(NodeFrame::UpdateType::Render, renderGate, 0, 4, false) ==
            NodeFrame::GateDecision::RunRecordingLast);
}

TEST_CASE("NodeFrame's two orders differ only in when the id is recorded") {
    // The same call with the two orders gives two different answers, which is the whole point
    // of keeping them apart: an update before records first so that a decline must be undone,
    // while the others record only on success.
    const NodeFrame::Gate gate{0, 0};
    REQUIRE(NodeFrame::decide(NodeFrame::UpdateType::Frame, gate, 1, 0, true) ==
            NodeFrame::GateDecision::RunRecordingFirst);
    REQUIRE(NodeFrame::decide(NodeFrame::UpdateType::Frame, gate, 1, 0, false) ==
            NodeFrame::GateDecision::RunRecordingLast);
    REQUIRE(NodeFrame::decide(NodeFrame::UpdateType::Frame, gate, 1, 0, true) !=
            NodeFrame::decide(NodeFrame::UpdateType::Frame, gate, 1, 0, false));
}

TEST_CASE("NodeFrame lets an object update through ungated") {
    // An object update is not gated at all, so it runs whatever the ids say.
    const NodeFrame::Gate gate{9, 9};
    REQUIRE(NodeFrame::decide(NodeFrame::UpdateType::Object, gate, 9, 9, true) ==
            NodeFrame::GateDecision::RunUngated);
    REQUIRE(NodeFrame::decide(NodeFrame::UpdateType::Object, gate, 9, 9, false) ==
            NodeFrame::GateDecision::RunUngated);
    // A node naming no kind of update is not run, which the source reaches by falling past all
    // three comparisons.
    REQUIRE(NodeFrame::decide(NodeFrame::UpdateType::None, gate, 0, 0, true) ==
            NodeFrame::GateDecision::Skip);
}

TEST_CASE("NodeFrame moves its counters on") {
    NodeFrame frame;
    // Everything starts at zero.
    REQUIRE(frame.frameId() == 0);
    REQUIRE(frame.time() == 0);
    REQUIRE(frame.seconds() == 0.0);
    REQUIRE(frame.renderId() == 0);
    // Each advance raises the frame counter by one and adds the step given.
    frame.advance(0.25);
    REQUIRE(frame.frameId() == 1);
    REQUIRE(frame.seconds() == 0.25);
    frame.advance(0.25);
    REQUIRE(frame.frameId() == 2);
    REQUIRE(frame.seconds() == 0.5);
    // The render counter is not moved by this at all, since only the frame counter is.
    REQUIRE(frame.renderId() == 0);
}