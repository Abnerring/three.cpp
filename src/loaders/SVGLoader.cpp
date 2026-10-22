#include "loaders/SVGLoader.h"

#include <cmath>
#include <cstdlib>
#include <stdexcept>

namespace {
bool isSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; }
}  // namespace

std::string SVGLoader::gradientId(const std::string& value) {
    std::size_t at = 0;
    // Whitespace before the word is allowed, and only before it.
    while (at < value.size() && isSpace(value[at])) ++at;
    if (value.compare(at, 4, "url(") != 0) return std::string();
    at += 4;

    // Whitespace, then an optional opening quote, then more whitespace.
    while (at < value.size() && isSpace(value[at])) ++at;
    if (at < value.size() && (value[at] == '"' || value[at] == '\'')) {
        ++at;
        while (at < value.size() && isSpace(value[at])) ++at;
    }

    // The identifier must open with a hash.
    if (at >= value.size() || value[at] != '#') return std::string();
    ++at;

    // It runs until a quote, a closing bracket or whitespace, and must not be empty.
    const std::size_t start = at;
    while (at < value.size()) {
        const char c = value[at];
        if (c == ')' || c == '"' || c == '\'' || isSpace(c)) break;
        ++at;
    }
    if (at == start) return std::string();
    const std::string id = value.substr(start, at - start);

    // What follows may be whitespace, an optional closing quote, more whitespace and the
    // bracket, and then only whitespace to the end. Anything else means the value is not a
    // single reference.
    while (at < value.size() && isSpace(value[at])) ++at;
    if (at < value.size() && (value[at] == '"' || value[at] == '\'')) {
        ++at;
        while (at < value.size() && isSpace(value[at])) ++at;
    }
    if (at >= value.size() || value[at] != ')') return std::string();
    ++at;
    while (at < value.size() && isSpace(value[at])) ++at;
    if (at != value.size()) return std::string();

    return id;
}

namespace {
bool isWhitespace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
bool isDigit(char c) { return c >= '0' && c <= '9'; }
bool isSign(char c) { return c == '-' || c == '+'; }
bool isPoint(char c) { return c == '.'; }
bool isComma(char c) { return c == ','; }
bool isExp(char c) { return c == 'e' || c == 'E'; }
bool isFlag(char c) { return c == '0' || c == '1'; }
}  // namespace

std::vector<double> SVGLoader::parseFloats(const std::string& input,
                                           const std::vector<int>& flagPositions, int stride) {
    enum State { SEP, INT, FLOAT, EXP };
    State state = SEP;
    bool seenComma = true;
    std::string number;
    std::string exponent;
    std::vector<double> result;

    // A number is only pushed once it is complete, and the exponent scales it afterwards.
    const auto newNumber = [&result, &number, &exponent]() {
        if (!number.empty()) {
            const double base = std::strtod(number.c_str(), nullptr);
            if (exponent.empty()) {
                result.push_back(base);
            } else {
                result.push_back(base * std::pow(10.0, std::strtod(exponent.c_str(), nullptr)));
            }
        }
        number.clear();
        exponent.clear();
    };

    for (std::size_t i = 0; i < input.size(); ++i) {
        const char current = input[i];

        // A flag is recognised by its place in the group and needs no separator around it.
        if (stride > 0) {
            const int position = static_cast<int>(result.size()) % stride;
            bool isFlagPosition = false;
            for (const int flag : flagPositions) {
                if (flag == position) isFlagPosition = true;
            }
            if (isFlagPosition && isFlag(current)) {
                state = INT;
                number = std::string(1, current);
                newNumber();
                continue;
            }
        }

        if (state == SEP) {
            if (isWhitespace(current)) continue;
            if (isDigit(current) || isSign(current)) {
                state = INT;
                number = std::string(1, current);
                continue;
            }
            if (isPoint(current)) {
                state = FLOAT;
                number = std::string(1, current);
                continue;
            }
            if (isComma(current)) {
                // Two commas with no number between them are a syntax error.
                if (seenComma) {
                    throw std::runtime_error("Unexpected character \"" + std::string(1, current) +
                                             "\" at index " + std::to_string(i) + ".");
                }
                seenComma = true;
            }
        }

        if (state == INT) {
            if (isDigit(current)) { number += current; continue; }
            if (isPoint(current)) { number += current; state = FLOAT; continue; }
            if (isExp(current)) { state = EXP; continue; }
            // A sign directly after a sign is an error, but a sign after a number separates it.
            if (isSign(current) && number.size() == 1 && isSign(number[0])) {
                throw std::runtime_error("Unexpected character \"" + std::string(1, current) +
                                         "\" at index " + std::to_string(i) + ".");
            }
        }

        if (state == FLOAT) {
            if (isDigit(current)) { number += current; continue; }
            if (isExp(current)) { state = EXP; continue; }
            // Two decimal points in one number are an error.
            if (isPoint(current) && !number.empty() && number.back() == '.') {
                throw std::runtime_error("Unexpected character \"" + std::string(1, current) +
                                         "\" at index " + std::to_string(i) + ".");
            }
        }

        if (state == EXP) {
            if (isDigit(current)) { exponent += current; continue; }
            // Only the first character of the exponent may be a sign.
            if (isSign(current) && exponent.empty()) { exponent += current; continue; }
        }

        // Anything else ends the number in progress.
        state = SEP;
        newNumber();
        if (isComma(current)) seenComma = true;
        else seenComma = false;
    }
    newNumber();
    return result;
}

