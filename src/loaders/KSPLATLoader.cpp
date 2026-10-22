#include "loaders/KSPLATLoader.h"

namespace {
// The component order per degree, flattened from the three bands.
const int kBandIndex[3][21] = {
    {0, 3, 6, 1, 4, 7, 2, 5, 8},
    {9, 14, 19, 10, 15, 20, 11, 16, 21, 12, 17, 22, 13, 18, 23},
    {24, 31, 38, 25, 32, 39, 26, 33, 40, 27, 34, 41, 28, 35, 42, 29, 36, 43, 30, 37, 44}};
}  // namespace

KSPLATLoader::Compression KSPLATLoader::compression(int level) {
    Compression result;
    // Level zero stores everything as floats, so a coordinate's range is one unit and there is
    // no implied scaling.
    if (level == 0) {
        result.bytesPerCenter = 12;
        result.bytesPerScale = 12;
        result.bytesPerRotation = 16;
        result.bytesPerColor = 4;
        result.bytesPerHarmonicComponent = 4;
        result.scaleOffset = 12;
        result.rotationOffset = 24;
        result.colorOffset = 40;
        result.scaleRange = 1;
        return result;
    }
    // Both compressed levels use six byte centres, six byte scales and eight byte rotations,
    // differing only in how wide a harmonic component is.
    if (level == 1 || level == 2) {
        result.bytesPerCenter = 6;
        result.bytesPerScale = 6;
        result.bytesPerRotation = 8;
        result.bytesPerColor = 4;
        result.bytesPerHarmonicComponent = level == 1 ? 2 : 1;
        result.scaleOffset = 6;
        result.rotationOffset = 12;
        result.colorOffset = 20;
        result.scaleRange = 32767;
    }
    return result;
}

int KSPLATLoader::harmonicIndex(int degree, int component) {
    if (degree < 1 || degree > 3) return -1;
    const int count = kDegreeComponents[degree];
    if (component < 0 || component >= count) return -1;
    return kBandIndex[degree - 1][component];
}

int KSPLATLoader::splatStride(int level, int degree) {
    const Compression layout = compression(level);
    if (layout.bytesPerCenter == 0 || degree < 0 || degree > 3) return 0;
    // The four fixed fields, then every harmonic component the degree asks for.
    return layout.bytesPerCenter + layout.bytesPerScale + layout.bytesPerRotation +
           layout.bytesPerColor +
           kDegreeComponents[degree] * layout.bytesPerHarmonicComponent;
}
