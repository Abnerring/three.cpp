#pragma once
#include <string>

// Port of the small confirmed parts of examples/jsm/loaders/3MFLoader.js at the pinned revision.
// The loader is one long parse over an archive of documents, so what is covered here is what can
// be stated on its own.
class ThreeMFLoader {
  public:
    // A 3MF package is a zip archive whose model document declares the unit its numbers are in.
    // The attribute is optional and the format names the fallback.
    static constexpr const char* kDefaultUnit = "millimeter";

    // The space the colours in a 3MF document are written in. The loader names it once at the top
    // of the file rather than writing it out at each use.
    static constexpr const char* kColourSpace = "srgb";

    // The unit a document declares, or the fallback when the attribute says nothing.
    static std::string unitOf(const std::string& declared);

    // Whether the loader scales the geometry by that unit. It does not: the source reads the
    // attribute exactly once, stores it, and never consults it again, so a model authored in
    // inches arrives with the numbers its document wrote. The format expects the unit to be
    // honoured, so this is a difference from the specification rather than a subtlety of it.
    static bool unitIsApplied();
};
