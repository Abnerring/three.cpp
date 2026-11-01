#include "loaders/EXRLoader.h"

#include "catch.hpp"

TEST_CASE("EXRLoader names the two block compressions") {
    // Only two codes are named.
    REQUIRE(EXRLoader::blockCompression(0) == EXRLoader::BlockCompression::Huffman);
    REQUIRE(EXRLoader::blockCompression(1) == EXRLoader::BlockCompression::Deflate);
    // Anything else leaves nothing to unpack, which the source lets happen rather than refusing
    // the file.
    REQUIRE(EXRLoader::blockCompression(2) == EXRLoader::BlockCompression::None);
    REQUIRE(EXRLoader::blockCompression(-1) == EXRLoader::BlockCompression::None);
    // The two constants are what the codes are.
    REQUIRE(EXRLoader::kStaticHuffman == 0);
    REQUIRE(EXRLoader::kDeflate == 1);
}

TEST_CASE("EXRLoader tells a present block from an absent one") {
    // A size of zero is an absent block; anything else is present.
    REQUIRE_FALSE(EXRLoader::blockPresent(0));
    REQUIRE(EXRLoader::blockPresent(1));
    REQUIRE(EXRLoader::blockPresent(1000));
}

TEST_CASE("EXRLoader advances by a different amount for each kind") {
    // The huffman kind advances by the compressed size.
    REQUIRE(EXRLoader::advanceAfterAc(EXRLoader::BlockCompression::Huffman, 100, 200) == 100);
    // The deflate kind advances by the uncompressed count, which is the discrepancy.
    REQUIRE(EXRLoader::advanceAfterAc(EXRLoader::BlockCompression::Deflate, 100, 200) == 200);
    // The two therefore disagree whenever the two sizes differ.
    REQUIRE(EXRLoader::advanceAfterAc(EXRLoader::BlockCompression::Huffman, 100, 200) !=
            EXRLoader::advanceAfterAc(EXRLoader::BlockCompression::Deflate, 100, 200));
    // An unknown kind advances by nothing at all.
    REQUIRE(EXRLoader::advanceAfterAc(EXRLoader::BlockCompression::None, 100, 200) == 0);
}

TEST_CASE("EXRLoader agrees when the two sizes are the same") {
    // With equal sizes the discrepancy cannot be seen, which is why it can go unnoticed.
    REQUIRE(EXRLoader::advanceAfterAc(EXRLoader::BlockCompression::Huffman, 64, 64) ==
            EXRLoader::advanceAfterAc(EXRLoader::BlockCompression::Deflate, 64, 64));
}