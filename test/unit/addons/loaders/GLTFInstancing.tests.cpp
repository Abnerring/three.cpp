#include "loaders/GLTFLoader.h"

#include "catch.hpp"

TEST_CASE("GLTFLoader knows the seven primitive modes") {
    // The seven the format defines, in order from zero.
    REQUIRE(GLTFLoader::kPoints == 0);
    REQUIRE(GLTFLoader::kLines == 1);
    REQUIRE(GLTFLoader::kLineLoop == 2);
    REQUIRE(GLTFLoader::kLineStrip == 3);
    REQUIRE(GLTFLoader::kTriangles == 4);
    REQUIRE(GLTFLoader::kTriangleStrip == 5);
    REQUIRE(GLTFLoader::kTriangleFan == 6);
    // The default is triangles.
    REQUIRE(GLTFLoader::defaultPrimitiveMode() == GLTFLoader::kTriangles);
}

TEST_CASE("GLTFLoader instances the three triangle modes") {
    // Triangles, strips and fans all build triangles.
    REQUIRE(GLTFLoader::supportsInstancing(true, GLTFLoader::kTriangles));
    REQUIRE(GLTFLoader::supportsInstancing(true, GLTFLoader::kTriangleStrip));
    REQUIRE(GLTFLoader::supportsInstancing(true, GLTFLoader::kTriangleFan));
}

TEST_CASE("GLTFLoader refuses to instance points and lines") {
    // The four modes that do not build triangles are all refused.
    REQUIRE_FALSE(GLTFLoader::supportsInstancing(true, GLTFLoader::kPoints));
    REQUIRE_FALSE(GLTFLoader::supportsInstancing(true, GLTFLoader::kLines));
    REQUIRE_FALSE(GLTFLoader::supportsInstancing(true, GLTFLoader::kLineLoop));
    REQUIRE_FALSE(GLTFLoader::supportsInstancing(true, GLTFLoader::kLineStrip));
}

TEST_CASE("GLTFLoader instances a primitive with no mode written") {
    // A missing mode is the default, which is a triangle mode, so the answer is yes.
    REQUIRE(GLTFLoader::supportsInstancing(false, GLTFLoader::kPoints));
    REQUIRE(GLTFLoader::supportsInstancing(false, 0));
}

TEST_CASE("GLTFLoader needs an attribute to instance with") {
    // One attribute is enough to say where the copies go.
    REQUIRE(GLTFLoader::hasInstanceAttributes(1));
    REQUIRE(GLTFLoader::hasInstanceAttributes(3));
    // None is not.
    REQUIRE_FALSE(GLTFLoader::hasInstanceAttributes(0));
}