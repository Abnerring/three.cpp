#include "loaders/FontLoader.h"

#include "extras/core/ShapePath.h"
#include <algorithm>
#include <cstdlib>

std::vector<std::string> Font::codePoints(const std::string& text) {
    // A minimal decoder: a lead byte announces how many continuation bytes follow.
    std::vector<std::string> out;
    std::size_t i = 0;
    while (i < text.size()) {
        const unsigned char lead = static_cast<unsigned char>(text[i]);
        std::size_t length = 1;
        if ((lead & 0x80) == 0) length = 1;
        else if ((lead & 0xE0) == 0xC0) length = 2;
        else if ((lead & 0xF0) == 0xE0) length = 3;
        else if ((lead & 0xF8) == 0xF0) length = 4;
        if (i + length > text.size()) length = 1;
        out.push_back(text.substr(i, length));
        i += length;
    }
    return out;
}

double Font::lineHeight(double size) const {
    const double scale = size / data.resolution;
    return (data.boundingBoxYMax - data.boundingBoxYMin + data.underlineThickness) * scale;
}

std::vector<Font::Placement> Font::layoutText(const std::string& text, double size,
                                             const std::string& direction) const {
    const double scale = size / data.resolution;
    const double height = lineHeight(size);

    auto chars = codePoints(text);
    if (direction == "rtl" || direction == "tb") std::reverse(chars.begin(), chars.end());

    std::vector<Placement> placed;
    double offsetX = 0;
    double offsetY = 0;

    for (const auto& character : chars) {
        if (character == "\n") {
            offsetX = 0;
            offsetY -= height;
            continue;
        }

        // The glyph comes from the font, falling back to the question mark as upstream does.
        auto found = data.glyphs.find(character);
        if (found == data.glyphs.end()) found = data.glyphs.find("?");
        if (found == data.glyphs.end()) continue;  // upstream logs and returns nothing

        placed.push_back(Placement{character, offsetX, offsetY});

        if (direction == "tb") {
            offsetX = 0;
            offsetY += data.ascender * scale;
        } else {
            // The advance comes from the glyph itself.
            offsetX += found->second.ha * scale;
        }
    }
    return placed;
}

Font::GlyphPath Font::createPath(const std::string& character, double scale, double offsetX,
                                 double offsetY, const Data& data) {
    GlyphPath result;
    auto found = data.glyphs.find(character);
    if (found == data.glyphs.end()) found = data.glyphs.find("?");
    if (found == data.glyphs.end()) return result;  // upstream logs and returns nothing

    const Glyph& glyph = found->second;
    result.advance = glyph.ha * scale;
    result.path = std::make_shared<ShapePath>();
    if (glyph.o.empty()) return result;

    // The command string is a flat list of tokens: a letter then its numbers.
    std::vector<std::string> tokens;
    {
        std::size_t i = 0;
        while (i < glyph.o.size()) {
            while (i < glyph.o.size() && glyph.o[i] == ' ') ++i;
            if (i >= glyph.o.size()) break;
            const std::size_t start = i;
            while (i < glyph.o.size() && glyph.o[i] != ' ') ++i;
            tokens.push_back(glyph.o.substr(start, i - start));
        }
    }

    const auto coordinate = [&](std::size_t index) {
        const double value = index < tokens.size() ? std::strtod(tokens[index].c_str(), nullptr) : 0.0;
        return value;
    };
    const auto toX = [&](double value) { return value * scale + offsetX; };
    const auto toY = [&](double value) { return value * scale + offsetY; };

    std::size_t i = 0;
    while (i < tokens.size()) {
        const std::string action = tokens[i++];
        if (action == "m") {
            result.path->moveTo(toX(coordinate(i)), toY(coordinate(i + 1)));
            i += 2;
        } else if (action == "l") {
            result.path->lineTo(toX(coordinate(i)), toY(coordinate(i + 1)));
            i += 2;
        } else if (action == "q") {
            // The first pair is the end point and the second the control, so the target is
            // passed last to the path builder.
            const double endX = coordinate(i);
            const double endY = coordinate(i + 1);
            const double controlX = coordinate(i + 2);
            const double controlY = coordinate(i + 3);
            result.path->quadraticCurveTo(toX(controlX), toY(controlY), toX(endX), toY(endY));
            i += 4;
        } else if (action == "b") {
            // The first pair is the end point, then two control points.
            const double endX = coordinate(i);
            const double endY = coordinate(i + 1);
            const double control1X = coordinate(i + 2);
            const double control1Y = coordinate(i + 3);
            const double control2X = coordinate(i + 4);
            const double control2Y = coordinate(i + 5);
            result.path->bezierCurveTo(toX(control1X), toY(control1Y), toX(control2X),
                                       toY(control2Y), toX(endX), toY(endY));
            i += 6;
        } else {
            // An unknown action stops the loop, as stray tokens cannot be sized.
            break;
        }
    }
    return result;
}