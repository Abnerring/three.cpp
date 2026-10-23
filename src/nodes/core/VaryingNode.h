#pragma once
#include <string>

// Port of the rules in src/nodes/core/VaryingNode.js at the pinned revision. The node passes one
// value from the vertex stage to the others, and five of its rules are worth writing down.
class VaryingNode {
  public:
    static constexpr bool isVaryingNode = true;
    static constexpr const char* kTypeName = "VaryingNode";

    // The stage the wrapped node is built in, which is the vertex stage whatever stage the
    // varying is used from. The sub-build around it is made with this name fixed, so a reader
    // who expected the stage to follow the use would be wrong.
    static constexpr const char* kNodeStage = "VERTEX";

    // What the interpolation flag becomes the first time it is set. The expression is a short
    // circuiting or with an assignment on its right, so it only assigns when the flag is falsy,
    // and the value assigned is whether the stage is the fragment one. Used from the vertex
    // stage the flag is therefore set to false, which is the same as leaving it, and used from
    // the fragment stage it is set to true.
    static bool needsInterpolationAssigned(const std::string& shaderStage);

    // Whether the flag is overwritten when it already holds a value, which it is not.
    static bool overwritesExistingFlag();

    // How often the vertex assignment is emitted. In the vertex stage the guard is the current
    // flow block, so the assignment is emitted once per block; in the fragment stage the guard is
    // nothing, so it is emitted once in total. The source's comment says the first half of that.
    enum class Emission { PerBlock, OnceInTotal };

    static Emission emissionFor(const std::string& shaderStage);

    // Whether the two methods that set the varying up are the same. They are: both build the
    // varying and then queue the node in the vertex stage, so one is a copy of the other rather
    // than a different step.
    static bool setupAndAnalyzeAreTheSame();

    // The hash of a node. A name that was given is used, and the ordinary hash otherwise, so two
    // varying nodes with the same explicit name hash alike while two unnamed ones do not.
    static std::string hash(bool hasName, const std::string& name, const std::string& ordinaryHash);

    // Whether the node is global, so that its cache is shared rather than per use.
    static constexpr bool globalByDefault = true;

    static bool isCacheable();
};
