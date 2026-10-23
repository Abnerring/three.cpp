#include "nodes/core/CameraNames.h"

#include "catch.hpp"

#include <string>

TEST_CASE("CameraNames holds the names the module uses") {
    REQUIRE(std::string(CameraNames::kIndexName) == "u_cameraIndex");
    REQUIRE(std::string(CameraNames::kNearName) == "cameraNear");
    REQUIRE(std::string(CameraNames::kFarName) == "cameraFar");
    REQUIRE(std::string(CameraNames::kProjectionMatrixArrayName) == "cameraProjectionMatrices");
    REQUIRE(std::string(CameraNames::kIndexType) == "uint");
}

TEST_CASE("CameraNames' index is the only name with a prefix") {
    // One of the four carries the prefix and the others do not, which is the inconsistency the
    // source has rather than a rule it follows.
    REQUIRE(CameraNames::indexHasPrefix());
    REQUIRE(std::string(CameraNames::kNearName).rfind("u_", 0) != 0);
    REQUIRE(std::string(CameraNames::kFarName).rfind("u_", 0) != 0);
    REQUIRE(std::string(CameraNames::kProjectionMatrixArrayName).rfind("u_", 0) != 0);
    // Counting them shows it is one of four, not most of them.
    int prefixed = 0;
    const char* names[] = {CameraNames::kIndexName, CameraNames::kNearName, CameraNames::kFarName,
                           CameraNames::kProjectionMatrixArrayName};
    for (const char* name : names) {
        if (std::string(name).rfind("u_", 0) == 0) prefixed += 1;
    }
    REQUIRE(prefixed == 1);
}

TEST_CASE("CameraNames picks the array form only for a non empty array camera") {
    // Not an array camera: the single form, whatever the count says.
    REQUIRE_FALSE(CameraNames::usesMatrixArray(false, 0));
    REQUIRE_FALSE(CameraNames::usesMatrixArray(false, 3));
    // An array camera holding none falls back to the single form rather than to an empty array.
    REQUIRE_FALSE(CameraNames::usesMatrixArray(true, 0));
    // One sub camera is enough for the array form.
    REQUIRE(CameraNames::usesMatrixArray(true, 1));
    REQUIRE(CameraNames::usesMatrixArray(true, 10));
}