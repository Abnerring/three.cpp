#include "loaders/ThreeMFLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ThreeMFLoader keeps only the eight metadata names") {
    // The format defines exactly eight, and they are listed in the source's own order.
    const auto& names = ThreeMFLoader::metadataNames();
    REQUIRE(names.size() == 8);
    REQUIRE(names[0] == "Title");
    REQUIRE(names[1] == "Designer");
    REQUIRE(names[2] == "Description");
    REQUIRE(names[3] == "Copyright");
    REQUIRE(names[4] == "LicenseTerms");
    REQUIRE(names[5] == "Rating");
    REQUIRE(names[6] == "CreationDate");
    REQUIRE(names[7] == "ModificationDate");
    // The two with capitals inside are camel case, not spaced or lower cased.
    REQUIRE(names[4] != "licenseterms");
    REQUIRE(names[7] != "Modification Date");
}

TEST_CASE("ThreeMFLoader accepts only those names") {
    // Every name on the list is accepted.
    for (const auto& name : ThreeMFLoader::metadataNames()) {
        REQUIRE(ThreeMFLoader::isMetadataName(name));
    }
    // Anything else is skipped, so an extension's own metadata never reaches the model.
    REQUIRE_FALSE(ThreeMFLoader::isMetadataName("Author"));
    REQUIRE_FALSE(ThreeMFLoader::isMetadataName("title"));
    REQUIRE_FALSE(ThreeMFLoader::isMetadataName("TITLE"));
    REQUIRE_FALSE(ThreeMFLoader::isMetadataName(""));
    REQUIRE_FALSE(ThreeMFLoader::isMetadataName("Title "));
}

TEST_CASE("ThreeMFLoader keeps a build item's id as written") {
    // The id is not turned into a number, so a document may use any identifier.
    REQUIRE(ThreeMFLoader::objectIdOf("1") == "1");
    REQUIRE(ThreeMFLoader::objectIdOf("42") == "42");
    // Leading zeros survive, which a conversion would drop.
    REQUIRE(ThreeMFLoader::objectIdOf("007") == "007");
    REQUIRE(ThreeMFLoader::objectIdOf("007") != ThreeMFLoader::objectIdOf("7"));
    // A non numeric id passes through too.
    REQUIRE(ThreeMFLoader::objectIdOf("object-1") == "object-1");
    REQUIRE(ThreeMFLoader::objectIdOf("").empty());
}

TEST_CASE("ThreeMFLoader tells an empty transform from a missing one") {
    // Both a missing attribute and an empty one give no transform, which is what testing the
    // string itself does.
    REQUIRE_FALSE(ThreeMFLoader::carriesTransform(""));
    // Any actual value counts, even one that is only spaces.
    REQUIRE(ThreeMFLoader::carriesTransform("1 0 0 0 1 0 0 0 1 0 0 0"));
    REQUIRE(ThreeMFLoader::carriesTransform(" "));
    // An identity transform is still a transform, so it is applied like any other.
    REQUIRE(ThreeMFLoader::carriesTransform("1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1"));
}