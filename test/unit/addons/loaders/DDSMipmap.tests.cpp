#include "loaders/DDSLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

namespace {
DDSLoader::Header sized(std::uint32_t width, std::uint32_t height) {
    DDSLoader::Header header;
    header.width = width;
    header.height = height;
    return header;
}
}  // namespace

TEST_CASE("DDSLoader works out the mipmap chain") {
    // A two hundred and fifty six by one hundred and twenty eight texture with five levels.
    const auto chain =
        DDSLoader::mipmapChain(sized(256, 128), 5, 128, 8, 0);
    REQUIRE(chain.size() == 5);
    // Each level halves both dimensions.
    REQUIRE(chain[0].width == 256);
    REQUIRE(chain[1].width == 128);
    REQUIRE(chain[1].height == 64);
    REQUIRE(chain[4].width == 16);
    REQUIRE(chain[4].height == 8);
    // A block covers four by four, so a level is its tile count times the block size.
    REQUIRE(chain[0].length == 64 * 32 * 8);
    REQUIRE(chain[1].length == 32 * 16 * 8);
    REQUIRE(chain[4].length == 4 * 2 * 8);
    // The offsets run on from the first.
    REQUIRE(chain[0].offset == 128);
    REQUIRE(chain[1].offset == 128 + 16384);
}

TEST_CASE("DDSLoader never lets a level reach zero") {
    // A two by two texture with three levels ends at one by one.
    const auto chain = DDSLoader::mipmapChain(sized(2, 2), 3, 0, 8, 0);
    REQUIRE(chain.size() == 3);
    REQUIRE(chain[1].width == 1);
    REQUIRE(chain[2].width == 1);
    REQUIRE(chain[2].height == 1);
    // A single tile is one block.
    REQUIRE(chain[2].length == 8);
}

TEST_CASE("DDSLoader sizes an uncompressed level by the pixel") {
    // No block size means the level is measured in pixels, three bytes each here.
    const auto chain = DDSLoader::mipmapChain(sized(4, 4), 2, 0, 0, 3);
    REQUIRE(chain[0].length == 4 * 4 * 3);
    // The second level is half the size in each direction.
    REQUIRE(chain[1].length == 2 * 2 * 3);
}

TEST_CASE("DDSLoader returns nothing for an empty chain") {
    REQUIRE(DDSLoader::mipmapChain(sized(4, 4), 0, 0, 8, 0).empty());
    REQUIRE(DDSLoader::mipmapChain(sized(0, 4), 2, 0, 8, 0).empty());
}
