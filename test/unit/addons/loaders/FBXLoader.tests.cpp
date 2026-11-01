#include "loaders/FBXLoader.h"

#include "catch.hpp"

#include <string>

using Encoding = FBXLoader::Encoding;

TEST_CASE("FBXLoader needs a different version for each encoding") {
    // The numbers are on different scales, so each is compared against its own.
    REQUIRE(FBXLoader::versionIsSupported(Encoding::Ascii, 7.0));
    REQUIRE(FBXLoader::versionIsSupported(Encoding::Ascii, 7.4));
    REQUIRE_FALSE(FBXLoader::versionIsSupported(Encoding::Ascii, 6.9));
    REQUIRE(FBXLoader::versionIsSupported(Encoding::Binary, 6400));
    REQUIRE(FBXLoader::versionIsSupported(Encoding::Binary, 7500));
    REQUIRE_FALSE(FBXLoader::versionIsSupported(Encoding::Binary, 6399));
    // And comparing a binary version against the text threshold is meaningless, which the two
    // numbers make plain: seven is far below six thousand four hundred.
    REQUIRE(FBXLoader::kMinimumAsciiVersion < FBXLoader::kMinimumBinaryVersion);
    REQUIRE(std::string(FBXLoader::kTypeName) == "FBXLoader");
}

TEST_CASE("FBXLoader's boundary is included in both encodings") {
    // Exactly the stated number is supported, in each case.
    REQUIRE(FBXLoader::versionIsSupported(Encoding::Ascii, FBXLoader::kMinimumAsciiVersion));
    REQUIRE(FBXLoader::versionIsSupported(Encoding::Binary,
                                          static_cast<double>(FBXLoader::kMinimumBinaryVersion)));
    // One step below is not, so the boundary really is the boundary.
    REQUIRE_FALSE(FBXLoader::versionIsSupported(Encoding::Ascii, 6.99));
    REQUIRE_FALSE(FBXLoader::versionIsSupported(Encoding::Binary, 6399.9));
}

TEST_CASE("FBXLoader keeps its parse state outside the class") {
    // Three names hold the tree, the connections and the graph, written before the walk and read
    // during it, so a second load begun during a first would overwrite the first one's state.
    REQUIRE(FBXLoader::moduleLevelStateNames() == 3);
    REQUIRE_FALSE(FBXLoader::isReentrant());
    // A port has to make that state local to the call rather than repeat it as it stands, which
    // is the one place in this set where copying the source exactly would be wrong.
    REQUIRE(FBXLoader::moduleLevelStateNames() > 0);
}

TEST_CASE("FBXLoader trims clips only when asked and ships its compression library") {
    // The trimming is off by default, so a file naming several clips on one timeline keeps them
    // there unless the caller says otherwise.
    REQUIRE_FALSE(FBXLoader::kTrimAnimationClipsByDefault);
    // The compression library is named by a relative path, so it travels with the code.
    REQUIRE(FBXLoader::compressionLibraryIsBundled());
    // The bytes are read rather than text, since the encoding is not known until they arrive.
    REQUIRE(std::string(FBXLoader::kResponseType) == "arraybuffer");
}