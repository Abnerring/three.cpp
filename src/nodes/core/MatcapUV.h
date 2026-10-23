#pragma once
#include <string>

// Port of the rules in src/nodes/utils/MatcapUV.js at the pinned revision, and of the types the
// three timer uniforms use. The matcap coordinates are built from two axes crossed against the
// view direction and then taken into the nought to one range by a scale and an offset.
class MatcapUV {
  public:
    // The scale and the offset the two coordinates are taken into range by. The scale is not a
    // half: the source uses nought point four nine five so that the coordinates stop just short
    // of the edges, which its comment says is to remove artifacts caused by undersized matcap
    // disks. Using a half would push the rim of the disk onto the last pixel.
    static constexpr double kScale = 0.495;
    static constexpr double kOffset = 0.5;

    // The range the coordinates therefore occupy, which is not nought to one.
    static constexpr double kMinimum = 0.005;
    static constexpr double kMaximum = 0.995;

    // One coordinate, from a dot product of the axis and the normal.
    static double coordinate(double dotProduct);

    // The first axis, built from two parts of the view direction with the second negated and the
    // vertical part set to nought. It is returned unnormalised; a caller that needs a direction
    // normalises it.
    struct Axis {
        double x = 0, y = 0, z = 0;
    };

    static Axis firstAxis(double viewDirectionX, double viewDirectionZ);

    // The three timer uniforms, two of which are floats and one of which is an unsigned integer.
    static constexpr const char* kTimeType = "float";
    static constexpr const char* kDeltaTimeType = "float";
    static constexpr const char* kFrameIdType = "uint";
};
