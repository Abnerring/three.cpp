#pragma once
#include <string>

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
};
