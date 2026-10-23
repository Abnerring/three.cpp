#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <string>
#include <utility>
#include <vector>

TEST_CASE("EXRLoader names the thirteen attribute kinds") {
    const std::vector<std::pair<std::string, EXRLoader::AttributeKind>> table{
        {"chlist", EXRLoader::AttributeKind::ChannelList},
        {"chromaticities", EXRLoader::AttributeKind::Chromaticities},
        {"compression", EXRLoader::AttributeKind::Compression},
        {"box2i", EXRLoader::AttributeKind::Box},
        {"envmap", EXRLoader::AttributeKind::EnvironmentMap},
        {"tiledesc", EXRLoader::AttributeKind::TileDescription},
        {"lineOrder", EXRLoader::AttributeKind::LineOrder},
        {"float", EXRLoader::AttributeKind::Float},
        {"v2f", EXRLoader::AttributeKind::Vector2},
        {"v3f", EXRLoader::AttributeKind::Vector3},
        {"int", EXRLoader::AttributeKind::Integer},
        {"rational", EXRLoader::AttributeKind::Rational},
    };
    for (const auto& row : table) {
        REQUIRE(EXRLoader::attributeKind(row.first) == row.second);
        REQUIRE(EXRLoader::attributeKindKnown(row.second));
    }
    // Thirteen kinds in all, counting the shared string one.
    REQUIRE(table.size() == 12);
}

TEST_CASE("EXRLoader groups the three string types") {
    // A fixed length string, a list of them and a profile are one kind on disk.
    REQUIRE(EXRLoader::attributeKind("string") == EXRLoader::AttributeKind::FixedString);
    REQUIRE(EXRLoader::attributeKind("stringvector") == EXRLoader::AttributeKind::FixedString);
    REQUIRE(EXRLoader::attributeKind("iccProfile") == EXRLoader::AttributeKind::FixedString);
    // Each is known.
    REQUIRE(EXRLoader::attributeKindKnown(EXRLoader::attributeKind("iccProfile")));
}

TEST_CASE("EXRLoader refuses an unknown type") {
    // Anything not in the table is unknown and therefore not known.
    REQUIRE(EXRLoader::attributeKind("box2f") == EXRLoader::AttributeKind::Unknown);
    REQUIRE(EXRLoader::attributeKind("") == EXRLoader::AttributeKind::Unknown);
    REQUIRE_FALSE(EXRLoader::attributeKindKnown(EXRLoader::attributeKind("box2f")));
    // The comparison is exact, so the case matters.
    REQUIRE(EXRLoader::attributeKind("FLOAT") == EXRLoader::AttributeKind::Unknown);
    REQUIRE(EXRLoader::attributeKind("chlist ") == EXRLoader::AttributeKind::Unknown);
}