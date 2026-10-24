#pragma once
#include <cstdint>
#include <optional>
#include <string>

// Port of the hash bookkeeping in src/nodes/core/NodeBuilder.js at the pinned revision. A node is
// found again by a hash, and the record of that is four lines with three consequences worth
// writing down.
class NodeBuilderHash {
  public:
    // What kind of thing the record is. It is a plain object rather than a map, so the hash is
    // used as a string key; that means any text works as a hash including text a map would have
    // kept apart, and it also means the record inherits whatever a plain object carries.
    static bool recordIsAPlainObject();

    // What setting an entry does. There is no check for an existing one, so a later node with the
    // same hash replaces an earlier one rather than being refused or kept alongside; the record
    // therefore holds one node per hash, the last one set, and a caller cannot tell from it that
    // another node once had that hash.
    static std::optional<std::uint64_t> afterSetting(std::optional<std::uint64_t> existing,
                                                     std::uint64_t newNode);

    // What an absent hash answers, which is nothing at all rather than a default node.
    static bool absentAnswersNothing(bool found);

    // When a node's hash is worked out. It is done once, when the node is added, and the record is
    // written then; so a node whose answer would change later keeps the hash it had at the moment
    // it was added, and the lookup is a plain read rather than a call.
    static bool hashIsComputedAtAddTime();

    // Whether adding the same node twice does anything. The set of nodes refuses a second copy, so
    // the second call sets no hash a second time and the node's hash is not recomputed.
    static bool addingTheSameNodeTwiceChangesNothing(bool alreadyPresent);
};
