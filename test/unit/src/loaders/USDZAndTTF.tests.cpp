#include "loaders/TTFLoader.h"
#include "loaders/USDZLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("USDZLoader is a deprecation shim and nothing else") {
    // It has no parsing of its own; every method comes from the loader it extends.
    REQUIRE_FALSE(USDZLoader::parsesAnythingItself());
    // The message names both classes, so a reader learns what to use instead.
    const std::string warning = USDZLoader::deprecationWarning();
    REQUIRE(warning.find("USDZLoader") != std::string::npos);
    REQUIRE(warning.find("USDLoader") != std::string::npos);
    REQUIRE(warning.find("deprecated") != std::string::npos);
    REQUIRE(std::string(USDZLoader::kTypeName) == "USDZLoader");
}

TEST_CASE("USDZLoader warns before it calls its parent") {
    // The message comes first, so a caller who used the old name hears about it even if the
    // parent's own initialisation then fails. That order is unusual in a constructor.
    REQUIRE(USDZLoader::warnsBeforeParentInitialises());
}

TEST_CASE("TTFLoader depends on a library it does not bundle") {
    // The address is named in the source, so the library is fetched when the module runs rather
    // than when the code is built.
    REQUIRE_FALSE(TTFLoader::dependencyIsBundled());
    const std::string url = TTFLoader::kDependencyUrl;
    REQUIRE(url.find("https://") == 0);
    REQUIRE(url.find("opentype") != std::string::npos);
    REQUIRE(std::string(TTFLoader::kTypeName) == "TTFLoader");
}

TEST_CASE("TTFLoader's flags and error path") {
    // The command order is in the file and the flag chooses between the order found and the one
    // turned round, so the flag is a conversion option.
    REQUIRE(TTFLoader::orderComesFromTheFile());
    REQUIRE_FALSE(TTFLoader::kReversedByDefault);
    // On failure the manager is told whichever branch was taken; only the caller's handler
    // depends on the caller having supplied one.
    REQUIRE(TTFLoader::reportsToManagerAlways(true));
    REQUIRE(TTFLoader::reportsToManagerAlways(false));
    // The bytes are asked for, not text, and the loader does not keep them.
    REQUIRE(std::string(TTFLoader::kResponseType) == "arraybuffer");
    REQUIRE_FALSE(TTFLoader::keepsTheBuffer());
}