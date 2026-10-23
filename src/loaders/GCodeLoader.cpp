#include "loaders/GCodeLoader.h"

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
