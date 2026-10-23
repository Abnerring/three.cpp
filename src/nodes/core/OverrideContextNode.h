#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the rules in src/nodes/core/OverrideContextNode.js at the pinned revision. The node
// replaces some target nodes for a flow, and four of its rules are worth writing down.
class OverrideContextNode {
  public:
    static constexpr bool isOverrideContextNode = true;
    static constexpr const char* kTypeName = "OverrideContextNode";

    // What an override ends up stored as. A function is kept as it is, a node is wrapped in one
    // that always answers it, and nothing at all is kept as nothing. The three cases are decided
    // by asking first whether there is anything and then whether it is a function, so a value
    // that is neither answers through a wrapper rather than being refused.
    enum class Stored { Function, Wrapper, Nothing };

    static Stored storedKind(bool hasValue, bool valueIsFunction);

    // The two factories differ in whether they ask about nothing at all. The single one asks only
    // whether the callback is a node, so a node is wrapped while anything else, including a
    // function, is taken as it is; the multiple one asks three questions in a row.
    static bool singleAsksWhetherThereIsAnything();
    static bool multipleAsksWhetherThereIsAnything();

    // The order of the arguments in the two forms. The free function takes the flow node last,
    // and the chained form takes it first and passes it through in last place, so a caller
    // writing the chain writes the arguments in the other order.
    static std::vector<std::string> freeFunctionArgumentOrder();
    static std::vector<std::string> chainedArgumentOrder();

    // How the maps from several context nodes are combined. All of them are gathered by walking
    // the tree and then turned into one map from every entry in turn, so a key that appears twice
    // takes its later value rather than its earlier one.
    static std::uint64_t mergedValue(const std::vector<std::uint64_t>& earlierThenLater);

    // Whether a traversal collects every override node it meets rather than only the nearest,
    // which it does.
    static bool gathersEveryAncestor();
};
