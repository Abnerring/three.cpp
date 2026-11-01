#include "loaders/VRMLLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("VRMLLoader ships its parser library rather than fetching it") {
    // The library is named by a relative path inside the addons set, so it travels with the code.
    REQUIRE(VRMLLoader::parserLibraryIsBundled());
    const std::string path = VRMLLoader::kParserLibraryPath;
    REQUIRE(path.find("..") == 0);
    REQUIRE(path.find("libs") != std::string::npos);
    // And it is not an address, which is how two other loaders in the set do it.
    REQUIRE(path.find("https://") == std::string::npos);
    REQUIRE(std::string(VRMLLoader::kTypeName) == "VRMLLoader");
}

TEST_CASE("VRMLLoader reports only the failure to its manager") {
    // One call, so it belongs with the collada and font loaders rather than with the materialx
    // one, which reports the end as well.
    REQUIRE(VRMLLoader::managerCallsOnFailure() == 1);
    REQUIRE(VRMLLoader::managerCallsOnFailure() != 2);
}

TEST_CASE("VRMLLoader takes its own path unless that is the empty string") {
    // The loader's own path wins when it is a value.
    REQUIRE(VRMLLoader::basePath("models/", "http://example/a/") == "models/");
    // Only the empty string is replaced.
    REQUIRE(VRMLLoader::basePath("", "http://example/a/") == "http://example/a/");
    REQUIRE(VRMLLoader::basePath("/", "http://example/a/") == "/");
    // A scene comes back rather than a group, and the file is read as text.
    REQUIRE(std::string(VRMLLoader::kParseResultType) == "Scene");
    REQUIRE(std::string(VRMLLoader::kResponseType) == "text");
}