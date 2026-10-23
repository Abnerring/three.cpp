#include "nodes/core/FlipNode.h"

const std::vector<std::string>& FlipNode::componentNames() {
    static const std::vector<std::string> names{"x", "y", "z", "w"};
    return names;
}

std::vector<bool> FlipNode::flipped(int length, const std::string& components) {
    std::vector<bool> out(static_cast<std::size_t>(length), false);
    std::size_t componentIndex = 0;
    const auto& names = componentNames();
    for (int i = 0; i < length; ++i) {
        const std::string& name = names[static_cast<std::size_t>(i)];
        // The walk looks for the next name in the list at this position; if it does not match,
        // the component is left alone and the walk does not advance.
        if (componentIndex < components.size() && name[0] == components[componentIndex]) {
            out[static_cast<std::size_t>(i)] = true;
            componentIndex += 1;
        }
    }
    return out;
}

double FlipNode::flip(double value) {
    return 1.0 - value;
}

bool FlipNode::isInWalkOrder(const std::string& components) {
    // The names must appear in the order the walk visits them.
    const auto& names = componentNames();
    int last = -1;
    for (const char c : components) {
        int found = -1;
        for (std::size_t i = 0; i < names.size(); ++i) {
            if (names[i][0] == c) found = static_cast<int>(i);
        }
        if (found <= last) return false;
        last = found;
    }
    return true;
}
