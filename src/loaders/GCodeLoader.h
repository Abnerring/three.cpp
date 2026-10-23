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
    // Whether the extrusion amount is read as an offset. The program may override this for the
    // extruder alone, and when it has not the global setting applies, so the answer is a choice
    // between three flags rather than two.
    static bool extrusionIsRelative(bool extrusionOverride, bool extrusionRelative,
                                    bool globalRelative);

    // The extrusion amount for a step, read the way the two flags above say.
    static double absoluteExtrusion(bool extrusionOverride, bool extrusionRelative,
                                    bool globalRelative, double current, double target);

    // The comment stripper. A semicolon begins a comment that runs to the end of its line, so the
    // text from the first one onwards goes and the newline that ends the line stays.
    static std::string stripComment(const std::string& line);

    // One argument of a command: a single letter and a number written straight after it. A token
    // that is empty has no letter and is skipped, which is what a run of spaces produces.
    struct Argument {
        bool present = false;
        char letter = 0;
        double value = 0;
    };
    static Argument parseArgument(const std::string& token);

    // Whether a step is laying down material. The program does not say so directly: a step
    // extrudes when its extrusion amount is greater than the one before it, so the change in the
    // amount is what decides, not the command that moved the head.
    static bool isExtruding(double previousAmount, double amount);

    // Whether a step begins a new layer. Only an extruding step can, and then only when there is
    // no layer yet or the height differs from the one the current layer started at.
    static bool beginsLayer(bool hasLayer, double layerZ, double z, bool extruding);
    // The four commands that change how later numbers are read. Each returns whether it applied,
    // so a caller can tell a mode change from a command it does not know.
    //
    // The two positioning commands do something easy to miss: besides setting their own mode they
    // also clear the extruder's override, so an M82 or M83 earlier in the program stops applying
    // as soon as a G90 or G91 appears.
    static bool applyModeCommand(const std::string& command, State& state);

    // Whether a command is one of the four.
    static bool isModeCommand(const std::string& command);

    // A position reset. Only the axes the command names are changed; the rest keep their values.
    // The source mutates the state object in place rather than building a new one, which amounts
    // to the same thing as long as every axis is written back.
    static void resetPosition(State& state, double x, bool hasX, double y, bool hasY, double z,
                              bool hasZ, double e, bool hasE);
    // The result of a movement step: where the head ends up, which list the segment belongs in,
    // and whether the step also opened a new layer.
    //
    // The order of these decisions matters and is easy to get wrong. The step first works out
    // where it ends, then decides whether it is extruding from the change in amount, then opens a
    // layer if that decision and the height say so, and only then records the segment. The
    // segment therefore runs from the position before the step to the one after it, but it is
    // filed by the extrusion state the step has already set, not the one it inherited.
    struct Move {
        State next;
        bool extruding = false;
        bool openedLayer = false;
        double from[3] = {0, 0, 0};
        double to[3] = {0, 0, 0};
    };

    // Applies one movement. Only the axes the caller names are changed; the rest keep the values
    // they had, which is what makes a command that moves only x leave y and z alone.
    static Move applyMove(const State& state, bool hasX, double x, bool hasY, double y, bool hasZ,
                          double z, bool hasE, double e, bool hasF, double f);
};