std::vector<SVGLoader::TransformOp> SVGLoader::parseTransformList(const std::string& text) {
    // The text is cut at every closing bracket, which leaves one piece per transform.
    std::vector<std::string> pieces;
    std::size_t at = 0;
    while (true) {
        const std::size_t close = text.find(')', at);
        if (close == std::string::npos) {
            pieces.push_back(text.substr(at));
            break;
        }
        pieces.push_back(text.substr(at, close - at));
        at = close + 1;
    }

    std::vector<TransformOp> ops;
    // The pieces are read from the last to the first.
    for (std::size_t i = pieces.size(); i > 0; --i) {
        std::string piece = pieces[i - 1];
        // The whitespace around a piece is not part of it.
        std::size_t begin = 0;
        while (begin < piece.size() && (piece[begin] == ' ' || piece[begin] == '\t' ||
                                       piece[begin] == '\n' || piece[begin] == '\r')) {
            ++begin;
        }
        std::size_t last = piece.size();
        while (last > begin && (piece[last - 1] == ' ' || piece[last - 1] == '\t' ||
                                piece[last - 1] == '\n' || piece[last - 1] == '\r')) {
            --last;
        }
        piece = piece.substr(begin, last - begin);
        // A piece with nothing in it belongs to a bracket pair that closed immediately.
        if (piece.empty()) continue;

        const std::size_t open = piece.find('(');
        // A transform needs both its name and its bracket.
        if (open == std::string::npos || open == 0) continue;

        TransformOp op;
        op.name = piece.substr(0, open);
        op.args = parseFloats(piece.substr(open + 1));
        ops.push_back(op);
    }
    return ops;
}

std::vector<SVGLoader::Point> SVGLoader::applyStraightCommand(LineCommand command,
                                                            const std::vector<double>& numbers,
                                                            PathCursor& cursor) {
    std::vector<Point> points;
    // Records a point as visited and, when it is the first of the command, as the path's start.
    const auto visit = [&cursor, &points](double x, double y) {
        Point next;
        next.x = x;
        next.y = y;
        cursor.point = next;
        cursor.control = next;
        // Visiting anything means the path now has something to close.
        cursor.hasSegments = true;
        if (cursor.setFirstPoint && points.empty()) {
            cursor.firstPoint = next;
        }
        points.push_back(next);
    };

    if (command == LineCommand::Horizontal) {
        // One number per step, and the other axis stays where it is.
        for (const double value : numbers) visit(value, cursor.point.y);
        return points;
    }
    if (command == LineCommand::Vertical) {
        for (const double value : numbers) visit(cursor.point.x, value);
        return points;
    }

    // Both of the remaining commands take pairs of numbers.
    for (std::size_t i = 0; i + 1 < numbers.size(); i += 2) {
        visit(numbers[i], numbers[i + 1]);
    }
    return points;
}

double SVGLoader::reflect(double point, double control) {
    // Twice the point less the control, written the way the source writes it.
    return point - (control - point);
}

