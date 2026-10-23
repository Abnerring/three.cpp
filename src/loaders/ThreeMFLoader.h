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
};
