#include "loaders/EXRLoader.h"

#include "catch.hpp"

#include <cstdint>
#include <vector>

TEST_CASE("EXRLoader fills every slot a short code reaches") {
    // A one bit code of zero fills half the table, since a leading zero covers every slot whose
    // top bit is zero.
    std::vector<int> codes(EXRLoader::kHufEncSize, 0);
    codes[0] = (0 << 6) | 1;
    const auto result = EXRLoader::hufBuildDecTable(codes, 0, 0);
    INFO("error: " << result.error);
    REQUIRE(result.valid);
    REQUIRE(result.entries.size() == static_cast<std::size_t>(EXRLoader::kHufDecodeSize));
    REQUIRE(result.entries[0].length == 1);
    REQUIRE(result.entries[0].literal == 0);
    REQUIRE(result.entries[8191].length == 1);
    // The other half is untouched, since those slots begin with a one.
    REQUIRE(result.entries[8192].length == 0);
    REQUIRE(result.entries[16383].length == 0);
}

TEST_CASE("EXRLoader puts a long code in a single slot") {
    // A fifteen bit code is one longer than the table's own width, so it lands in one slot with
    // itself as the only candidate.
    std::vector<int> codes(EXRLoader::kHufEncSize, 0);
    codes[3] = (0 << 6) | 15;
    const auto result = EXRLoader::hufBuildDecTable(codes, 3, 3);
    INFO("error: " << result.error);
    REQUIRE(result.valid);
    REQUIRE(result.entries[0].hasCandidates);
    REQUIRE(result.entries[0].candidates.size() == 1);
    REQUIRE(result.entries[0].candidates[0] == 3);
    // Its length slot stays empty, since a candidate list and a length never share an entry.
    REQUIRE(result.entries[0].length == 0);
}

TEST_CASE("EXRLoader refuses a code that does not fit its length") {
    // A one bit length cannot hold the code two, which needs two bits.
    std::vector<int> codes(EXRLoader::kHufEncSize, 0);
    codes[1] = (2 << 6) | 1;
    const auto result = EXRLoader::hufBuildDecTable(codes, 1, 1);
    REQUIRE_FALSE(result.valid);
    REQUIRE(result.error == EXRLoader::hufEntryMessage());
    // A length of zero is skipped rather than refused, since the symbol simply does not appear.
    std::vector<int> none(EXRLoader::kHufEncSize, 0);
    REQUIRE(EXRLoader::hufBuildDecTable(none, 0, 5).valid);
}

TEST_CASE("EXRLoader names the table entry error") {
    REQUIRE(EXRLoader::hufEntryMessage() == "THREE.EXRLoader: Invalid table entry");
}