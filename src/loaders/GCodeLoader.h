#pragma once
#include <cstddef>
#include <string>
#include <vector>

// Port of the state machine in examples/jsm/loaders/GCodeLoader.js at the pinned revision. A
// program is a list of commands that move a head around, and the loader turns the movements into
// line segments grouped by layer.
class GCodeLoader {
  public:
    // Where the head is and how the program has asked for the numbers to be read. The extrusion
    // amount is tracked beside the position rather than as a fourth axis, since it says how much
    // material was laid down and not where.
    struct State {
        double x = 0, y = 0, z = 0;
        double e = 0;
        double f = 0;
        bool extruding = false;
        // Whether coordinates are read as offsets from the current position.
        bool relative = false;
        // Whether the extrusion amount has been overridden, and whether that override is itself
        // an offset. The two are separate so that a program may mix them.
        bool extrusionOverride = false;
        bool extrusionRelative = false;
    };

    // One step of a coordinate. A program may write either an absolute position or an offset from
    // where the head already is, and the current position is the first argument either way.
    static double absoluteValue(bool relative, double current, double target);

    // Which of a layer's two lists a segment belongs in. Extruding material is the part that
    // becomes a surface; moving without extruding only travels, so it goes in the path list.
    enum class SegmentList { Extruded, Path };
    static SegmentList segmentList(bool extruding);

    // A layer holds both lists and the height it was started at, which is the height of the move
    // that began it rather than a value the program states.
    struct Layer {
        std::vector<double> extruded;
        std::vector<double> path;
        double z = 0;
    };

    // Appends a segment to a layer, six numbers at a time, choosing the list by whether the
    // program was extruding when the segment was made.
    static void addSegment(Layer& layer, bool extruding, const double from[3], const double to[3]);

    // The name a line material is given for one of the two lists.
    static std::string segmentListName(SegmentList list);
};
