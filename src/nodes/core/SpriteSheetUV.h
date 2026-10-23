#pragma once

// Port of the rules in src/nodes/utils/SpriteSheetUV.js at the pinned revision. The function
// turns a frame number into the offset of one cell in a grid of cells, and the row it works out
// is written in a way that does not look like the obvious one.
class SpriteSheetUV {
  public:
    // The frame number a value stands for, taken into the range of the grid by the remainder
    // and then cut to a whole number. The order matters: the remainder is taken first, so a
    // value that is not whole still lands in the grid.
    static int frameNumber(double frame, int width, int height);

    // The column of a frame, which is its remainder against the width.
    static int column(int frameNumber, int width);

    // The row of a frame, written as the height less the ceiling of one more than the frame
    // divided by the width. This is the same as the height less one less the frame divided by
    // the width with the remainder dropped, which a test pins over a sweep rather than by a
    // remark; the longer form is what the source has, so it is what is written here.
    static int row(int frameNumber, int width, int height);

    // The row the shorter form gives, which the one above must always agree with.
    static int rowWouldBe(int frameNumber, int width, int height);

    // The scale a cell's offset is multiplied by, which is one over the count in each direction.
    static double scale(int count);
};
