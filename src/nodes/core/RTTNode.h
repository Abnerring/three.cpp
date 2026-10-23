#pragma once
#include <optional>
#include <string>

// Port of the rules in src/nodes/utils/RTTNode.js at the pinned revision. The node renders another
// node into a texture, and four of its rules are worth writing down.
class RTTNode {
  public:
    static constexpr bool isRTTNode = true;
    static constexpr const char* kTypeName = "RTTNode";

    // The size the internal target gets when no width or height was given. It is one by one rather
    // than any of the sizes a person would pick, because the target is meant to be resized later
    // when the node is set to resize itself.
    static constexpr int kDefaultSize = 1;

    // Whether the target resizes itself. There is no flag for this: it is answered by asking
    // whether a width was given, so it is read only and follows the width rather than being set
    // beside it. Giving a width turns resizing off, and leaving it out turns it on.
    static bool autoResize(bool hasWidth);

    // The size the target is made at. A missing width means the default, while a width of nought
    // is kept as nought: the test is for missing rather than for falsy, so a caller who asks for
    // nothing gets nothing instead of the default.
    static int sizeOr(int size, bool hasSize);

    // Which texture type the target ends up with. The option list is spread over a default that
    // already names a type, so a caller who names one overrides it while one who does not keeps
    // the default; the spread comes second, so it wins.
    static std::string textureType(bool callerNamedType, const std::string& callerType);

    // Where the renderer state saved around the render is kept. It is kept per node rather than
    // shared, because a node rendered from inside another's render would otherwise overwrite the
    // outer one's state.
    static bool stateIsPerInstance();

    // The kind of refresh the node asks for, which is once per frame.
    static constexpr const char* kUpdateType = "FRAME";

    static bool isCacheable();
};
