#pragma once
#include <map>
#include <string>
#include <vector>

// Port of the text layout half of examples/jsm/loaders/FontLoader.js at the pinned revision.
// Font.parse is a single construction, and the glyph outline interpreter that turns a glyph's
// command string into a ShapePath needs the ShapePath layer, so this slice covers the data
// model and the placement arithmetic that decides where each character sits.
class Font {
  public:
    struct Glyph {
        // Horizontal advance, in font units; the layout multiplies it by the scale.
        double ha = 0;
        // The outline command string, kept for the layer that interprets it.
        std::string o;
    };

    struct Data {
        std::string familyName;
        double resolution = 1;
        double boundingBoxYMin = 0;
        double boundingBoxYMax = 0;
        double underlineThickness = 0;
        double ascender = 0;
        std::map<std::string, Glyph> glyphs;
    };

    // One character placed by the layout, at the offset it was drawn with.
    struct Placement {
        std::string character;
        double x = 0;
        double y = 0;
    };

    static constexpr bool isFont = true;
    std::string type = "Font";

    explicit Font(Data data) : data(std::move(data)) {}

    // Splits text into code points, as upstream does with Array.from, so a multi byte
    // character counts as one glyph.
    static std::vector<std::string> codePoints(const std::string& text);

    // The line height in device units, which is what a newline advances by.
    double lineHeight(double size) const;

    // Places every character. Right to left and top to bottom reverse the character order
    // first; a newline returns to the left edge and drops a line, and a top down layout
    // advances the vertical offset by the ascender instead of the horizontal one.
    std::vector<Placement> layoutText(const std::string& text, double size = 100,
                                      const std::string& direction = "ltr") const;

    Data data;
};
