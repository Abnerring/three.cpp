#include "loaders/ThreeMFLoader.h"

#include "catch.hpp"

#include <string>

TEST_CASE("ThreeMFLoader reads the resource kinds in order") {
    // Four kinds, each collected into its own map.
    const auto& kinds = ThreeMFLoader::resourceKinds();
    REQUIRE(kinds.size() == 4);
    REQUIRE(kinds[0] == "basematerials");
    REQUIRE(kinds[1] == "texture2d");
    REQUIRE(kinds[2] == "colorgroup");
    REQUIRE(kinds[3] == "object");
    // The names are the element names, all lower case, with no capitals inside.
    for (const auto& kind : kinds) {
        REQUIRE(kind.find_first_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ") == std::string::npos);
    }
}

TEST_CASE("ThreeMFLoader keeps an object's type unchecked") {
    // The type is taken as written, even one the loader does not know.
    REQUIRE(ThreeMFLoader::objectTypeOf("model") == "model");
    REQUIRE(ThreeMFLoader::objectTypeOf("support") == "support");
    REQUIRE(ThreeMFLoader::objectTypeOf("nonsense") == "nonsense");
    // Case is not folded, so a capitalised type is a different string.
    REQUIRE(ThreeMFLoader::objectTypeOf("Model") == "Model");
    REQUIRE(ThreeMFLoader::objectTypeOf("Model") != ThreeMFLoader::objectTypeOf("model"));
    // An absent type gives the empty string rather than a fallback.
    REQUIRE(ThreeMFLoader::objectTypeOf("").empty());
}

TEST_CASE("ThreeMFLoader builds only the three object types it knows") {
    // The three the loader builds.
    REQUIRE(ThreeMFLoader::buildsObjectType("model"));
    REQUIRE(ThreeMFLoader::buildsObjectType("solidsupport"));
    REQUIRE(ThreeMFLoader::buildsObjectType("support"));
    // Anything else is not built, even though its type survives.
    REQUIRE_FALSE(ThreeMFLoader::buildsObjectType("other"));
    REQUIRE_FALSE(ThreeMFLoader::buildsObjectType("Model"));
    REQUIRE_FALSE(ThreeMFLoader::buildsObjectType(""));
    // So a type can be kept and yet build nothing, which is the distinction worth recording.
    REQUIRE(ThreeMFLoader::objectTypeOf("other") == "other");
    REQUIRE_FALSE(ThreeMFLoader::buildsObjectType(ThreeMFLoader::objectTypeOf("other")));
}

TEST_CASE("ThreeMFLoader spells the two type attributes differently") {
    // A relationship's type is read from an attribute with a capital T, an object's in lower
    // case. The two are not interchangeable.
    REQUIRE(std::string(ThreeMFLoader::kRelationshipTypeAttribute) == "Type");
    REQUIRE(std::string(ThreeMFLoader::kObjectTypeAttribute) == "type");
    REQUIRE(std::string(ThreeMFLoader::kRelationshipTypeAttribute) !=
            std::string(ThreeMFLoader::kObjectTypeAttribute));
}

TEST_CASE("ThreeMFLoader finds the model relationship whatever its case") {
    // The extension is lowered first, so capitals are found.
    REQUIRE(ThreeMFLoader::isModelRelationship("model"));
    REQUIRE(ThreeMFLoader::isModelRelationship("Model"));
    REQUIRE(ThreeMFLoader::isModelRelationship("MODEL"));
    // Anything that is not the model is not it.
    REQUIRE_FALSE(ThreeMFLoader::isModelRelationship("thumbnail"));
    REQUIRE_FALSE(ThreeMFLoader::isModelRelationship(""));
    // Note the contrast: here case does not matter, while an object's type keeps its case.
    REQUIRE_FALSE(ThreeMFLoader::buildsObjectType("Model"));
}