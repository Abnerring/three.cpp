#pragma once
#include <string>
#include <vector>

// Port of the rules in src/nodes/core/MRTNode.js at the pinned revision. The node names several
// render targets at once, and four of its rules are worth writing down.
class MRTNode {
  public:
    static constexpr bool isMRTNode = true;
    static constexpr const char* kTypeName = "MRTNode";

    // The index a name has among the render target's textures, or minus one when it has none.
    // The minus one is the array idiom for a search that missed rather than an exception, so a
    // caller that compared against nought would take the first texture for a miss.
    static int textureIndex(const std::vector<std::string>& textureNames, const std::string& name);

    // The blend kinds the module prepares.
    static constexpr const char* kNoBlending = "NoBlending";
    static constexpr const char* kMaterialBlending = "MaterialBlending";

    // The blend mode a name gets. The one name the module gives a default to is the output, and
    // that default is material blending; every other name falls back to no blending, which is a
    // different answer. So an unset name and the output name do not fall back to the same thing,
    // and a reader who took the fallback for the default would be wrong.
    static std::string blendMode(bool outputWasGiven, bool nameWasSet,
                                 const std::string& givenMode);

    // Why an output might not appear in the members. It is skipped when the render target has no
    // texture of that name, which the source's own comment explains as being for an output that
    // exists in the node but has never been used. So a declared output can vanish without a word.
    static bool skipsUnusedOutput();

    // How the members are placed. Each is put at the index its texture has rather than in the
    // order the outputs were declared, so a target that uses the second texture but not the
    // first leaves a gap rather than closing it up.
    static bool membersArePlacedByIndex();

    // A gap really is possible, given which indices have textures and which outputs were
    // declared: the result has an entry for each declared output that found a texture, at that
    // texture's index, so the length is the highest such index plus one and some entries are
    // absent.
    static bool leavesGaps(bool declaresSecondOnly);

    // What merging does when both nodes name the same output. The argument's value wins, since
    // the two dictionaries are spread with the argument second.
    static bool argumentWinsOnConflict();

    static bool isCacheable();
};
