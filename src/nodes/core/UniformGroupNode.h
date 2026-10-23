#pragma once
#include <string>

// Port of the rules in src/nodes/core/UniformGroupNode.js at the pinned revision. The node groups
// uniforms so that they can share one buffer, and five of its rules are worth writing down.
class UniformGroupNode {
  public:
    static constexpr bool isUniformGroup = true;
    static constexpr const char* kTypeName = "UniformGroupNode";

    // The type the node reports, which is the word for a string rather than anything to do with
    // uniforms. A reader looking for the buffer's type here would not find it.
    static constexpr const char* kNodeType = "string";

    // The default sort key a plain group carries and the one a shared group carries. They differ,
    // and the shared one is lower, so shared groups are placed first without a caller having to
    // say so.
    static constexpr int kPlainOrder = 1;
    static constexpr int kSharedOrder = 0;

    // The order a group gets when none is given, which depends on which of the two factory
    // functions made it rather than on the group itself.
    static int defaultOrder(bool shared);

    // The three groups the module prepares, and their settings. Two are shared and updated once
    // per frame and once per render; the third is not shared and is updated once per object, so
    // it is the odd one of the three.
    struct Preset {
        std::string name;
        bool shared = false;
        int order = 1;
        std::string updateType;
    };

    static Preset frameGroup();
    static Preset renderGroup();
    static Preset objectGroup();

    // What serialising writes out and what it leaves behind. The name, the version and whether
    // the group is shared are written; the order and the update kind are not, so a group read
    // back from the serialised form has the order a fresh one would and no update kind at all
    // rather than the ones it was made with. That makes the round trip lossy for two of the five
    // settings.
    static bool serialisesOrder();
    static bool serialisesUpdateType();

    // What updating a group does. It sets a flag saying the group needs an update, and that flag
    // is never given a starting value in the constructor, so before the first update it is
    // absent rather than false.
    static bool setsNeedsUpdateFlag();
    static bool initialisesNeedsUpdate();

    static bool isCacheable();
};
