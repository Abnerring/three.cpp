#include "loaders/EXRLoader.h"

#include "catch.hpp"

TEST_CASE("EXRLoader builds an empty decode table") {
    // One entry per slot of the first level.
    const auto table = EXRLoader::emptyDecodeTable();
    REQUIRE(table.size() == static_cast<std::size_t>(EXRLoader::kHufDecodeSize));
    // Every entry starts free.
    REQUIRE(EXRLoader::decodeEntryFree(table.front()));
    REQUIRE(EXRLoader::decodeEntryFree(table.back()));
    REQUIRE(table.front().length == 0);
    REQUIRE(table.front().candidates.empty());
    REQUIRE_FALSE(table.front().hasCandidates);
}

TEST_CASE("EXRLoader tells a free entry from a taken one") {
    // A fresh entry has nothing in it.
    EXRLoader::DecodeEntry fresh;
    REQUIRE(EXRLoader::decodeEntryFree(fresh));
    // A length in it means it is taken.
    EXRLoader::DecodeEntry named;
    named.length = 3;
    named.literal = 7;
    REQUIRE_FALSE(EXRLoader::decodeEntryFree(named));
    // So does a run of candidates, even with no length.
    EXRLoader::DecodeEntry running;
    running.hasCandidates = true;
    running.candidates = {1, 2};
    REQUIRE_FALSE(EXRLoader::decodeEntryFree(running));
    // An entry that claims candidates without holding any is still not free.
    EXRLoader::DecodeEntry claimed;
    claimed.hasCandidates = true;
    REQUIRE_FALSE(EXRLoader::decodeEntryFree(claimed));
}

TEST_CASE("EXRLoader sends only long codes to a run") {
    // A code that fits the first level names a symbol directly.
    REQUIRE_FALSE(EXRLoader::decodeEntryWantsCandidates(1));
    REQUIRE_FALSE(EXRLoader::decodeEntryWantsCandidates(14));
    // One longer does not, so it needs a run.
    REQUIRE(EXRLoader::decodeEntryWantsCandidates(15));
    REQUIRE(EXRLoader::decodeEntryWantsCandidates(EXRLoader::kHufMaxCodeLength));
}

TEST_CASE("EXRLoader agrees with the index rule") {
    // The two questions are the same one asked twice, so they never disagree.
    for (int length = 1; length <= EXRLoader::kHufMaxCodeLength; ++length) {
        REQUIRE(EXRLoader::decodeEntryWantsCandidates(length) ==
                EXRLoader::hufUsesSubTable(length));
    }
}