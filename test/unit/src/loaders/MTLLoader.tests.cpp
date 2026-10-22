#include "loaders/MTLLoader.h"

#include "catch.hpp"

#include <string>

namespace {
const char* kMtl =
    "# a comment\n"
    "newmtl Red\n"
    "Ka 0.1 0.2 0.3\n"
    "Kd 0.5 0.6 0.7\n"
    "Ks 0 0 0\n"
    "Ke 1 1 1\n"
    "Ns 250.0\n"
    "map_Kd texture.png\n"
    "\n"
    "newmtl Blue\n"
    "Kd 0 0 1\n";
}

TEST_CASE("MTLLoader splits the text into one record per material") {
    MTLLoader loader;
    const auto materials = loader.parse(kMtl);
    REQUIRE(materials.size() == 2);
    REQUIRE(materials.count("Red") == 1);
    REQUIRE(materials.count("Blue") == 1);
    // The name line is stored alongside the properties.
    REQUIRE(std::get<std::string>(materials.at("Red").at("name")) == "Red");
}

TEST_CASE("MTLLoader reads the four colour keys as three numbers") {
    MTLLoader loader;
    const auto materials = loader.parse(kMtl);
    const auto& red = materials.at("Red");
    const auto ka = std::get<MTLLoader::Color>(red.at("ka"));
    REQUIRE(ka[0] == Approx(0.1).epsilon(1e-9));
    REQUIRE(ka[1] == Approx(0.2).epsilon(1e-9));
    REQUIRE(ka[2] == Approx(0.3).epsilon(1e-9));
    const auto kd = std::get<MTLLoader::Color>(red.at("kd"));
    REQUIRE(kd[0] == Approx(0.5).epsilon(1e-9));
    const auto ks = std::get<MTLLoader::Color>(red.at("ks"));
    REQUIRE(ks[0] == 0.0);
    const auto ke = std::get<MTLLoader::Color>(red.at("ke"));
    REQUIRE(ke[2] == Approx(1.0).epsilon(1e-9));

    const auto blueKd = std::get<MTLLoader::Color>(materials.at("Blue").at("kd"));
    REQUIRE(blueKd[0] == 0.0);
    REQUIRE(blueKd[2] == Approx(1.0).epsilon(1e-9));
}

TEST_CASE("MTLLoader keeps every other key as its trimmed string") {
    MTLLoader loader;
    const auto materials = loader.parse(kMtl);
    const auto& red = materials.at("Red");
    REQUIRE(std::get<std::string>(red.at("ns")) == "250.0");
    REQUIRE(std::get<std::string>(red.at("map_kd")) == "texture.png");
}

TEST_CASE("MTLLoader lowercases keys and ignores blank lines and comments") {
    MTLLoader loader;
    const auto materials = loader.parse("NEWMTL Body\nMAP_KD  some/path.png  \n");
    REQUIRE(materials.size() == 1);
    const auto& body = materials.at("Body");
    REQUIRE(std::get<std::string>(body.at("map_kd")) == "some/path.png");
    // The value is trimmed on both sides.
    REQUIRE(std::get<std::string>(body.at("name")) == "Body");
}
