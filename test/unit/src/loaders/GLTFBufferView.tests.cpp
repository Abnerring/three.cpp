#include "loaders/GLTFLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <string>
#include <vector>

TEST_CASE("GLTFLoader slices a buffer view") {
    const std::vector<std::uint8_t> buffer{1, 2, 3, 4, 5, 6};
    // An offset and a length pick the middle three.
    const auto middle = GLTFLoader::sliceBufferView(buffer, 2, 3);
    REQUIRE(middle.size() == 3);
    REQUIRE(middle[0] == 3);
    REQUIRE(middle[2] == 5);
    // An offset of zero with the whole length is the whole buffer.
    const auto all = GLTFLoader::sliceBufferView(buffer, 0, 6);
    REQUIRE(all.size() == 6);
    REQUIRE(all[0] == 1);
    REQUIRE(all[5] == 6);
}

TEST_CASE("GLTFLoader cuts a buffer view at the end of the buffer") {
    const std::vector<std::uint8_t> buffer{1, 2, 3, 4};
    // A length past the end is cut rather than refused.
    const auto cut = GLTFLoader::sliceBufferView(buffer, 2, 10);
    REQUIRE(cut.size() == 2);
    REQUIRE(cut[0] == 3);
    REQUIRE(cut[1] == 4);
}

TEST_CASE("GLTFLoader reads nothing from an empty view") {
    const std::vector<std::uint8_t> buffer{1, 2, 3, 4};
    // A length of zero covers nothing.
    REQUIRE(GLTFLoader::sliceBufferView(buffer, 0, 0).empty());
    // An offset at or past the end covers nothing either.
    REQUIRE(GLTFLoader::sliceBufferView(buffer, 4, 2).empty());
    REQUIRE(GLTFLoader::sliceBufferView(buffer, 99, 2).empty());
    // An empty buffer gives nothing whatever the view says.
    REQUIRE(GLTFLoader::sliceBufferView({}, 0, 4).empty());
}

TEST_CASE("GLTFLoader counts the components of an accessor") {
    // Each element holds as many components as its shape.
    REQUIRE(GLTFLoader::accessorComponentCount(3, "VEC3") == 9);
    REQUIRE(GLTFLoader::accessorComponentCount(1, "SCALAR") == 1);
    REQUIRE(GLTFLoader::accessorComponentCount(2, "MAT4") == 32);
    REQUIRE(GLTFLoader::accessorComponentCount(0, "VEC4") == 0);
    // An unknown shape is refused.
    REQUIRE_THROWS_AS(GLTFLoader::accessorComponentCount(1, "VEC5"), std::runtime_error);
}