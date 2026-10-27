#include "nodes/display/ColorSpaceNode.h"

bool ColorSpaceNode::converts(bool managementEnabled, const std::string& source,
                              const std::string& target) {
    // Management exactly off skips everything.
    if (!managementEnabled) return false;
    // Equal names mean nothing to do.
    if (source == target) return false;
    // An empty name skips as well, tested for truth rather than for being there.
    if (source.empty()) return false;
    if (target.empty()) return false;
    return true;
}

std::vector<ColorSpaceNode::Step> ColorSpaceNode::steps(bool sourceIsSrgb, bool primariesDiffer,
                                                       bool targetIsSrgb) {
    // Undo the source's transfer, then apply the matrix if the primaries differ, then apply the
    // target's transfer. The matrix therefore sits between the two transfers rather than beside
    // them.
    std::vector<Step> out;
    if (sourceIsSrgb) out.push_back(Step::UndoSourceTransfer);
    if (primariesDiffer) out.push_back(Step::PrimariesMatrix);
    if (targetIsSrgb) out.push_back(Step::ApplyTargetTransfer);
    return out;
}

std::string ColorSpaceNode::outputColorSpace(const std::string& contextColorSpace,
                                             const std::string& rendererColorSpace) {
    // The context's own when it is truthy, the renderer's otherwise.
    if (!contextColorSpace.empty()) return contextColorSpace;
    return rendererColorSpace;
}

std::string ColorSpaceNode::resolve(const std::string& name, const std::string& working,
                                    const std::string& resolvedOutput) {
    if (name == kWorkingName) return working;
    if (name == kOutputName) return resolvedOutput;
    // Anything else is its own answer.
    return name;
}
