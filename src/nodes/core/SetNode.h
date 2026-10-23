#pragma once
#include <string>
#include <vector>

// Port of the walk in src/nodes/utils/SetNode.js at the pinned revision. The node replaces some
// of a value's components and keeps the rest, and the walk that decides which ones has two
// consequences that the name does not suggest.
class SetNode {
  public:
    static constexpr const char* kTypeName = "SetNode";

    // The names the components go by, in the order the walk visits them.
    static const std::vector<std::string>& componentNames();

    // Which components are replaced. The walk compares only the first name it was given, and on
    // a match it skips as many places as there are names. That assumes the names are contiguous
    // and in order: a list naming the first and third matches the first, skips the second, and
    // never looks for the third, so the third is left as it was rather than replaced. A list out
    // of order matches later and skips too far, and can end up producing fewer values than the
    // value has components.
    static std::vector<bool> replaced(int length, const std::string& components);

    // How many values the walk produces for a value of a given width, which equals the width
    // only when the list is contiguous and in order.
    static int producedCount(int length, const std::string& components);

    // Whether a list is one the walk handles correctly, meaning contiguous and in order.
    static bool isContiguousAndOrdered(const std::string& components);

    // The width of the replacement value, which is the number of names given rather than the
    // width they span. A list naming the first and third therefore expects a two part
    // replacement even though it touches the first three positions.
    static int targetWidth(const std::string& components);
};
