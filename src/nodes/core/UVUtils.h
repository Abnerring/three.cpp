#pragma once

// Port of the three helpers in src/nodes/utils/UVUtils.js at the pinned revision. Two of them
// change a coordinate pair and one of them decides how a replacement is asked for.
class UVUtils {
  public:
    // The centre both helpers rotate or warp about when a caller names none.
    static constexpr double kDefaultCenter = 0.5;

    // How a replacement for the default coordinate is supplied. The source accepts either a
    // function or a value, and tells them apart by asking whether what it was given is a
    // function: anything that is not is wrapped so that it answers the same value however often
    // it is asked. A caller that passed a node where a function was expected therefore gets a
    // replacement that ignores the texture it is handed, rather than an error.
    enum class ReplacementKind { Function, Constant };

    static ReplacementKind kindOf(bool isFunction);

    // What a replacement answers. A function's answer is whatever it returns; a constant's is
    // the constant itself, whichever texture asks.
    static double answer(ReplacementKind kind, double functionAnswer, double constantValue);

    // A coordinate turned about a centre by an angle. The turning is done on the offset from the
    // centre and the centre is added back, which is what makes the centre stay still; turning
    // the coordinate itself would swing it about the origin instead.
    struct Pair {
        double u = 0, v = 0;
    };

    static Pair rotate(const Pair& uv, double radians, const Pair& centre);

    // A coordinate pushed away from the centre by a displacement that falls off as the fifth
    // power of the distance. The source works the fourth power out by squaring the squared
    // distance and then multiplies by the distance again, so the displacement is the fifth power
    // times the strength; a linear displacement would be the obvious thing and is not this.
    static Pair spherize(const Pair& uv, double strength, const Pair& centre);

    // The displacement factor the spherize uses, given the squared distance from the centre.
    static double spherizeOffset(double squaredDistance, double strength);
};
