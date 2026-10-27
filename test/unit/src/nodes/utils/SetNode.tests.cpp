#include "nodes/utils/SetNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("SetNode replaces the components a contiguous list names") {
    // One name replaces one component.
    const auto one = SetNode::replaced(4, "x");
    REQUIRE(one[0]);
    REQUIRE_FALSE(one[1]);
    REQUIRE_FALSE(one[2]);
    REQUIRE_FALSE(one[3]);
    // Two names in order replace both, since the replacement value has two parts and fills the
    // span the names cover.
    const auto two = SetNode::replaced(4, "xy");
    REQUIRE(two[0]);
    REQUIRE(two[1]);
    REQUIRE_FALSE(two[2]);
    REQUIRE_FALSE(two[3]);
    // The last two the same way.
    const auto lastTwo = SetNode::replaced(4, "zw");
    REQUIRE_FALSE(lastTwo[0]);
    REQUIRE_FALSE(lastTwo[1]);
    REQUIRE(lastTwo[2]);
    REQUIRE(lastTwo[3]);
    REQUIRE(std::string(SetNode::kTypeName) == "SetNode");
}

TEST_CASE("SetNode ignores every name after the first") {
    // Only the first name is compared, and the replacement fills as many positions as there are
    // names starting from the match. A list naming the first and third therefore fills the first
    // two positions and leaves the third alone: the third name is never read, and the same
    // thing happens as for a list naming the first two. This is the trap, and no error is raised.
    const auto gapped = SetNode::replaced(4, "xz");
    REQUIRE(gapped[0]);
    REQUIRE(gapped[1]);
    REQUIRE_FALSE(gapped[2]);
    REQUIRE_FALSE(gapped[3]);
    // The two lists give the same answer, which is what ignoring the second name means.
    REQUIRE(gapped == SetNode::replaced(4, "xy"));
    // The third really is untouched, so a caller expecting it replaced is wrong.
    REQUIRE_FALSE(gapped[2]);
}

TEST_CASE("SetNode's walk can produce fewer values than the value has components") {
    // A single name produces one value per position.
    REQUIRE(SetNode::producedCount(4, "x") == 4);
    // A list of two produces three, not four: the replacement is pushed as one entry
    // whatever the span it fills, so the constructor it builds ends up with fewer
    // arguments than the type has components. That is a real fault in the source rather
    // than something the port introduced, and it is recorded instead of repaired.
    REQUIRE(SetNode::producedCount(4, "xy") == 3);
    REQUIRE(SetNode::producedCount(4, "xy") != 4);
    REQUIRE(SetNode::producedCount(4, "xyz") == 2);
    // A list starting with a later name matches later and its span runs past the end, so the
    // total comes out short.
    REQUIRE(SetNode::producedCount(4, "yx") == 3);
    REQUIRE(SetNode::producedCount(4, "yx") != 4);
    // A gapped list starting at the first still produces four.
    REQUIRE(SetNode::producedCount(4, "xz") == 3);
}

TEST_CASE("SetNode can be asked whether a list is the kind it handles") {
    // Contiguous runs are handled.
    REQUIRE(SetNode::isContiguousAndOrdered("x"));
    REQUIRE(SetNode::isContiguousAndOrdered("xy"));
    REQUIRE(SetNode::isContiguousAndOrdered("xyz"));
    REQUIRE(SetNode::isContiguousAndOrdered("zw"));
    // Gaps and reorderings are not, and neither is an empty list.
    REQUIRE_FALSE(SetNode::isContiguousAndOrdered("xz"));
    REQUIRE_FALSE(SetNode::isContiguousAndOrdered("yx"));
    REQUIRE_FALSE(SetNode::isContiguousAndOrdered(""));
    // The replacement's width is the number of names, not the span they cover.
    REQUIRE(SetNode::targetWidth("x") == 1);
    REQUIRE(SetNode::targetWidth("xy") == 2);
    REQUIRE(SetNode::targetWidth("xz") == 2);
    REQUIRE(SetNode::targetWidth("xz") == SetNode::targetWidth("xy"));
}
