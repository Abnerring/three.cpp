#pragma once
#include <string>
#include <vector>

// Port of the gradient reference reading in examples/jsm/loaders/SVGLoader.js at the pinned
// revision. A fill or a stroke may name a gradient by writing url(#id), with the quotes and the
// whitespace around it being optional. Reading the colours themselves needs a canvas, so only
// the reference is portable.
class SVGLoader {
  public:
    // The colour space a fill is read in.
    static constexpr const char* kColorSpace = "srgb";

    // The identifier a paint value names, or an empty string when the value is not a reference
    // at all. The whole value has to be one reference: leading and trailing whitespace is
    // allowed, the identifier may be quoted on either side, and the identifier runs until a
    // quote, a closing bracket or whitespace.
    static std::string gradientId(const std::string& value);

    // The numbers on a path command. A path may write its numbers with or without separators,
    // may use either comma or whitespace as one, and may even use a sign as one, so the reader
    // walks the text as a small state machine. Throws std::runtime_error for a malformed run,
    // naming the character and its place.
    //
    // The flag positions are the ones a stride of seven needs for an arc, where the four
    // numbers after the first are a rotation, two single digit flags and two coordinates; a
    // flag may be written with no separator at all, so it is recognised by position.
    static std::vector<double> parseFloats(const std::string& input,
                                           const std::vector<int>& flagPositions = {},
                                           int stride = 0);
};
