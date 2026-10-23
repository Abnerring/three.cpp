#pragma once
#include <string>
#include <vector>

// Port of the rules in src/nodes/utils/JoinNode.js at the pinned revision. The node puts several
// values together into one, and three of its rules are worth writing down because they are
// about what it does when the parts do not fit.
class JoinNode {
  public:
    static constexpr const char* kTypeName = "JoinNode";

    // The width of the joined type, which is the sum of the widths of the parts. A sum that no
    // type has can come out of it: two parts of three and two make five, and the node builds a
    // five part type rather than refusing, because nothing checks that the sum is a width a type
    // exists for. An empty list sums to nought for the same reason.
    static int joinedWidth(const std::vector<int>& partWidths);

    // What happens to a part that does not fit. Two checks guard the loop and they use different
    // comparisons: one asks whether the running count has already reached the maximum, which
    // stops the loop, and the other asks whether adding this part would pass it, which cuts the
    // part short and carries on. Both report an error first and neither stops the work, so a
    // caller that ignores the message gets a patched result rather than a failure.
    enum class Overrun { None, AlreadyFull, Truncated };

    static Overrun checkOverrun(int length, int maxLength, int partWidth);

    // How many parts of a cut part are kept, which is what is left of the maximum.
    static int truncatedWidth(int length, int maxLength);

    // Whether a message is printed for each of the two cases.
    static bool printsMessage(Overrun overrun);

    // The two messages, each naming the type it was building.
    static std::string alreadyFullMessage(const std::string& type);
    static std::string truncatedMessage(const std::string& type);
};
