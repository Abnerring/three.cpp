#include "nodes/core/EventNode.h"

EventNode::Slot EventNode::slotFor(const std::string& eventType) {
    // The plain slot for the three unary names, the after slot for the one after name, the
    // before slot for the three before ones, and no slot for the two pipeline names, which are
    // handled by pushing a callback rather than by filling a slot.
    if (eventType == kObject || eventType == kMaterial || eventType == kFrame) return Slot::Plain;
    if (eventType == kAfterObject) return Slot::After;
    if (eventType == kBeforeObject || eventType == kBeforeMaterial || eventType == kBeforeFrame) {
        return Slot::Before;
    }
    if (eventType == kBeforePipeline || eventType == kAfterPipeline) return Slot::Pipeline;
    return Slot::None;
}

EventNode::Kind EventNode::kindFor(const std::string& eventType) {
    // The material names use the render kind rather than a material one, since there is no
    // material kind to use.
    if (eventType == kObject || eventType == kAfterObject || eventType == kBeforeObject) {
        return Kind::Object;
    }
    if (eventType == kMaterial || eventType == kBeforeMaterial) return Kind::Render;
    if (eventType == kFrame || eventType == kBeforeFrame) return Kind::Frame;
    return Kind::None;
}

bool EventNode::isKnown(const std::string& eventType) {
    return slotFor(eventType) != Slot::None;
}

bool EventNode::isCacheable() {
    return false;
}
