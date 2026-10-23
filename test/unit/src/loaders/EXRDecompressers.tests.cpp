#include "loaders/EXRLoader.h"

#include "catch.hpp"

TEST_CASE("EXRLoader sends each compression number to a reader") {
    // The four that have a reader of their own.
    REQUIRE(EXRLoader::decompresserFor(0) == EXRLoader::Decompresser::None);
    REQUIRE(EXRLoader::decompresserFor(1) == EXRLoader::Decompresser::RunLength);
    REQUIRE(EXRLoader::decompresserFor(4) == EXRLoader::Decompresser::Wavelet);
    REQUIRE(EXRLoader::decompresserFor(5) == EXRLoader::Decompresser::Pxr);
}

TEST_CASE("EXRLoader shares a reader between each pair") {
    // The two zips share one, as do the two B44s and the two deep ones.
    REQUIRE(EXRLoader::decompresserFor(2) == EXRLoader::decompresserFor(3));
    REQUIRE(EXRLoader::decompresserFor(2) == EXRLoader::Decompresser::Zip);
    REQUIRE(EXRLoader::decompresserFor(6) == EXRLoader::decompresserFor(7));
    REQUIRE(EXRLoader::decompresserFor(6) == EXRLoader::Decompresser::B44);
    REQUIRE(EXRLoader::decompresserFor(8) == EXRLoader::decompresserFor(9));
    REQUIRE(EXRLoader::decompresserFor(8) == EXRLoader::Decompresser::Deep);
}

TEST_CASE("EXRLoader uses seven readers for ten numbers") {
    // Every number from nought to nine names a reader, and only seven different ones appear.
    std::vector<EXRLoader::Decompresser> seen;
    for (int code = 0; code < 10; ++code) {
        const auto reader = EXRLoader::decompresserFor(code);
        bool present = false;
        for (const auto known : seen) if (known == reader) present = true;
        if (!present) seen.push_back(reader);
    }
    REQUIRE(seen.size() == 7);
    // A number outside the ten names no reader, which is also what the uncompressed case names,
    // so a caller has to look at the number as well as the answer.
    REQUIRE(EXRLoader::decompresserFor(10) == EXRLoader::Decompresser::None);
    REQUIRE(EXRLoader::decompresserFor(10) == EXRLoader::decompresserFor(0));
}