#include "nodes/core/FunctionOverloadingNode.h"

bool FunctionOverloadingNode::isConsidered(const Candidate& candidate, std::size_t argumentCount) {
    // The counts must be equal; a different length is not scored at all.
    return candidate.inputTypes.size() == argumentCount;
}

int FunctionOverloadingNode::score(const Candidate& candidate,
                                   const std::vector<std::string>& argumentTypes) {
    int total = 0;
    for (std::size_t i = 0; i < argumentTypes.size(); ++i) {
        if (i < candidate.inputTypes.size() && argumentTypes[i] == candidate.inputTypes[i]) {
            total += 1;
        }
    }
    return total;
}

int FunctionOverloadingNode::choose(const std::vector<Candidate>& candidates,
                                    const std::vector<std::string>& argumentTypes) {
    int best = -1;
    int bestScore = kStartingScore;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        if (!isConsidered(candidates[i], argumentTypes.size())) continue;
        const int current = score(candidates[i], argumentTypes);
        // Strictly greater, so a tie keeps the earlier one.
        if (current > bestScore) {
            best = static_cast<int>(i);
            bestScore = current;
        }
    }
    return best;
}

bool FunctionOverloadingNode::remembersChoice() {
    return true;
}
