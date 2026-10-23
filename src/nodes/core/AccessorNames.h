#pragma once
#include <string>

// Port of the naming rules the accessor modules build their attribute names with, from
// src/nodes/accessors/UV.js and ModelViewProjectionNode.js at the pinned revision.
class AccessorNames {
  public:
    // The attribute name a uv index stands for. The index is appended only when it is greater
    // than nought, so index nought gives the plain name with nothing on the end. A negative
    // index is not refused: it fails the same test as nought and so gives the plain name too,
    // which is a property of the comparison rather than a rule about indices.
    static std::string uvAttribute(int index);

    // The shared name the model view projection accessor is built under, with the prefix a
    // varying carries.
    static std::string modelViewProjectionName();
};
