#pragma once

// Port of three of the blend modes in src/nodes/display/BlendModes.js at the pinned revision.
// Each takes two colours and answers a third. What is worth writing down is not the arithmetic
// alone but the two ends: the colour that leaves the base alone is white for one of them and
// black for another, and the clamping is written one way in one and the other way in the next
// even though the two forms are the same.
class BlendModes {
  public:
    // Colour burn, which darkens the base by the blend. The dividing is done on the inverted
    // base, the result is clamped from above, and it is inverted again. A white blend leaves
    // the base alone, since one divided by one is one and inverting twice gives the base back.
    static double burn(double base, double blend);

    // Colour dodge, which lightens the base by the blend. Here the base is divided by the
    // inverted blend; a black blend leaves the base alone, since the divisor is one.
    static double dodge(double base, double blend);

    // Screen, which also lightens but less sharply: the two inverted colours are multiplied and
    // the answer inverted. A black blend leaves the base alone and a white one gives white.
    static double screen(double base, double blend);
};
