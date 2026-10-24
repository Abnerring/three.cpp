#pragma once
#include <cstdint>
#include <string>

// Port of the accessor defaults in src/nodes/core/Node.js at the pinned revision. Five of them
// answer with something a caller could mistake for a computed result, and one of them settles a
// question the classes below it disagree about.
class NodeAccessors {
  public:
    // The type a member has when nothing below has said otherwise. The base class answers with
    // the empty type, and three of its subclasses replace that with a float while two others keep
    // it; so the empty type is the rule and the float is the exception, not the other way round.
    static constexpr const char* kMemberTypeDefault = "void";

    // The scope of a node, which by default is the node itself rather than a name.
    static bool scopeIsTheNodeItself();

    // The hash of a node, which by default is its identifier written as text. A class below may
    // answer with a name instead, and the attribute and varying ones do.
    static std::string hash(std::uint64_t id);

    // The custom cache key, which by default is the identifier as a number rather than as text;
    // a class below may answer with something derived from what it holds.
    static std::uint64_t customCacheKey(std::uint64_t id);

    // The name a child is stored under, with its last four characters removed. Children are held
    // under the names of the properties that own them, and those names all end in the word for a
    // node, so removing it leaves the property's own name. A caller who took the stored name for
    // the property's name would be four characters out.
    static std::string propertyNameFromStorageName(const std::string& storageName);

    // Whether a cache key is recomputed. It is recomputed when the caller asks or when the node's
    // version has moved past the version the key was made at, so a node that was changed
    // invalidates its own key without anyone forcing it. The second condition is or-ed into the
    // caller's request, so the two are not distinguishable afterwards.
    static bool shouldRecompute(bool callerAsked, int version, int keyVersion);
};
