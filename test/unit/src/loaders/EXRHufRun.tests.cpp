#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>

TEST_CASE("EXRLoader recognises the run marker") {
    // The marker is whatever the block says it is, compared like any other symbol.
    REQUIRE(EXRLoader::hufIsRunMarker(42, 42));
    REQUIRE_FALSE(EXRLoader::hufIsRunMarker(41, 42));
    REQUIRE_FALSE(EXRLoader::hufIsRunMarker(0, 0) == false);
    // Nought can be a marker, so a run of a zero byte is expressible.
    REQUIRE(EXRLoader::hufIsRunMarker(0, 0));
}

TEST_CASE("EXRLoader keeps a run's count to one byte") {
    // A count inside a byte passes through.
    REQUIRE(EXRLoader::hufRunLength(0) == 0);
    REQUIRE(EXRLoader::hufRunLength(1) == 1);
    REQUIRE(EXRLoader::hufRunLength(255) == 255);
    // A larger one is truncated rather than refused, which is what storing it in a byte does.
    REQUIRE(EXRLoader::hufRunLength(256) == 0);
    REQUIRE(EXRLoader::hufRunLength(257) == 1);
    REQUIRE(EXRLoader::hufRunLength(0x1FF) == 255);
}

TEST_CASE("EXRLoader reports a run that does not fit") {
    // Exactly filling the room is allowed.
    REQUIRE(EXRLoader::hufRoomFor(0, 4, 4));
    REQUIRE(EXRLoader::hufRoomFor(6, 4, 10));
    // One more than the room is not.
    REQUIRE_FALSE(EXRLoader::hufRoomFor(0, 5, 4));
    REQUIRE_FALSE(EXRLoader::hufRoomFor(7, 4, 10));
    // A single value needs a room of at least one.
    REQUIRE(EXRLoader::hufRoomFor(9, 1, 10));
    REQUIRE_FALSE(EXRLoader::hufRoomFor(10, 1, 10));
    // A run of nothing always fits, since it writes nothing.
    REQUIRE(EXRLoader::hufRoomFor(10, 0, 10));
}