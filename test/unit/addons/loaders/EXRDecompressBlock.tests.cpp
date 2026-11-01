#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <cstddef>
#include <vector>

TEST_CASE("EXRLoader passes an uncompressed block through") {
    // Nought says the block was stored plainly, so the bytes are the answer unchanged.
    const std::vector<std::uint8_t> block{1, 2, 3, 4};
    const auto result = EXRLoader::decompressBlock(block, 0, 2, 1, 1, 1, {1});
    INFO("error: " << result.error);
    REQUIRE(result.valid);
    REQUIRE(result.bytes == block);
    // An empty block passes through as an empty answer.
    const auto none = EXRLoader::decompressBlock({}, 0, 2, 1, 1, 1, {1});
    REQUIRE(none.valid);
    REQUIRE(none.bytes.empty());
}

TEST_CASE("EXRLoader refuses the two compressions it has not written") {
    // The block compression is understood well enough to know its name but its reader is not
    // written, so a block naming it is refused rather than passed through.
    const auto b44 = EXRLoader::decompressBlock({1, 2, 3}, 6, 2, 1, 1, 1, {1});
    REQUIRE_FALSE(b44.valid);
    REQUIRE(b44.error == "THREE.EXRLoader: the block compression is not read yet");
    // The deep compression likewise.
    const auto dwa = EXRLoader::decompressBlock({1, 2, 3}, 8, 2, 1, 1, 1, {1});
    REQUIRE_FALSE(dwa.valid);
    REQUIRE(dwa.error == "THREE.EXRLoader: the deep compression is not read yet");
    // The second kind of each pair is refused the same way, since they share a reader.
    REQUIRE(EXRLoader::decompressBlock({1}, 7, 2, 1, 1, 1, {1}).error == b44.error);
    REQUIRE(EXRLoader::decompressBlock({1}, 9, 2, 1, 1, 1, {1}).error == dwa.error);
}

TEST_CASE("EXRLoader refuses a compressed block whose bytes are not a stream") {
    // The zip reader is called and refuses bytes that are not a stream, so the refusal comes
    // from the reader rather than from the dispatcher.
    const auto zip = EXRLoader::decompressBlock({1, 2, 3, 4}, 3, 2, 1, 1, 1, {1});
    REQUIRE_FALSE(zip.valid);
    REQUIRE_FALSE(zip.error.empty());
    // The run length reader accepts almost anything, so it is asked for a stream that runs out
    // instead of one that is malformed.
    const auto rle = EXRLoader::decompressBlock({}, 1, 2, 1, 1, 1, {1});
    // An empty run expands to nothing rather than failing, which is what the reader does.
    REQUIRE(rle.valid);
    REQUIRE(rle.bytes.empty());
}

TEST_CASE("EXRLoader takes an unknown number for the uncompressed case") {
    // This is the trap recorded when the numbers were mapped: a number the format does not
    // define reaches the same answer as the uncompressed one, so the dispatcher cannot tell
    // them apart and hands the block back unchanged.
    const std::vector<std::uint8_t> block{9, 9};
    const auto unknown = EXRLoader::decompressBlock(block, 99, 1, 1, 1, 1, {1});
    REQUIRE(unknown.valid);
    REQUIRE(unknown.bytes == block);
    REQUIRE(EXRLoader::decompresserFor(99) == EXRLoader::decompresserFor(0));
}