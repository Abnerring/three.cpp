#pragma once
#include <string>
#include <vector>

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
    // The eight metadata names the format defines. A metadata element with any other name is
    // skipped rather than kept, so an extension's own metadata never reaches the model.
    static const std::vector<std::string>& metadataNames();

    // Whether a name is one of them. The comparison is exact and case sensitive.
    static bool isMetadataName(const std::string& name);

    // A build item's object id is kept as the string the document wrote, not turned into a
    // number, because the format lets it be any identifier and the loader only uses it to look
    // the object up again.
    static std::string objectIdOf(const std::string& written);

    // Whether a build item carries a transform. An attribute that is present but empty reads as
    // the empty string, which does not count, so a document writing transform="" gets no
    // transform rather than an identity one.
    static bool carriesTransform(const std::string& transform);
};
