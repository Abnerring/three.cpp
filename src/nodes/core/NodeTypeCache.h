#pragma once
#include <map>
#include <optional>
#include <string>

// Port of the type caching in src/nodes/core/Node.js at the pinned revision. A node's type is asked
// for more than once and is not cheap to work out, so it is remembered; the remembering uses two
// different places depending on whether an output was named.
class NodeTypeCache {
  public:
    // Where a type is remembered. With an output named, the answer goes into a map keyed by that
    // output, so two questions about two outputs are remembered separately and each may give a
    // different answer. With no output named, the answer goes into a single slot. So the cache is
    // not one thing but two, and a caller switching between the two forms reaches different
    // answers rather than different spellings of one.
    enum class CacheSlot { Single, PerOutput };

    static CacheSlot slotFor(bool outputWasNamed);

    // What a lookup and a miss look like. A remembered type is used as it stands; a miss works the
    // type out and then remembers it. The test is against the absence of an entry rather than
    // against a falsy value, so a type remembered as the empty string is still remembered and is
    // not worked out again.
    struct Lookup {
        bool wasRemembered = false;
        std::string type;
    };

    static Lookup lookup(const std::optional<std::string>& remembered,
                         const std::string& computedOnMiss);

    // Whether an empty type is remembered, which it is: the entry exists, so the next question
    // finds it and does no work. A test against a falsy value would redo the work every time.
    static bool emptyTypeIsRemembered();

    // What the default type computation does. If the node has an output node of its own the
    // question is passed to it, output and all, so a wrapper answers with what it wraps; otherwise
    // the node's own declared type is answered. So the default is a pass-through rather than a
    // calculation, and a class that computes something has replaced it.
    enum class Source { DelegatedToOutputNode, OwnDeclaredType };

    static Source sourceFor(bool hasOutputNode);

    // Whether the output is passed along when the question is delegated, which it is, so the
    // wrapped node is asked the same question rather than a simpler one.
    static bool outputIsPassedThrough();
};
