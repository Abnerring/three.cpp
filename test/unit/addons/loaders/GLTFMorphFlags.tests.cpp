#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <vector>

TEST_CASE("GLTFLoader turns a morph flag on for any one target") {
    // Nothing named anywhere leaves all three off.
    const auto none = GLTFLoader::morphFlags({false}, {false}, {false});
    REQUIRE_FALSE(none.position);
    REQUIRE_FALSE(none.normal);
    REQUIRE_FALSE(none.colour);
    // A single target naming one attribute is enough.
    const auto one = GLTFLoader::morphFlags({false, true}, {false, false}, {false, false});
    REQUIRE(one.position);
    REQUIRE_FALSE(one.normal);
    REQUIRE_FALSE(one.colour);
    // One attribute per target still turns all three on.
    const auto all = GLTFLoader::morphFlags({true, false}, {false, true}, {true, false});
    REQUIRE(all.position);
    REQUIRE(all.normal);
    REQUIRE(all.colour);
}

TEST_CASE("GLTFLoader leaves the geometry alone when nothing morphs") {
    // All three off means no work.
    REQUIRE_FALSE(GLTFLoader::morphsAnything({}));
    // Any one on means work.
    GLTFLoader::MorphFlags normal;
    normal.normal = true;
    REQUIRE(GLTFLoader::morphsAnything(normal));
    GLTFLoader::MorphFlags colour;
    colour.colour = true;
    REQUIRE(GLTFLoader::morphsAnything(colour));
}

TEST_CASE("GLTFLoader falls back to the mesh attribute") {
    // A flag that is off never reads the target.
    REQUIRE(GLTFLoader::morphSource(false, true) == GLTFLoader::MorphSource::Base);
    REQUIRE(GLTFLoader::morphSource(false, false) == GLTFLoader::MorphSource::Base);
    // A flag that is on reads the target when the target names the attribute.
    REQUIRE(GLTFLoader::morphSource(true, true) == GLTFLoader::MorphSource::Target);
    // And falls back to the mesh's own when it does not.
    REQUIRE(GLTFLoader::morphSource(true, false) == GLTFLoader::MorphSource::Base);
}