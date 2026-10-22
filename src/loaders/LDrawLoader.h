#pragma once
#include <array>
#include <string>

// Port of the line cursor in examples/jsm/loaders/LDrawLoader.js at the pinned revision. The
// format is plain text, one directive per line, so every reader starts by cutting a line into
// tokens. The cursor keeps its place and skips the whitespace between tokens as it goes.
class LDrawLoader {
  public:
    class LineParser {
      public:
        // The line to walk and the number it had in the file, which a diagnostic repeats back.
        // A negative number means the line came from somewhere without one.
        LineParser(std::string line, int lineNumber);

        // Moves the cursor to the next character that is neither a space nor a tab, leaving it
        // at the end when the line has none left.
        void seekNonSpace();

        // The next token. The first character of the token is stepped over on the way in, which
        // relies on the cursor already standing on it, and the whitespace after the token is
        // skipped before returning. At the end of the line the token is empty.
        std::string getToken();

        // Three numbers read as tokens.
        std::array<double, 3> getVector();

        // Everything from the cursor to the end, taken as one piece.
        std::string getRemainingString() const;

        // Whether the cursor has reached the end.
        bool isAtTheEnd() const;

        // Moves the cursor to the end.
        void setToEnd();

        // A suffix naming the line, or nothing when it has no number.
        std::string getLineNumberString() const;

        // The place the cursor stands at.
        std::size_t position() const { return index_; }

      private:
        std::string line_;
        std::size_t length_ = 0;
        std::size_t index_ = 0;
        int lineNumber_ = -1;
    };

    // A reference line, the first kind of directive. It names a colour, a position, a three by
    // three rotation written row by row and the part file to place.
    struct ReferenceLine {
        std::string colorCode;
        std::array<double, 3> position{};
        std::array<double, 9> rotation{};
        std::string fileName;
    };

    // Reads a reference line. The colour token is kept as it was written, since it may be a
    // number or a name, and the nine rotation numbers are read one at a time in the order the
    // format writes them. The file name is whatever follows, taken as one token.
    static ReferenceLine parseReferenceLine(LineParser& parser);
};
