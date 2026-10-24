#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Port of the before-chain rules and the build contract in src/nodes/core/Node.js at the pinned
// revision. Two things about them are worth writing down: the record is a list that grows on every
// call, and one of the three build stages answers nothing at all.
class NodeBeforeChain {
  public:
    // The list of nodes to run before this one. It starts absent rather than empty and is made the
    // first time one is added, so a node that never has anything placed before it carries nothing
    // rather than an empty list.
    static bool listStartsAbsent();

    // What adding one does. It is pushed, so the order is the order of the calls and the same node
    // added twice appears twice; there is no de-duplication, so a caller that added one twice
    // would run it twice.
    static std::vector<std::uint64_t> afterAdding(std::vector<std::uint64_t> chain,
                                                  std::uint64_t nodeId);

    // What adding returns, which is the node itself so that calls can be chained.
    static bool returnsTheNode();

    // The three build stages and what each answers. The first prepares the node and may make new
    // ones, so it answers with the node itself or a variant of it; the second only analyses and
    // answers nothing; the third produces the text. A caller cannot use one stage's answer as
    // another's.
    enum class StageAnswer { Node, Nothing, Text };

    static StageAnswer answerFor(const std::string& stage);

    // Whether the middle stage's answer is nothing rather than an empty piece of text, which is a
    // different thing: nothing is the absence of an answer, while an empty text is a text of no
    // length.
    static bool middleStageAnswersNothing();

    // Whether the stages run in the order they are listed, which they do, so a node that needs the
    // analysis before generating gets it.
    static bool stagesRunInOrder();
};
