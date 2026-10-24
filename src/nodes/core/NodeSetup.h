#pragma once
#include <optional>
#include <string>

// Port of the reference and setup rules in src/nodes/core/Node.js at the pinned revision. Four of
// them are worth writing down, and two are recursion guards whose order matters.
class NodeSetup {
  public:
    // What the reference method answers when nothing has replaced it, which is the node itself.
    // A node that refers to nothing is its own reference, so a caller always has something.
    static bool defaultReferenceIsTheNode();

    // What installing a callback does. It replaces the method on that one instance with a bound
    // function rather than recording a callback to be consulted later, so the class's own method
    // is shadowed for that instance and the replacement runs where the method would have.
    static bool replacesTheMethodOnTheInstance();

    // Whether the global answer is the node's flag, which it is by default; a class may compute it
    // instead of reading the flag.
    static bool globalAnswerIsTheFlag();

    // When a node counts as set up. A record is marked before the work runs, so a call that comes
    // back round to the same node during its own setup finds the mark and does not set it up a
    // second time. Setting the mark afterwards would let the second call in and the work would run
    // twice, which is the opposite of what the mark is for.
    static bool marksBeforeTheWork();

    // What the output node becomes, which is the first of three that is a value: what setting up
    // answered, else what was already recorded, else nothing. So a class whose setup answers
    // nothing does not clear an output node that something else had already put there.
    enum class OutputSource { FromSetup, FromBefore, None };

    static OutputSource outputSource(bool setupAnswered, bool hadOneBefore);

    // Whether a child is registered as a parent for the node, which needs its flag to be exactly
    // true rather than merely a value.
    static bool registersAsParent(bool parentsFlagIsExactlyTrue);
};
