#include "loaders/IESLoader.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <stdexcept>

std::vector<std::string> IESLoader::splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find('\n', start);
        std::string line = end == std::string::npos ? text.substr(start) : text.substr(start, end - start);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return lines;
}

std::vector<double> IESLoader::textToNumbers(const std::string& text) {
    // Trim, treat a comma as a separator, then collapse runs of whitespace before splitting.
    std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    std::size_t last = text.find_last_not_of(" \t\r\n");
    std::string cleaned = text.substr(first, last - first + 1);
    for (auto& c : cleaned)
        if (c == ',') c = ' ';

    std::vector<double> numbers;
    std::size_t at = 0;
    while (at < cleaned.size()) {
        while (at < cleaned.size() && std::isspace(static_cast<unsigned char>(cleaned[at]))) ++at;
        if (at >= cleaned.size()) break;
        const std::size_t begin = at;
        while (at < cleaned.size() && !std::isspace(static_cast<unsigned char>(cleaned[at]))) ++at;
        numbers.push_back(std::strtod(cleaned.substr(begin, at - begin).c_str(), nullptr));
    }
    return numbers;
}

std::vector<double> IESLoader::readNumbers(const std::vector<std::string>& lines,
                                           std::size_t& cursor, std::size_t count) {
    std::vector<double> numbers;
    // Keep taking whole lines until the count is reached, so a table may be wrapped.
    while (numbers.size() < count && cursor < lines.size()) {
        const auto values = textToNumbers(lines[cursor++]);
        for (const double value : values) numbers.push_back(value);
    }
    // A table with more values than asked for keeps only the count.
    if (numbers.size() > count) numbers.resize(count);
    return numbers;
}

IESLoader::Tilt IESLoader::readTilt(const std::vector<std::string>& lines, std::size_t& cursor) {
    Tilt tilt;
    if (cursor >= lines.size()) return tilt;

    // The first line is the lamp to luminaire geometry value.
    const auto geometry = textToNumbers(lines[cursor++]);
    tilt.lampToLuminaireGeometry = geometry.empty() ? 0.0 : geometry[0];

    if (cursor >= lines.size()) return tilt;
    const auto count = textToNumbers(lines[cursor++]);
    tilt.numAngles = count.empty() ? 0 : static_cast<int>(count[0]);
    if (tilt.numAngles <= 0) return tilt;

    tilt.angles = readNumbers(lines, cursor, static_cast<std::size_t>(tilt.numAngles));
    tilt.multipliers = readNumbers(lines, cursor, static_cast<std::size_t>(tilt.numAngles));
    tilt.present = true;
    return tilt;
}

IESLoader::LampValues IESLoader::readLampValues(const std::vector<std::string>& lines,
                                                 std::size_t& cursor) {
    LampValues values;
    const auto numbers = readNumbers(lines, cursor, 10);
    if (numbers.size() < 10) return values;

    // The order is fixed by the format.
    values.count = static_cast<int>(numbers[0]);
    values.lumens = numbers[1];
    values.multiplier = numbers[2];
    values.numVerticalAngles = static_cast<int>(numbers[3]);
    values.numHorizontalAngles = static_cast<int>(numbers[4]);
    values.gonioType = static_cast<int>(numbers[5]);
    values.units = static_cast<int>(numbers[6]);
    values.width = numbers[7];
    values.length = numbers[8];
    values.height = numbers[9];
    return values;
}

IESLoader::LampFactors IESLoader::readLampFactors(const std::vector<std::string>& lines,
                                                   std::size_t& cursor) {
    LampFactors factors;
    const auto numbers = readNumbers(lines, cursor, 3);
    if (numbers.size() < 3) return factors;

    factors.ballast = numbers[0];
    factors.ballastLightOutput = numbers[1];
    factors.inputWatts = numbers[2];
    return factors;
}

IESLoader::Tables IESLoader::readTables(const std::vector<std::string>& lines,
                                        std::size_t& cursor, int numVertical, int numHorizontal) {
    Tables tables;
    if (numVertical <= 0 || numHorizontal <= 0) return tables;

    tables.verticalAngles = readNumbers(lines, cursor, static_cast<std::size_t>(numVertical));
    tables.horizontalAngles = readNumbers(lines, cursor, static_cast<std::size_t>(numHorizontal));

    // One row of vertical readings per horizontal angle.
    tables.candela.reserve(static_cast<std::size_t>(numHorizontal));
    for (int i = 0; i < numHorizontal; ++i)
        tables.candela.push_back(readNumbers(lines, cursor, static_cast<std::size_t>(numVertical)));
    return tables;
}

double IESLoader::normalizeTables(Tables& tables, double multiplier, double ballast,
                                  double ballastLightOutput) {
    // The readings are relative, so they scale by the lamp multiplier and both ballast factors.
    const double scale = multiplier * ballast * ballastLightOutput;
    for (auto& row : tables.candela)
        for (auto& value : row) value *= scale;

    // A missing horizontal table has no base angle to shift by.
    if (tables.horizontalAngles.empty()) return -1.0;

    // The measured range starts at its first angle, so shifting makes it begin at zero and lets
    // the sampler mirror it across a full cycle.
    const double baseAngle = tables.horizontalAngles[0];
    double maximum = -1.0;
    for (std::size_t i = 0; i < tables.horizontalAngles.size(); ++i) {
        tables.horizontalAngles[i] -= baseAngle;
        if (i >= tables.candela.size()) continue;
        for (const double value : tables.candela[i])
            if (value > maximum) maximum = value;
    }
    return maximum;
}

