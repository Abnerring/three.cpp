#pragma once

// Port of the decision in src/nodes/utils/ViewportUtils.js at the pinned revision. The function
// adjusts the coordinate used to read a viewport texture so that something in front of a
// refracting surface does not appear on it, and which of two coordinates it answers with is the
// part worth writing down.
class ViewportUtils {
  public:
    // Which coordinate the function answers with. The two are the plain screen coordinate and
    // the one it was given, and the order they are offered to the choice is the opposite of what
    // the reason might suggest: when the sampled depth is nearer than the surface, which is the
    // case the function exists to correct, it answers with the screen coordinate and throws the
    // given one away.
    enum class Choice { Screen, Given };

    // The choice from the difference between the two depths. A difference below nought means the
    // sample is in front, and only that case is corrected; exactly nought counts as behind, so
    // a surface compared with itself keeps its own coordinate.
    static Choice choose(double depthDifference);

    // The name of the depth function the difference is taken against, which the module asks of
    // the linear depth rather than of the raw one.
    static constexpr const char* kDepthKind = "linear";
};
