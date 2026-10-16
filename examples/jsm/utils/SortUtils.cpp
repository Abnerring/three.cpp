// Module-scope state of the pinned examples/jsm/utils/SortUtils.js port.
// The JavaScript module allocates the counting bins once at import time and reuses the
// same buffer for every call; the native module keeps that ownership here instead of
// duplicating a storage block in each translation unit that includes the header.
#include "utils/SortUtils.h"

namespace SortUtils {

std::uint32_t* BinStorage::bin(int index) {
    return values.data() + std::size_t(index) * BIN_SIZE;
}

BinStorage& moduleBins() {
    static thread_local BinStorage storage;
    return storage;
}

std::uint32_t toUint32(double value) {
    if (!std::isfinite(value) || value == 0) return 0;
    constexpr double modulus = 4294967296.;
    double wrapped = std::fmod(std::trunc(value), modulus);
    if (wrapped < 0) wrapped += modulus;
    return static_cast<std::uint32_t>(wrapped);
}

} // namespace SortUtils
