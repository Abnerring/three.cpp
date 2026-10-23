#include "loaders/EXRLoader.h"

int EXRLoader::hufLength(int code) {
    // The low six bits.
    return code & 63;
}

int EXRLoader::hufCode(int code) {
    // Everything above them.
    return code >> 6;
}

bool EXRLoader::hufEntryValid(int code) {
    const int length = hufLength(code);
    // No code can be empty.
    if (length == 0) return false;
    // A code that reaches past its own length cannot be encoded.
    return (hufCode(code) >> length) == 0;
}

const std::array<int, 64>& EXRLoader::unZigZagOrder() {
    // The order the source spells out, one line per ten entries.
    static const std::array<int, 64> order{
        0,  1,  5,  6,  14, 15, 27, 28, 2,  4,  7,  13, 16, 26, 29, 42,
        3,  8,  12, 17, 25, 30, 41, 43, 9,  11, 18, 24, 31, 40, 44, 53,
        10, 19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38, 46, 51, 55, 60,
        21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48, 49, 57, 58, 62, 63,
    };
    return order;
}

int EXRLoader::unZigZagIndex(int k) {
    // An index outside the block reads nothing; the caller is expected to stay in range but this
    // keeps a bad file from reaching past the table.
    if (k < 0 || k >= 64) return -1;
    return unZigZagOrder()[static_cast<std::size_t>(k)];
}