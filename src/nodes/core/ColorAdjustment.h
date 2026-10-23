#pragma once

// Port of the parts of src/nodes/display/ColorAdjustment.js at the pinned revision that can be
// worked out without a shader builder. Three of the four adjustments have a neutral value that
// is not the obvious nought, and the hue one carries a constant that is shorter than the value it
// stands for.
class ColorAdjustment {
  public:
    // The luminance of a colour, which the module gets from a helper rather than writing here;
    // it is passed in so that these tests need not stand up a whole colour space.
    static double luminance(double r, double g, double b);

    // The saturation. The adjustment is the mix parameter, so one leaves the colour alone and
    // nought pulls it all the way to the luminance; above one the colour is pushed past itself,
    // which is what super saturating means.
    static double saturation(double colour, double luminance, double adjustment);

    // The vibrance. The amount is the distance of the largest part from the average, scaled by
    // the adjustment and multiplied by minus three, so a positive adjustment makes the mix
    // parameter negative rather than positive.
    struct Vibrance {
        double average = 0;
        double maxPart = 0;
        double amount = 0;
    };

    static Vibrance vibranceAmounts(double r, double g, double b, double adjustment);

    // The mix the vibrance applies, which moves the colour towards its largest part.
    static double vibranceMix(double colour, double maxPart, double amount);

    // The axis the hue rotates about, which is the unit vector with all three parts equal. The
    // constant is written to five decimal places rather than being the exact root, so the axis
    // is not quite of unit length; the exact value is one over the square root of three.
    static constexpr double kHueAxis = 0.57735;

    // Whether the constant really is the exact value, which it is not.
    static bool hueAxisIsExact();
};
