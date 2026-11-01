#pragma once
#include <string>
#include <vector>

// Port of the gradient reference reading in examples/jsm/loaders/SVGLoader.js at the pinned
// revision. A fill or a stroke may name a gradient by writing url(#id), with the quotes and the
// whitespace around it being optional. Reading the colours themselves needs a canvas, so only
// the reference is portable.
class SVGLoader {
  public:
    // The colour space a fill is read in.
    static constexpr const char* kColorSpace = "srgb";

    // The identifier a paint value names, or an empty string when the value is not a reference
    // at all. The whole value has to be one reference: leading and trailing whitespace is
    // allowed, the identifier may be quoted on either side, and the identifier runs until a
    // quote, a closing bracket or whitespace.
    static std::string gradientId(const std::string& value);

    // The numbers on a path command. A path may write its numbers with or without separators,
    // may use either comma or whitespace as one, and may even use a sign as one, so the reader
    // walks the text as a small state machine. Throws std::runtime_error for a malformed run,
    // naming the character and its place.
    //
    // The flag positions are the ones a stride of seven needs for an arc, where the four
    // numbers after the first are a rotation, two single digit flags and two coordinates; a
    // flag may be written with no separator at all, so it is recognised by position.
    static std::vector<double> parseFloats(const std::string& input,
                                           const std::vector<int>& flagPositions = {},
                                           int stride = 0);

    // One transform from a transform attribute: its name and the numbers in its parentheses.
    struct TransformOp {
        std::string name;
        std::vector<double> args;
    };

    // The transforms an attribute lists. The text is cut at each closing bracket and the pieces
    // are read from the last to the first, because a transform is applied to a point from the
    // right, so the rightmost is the one that acts first. A piece with nothing in it is skipped
    // and a piece with no opening bracket is not a transform at all. A rotation angle is given
    // in degrees and is left as read; the caller scales it.
    static std::vector<TransformOp> parseTransformList(const std::string& text);

    // A point on a path.
    struct Point {
        double x = 0;
        double y = 0;
        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };

    // Where a path has reached. The control point is kept for the smooth commands that follow,
    // and the first point is remembered so a closed path can return to it.
    struct PathCursor {
        Point point;
        Point control;
        Point firstPoint;
        bool isFirstPoint = false;
        // Whether any segment has been visited, which decides whether a close has anything to do.
        // Whether the first point of a command becomes the path's first point. When it is
        // set the first point is overwritten each time a command begins.
        bool setFirstPoint = true;
        bool hasSegments = false;
        // Whether the next command begins a fresh subpath, which closing sets.
    };

    // The straight line commands.
    enum class LineCommand { Move, Line, Horizontal, Vertical };

    // Applies a straight line command. A move takes pairs and the first pair starts the path
    // while the rest continue it, a line takes pairs and continues the path throughout, and the
    // two single axis commands take one number each and leave the other axis where it was. The
    // points the command visits are returned in order, and the first of a move is marked as the
    // start of a subpath.
    static std::vector<Point> applyStraightCommand(LineCommand command,
                                                   const std::vector<double>& numbers,
                                                   PathCursor& cursor);

    // A curve a command describes: two control points for a cubic, one for a quadratic, and
    // the point it ends on either way.
    struct CurveSegment {
        Point control1;
        Point control2;
        Point end;
        // Whether both control points matter, which is true for a cubic and false for a
        // quadratic where only the first is read.
        bool cubic = false;
    };

    // The curve commands that carry their own control points, and the two smooth ones that
    // take a reflected control point instead.
    enum class CurveCommand { Cubic, SmoothCubic, Quadratic, SmoothQuadratic };

    // Reflects one coordinate through another: the value a control point takes when it is
    // mirrored across the point the path has reached.
    static double reflect(double point, double control);

    // Applies a curve command, returning the segments it describes and moving the cursor to the
    // last end point. A smooth command reflects the cursor's control point to make its own; the
    // control point it leaves behind is the one the next smooth command will reflect.
    static std::vector<CurveSegment> applyCurveCommand(CurveCommand command,
                                                       const std::vector<double>& numbers,
                                                       PathCursor& cursor);

    // Closes the current subpath. A path with no segments yet has nothing to close, so the
    // cursor is left alone; otherwise the point returns to the first one and the next command
    // is treated as the start of a fresh subpath. The return says whether anything closed.
    static bool applyCloseCommand(PathCursor& cursor);

    // A two by three affine transform, written the way the format writes one: the first two
    // numbers are the first column, the next two the second, and the last two the translation.
    struct Matrix {
        double a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;
    };

    // A point moved by a transform. The first two numbers stretch the two axes, so x picks a
    // and c while y picks b and d, and the last two are added afterwards.
    static Point transformPoint(const Matrix& matrix, const Point& point);

    // The transform a translate, a scale or a rotate describes, given its numbers. A translate
    // takes one or two, a scale takes one or two with the second defaulting to the first, and a
    // rotate takes an angle in degrees and optionally a centre to turn about.
    static Matrix translateMatrix(const std::vector<double>& args);
    static Matrix scaleMatrix(const std::vector<double>& args);
    static Matrix rotateMatrix(const std::vector<double>& args);

    // A skew along one axis. The angle is in degrees and leans the axis it names, so a
    // horizontal skew moves a point sideways in proportion to its height and leaves the height
    // alone.
    static Matrix skewXMatrix(const std::vector<double>& args);
    static Matrix skewYMatrix(const std::vector<double>& args);

    // One transform followed by another. A point is moved by the right hand one first, which is
    // why the transform list is read from its right end.
    static Matrix multiply(const Matrix& left, const Matrix& right);

    // The seven numbers an arc carries: two radii, a rotation, two flags and an end point.
    struct ArcSegment {
        double rx = 0;
        double ry = 0;
        double rotation = 0;
        // Whether the arc takes the longer way round, which is any value other than zero.
        bool largeArc = false;
        // Whether the arc goes the way the coordinates run, which is likewise any nonzero.
        bool sweep = false;
        Point end;
    };

    // Groups a flat run of numbers into arcs. A run that does not fill a group leaves the rest
    // unread.
    static std::vector<ArcSegment> parseArcSegments(const std::vector<double>& numbers);

    // Whether an arc from one point to another is skipped. The two are compared exactly, so an
    // arc that ends a hair away from where it began is still drawn; the source tests them with
    // the same exact comparison.
    static bool shouldSkipArc(const Point& start, const Point& end);

    // The commands a path may carry. A lowercase letter is the relative form of the command
    // its uppercase spells, which is a property of the letter rather than a separate command.
    enum class CommandKind {
        Move,
        Line,
        Horizontal,
        Vertical,
        Cubic,
        SmoothCubic,
        Quadratic,
        SmoothQuadratic,
        Arc,
        Close,
        Unknown,
    };

    // The command a letter names, ignoring case.
    static CommandKind commandKind(char letter);

    // Whether a letter is written in its relative form, which is what a lowercase letter marks.
    static bool isRelativeCommand(char letter);

    // Whether a letter names a command at all.
    static bool isCommandLetter(char letter);
};
