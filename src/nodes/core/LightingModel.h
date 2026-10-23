#pragma once
#include <string>
#include <vector>

// Port of the structure of src/nodes/core/LightingModel.js at the pinned revision. The class is a
// set of hooks a concrete lighting model fills in, and the shape of that set is what is taken
// here: which hooks are called by whom, and which one is not called at all.
class LightingModel {
  public:
    static constexpr const char* kTypeName = "LightingModel";

    // The six hooks, in the order the source declares them.
    enum class Hook {
        Start,
        Finish,
        Direct,
        DirectRectArea,
        Indirect,
        AmbientOcclusion
    };

    static std::vector<Hook> hooks();

    // Whether a hook has a body of its own. Only the first does: the other five are empty, which
    // makes the class abstract in the sense the source means rather than in the language's.
    static bool hasBody(Hook hook);

    // Who calls a hook. The first is called by the framework and calls the indirect one itself;
    // the four middle ones are called by the framework; and the last is called by nobody, its
    // own documentation saying that a lighting model must call it by hand from its indirect term.
    // So a concrete model that never mentions it simply has no ambient occlusion.
    enum class Caller { Framework, TheFirstHook, Nobody };

    static Caller callerOf(Hook hook);

    // What the first hook does, in order: it sets the direct lights up from the light nodes, and
    // then it calls the indirect hook. Both halves are its responsibility, so an override that
    // does not call through skips both rather than only one.
    static std::vector<std::string> startSteps();

    // The sentence the source uses for the ambient occlusion hook's calling convention, which is
    // the clearest statement that it is not in the automatic path.
    static std::string ambientOcclusionNote();
};
