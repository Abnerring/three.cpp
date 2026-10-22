#include "loaders/FontLoader.h"

#include <algorithm>

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
