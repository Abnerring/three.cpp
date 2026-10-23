#include "nodes/core/UniformGroupNode.h"

int UniformGroupNode::defaultOrder(bool shared) {
    // The shared factory's default is one lower than the plain one's.
    return shared ? kSharedOrder : kPlainOrder;
}

UniformGroupNode::Preset UniformGroupNode::frameGroup() {
    Preset preset;
    preset.name = "frame";
    preset.shared = true;
    preset.order = kSharedOrder;
    preset.updateType = "FRAME";
    return preset;
}

UniformGroupNode::Preset UniformGroupNode::renderGroup() {
    Preset preset;
    preset.name = "render";
    preset.shared = true;
    preset.order = kSharedOrder;
    preset.updateType = "RENDER";
    return preset;
}

UniformGroupNode::Preset UniformGroupNode::objectGroup() {
    Preset preset;
    preset.name = "object";
    // The odd one: not shared, and so at the plain order.
    preset.shared = false;
    preset.order = kPlainOrder;
    preset.updateType = "OBJECT";
    return preset;
}

bool UniformGroupNode::serialisesOrder() {
    // The order is not written out.
    return false;
}

bool UniformGroupNode::serialisesUpdateType() {
    // Nor is the update kind.
    return false;
}

bool UniformGroupNode::setsNeedsUpdateFlag() {
    return true;
}

bool UniformGroupNode::initialisesNeedsUpdate() {
    // Nothing in the constructor gives the flag a value, so before the first update it is absent.
    return false;
}

bool UniformGroupNode::isCacheable() {
    return false;
}
