#include "loaders/KTX2Loader.h"

std::string KTX2Loader::colorSpaceFor(const std::string& primaries) {
    // The ordinary gamut takes the sRGB space and the wide one its own.
    if (primaries == kPrimariesBt709) return "SRGBColorSpace";
    if (primaries == kPrimariesDisplayP3) return "DisplayP3ColorSpace";
    // Unspecified, and anything else, claims nothing rather than guessing.
    return "NoColorSpace";
}

bool KTX2Loader::isReadableSupercompression(const std::string& scheme) {
    // Stored as they are, or wrapped in a stream the loader can unwrap.
    return scheme == kSupercompressionNone || scheme == kSupercompressionZstd;
}

int KTX2Loader::activeLoaderCount(bool created, bool disposed) {
    // The count lives outside the class, so it is shared by every loader rather than per
    // instance; one creation raises it and one disposal lowers it.
    int count = 0;
    if (created) count += 1;
    if (disposed) count -= 1;
    return count;
}
