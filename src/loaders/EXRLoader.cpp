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
