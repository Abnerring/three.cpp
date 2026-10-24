#pragma once
#include <cstdint>
#include <string>

// Port of the field rules at the head of src/nodes/core/Node.js at the pinned revision. This is the
// base class every node derives from, and four of its defaults are worth writing down because the
// classes that derive from it do not all keep them.
class NodeBaseFields {
  public:
    static constexpr bool isNode = true;
    static constexpr const char* kTypeName = "Node";

    // The name a fresh node carries. It is the empty string here, while three of the classes
    // below use an absent name instead, so a caller inspecting the name of an arbitrary node
    // cannot assume it is either one.
    static constexpr const char* kNameDefault = "";

    // Whether a fresh node is global and whether it asks for its parents to be tracked. Both are
    // off here, and the classes that derive from it turn them on where needed: an attribute is
    // global, a variable is global and also asks for its parents. So the base class is the
    // quietest of the family rather than the typical one.
    static constexpr bool kGlobalDefault = false;
    static constexpr bool kParentsDefault = false;

    // The three update kinds a fresh node declares. All three are the word for none rather than a
    // number, so a node that never updates still carries three values that a truth test would
    // pass; only a comparison against the word tells them apart.
    static constexpr const char* kUpdateTypeDefault = "none";

    // The version a fresh node carries, and what setting the update flag does to it. The flag is a
    // setter rather than a field, and it raises the version only when the value assigned is
    // exactly true, so assigning false changes nothing at all and a caller cannot lower a version
    // by doing so.
    static constexpr int kVersionDefault = 0;

    static int versionAfterAssigning(int version, bool valueIsExactlyTrue);

    // Where a node's identifier comes from. It is a counter outside the class, raised for each
    // node made, so two nodes never share one and their order tells which was made first.
    static bool identifierComesFromACounter();

    // Whether every node records where it was made. The base class asks this of a flag on the
    // class itself rather than of the instance, so turning it on records a trace for every node
    // in the page rather than for the ones a caller names.
    static bool traceIsAClassLevelSetting();

    // Whether caching a node's result in a temporary variable is allowed by default, which it is;
    // a class that must not be cached has to say so.
    static constexpr bool kCacheableByDefault = true;
};
