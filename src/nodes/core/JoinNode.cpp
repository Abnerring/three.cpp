#include "nodes/core/JoinNode.h"

int JoinNode::joinedWidth(const std::vector<int>& partWidths) {
    // The sum, with no check that a type of that width exists.
    int total = 0;
    for (const int width : partWidths) total += width;
    return total;
}

JoinNode::Overrun JoinNode::checkOverrun(int length, int maxLength, int partWidth) {
    // The first check uses greater or equal and stops the loop; the second uses greater and cuts
    // the part down.
    if (length >= maxLength) return Overrun::AlreadyFull;
    if (length + partWidth > maxLength) return Overrun::Truncated;
    return Overrun::None;
}

int JoinNode::truncatedWidth(int length, int maxLength) {
    return maxLength - length;
}

bool JoinNode::printsMessage(Overrun overrun) {
    // Both of the overruns report, and the ordinary case does not.
    return overrun != Overrun::None;
}

std::string JoinNode::alreadyFullMessage(const std::string& type) {
    return "TSL: Length of parameters exceeds maximum length of function '" + type + "()' type.";
}

std::string JoinNode::truncatedMessage(const std::string& type) {
    return "TSL: Length of '" + type + "()' data exceeds maximum length of output type.";
}
