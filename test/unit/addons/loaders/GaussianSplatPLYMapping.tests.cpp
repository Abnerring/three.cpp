#include "loaders/GaussianSplatPLYLoader.h"

#include "catch.hpp"

TEST_CASE("GaussianSplatPLYLoader maps the fixed fields") {
    const auto mapping = GaussianSplatPLYLoader::propertyMapping(0);
    REQUIRE(mapping.at("scale").size() == 3);
    REQUIRE(mapping.at("scale")[0] == "scale_0");
    REQUIRE(mapping.at("scale")[2] == "scale_2");
    REQUIRE(mapping.at("rotation").size() == 4);
    REQUIRE(mapping.at("rotation")[3] == "rot_3");
    REQUIRE(mapping.at("f_dc").size() == 3);
    REQUIRE(mapping.at("opacity").size() == 1);
    REQUIRE(mapping.at("opacity")[0] == "opacity");
}

TEST_CASE("GaussianSplatPLYLoader grows the rest band with the degree") {
    // The counts match the header scan table.
    REQUIRE(GaussianSplatPLYLoader::propertyMapping(0).at("f_rest").empty());
    REQUIRE(GaussianSplatPLYLoader::propertyMapping(1).at("f_rest").size() == 9);
    REQUIRE(GaussianSplatPLYLoader::propertyMapping(2).at("f_rest").size() == 24);
    REQUIRE(GaussianSplatPLYLoader::propertyMapping(3).at("f_rest").size() == 45);
}

TEST_CASE("GaussianSplatPLYLoader names the rest properties in order") {
    const auto rest = GaussianSplatPLYLoader::propertyMapping(2).at("f_rest");
    REQUIRE(rest.front() == "f_rest_0");
    REQUIRE(rest[1] == "f_rest_1");
    // Twenty four names run from zero to twenty three.
    REQUIRE(rest.back() == "f_rest_23");
}

TEST_CASE("GaussianSplatPLYLoader treats an unknown degree as no rest band") {
    // A degree outside the table lists nothing rather than guessing.
    REQUIRE(GaussianSplatPLYLoader::propertyMapping(9).at("f_rest").empty());
    REQUIRE(GaussianSplatPLYLoader::propertyMapping(-1).at("f_rest").empty());
}