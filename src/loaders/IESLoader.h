#pragma once
#include <string>
#include <vector>

// Port of the header half of examples/jsm/loaders/IESLoader.js at the pinned revision. An IES
// file is the text photometric description of a lamp: a keyword block with tilt data, then a
// table of vertical and horizontal angles with a candela value for each pair. This slice reads
// the text helpers and the tilt block; the tables are the layer above.
class IESLoader {
  public:
    // The tilt block: whether the lamp needs tilting, how much, and the angle and multiplier
    // tables that go with it.
    struct Tilt {
        bool present = false;
        double lampToLuminaireGeometry = 0;
        int numAngles = 0;
        std::vector<double> angles;
        std::vector<double> multipliers;
    };

    // Splits a line into numbers. Commas separate as well as whitespace, and runs of spaces
    // collapse, so the several dialects the format allows all read the same.
    static std::vector<double> textToNumbers(const std::string& text);

    // Reads count numbers spread over as many lines as it takes, starting at the given line.
    // The cursor advanced past those lines.
    static std::vector<double> readNumbers(const std::vector<std::string>& lines,
                                           std::size_t& cursor, std::size_t count);

    // Reads a tilt block starting at the given line, where the first line is the lamp to
    // luminaire geometry value.
    static Tilt readTilt(const std::vector<std::string>& lines, std::size_t& cursor);

    // Splits text into lines, dropping a trailing carriage return from each.
    static std::vector<std::string> splitLines(const std::string& text);
};
