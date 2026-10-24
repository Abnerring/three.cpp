#include "nodes/core/NodeBeforeChain.h"

bool NodeBeforeChain::listStartsAbsent() {
    return true;
}

std::vector<std::uint64_t> NodeBeforeChain::afterAdding(std::vector<std::uint64_t> chain,
                                                        std::uint64_t nodeId) {
    // Pushed, not inserted and not compared.
    chain.push_back(nodeId);
    return chain;
}

bool NodeBeforeChain::returnsTheNode() {
    return true;
}

NodeBeforeChain::StageAnswer NodeBeforeChain::answerFor(const std::string& stage) {
    if (stage == "setup") return StageAnswer::Node;
    if (stage == "analyze") return StageAnswer::Nothing;
    if (stage == "generate") return StageAnswer::Text;
    return StageAnswer::Nothing;
}

bool NodeBeforeChain::middleStageAnswersNothing() {
    return true;
}

bool NodeBeforeChain::stagesRunInOrder() {
    return true;
}
