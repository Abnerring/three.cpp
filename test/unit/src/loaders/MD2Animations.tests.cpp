#include "loaders/MD2Loader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("MD2Loader groups frame names by their trailing number") {
    // Two frames of the same sequence become one clip named after the prefix.
    const auto clips = MD2Loader::createAnimations({"stand01", "stand02"});
    REQUIRE(clips.size() == 1);
    REQUIRE(clips[0] != nullptr);
    REQUIRE(clips[0]->name.toString() == "stand");
}

TEST_CASE("MD2Loader makes one clip per prefix") {
    const auto clips = MD2Loader::createAnimations({"stand01", "stand02", "run01", "run02"});
    REQUIRE(clips.size() == 2);
    // Both prefixes appear, and the helper emits them in the order the groups were created.
    bool sawStand = false;
    bool sawRun = false;
    for (const auto& clip : clips) {
        if (clip->name.toString() == "stand") sawStand = true;
        if (clip->name.toString() == "run") sawRun = true;
    }
    REQUIRE(sawStand);
    REQUIRE(sawRun);
}

TEST_CASE("MD2Loader produces no clips for names without trailing digits") {
    // Upstream's pattern requires digits at the end, so plain names join no group.
    const auto clips = MD2Loader::createAnimations({"a", "b"});
    REQUIRE(clips.empty());
}

TEST_CASE("MD2Loader produces no clips for no frames") {
    REQUIRE(MD2Loader::createAnimations({}).empty());
}
