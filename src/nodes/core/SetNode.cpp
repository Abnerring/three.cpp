#include "nodes/core/SetNode.h"

const std::vector<std::string>& SetNode::componentNames() {
    static const std::vector<std::string> names{"x", "y", "z", "w"};
    return names;
}

std::vector<bool> SetNode::replaced(int length, const std::string& components) {
    std::vector<bool> out(static_cast<std::size_t>(length), false);
    const auto& names = componentNames();
    for (int i = 0; i < length; i += 1) {
        const std::string& name = names[static_cast<std::size_t>(i)];
        if (!components.empty() && name[0] == components[0]) {
            // The replacement value has as many parts as there are names, so it fills the whole
            // span the names cover; every position in it is replaced, and the walk jumps over
            // the rest.
            for (std::size_t k = 0; k < components.size(); ++k) {
                const std::size_t at = static_cast<std::size_t>(i) + k;
                if (at < out.size()) out[at] = true;
            }
            i += static_cast<int>(components.size()) - 1;
        }
    }
    return out;
}

int SetNode::producedCount(int length, const std::string& components) {
    const auto& names = componentNames();
    int produced = 0;
    for (int i = 0; i < length; i += 1) {
        const std::string& name = names[static_cast<std::size_t>(i)];
        if (!components.empty() && name[0] == components[0]) {
            produced += 1;
            i += static_cast<int>(components.size()) - 1;
        } else {
            produced += 1;
        }
    }
    return produced;
}

bool SetNode::isContiguousAndOrdered(const std::string& components) {
    // The names must be a run of the component names with no gap and no reordering.
    if (components.empty()) return false;
    int last = -1;
    for (std::size_t k = 0; k < components.size(); ++k) {
        int found = -1;
        for (std::size_t i = 0; i < componentNames().size(); ++i) {
            if (componentNames()[i][0] == components[k]) found = static_cast<int>(i);
        }
        if (k == 0) {
            last = found;
        } else {
            if (found != last + 1) return false;
            last = found;
        }
    }
    return true;
}

int SetNode::targetWidth(const std::string& components) {
    return static_cast<int>(components.size());
}