IESLoader::Lamp IESLoader::parse(const std::string& text) {
    Lamp lamp;
    const auto lines = splitLines(text);
    std::size_t cursor = 0;

    // Find the line that names the tilt: everything before it is metadata the port ignores.
    bool foundTilt = false;
    while (cursor < lines.size()) {
        const std::string& line = lines[cursor++];
        if (line.find("TILT") != std::string::npos) {
            foundTilt = true;
            // The keyword itself is the only tilt information when it says none.
            if (line.find("NONE") == std::string::npos && line.find("INCLUDE") != std::string::npos) {
                lamp.tilt = readTilt(lines, cursor);
                lamp.hasTilt = lamp.tilt.present;
            }
            // Upstream ignores a tilt that names a file, so nothing more is read here either.
            break;
        }
    }
    if (!foundTilt) throw std::runtime_error("THREE.IESLoader: TILT keyword not found");

    // The blocks follow in a fixed order.
    lamp.values = readLampValues(lines, cursor);
    lamp.factors = readLampFactors(lines, cursor);
    lamp.tables = readTables(lines, cursor, lamp.values.numVerticalAngles,
                             lamp.values.numHorizontalAngles);
    lamp.maximum = normalizeTables(lamp.tables, lamp.values.multiplier, lamp.factors.ballast,
                                   lamp.factors.ballastLightOutput);
    return lamp;
}

double IESLoader::interpolateCandela(const Tables& tables, double phi, double theta) {
    int thetaIndex = 0, phiIndex = 0;
    double startTheta = 0, endTheta = 0, startPhi = 0, endPhi = 0;
    const int numHorizontal = static_cast<int>(tables.horizontalAngles.size());
    const int numVertical = static_cast<int>(tables.verticalAngles.size());

    // Find the horizontal interval the sample falls in, using the last one as a fallback.
    for (int i = 0; i < numHorizontal - 1; ++i) {
        if (theta < tables.horizontalAngles[i + 1] || i == numHorizontal - 2) {
            thetaIndex = i;
            startTheta = tables.horizontalAngles[i];
            endTheta = tables.horizontalAngles[i + 1];
            break;
        }
    }
    // The same for the vertical interval.
    for (int i = 0; i < numVertical - 1; ++i) {
        if (phi < tables.verticalAngles[i + 1] || i == numVertical - 2) {
            phiIndex = i;
            startPhi = tables.verticalAngles[i];
            endPhi = tables.verticalAngles[i + 1];
            break;
        }
    }

    const double deltaTheta = endTheta - startTheta;
    const double deltaPhi = endPhi - startPhi;
    // A zero height interval means the sample is outside the measured range.
    if (deltaPhi == 0) return 0;

    const double t1 = deltaTheta == 0 ? 0 : (theta - startTheta) / deltaTheta;
    const double t2 = (phi - startPhi) / deltaPhi;
    const int nextThetaIndex = deltaTheta == 0 ? thetaIndex : thetaIndex + 1;

    const auto lerp = [](double a, double b, double t) { return a + (b - a) * t; };
    if (thetaIndex >= static_cast<int>(tables.candela.size())) return 0;
    if (nextThetaIndex >= static_cast<int>(tables.candela.size())) return 0;
    if (phiIndex + 1 >= static_cast<int>(tables.candela[static_cast<std::size_t>(thetaIndex)].size()))
        return 0;

    // Interpolate across the horizontal direction at both vertical ends, then between them.
    const auto& rowA = tables.candela[static_cast<std::size_t>(thetaIndex)];
    const auto& rowB = tables.candela[static_cast<std::size_t>(nextThetaIndex)];
    const double v1 = lerp(rowA[static_cast<std::size_t>(phiIndex)],
                           rowB[static_cast<std::size_t>(phiIndex)], t1);
    const double v2 = lerp(rowA[static_cast<std::size_t>(phiIndex) + 1],
                           rowB[static_cast<std::size_t>(phiIndex) + 1], t1);
    return lerp(v1, v2, t2);
}

double IESLoader::mirrorHorizontalAngle(double theta, double endAngle) {
    // A lamp measured over part of the cycle is mirrored to cover the rest.
    if (endAngle == 0 || theta <= endAngle) return theta;
    double sample = std::fmod(theta, endAngle * 2.0);
    if (sample > endAngle) sample = endAngle * 2.0 - sample;
    return sample;
}

std::vector<double> IESLoader::sampleValues(const Tables& tables, int numHorizontalAngles) {
    const int width = numHorizontalAngles == 1 ? 1 : 360;
    const int height = 180;
    std::vector<double> data(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0);
    if (tables.horizontalAngles.empty() || tables.verticalAngles.empty()) return data;

    const double endTheta = tables.horizontalAngles.back();
    for (int theta = 0; theta < width; ++theta) {
        const double sampleTheta = mirrorHorizontalAngle(theta, endTheta);
        for (int phi = 0; phi < height; ++phi) {
            const double samplePhi = phi * 180.0 / (height - 1);
            // A column is one horizontal angle, so theta multiplies the row count.
            data[static_cast<std::size_t>(phi) +
                 static_cast<std::size_t>(theta) * static_cast<std::size_t>(height)] =
                interpolateCandela(tables, samplePhi, sampleTheta);
        }
    }
    return data;
}