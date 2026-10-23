#include "nodes/core/SpriteSheetUV.h"

#include <cmath>

int SpriteSheetUV::frameNumber(double frame, int width, int height) {
    // The remainder against the whole grid first, then cut down to a whole number.
    const double cells = static_cast<double>(width) * static_cast<double>(height);
    double remainder = std::fmod(frame, cells);
    if (remainder < 0.0) remainder += cells;
    return static_cast<int>(std::floor(remainder));
}

int SpriteSheetUV::column(int frameNumber, int width) {
    // The remainder against the width.
    return frameNumber % width;
}

int SpriteSheetUV::row(int frameNumber, int width, int height) {
    // The longer form the source uses.
    const int ceiling = static_cast<int>(std::ceil((frameNumber + 1.0) / static_cast<double>(width)));
    return height - ceiling;
}

int SpriteSheetUV::rowWouldBe(int frameNumber, int width, int height) {
    // The shorter form, which the longer one is equivalent to.
    return height - 1 - frameNumber / width;
}

double SpriteSheetUV::scale(int count) {
    return 1.0 / static_cast<double>(count);
}
