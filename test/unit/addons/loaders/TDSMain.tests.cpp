#include "loaders/TDSLoader.h"

#include "catch.hpp"

#include <cstdint>

TEST_CASE("TDSLoader accepts three magic numbers") {
    // The three ways a 3DS file can begin.
    REQUIRE(TDSLoader::acceptsMagic(0x4D4D));
    REQUIRE(TDSLoader::acceptsMagic(0x3DAA));
    REQUIRE(TDSLoader::acceptsMagic(0xC23D));
    // A file that begins with anything else is not one.
    REQUIRE_FALSE(TDSLoader::acceptsMagic(0x0000));
    REQUIRE_FALSE(TDSLoader::acceptsMagic(0x4141));
    REQUIRE_FALSE(TDSLoader::acceptsMagic(0x4D4E));
}

TEST_CASE("TDSLoader sorts the main chunks") {
    // The version chunk carries the format number.
    REQUIRE(TDSLoader::mainChunkKind(0x0002) == TDSLoader::MainChunk::Version);
    // The mesh data chunk opens everything the loader builds.
    REQUIRE(TDSLoader::mainChunkKind(0x3D3D) == TDSLoader::MainChunk::MeshData);
    // Anything else is skipped quietly.
    REQUIRE(TDSLoader::mainChunkKind(0x0100) == TDSLoader::MainChunk::Unknown);
    REQUIRE(TDSLoader::mainChunkKind(0x4D4D) == TDSLoader::MainChunk::Unknown);
}

TEST_CASE("TDSLoader tells the magic apart from the version") {
    // The version chunk id is not a magic number even though both are small words.
    REQUIRE_FALSE(TDSLoader::acceptsMagic(0x0002));
    // And the magic number that opens the file is not a version chunk.
    REQUIRE(TDSLoader::mainChunkKind(0x4D4D) != TDSLoader::MainChunk::Version);
}