std::vector<SVGLoader::CurveSegment> SVGLoader::applyCurveCommand(
    CurveCommand command, const std::vector<double>& numbers, PathCursor& cursor) {
    std::vector<CurveSegment> segments;

    // Each command reads a different number of values per segment: a cubic writes six, a
    // smooth cubic four, a quadratic four and a smooth quadratic two.
    std::size_t stride = 0;
    if (command == CurveCommand::Cubic) stride = 6;
    else if (command == CurveCommand::SmoothCubic) stride = 4;
    else if (command == CurveCommand::Quadratic) stride = 4;
    else stride = 2;

    for (std::size_t i = 0; i + stride <= numbers.size(); i += stride) {
        CurveSegment segment;
        if (command == CurveCommand::Cubic) {
            // Both controls are written, then the end point.
            segment.control1.x = numbers[i];
            segment.control1.y = numbers[i + 1];
            segment.control2.x = numbers[i + 2];
            segment.control2.y = numbers[i + 3];
            segment.end.x = numbers[i + 4];
            segment.end.y = numbers[i + 5];
            segment.cubic = true;
            // The second control is the one a following smooth command reflects.
            cursor.control = segment.control2;
        } else if (command == CurveCommand::SmoothCubic) {
            // The first control is reflected and the second is written.
            segment.control1.x = reflect(cursor.point.x, cursor.control.x);
            segment.control1.y = reflect(cursor.point.y, cursor.control.y);
            segment.control2.x = numbers[i];
            segment.control2.y = numbers[i + 1];
            segment.end.x = numbers[i + 2];
            segment.end.y = numbers[i + 3];
            segment.cubic = true;
            cursor.control = segment.control2;
        } else if (command == CurveCommand::Quadratic) {
            // A quadratic writes one control and the end point.
            segment.control1.x = numbers[i];
            segment.control1.y = numbers[i + 1];
            segment.end.x = numbers[i + 2];
            segment.end.y = numbers[i + 3];
            segment.cubic = false;
            cursor.control = segment.control1;
        } else {
            // A smooth quadratic reflects its control and reads only the end point.
            segment.control1.x = reflect(cursor.point.x, cursor.control.x);
            segment.control1.y = reflect(cursor.point.y, cursor.control.y);
            segment.end.x = numbers[i];
            segment.end.y = numbers[i + 1];
            segment.cubic = false;
            cursor.control = segment.control1;
        }
        cursor.point = segment.end;
        segments.push_back(segment);
    }
    return segments;
}

bool SVGLoader::applyCloseCommand(PathCursor& cursor) {
    // A path with nothing in it cannot be closed, so the cursor stays where it is.
    if (!cursor.hasSegments) return false;
    // The point returns to the first one and the next command starts a fresh subpath.
    cursor.point = cursor.firstPoint;
    cursor.isFirstPoint = true;
    return true;
}

SVGLoader::Point SVGLoader::transformPoint(const Matrix& matrix, const Point& point) {
    Point moved;
    // The first two numbers are the image of the x axis and the next two that of the y axis.
    moved.x = matrix.a * point.x + matrix.c * point.y + matrix.e;
    moved.y = matrix.b * point.x + matrix.d * point.y + matrix.f;
    return moved;
}

SVGLoader::Matrix SVGLoader::translateMatrix(const std::vector<double>& args) {
    Matrix matrix;
    if (args.empty()) return matrix;
    matrix.e = args[0];
    // A single number moves only the horizontal axis.
    matrix.f = args.size() >= 2 ? args[1] : 0.0;
    return matrix;
}

SVGLoader::Matrix SVGLoader::scaleMatrix(const std::vector<double>& args) {
    Matrix matrix;
    if (args.empty()) return matrix;
    matrix.a = args[0];
    // A single number scales both axes by the same amount.
    matrix.d = args.size() >= 2 ? args[1] : args[0];
    return matrix;
}

SVGLoader::Matrix SVGLoader::rotateMatrix(const std::vector<double>& args) {
    Matrix matrix;
    if (args.empty()) return matrix;
    // The angle is written in degrees and turns clockwise on the page, which is the direction
    // the coordinates run.
    const double radians = args[0] * 3.14159265358979323846 / 180.0;
    const double cosine = std::cos(radians);
    const double sine = std::sin(radians);
    matrix.a = cosine;
    matrix.b = sine;
    matrix.c = -sine;
    matrix.d = cosine;

    // With a centre the turn happens about it rather than about the origin.
    if (args.size() >= 3) {
        const double cx = args[1];
        const double cy = args[2];
        matrix.e = cx - cosine * cx + sine * cy;
        matrix.f = cy - sine * cx - cosine * cy;
    }
    return matrix;
}