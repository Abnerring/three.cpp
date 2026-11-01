#include "loaders/TDSLoader.h"

#include "catch.hpp"

#include <cstdint>

TEST_CASE("TDSLoader sorts the mesh data chunks") {
    // The four that mean something.
    REQUIRE(TDSLoader::meshDataKind(0x3D3E) == TDSLoader::MeshDataChunk::Version);
    REQUIRE(TDSLoader::meshDataKind(0x0100) == TDSLoader::MeshDataChunk::MasterScale);
    REQUIRE(TDSLoader::meshDataKind(0x4000) == TDSLoader::MeshDataChunk::NamedObject);
    REQUIRE(TDSLoader::meshDataKind(0xAFFF) == TDSLoader::MeshDataChunk::MaterialEntry);
    // Anything else is skipped.
    REQUIRE(TDSLoader::meshDataKind(0x0000) == TDSLoader::MeshDataChunk::Unknown);
    REQUIRE(TDSLoader::meshDataKind(0x4110) == TDSLoader::MeshDataChunk::Unknown);
}

TEST_CASE("TDSLoader sorts the named object chunks") {
    // A triangle object carries the geometry.
    REQUIRE(TDSLoader::namedObjectKind(0x4100) == TDSLoader::NamedObjectChunk::TriangleObject);
    // A point array on its own is inside one, not beside it, so it is not recognised here.
    REQUIRE(TDSLoader::namedObjectKind(0x4110) == TDSLoader::NamedObjectChunk::Unknown);
    REQUIRE(TDSLoader::namedObjectKind(0x0000) == TDSLoader::NamedObjectChunk::Unknown);
}

TEST_CASE("TDSLoader applies the master scale to every axis") {
    // The single value reaches all three axes.
    const auto scaled = TDSLoader::masterScale(2.5);
    REQUIRE(scaled[0] == 2.5);
    REQUIRE(scaled[1] == 2.5);
    REQUIRE(scaled[2] == 2.5);
    // A scale of one leaves the group as it is.
    const auto unchanged = TDSLoader::masterScale(1.0);
    REQUIRE(unchanged[0] == 1.0);
    REQUIRE(unchanged[2] == 1.0);
    // A scale of zero collapses it.
    const auto collapsed = TDSLoader::masterScale(0.0);
    REQUIRE(collapsed[0] == 0.0);
    REQUIRE(collapsed[1] == 0.0);
}

TEST_CASE("TDSLoader keeps the master scale apart from the main chunks") {
    // The master scale identifier is not one of the three magic numbers that open a file, even
    // though it is also a small word.
    REQUIRE_FALSE(TDSLoader::acceptsMagic(0x0100));
    // Nor is it a main chunk.
    REQUIRE(TDSLoader::mainChunkKind(0x0100) == TDSLoader::MainChunk::Unknown);
    // But it is a mesh data chunk.
    REQUIRE(TDSLoader::meshDataKind(0x0100) == TDSLoader::MeshDataChunk::MasterScale);
}
