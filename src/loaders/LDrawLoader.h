#pragma once
#include <array>
#include <vector>
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

    // What a comment line names, if anything. A line that begins with a bare comment names
    // nothing and is ignored.
    enum class MetaKind { None, LDrawOrg, Colour, Category, Keywords, File, Bfc };

    static MetaKind metaKind(const std::string& token);

    // The keywords a keywords line carries: the rest of the line split on commas with each
    // piece trimmed. A piece is produced even when the rest is empty, because splitting an
    // empty string gives one empty piece rather than none.
    static std::vector<std::string> parseKeywords(const std::string& rest);

    // A colour token from a value or edge tag, rewritten in the form the port keeps colours
    // in. A leading 0x becomes a hash; a token that already starts with one is left alone.
    // Anything else is not a colour and is reported, naming the line when it has a number.
    static std::string normalizeColour(const std::string& token, int lineNumber);

    // An alpha byte turned into a fraction, clamped to the range. A value above the byte range
    // reads as fully opaque and a negative one as fully transparent.
    static double alphaFromByte(int value);

    // Whether an alpha below fully opaque makes the material transparent.
    static bool isTransparentAlpha(double alpha);

    // Three vertices read for a triangle. When the winding is not counter clockwise the format
    // lists them the other way round, so the first one read becomes the last of the face and
    // the order is reversed after reading.
    static std::array<std::array<double, 3>, 3> readTriangle(LineParser& parser, bool ccw);

    // Four vertices read for a quadrilateral, reversed the same way when the winding is not
    // counter clockwise.
    static std::array<std::array<double, 3>, 4> readQuad(LineParser& parser, bool ccw);

    // Whether a face shows from both sides. A file that never certified its winding, or one
    // that certified but does not cull, is visible from both.
    static bool isDoubleSided(bool certified, bool cull);

    // How many face records a triangle contributes, which is two when it shows from both sides.
    static int triangleFaceCount(bool doubleSided);

    // A part file name tidied for lookup: the whitespace around it removed and every backslash
    // turned into a forward slash, since the format is written on systems that use the former.
    static std::string normalizeFileName(const std::string& raw);

    // Where a part actually lives. A name the caller mapped is used as given; otherwise the two
    // standard folders are taken into account, so a name under s is looked for under parts and
    // one under 48 under p, keeping the rest of the path as it was.
    static std::string resolveFileName(const std::string& name,
                                       const std::vector<std::string>& mappedNames,
                                       const std::vector<std::string>& mappedPaths);

    // The transform a reference places its part with. The nine rotation numbers arrive row by
    // row and the position follows, so the four by four is written out in the column major
    // order the engine stores.
    static std::array<double, 16> referenceMatrix(const ReferenceLine& reference);

    // A ray with a unit direction and its origin at the foot of the perpendicular from the
    // origin of the world, which is what lets two segments along the same line be compared
    // regardless of which end each was written from.
    struct NormalizedRay {
        std::array<double, 3> origin{};
        std::array<double, 3> direction{};
    };

    static NormalizedRay toNormalizedRay(const std::array<double, 3>& v0,
                                        const std::array<double, 3>& v1);

    // One vertex as a key: each coordinate scaled by the multiplier and truncated towards zero
    // as a thirty two bit value, joined by commas.
    static std::string hashVertex(const std::array<double, 3>& v, double multiplier);

    // Two vertices as a key. The pair is joined with an underscore in the order given, so the
    // same edge read from the other end gives a different key and both have to be recorded.
    static std::string hashEdge(const std::array<double, 3>& v0, const std::array<double, 3>& v1,
                                double multiplier);

    // The cosine below which two faces meeting at an edge are treated as a hard edge rather
    // than a smooth one. The source comment calls this sixty seven and a half degrees, which
    // would be about three eighths; a quarter is nearer seventy five and a half, so the number
    // is what the port follows.
    static constexpr double kHardEdgeCosine = 0.25;

    // The normal of the face three vertices describe: the cross product of the two edge
    // vectors, narrowed to unit length. A degenerate face has no normal and returns zero.
    static std::array<double, 3> faceNormal(const std::array<double, 3>& v0,
                                           const std::array<double, 3>& v1,
                                           const std::array<double, 3>& v2);

    // Whether two faces meeting at an edge should be smoothed across it. The comparison is on
    // the absolute value of the dot product, so a face and its opposite count as smooth.
    static bool isSmoothPair(double dotOfFaceNormals);

    // The two codes that stand for "use the colour of whatever contains me", one for faces and
    // one for edges. They are written as text because that is how the file spells them and
    // because a code may also be a name.
    static constexpr const char* kMainColourCode = "16";
    static constexpr const char* kMainEdgeColourCode = "24";

    // The key a file is cached under, which is its name lower cased so that lookups ignore case.
    static std::string cacheKey(const std::string& fileName);

    // Whether a code asks for the containing colour. A face asks with sixteen and an edge with
    // twenty four, so the same number means different things depending on what it is on.
    static bool isPassthroughCode(const std::string& code, bool forEdge);

    // The code to look a material up by, with a passthrough replaced by the parent's.
    static std::string resolveColourCode(const std::string& code, const std::string& parentCode,
                                         bool forEdge);

    // The code an edge takes when its face asked for the containing colour: the main colour
    // becomes the main edge colour and every other code is left alone.
    static std::string edgeColourCode(const std::string& code);

    // The lines of a file, with the two character line endings of one platform reduced to the
    // single character of the other first. A single character ending is left as it is, and no
    // trailing empty line is invented.
    static std::vector<std::string> splitLines(const std::string& text);

    // An embedded file: the name it declares and the text that follows.
    struct EmbeddedFile {
        std::string name;
        std::string text;
    };

    // Splits a file that carries others inside it. The body is everything before the first
    // embedded header after the opening line; everything from there on belongs to the embedded
    // files, each beginning with a header that names it. The text of a file keeps the newlines
    // that separated its lines, so it ends with one whenever it had any lines at all.
    struct EmbeddedSplit {
        std::size_t bodyLineCount = 0;
        std::vector<EmbeddedFile> files;
    };

    static EmbeddedSplit splitEmbeddedFiles(const std::vector<std::string>& lines);
};
