#pragma once
#include <cstdint>
#include <vector>

// Port of the layout work in examples/jsm/loaders/LUTImageLoader.js at the pinned revision. A
// lookup table image stores its slices side by side horizontally, and the loader turns that
// into a vertical stack before sampling. Reading the image itself needs a decoder the port
// does not carry, so only the rearrangement is portable.
class LUTImageLoader {
  public:
    // Rearranges a horizontal strip layout into a vertical stack. Each slice is a square of
    // side equal to the source height, copied from a column of strips down a row of strips.
    // The output is therefore as wide as the source was tall and as tall as the source was
    // wide. Four bytes per pixel are copied as they are.
    static std::vector<std::uint8_t> horizontalToVertical(
        const std::vector<std::uint8_t>& source, int sourceWidth, int sourceHeight, bool flip);
};
