#pragma once
#include <string>
#include <vector>

// Port of the candidate choice in src/nodes/utils/FunctionOverloadingNode.js at the pinned
// revision. The node picks one of several definitions by counting how many of the arguments
// match by type, and three of its rules do not follow from that description.
class FunctionOverloadingNode {
  public:
    static constexpr const char* kTypeName = "FunctionOverloadingNode";

    // One definition: how many parameters it takes, and the type of each.
    struct Candidate {
        std::vector<std::string> inputTypes;
    };

    // The index of the chosen candidate, or minus one when none fits.
    //
    // Only a candidate whose parameter count equals the argument count is considered at all, so
    // a definition of the right shape but the wrong length is not merely scored low, it is
    // passed over. Among those that are considered, each argument whose type matches scores one
    // and the highest score wins; the count starts at minus one rather than at nought, so a
    // candidate that matches no type at all still beats the starting point and is chosen. Ties
    // keep the earliest, because the comparison is strictly greater, which is the opposite of
    // what the convert node does with its matches.
    static int choose(const std::vector<Candidate>& candidates, const std::vector<std::string>& argumentTypes);

    // Whether a candidate is considered at all, which needs its parameter count to equal the
    // argument count.
    static bool isConsidered(const Candidate& candidate, std::size_t argumentCount);

    // The score a considered candidate gets, which is the number of matching types.
    static int score(const Candidate& candidate, const std::vector<std::string>& argumentTypes);

    // The score the search starts from, which is minus one rather than nought.
    static constexpr int kStartingScore = -1;

    // Whether the choice is remembered. It is, and the check for having remembered one is
    // against null, so once a candidate has been chosen the search is not run again even if the
    // arguments would now choose differently.
    static bool remembersChoice();
};
