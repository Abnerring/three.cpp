#pragma once
#include <string>

// Port of the two assembly steps in src/nodes/core/NodeBuilder.js at the pinned revision. Both put
// values into a constructor call and fill the gaps, but they ask different questions of a value,
// which is the part worth writing down.
class NodeBuilderAssembly {
  public:
    // What a place in a list is filled with. A value that is there is built; one that is not is a
    // constant of the list's type. The list itself may be missing, in which case every place is a
    // constant, and a place whose value is missing is filled rather than left empty, so a partial
    // list is completed with defaults.
    enum class Filled { Built, Constant };

    static Filled arrayPlace(bool hasValues, bool valueIsPresent);

    // What a place in a structure is filled with. Three questions are asked in a row: whether there
    // are values at all, whether this member has one, and whether that one is a node. Only the
    // third passing means the value is built; anything else becomes a constant, so a member given
    // something that is not a node is quietly replaced by the default rather than failing.
    static Filled structPlace(bool hasValues, bool memberHasValue, bool memberValueIsNode);

    // Whether the two steps ask the same question of a value. They do not: the list asks only
    // whether one is there, while the structure also asks whether it is a node. So the same value
    // would be built into a list and replaced by a constant in a structure, which is a difference
    // a reader would not guess from either name.
    static bool bothAskTheSame();

    // How the places are separated, which is the usual rule of putting a separator after every
    // place but the last, so a list of one has none and a list of none has no parentheses to fill.
    static std::string joined(int placeCount);

    // The name the structure's constructor is called by. The loop destructures each member into a
    // name and a type of its own, which shadows the structure's own type for the length of one
    // iteration; the member's type is used to build or default that member, while the structure's
    // own type names the constructor at the end, so the two are both used and are not the same.
    static bool memberTypeShadowsStructureType();
};
