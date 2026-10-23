#include "nodes/core/RenderOutput.h"

std::vector<RenderOutput::Step> RenderOutput::steps(bool premultipliedAlpha, bool appliesToneMapping,
                                                    bool convertsColorSpace) {
    // The clamp first, then the alpha undone, then the tone mapping, then the colour space, then
    // the alpha put back.
    std::vector<Step> out;
    out.push_back(Step::ClampAlpha);
    if (premultipliedAlpha) out.push_back(Step::Unpremultiply);
    if (appliesToneMapping) out.push_back(Step::ToneMapping);
    if (convertsColorSpace) out.push_back(Step::WorkingToOutput);
    if (premultipliedAlpha) out.push_back(Step::Premultiply);
    return out;
}

int RenderOutput::toneMapping(bool hasOwn, int own, int context, int noneValue) {
    // The own value when it is not null, the context's otherwise, and the none value when that
    // answer is falsy.
    const int chosen = hasOwn ? own : context;
    return chosen != 0 ? chosen : noneValue;
}

std::string RenderOutput::outputColorSpace(bool hasOwn, const std::string& own,
                                           const std::string& context,
                                           const std::string& noneValue) {
    const std::string chosen = hasOwn ? own : context;
    return chosen.empty() ? noneValue : chosen;
}

bool RenderOutput::converts(bool isNoneSpace, bool isWorkingSpace) {
    // Two conditions, either of which skips it.
    return !isNoneSpace && !isWorkingSpace;
}

double RenderOutput::clampAlpha(double alpha) {
    if (alpha < 0.0) return 0.0;
    if (alpha > 1.0) return 1.0;
    return alpha;
}
