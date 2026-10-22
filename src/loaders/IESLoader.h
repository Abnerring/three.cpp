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

    // The ten values the lamp block carries, in the order the format writes them.
    struct LampValues {
        int count = 0;
        double lumens = 0;
        double multiplier = 0;
        int numVerticalAngles = 0;
        int numHorizontalAngles = 0;
        int gonioType = 0;
        int units = 0;
        double width = 0;
        double length = 0;
        double height = 0;
    };

    // The three factors that follow the angle tables.
    struct LampFactors {
        double ballast = 0;
        double ballastLightOutput = 0;
        double inputWatts = 0;
    };

    // Reads the ten lamp values as a single table spread over as many lines as needed.
    static LampValues readLampValues(const std::vector<std::string>& lines, std::size_t& cursor);

    // Reads the three factors the same way.
    static LampFactors readLampFactors(const std::vector<std::string>& lines, std::size_t& cursor);

    // The three tables of a lamp: the vertical angles, the horizontal angles and the candela
    // readings. The readings are indexed by horizontal first and vertical second, which is the
    // order the file writes them in and the opposite of what a row major reading would suggest.
    struct Tables {
        std::vector<double> verticalAngles;
        std::vector<double> horizontalAngles;
        std::vector<std::vector<double>> candela;
    };

    // Reads the vertical angles, then the horizontal ones, then one row of candela values per
    // horizontal angle.
    static Tables readTables(const std::vector<std::string>& lines, std::size_t& cursor,
                             int numVertical, int numHorizontal);

    // Scales every reading by the multiplier and the two ballast factors, then shifts the
    // horizontal angles so the first one is zero, returning the largest scaled reading.
    static double normalizeTables(Tables& tables, double multiplier, double ballast,
                                  double ballastLightOutput);

    // Everything one lamp file describes.
    struct Lamp {
        bool hasTilt = false;
        Tilt tilt;
        LampValues values;
        LampFactors factors;
        Tables tables;
        // The largest candela reading after scaling, or minus one when there were no tables.
        double maximum = -1;
    };

    // Reads a whole lamp: scans for the line naming the tilt, reads the optional tilt block,
    // then the ten values, the three factors and the tables, and finally scales them. Throws
    // std::runtime_error when the file has no tilt keyword at all.
    static Lamp parse(const std::string& text);
};
