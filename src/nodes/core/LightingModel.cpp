#include "nodes/core/LightingModel.h"

std::vector<LightingModel::Hook> LightingModel::hooks() {
    return {Hook::Start, Hook::Finish, Hook::Direct, Hook::DirectRectArea, Hook::Indirect,
            Hook::AmbientOcclusion};
}

bool LightingModel::hasBody(Hook hook) {
    // Only the first hook has anything of its own to do.
    return hook == Hook::Start;
}

LightingModel::Caller LightingModel::callerOf(Hook hook) {
    if (hook == Hook::Start || hook == Hook::Finish || hook == Hook::Direct ||
        hook == Hook::DirectRectArea) {
        return Caller::Framework;
    }
    if (hook == Hook::Indirect) {
        // The indirect term is reached through the first hook rather than directly.
        return Caller::TheFirstHook;
    }
    // The ambient occlusion one is reached by nobody automatically.
    return Caller::Nobody;
}

std::vector<std::string> LightingModel::startSteps() {
    // The direct lights first, then the indirect term.
    return {"setupLights", "indirect"};
}

std::string LightingModel::ambientOcclusionNote() {
    return "Unlike other methods, this method must be called manually by the lighting model in its "
           "indirect term.";
}
