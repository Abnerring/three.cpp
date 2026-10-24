#pragma once
#include <string>

// Port of the rules in examples/jsm/loaders/FBXLoader.js at the pinned revision. The loader reads a
// model format in two quite different encodings, and three of its rules are worth writing down.
class FBXLoader {
  public:
    static constexpr const char* kTypeName = "FBXLoader";

    // The two encodings the format comes in, and the version each must reach. The numbers are on
    // different scales, so comparing one against the other's threshold is meaningless: a text
    // file must claim seven or more and a binary one six thousand four hundred or more, and a
    // file of either kind below its own number is old enough to be untrustworthy rather than
    // refused outright.
    static constexpr double kMinimumAsciiVersion = 7.0;
    static constexpr int kMinimumBinaryVersion = 6400;

    enum class Encoding { Ascii, Binary };

    // Whether a version meets the requirement for its encoding, each against its own number.
    static bool versionIsSupported(Encoding encoding, double version);

    // Where the parse keeps its working state: three names outside the class, written before the
    // tree is walked and read while it is. So a second load begun while a first is still parsing
    // would overwrite the first one's state, and the parse is not re-entrant; a port has to make
    // that state local to the call rather than repeated as it stands.
    static int moduleLevelStateNames();

    // Whether the parse would be safe to run twice at once, which it would not.
    static bool isReentrant();

    // The one setting a caller can make, which decides whether clips are trimmed to the range
    // their animation stacks declare and shifted to start at zero. It is off by default, so a
    // file naming several clips on one timeline keeps them on that timeline unless asked.
    static constexpr bool kTrimAnimationClipsByDefault = false;

    // Whether a compression library is bundled, which it is: the loader names a relative path for
    // it rather than an address, so the one thing it needs beyond the core travels with the code.
    static bool compressionLibraryIsBundled();

    // The kind of data read from the file, which is bytes rather than text, because the encoding
    // is not known until the bytes have been looked at.
    static constexpr const char* kResponseType = "arraybuffer";
};
