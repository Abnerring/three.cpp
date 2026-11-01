#pragma once
#include <string>

// Port of the rules in examples/jsm/loaders/TTFLoader.js at the pinned revision. The loader turns
// a font file into the JSON a typeface is built from, and it depends on a library that is fetched
// at run time, which is the most noteworthy thing about it.
class TTFLoader {
  public:
    static constexpr const char* kTypeName = "TTFLoader";

    // The library the loader needs. It is not bundled: the source imports it from a content
    // delivery address, so a build that used this module would have to fetch it when the page
    // runs rather than when the code is built. A C++ port has to decide where it comes from, and
    // this port records the fact rather than hiding it.
    static constexpr const char* kDependencyUrl =
        "https://cdn.jsdelivr.net/npm/opentype.js@1.3.4/+esm";

    // Whether the dependency is resolved when the code is built, which it is not.
    static bool dependencyIsBundled();

    // Whether the font's commands are reversed. A font file stores its curves in one order and
    // the converter can be asked for the other, so the flag is a conversion option rather than a
    // detail of reading the file.
    static constexpr bool kReversedByDefault = false;

    // Whether a command order is stored in the file, which it is: the flag chooses between the
    // order found and the one turned round.
    static bool orderComesFromTheFile();

    // What happens when parsing fails. The error is handed to the caller's handler if there is
    // one and printed otherwise, and either way the manager is told the item failed; so the
    // manager hears about it in both branches while the caller may not.
    static bool reportsToManagerAlways(bool hasErrorHandler);

    // The kind of data the loader asks its file loader for, which is the raw bytes rather than
    // text.
    static constexpr const char* kResponseType = "arraybuffer";

    // Whether the loader can be told to keep the raw buffer, which it cannot.
    static bool keepsTheBuffer();
};
