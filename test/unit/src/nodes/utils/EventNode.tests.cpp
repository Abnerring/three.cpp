#include "nodes/utils/EventNode.h"

#include "catch.hpp"

#include <string>

using Slot = EventNode::Slot;
using Kind = EventNode::Kind;

TEST_CASE("EventNode reports a void type") {
    // The node runs a callback and produces no value, so its type says so rather than being one
    // of the value types the rest of the layer uses.
    REQUIRE(std::string(EventNode::kNodeType) == "void");
    REQUIRE(std::string(EventNode::kNodeType) != "float");
    REQUIRE(std::string(EventNode::kTypeName) == "EventNode");
}

TEST_CASE("EventNode's names fill three different slots") {
    // The three unary names use the plain slot.
    REQUIRE(EventNode::slotFor(EventNode::kObject) == Slot::Plain);
    REQUIRE(EventNode::slotFor(EventNode::kMaterial) == Slot::Plain);
    REQUIRE(EventNode::slotFor(EventNode::kFrame) == Slot::Plain);
    // The after name uses the after slot, so the name decides the slot as well as the timing.
    REQUIRE(EventNode::slotFor(EventNode::kAfterObject) == Slot::After);
    REQUIRE(EventNode::slotFor(EventNode::kAfterObject) != Slot::Plain);
    // The three before names use the before slot.
    REQUIRE(EventNode::slotFor(EventNode::kBeforeObject) == Slot::Before);
    REQUIRE(EventNode::slotFor(EventNode::kBeforeMaterial) == Slot::Before);
    REQUIRE(EventNode::slotFor(EventNode::kBeforeFrame) == Slot::Before);
}

TEST_CASE("EventNode's two pipeline names bypass the slots") {
    // They are handled by pushing a callback into the context rather than by filling an update
    // slot, so their slot is a fourth kind of its own.
    REQUIRE(EventNode::slotFor(EventNode::kBeforePipeline) == Slot::Pipeline);
    REQUIRE(EventNode::slotFor(EventNode::kAfterPipeline) == Slot::Pipeline);
    REQUIRE(EventNode::slotFor(EventNode::kBeforePipeline) != Slot::Plain);
    // And they have no update kind at all, since no kind would be right for them.
    REQUIRE(EventNode::kindFor(EventNode::kBeforePipeline) == Kind::None);
    REQUIRE(EventNode::kindFor(EventNode::kAfterPipeline) == Kind::None);
}

TEST_CASE("EventNode's material names use the render kind") {
    // There is no material kind, so the two material names use the render one.
    REQUIRE(EventNode::kindFor(EventNode::kMaterial) == Kind::Render);
    REQUIRE(EventNode::kindFor(EventNode::kBeforeMaterial) == Kind::Render);
    // The object names use the object kind and the frame ones the frame kind.
    REQUIRE(EventNode::kindFor(EventNode::kObject) == Kind::Object);
    REQUIRE(EventNode::kindFor(EventNode::kBeforeObject) == Kind::Object);
    REQUIRE(EventNode::kindFor(EventNode::kFrame) == Kind::Frame);
    REQUIRE(EventNode::kindFor(EventNode::kBeforeFrame) == Kind::Frame);
    // The two do not coincide, which is the point of having two questions.
    REQUIRE(EventNode::kindFor(EventNode::kMaterial) != EventNode::kindFor(EventNode::kObject));
}

TEST_CASE("EventNode's unknown name fills no slot and says nothing") {
    // A name it does not know fills no slot, so its callback never runs; nothing is reported,
    // which is a silence rather than an error.
    REQUIRE_FALSE(EventNode::isKnown("somethingElse"));
    REQUIRE_FALSE(EventNode::isKnown(""));
    REQUIRE(EventNode::slotFor("somethingElse") == Slot::None);
    REQUIRE(EventNode::kindFor("somethingElse") == Kind::None);
    // Every name it does know is known.
    for (const char* name : {EventNode::kObject, EventNode::kMaterial, EventNode::kFrame,
                             EventNode::kAfterObject, EventNode::kBeforeObject,
                             EventNode::kBeforeMaterial, EventNode::kBeforeFrame,
                             EventNode::kBeforePipeline, EventNode::kAfterPipeline}) {
        REQUIRE(EventNode::isKnown(name));
    }
    REQUIRE_FALSE(EventNode::isCacheable());
}