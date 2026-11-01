#include "loaders/LUTImageLoader.h"

std::vector<std::uint8_t> LUTImageLoader::horizontalToVertical(
    const std::vector<std::uint8_t>& source, int sourceWidth, int sourceHeight, bool flip) {
    // A slice is square, so its side is the source height.
    const int width = sourceHeight;
    const int height = sourceWidth;
    if (width <= 0 || height <= 0) return {};

    const std::size_t sourceStride = static_cast<std::size_t>(sourceWidth) * 4;
    if (source.size() < sourceStride * static_cast<std::size_t>(sourceHeight)) return {};

    // The output is as wide as the source is tall.
    std::vector<std::uint8_t> out(static_cast<std::size_t>(width) * 4 *
                                  static_cast<std::size_t>(height), 0);
    const std::size_t outStride = static_cast<std::size_t>(width) * 4;

    for (int i = 0; i < width; ++i) {
        // The source strip is a square at column i of the strip row.
        const std::size_t sy = static_cast<std::size_t>(i) * static_cast<std::size_t>(width);
        const std::size_t dy = flip ? static_cast<std::size_t>(height - i * width)
                                    : static_cast<std::size_t>(i * width);
        // Copy that square to the destination.
        for (int row = 0; row < width; ++row) {
            for (int column = 0; column < width; ++column) {
                const std::size_t from = (sy + static_cast<std::size_t>(row)) * sourceStride +
                                         static_cast<std::size_t>(column) * 4;
                const std::size_t to = (dy + static_cast<std::size_t>(row)) * outStride +
                                       static_cast<std::size_t>(column) * 4;
                if (from + 4 > source.size() || to + 4 > out.size()) continue;
                for (int b = 0; b < 4; ++b) out[to + static_cast<std::size_t>(b)] =
                    source[from + static_cast<std::size_t>(b)];
            }
        }
    }
    return out;
}
