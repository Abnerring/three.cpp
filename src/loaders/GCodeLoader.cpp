#include "loaders/GCodeLoader.h"

#include <cstdlib>

double GCodeLoader::absoluteValue(bool relative, double current, double target) {
    // An offset is added to where the head is; an absolute value replaces it.
    return relative ? current + target : target;
}

GCodeLoader::SegmentList GCodeLoader::segmentList(bool extruding) {
    return extruding ? SegmentList::Extruded : SegmentList::Path;
}

void GCodeLoader::addSegment(Layer& layer, bool extruding, const double from[3],
                             const double to[3]) {
    // Both ends go in, one after the other, which is what makes a list of separate segments
    // rather than one long run.
    std::vector<double>& list =
        extruding ? layer.extruded : layer.path;
    list.push_back(from[0]);
    list.push_back(from[1]);
    list.push_back(from[2]);
    list.push_back(to[0]);
    list.push_back(to[1]);
    list.push_back(to[2]);
}

std::string GCodeLoader::segmentListName(SegmentList list) {
    // The two names the materials carry, so a caller can find them again.
    return list == SegmentList::Extruded ? "extruded" : "path";
}

bool GCodeLoader::extrusionIsRelative(bool extrusionOverride, bool extrusionRelative,
                                      bool globalRelative) {
    // The extruder's own override wins when it was set.
    return extrusionOverride ? extrusionRelative : globalRelative;
}

double GCodeLoader::absoluteExtrusion(bool extrusionOverride, bool extrusionRelative,
                                      bool globalRelative, double current, double target) {
    return extrusionIsRelative(extrusionOverride, extrusionRelative, globalRelative)
               ? current + target
               : target;
}

std::string GCodeLoader::stripComment(const std::string& line) {
    // Everything from the first semicolon onwards goes.
    const std::size_t at = line.find(';');
    if (at == std::string::npos) return line;
    return line.substr(0, at);
}

GCodeLoader::Argument GCodeLoader::parseArgument(const std::string& token) {
    Argument argument;
    // An empty token has no letter and is skipped, which is what a run of spaces leaves.
    if (token.empty()) return argument;
    argument.present = true;
    // The letter is lower cased; the number is whatever follows it.
    const char letter = token[0];
    argument.letter =
        static_cast<char>(letter >= 'A' && letter <= 'Z' ? letter - 'A' + 'a' : letter);
    // A number that does not parse reads as not a number, which the source lets through.
    argument.value = std::strtod(token.c_str() + 1, nullptr);
    return argument;
}

bool GCodeLoader::isExtruding(double previousAmount, double amount) {
    // More material than before means the head is laying it down.
    return amount > previousAmount;
}

bool GCodeLoader::beginsLayer(bool hasLayer, double layerZ, double z, bool extruding) {
    // Only an extruding step can start a layer, and then only at a new height.
    if (!extruding) return false;
    if (!hasLayer) return true;
    return z != layerZ;
}