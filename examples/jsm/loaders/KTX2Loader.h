#pragma once
#include <string>

// Port of the rules in examples/jsm/loaders/KTX2Loader.js at the pinned revision. The loader reads
// a texture container whose header names the primaries its colour was authored in, and the
// mapping from those primaries to a colour space is the part worth writing down.
class KTX2Loader {
  public:
    static constexpr const char* kTypeName = "KTX2Loader";

    // The primaries a container may name, and the colour space each implies. The first is the
    // ordinary one and the second is the wide gamut a display may offer; the third says the
    // file does not know, and the loader then claims no colour space at all rather than guessing
    // one, so a texture authored without the information is not silently treated as ordinary.
    static constexpr const char* kPrimariesBt709 = "BT709";
    static constexpr const char* kPrimariesDisplayP3 = "DISPLAY_P3";
    static constexpr const char* kPrimariesUnspecified = "UNSPECIFIED";

    static std::string colorSpaceFor(const std::string& primaries);

    // The transfer function the container may name, which decides whether the values are already
    // in an encoding that a colour space expects. The one the loader understands is the sRGB
    // curve; a file naming anything else falls back to an unspecified space too.
    static constexpr const char* kTransferSrgb = "SRGB";

    // The two supercompression schemes the loader handles. One means the blocks are stored as
    // they are, and the other means they are wrapped in a stream that has to be unwrapped first;
    // a container naming a third is refused rather than read as if it were one of these.
    static constexpr const char* kSupercompressionNone = "NONE";
    static constexpr const char* kSupercompressionZstd = "ZSTD";

    static bool isReadableSupercompression(const std::string& scheme);

    // How many loaders are alive. The count is kept outside the class, so it belongs to the
    // module rather than to any instance and is shared by every loader in the page; it is raised
    // when a loader is made and lowered when it is disposed.
    static int activeLoaderCount(bool created, bool disposed);

    // Whether the container's alpha is premultiplied, which the loader has to know to decide
    // whether the values need dividing before use.
    static constexpr const char* kFlagAlphaPremultiplied = "ALPHA_PREMULTIPLIED";
};
