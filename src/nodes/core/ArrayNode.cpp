#include "nodes/core/ArrayNode.h"

ArrayNode::Shape ArrayNode::fromOneArgument(const std::vector<std::uint64_t>& valueIds) {
    // A list of values: no type of its own, and the count is the list's length.
    Shape shape;
    shape.hasType = false;
    shape.count = static_cast<int>(valueIds.size());
    shape.hasValues = true;
    return shape;
}

ArrayNode::Shape ArrayNode::fromTwoArguments(const std::string& type, int count) {
    // A type and a count, and no values at all.
    Shape shape;
    shape.hasType = true;
    shape.type = type;
    shape.count = count;
    shape.hasValues = false;
    return shape;
}

std::string ArrayNode::nodeType(bool hasType, const std::string& givenType, bool hasValues,
                                const std::string& firstValueType) {
    // The given type when there is one; otherwise the first value's, which needs there to be a
    // first value.
    if (hasType) return givenType;
    if (hasValues) return firstValueType;
    return std::string();
}

bool ArrayNode::typeIsGiven(const Shape& shape) {
    return shape.hasType;
}

std::vector<std::uint64_t> ArrayNode::filled(int count, std::uint64_t nodeId) {
    // The same node in every place.
    return std::vector<std::uint64_t>(static_cast<std::size_t>(count), nodeId);
}
