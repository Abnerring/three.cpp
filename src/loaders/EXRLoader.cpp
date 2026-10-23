#include "loaders/EXRLoader.h"

#include <cmath>
#include <cstring>

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

int EXRLoader::uint16Bits(int value) {
    // The low sixteen bits.
    return value & 0xFFFF;
}

int EXRLoader::int16Bits(int value) {
    const int bits = uint16Bits(value);
    // Above the signed maximum the value is negative, a whole range lower.
    return bits > 0x7FFF ? bits - 0x10000 : bits;
}

EXRLoader::WaveletPair EXRLoader::wdec14(int l, int h) {
    const int low = int16Bits(l);
    const int high = int16Bits(h);
    // Half of the high coefficient, rounded up by its parity, recovers the first sample.
    const int sum = low + (high & 1) + (high >> 1);
    WaveletPair pair;
    pair.a = sum;
    pair.b = sum - high;
    return pair;
}

void EXRLoader::csc709Inverse(YcbcrPixel& pixel) {
    // The three inputs are read before any output is written, which matters because they share
    // the same storage.
    const double y = pixel.y;
    const double cb = pixel.cb;
    const double cr = pixel.cr;
    pixel.y = y + 1.5747 * cr;
    pixel.cb = y - 0.1873 * cb - 0.4682 * cr;
    pixel.cr = y + 1.8556 * cb;
}

void EXRLoader::csc709InverseBlock(YcbcrPixel* pixels, std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) csc709Inverse(pixels[i]);
}

std::uint16_t EXRLoader::floatToHalfBits(float value) {
    // The bits of the number, read without changing them.
    std::uint32_t bits = 0;
    static_assert(sizeof(float) == sizeof(bits), "a float must be four bytes here");
    std::memcpy(&bits, &value, sizeof(bits));

    const std::uint32_t sign = (bits >> 16) & 0x8000u;
    const std::uint32_t magnitude = bits & 0x7fffffffu;

    // Too large for a half: infinity, or not a number if it already was one.
    if (magnitude >= 0x47800000u) {
        return static_cast<std::uint16_t>(sign | (magnitude > 0x7f800000u ? kHalfNotANumber
                                                                        : kHalfInfinity));
    }

    // Too small for a normal half. Half a step is added first, which puts the value on the grid
    // the subnormal patterns share, and the bias is then removed.
    if (magnitude < 0x38800000u) {
        const float nudged = std::fabs(value) + 0.5f;
        std::uint32_t nudgedBits = 0;
        std::memcpy(&nudgedBits, &nudged, sizeof(nudgedBits));
        return static_cast<std::uint16_t>(sign | (nudgedBits - 0x3f000000u));
    }

    // Round to nearest, with a tie going to the even pattern.
    const std::uint32_t rounded = magnitude + 0xfffu + ((magnitude >> 13) & 1u);
    return static_cast<std::uint16_t>(sign | ((rounded - 0x38000000u) >> 13));
}

EXRLoader::ReverseLut EXRLoader::reverseLutFromBitmap(const std::vector<std::uint8_t>& bitmap) {
    ReverseLut result;
    result.lut.assign(static_cast<std::size_t>(kUshortRange), 0);
    int k = 0;
    for (int i = 0; i < kUshortRange; ++i) {
        // Zero is always listed, and the rest when their bit is set.
        const std::size_t byteIndex = static_cast<std::size_t>(i) >> 3;
        const bool bitSet =
            byteIndex < bitmap.size() && (bitmap[byteIndex] & (1 << (i & 7))) != 0;
        if (i == 0 || bitSet) result.lut[static_cast<std::size_t>(k++)] = static_cast<std::uint16_t>(i);
    }
    // The count minus one, which is the last index rather than a length.
    result.lastIndex = k - 1;
    return result;
}

void EXRLoader::applyLut(const std::vector<std::uint16_t>& lut,
                         std::vector<std::uint16_t>& data) {
    for (auto& value : data) {
        // A value outside the table is left as it is.
        if (static_cast<std::size_t>(value) < lut.size()) value = lut[value];
    }
}

void EXRLoader::predictor(std::vector<std::uint16_t>& source) {
    // Forwards, because each step reads the value the previous step wrote.
    for (std::size_t t = 1; t < source.size(); ++t) {
        const int sum = static_cast<int>(source[t - 1]) + static_cast<int>(source[t]) - 128;
        source[t] = static_cast<std::uint16_t>(sum);
    }
}

void EXRLoader::interleaveScalar(const std::vector<std::uint16_t>& source,
                                 std::vector<std::uint16_t>& out) {
    if (source.empty()) return;
    std::size_t first = 0;
    std::size_t second = (source.size() + 1) / 2;
    std::size_t s = 0;
    // Alternate between the halves until the output is full.
    while (true) {
        if (s >= source.size()) break;
        out[s++] = source[first++];
        if (s >= source.size()) break;
        out[s++] = source[second++];
    }
}

std::vector<std::uint8_t> EXRLoader::decodeRunLength(const std::vector<std::uint8_t>& source) {
    std::vector<std::uint8_t> out;
    // The size runs down as the stream is consumed, and a stream that declares more than it holds
    // simply stops.
    long long size = static_cast<long long>(source.size());
    std::size_t p = 0;

    while (size > 0 && p < source.size()) {
        // The header is signed.
        const int header = static_cast<std::int8_t>(source[p++]);
        if (header < 0) {
            // A verbatim run: the count bytes follow as they are.
            const int count = -header;
            size -= count + 1;
            for (int i = 0; i < count && p < source.size(); ++i) out.push_back(source[p++]);
        } else {
            // A repeat run: one byte, written one more time than the header says.
            size -= 2;
            if (p >= source.size()) break;
            const std::uint8_t value = source[p++];
            for (int i = 0; i < header + 1; ++i) out.push_back(value);
        }
    }
    return out;
}

void EXRLoader::dctInverse(double data[64]) {
    // The five places of pi the source writes, not a fuller constant.
    const double pi = 3.14159;
    const double a = 0.5 * std::cos(pi / 4.0);
    const double b = 0.5 * std::cos(pi / 16.0);
    const double c = 0.5 * std::cos(pi / 8.0);
    const double d = 0.5 * std::cos(3.0 * pi / 16.0);
    const double e = 0.5 * std::cos(5.0 * pi / 16.0);
    const double f = 0.5 * std::cos(3.0 * pi / 8.0);
    const double g = 0.5 * std::cos(7.0 * pi / 16.0);

    double alpha[4];
    double beta[4];
    double theta[4];
    double gamma[4];

    // Along each row.
    for (int row = 0; row < 8; ++row) {
        const int rowPtr = row * 8;

        alpha[0] = c * data[rowPtr + 2];
        alpha[1] = f * data[rowPtr + 2];
        alpha[2] = c * data[rowPtr + 6];
        alpha[3] = f * data[rowPtr + 6];

        beta[0] = b * data[rowPtr + 1] + d * data[rowPtr + 3] + e * data[rowPtr + 5] +
                  g * data[rowPtr + 7];
        beta[1] = d * data[rowPtr + 1] - g * data[rowPtr + 3] - b * data[rowPtr + 5] -
                  e * data[rowPtr + 7];
        beta[2] = e * data[rowPtr + 1] - b * data[rowPtr + 3] + g * data[rowPtr + 5] +
                  d * data[rowPtr + 7];
        beta[3] = g * data[rowPtr + 1] - e * data[rowPtr + 3] + d * data[rowPtr + 5] -
                  b * data[rowPtr + 7];

        theta[0] = a * (data[rowPtr + 0] + data[rowPtr + 4]);
        theta[3] = a * (data[rowPtr + 0] - data[rowPtr + 4]);
        theta[1] = alpha[0] + alpha[3];
        theta[2] = alpha[1] - alpha[2];

        gamma[0] = theta[0] + theta[1];
        gamma[1] = theta[3] + theta[2];
        gamma[2] = theta[3] - theta[2];
        gamma[3] = theta[0] - theta[1];

        data[rowPtr + 0] = gamma[0] + beta[0];
        data[rowPtr + 1] = gamma[1] + beta[1];
        data[rowPtr + 2] = gamma[2] + beta[2];
        data[rowPtr + 3] = gamma[3] + beta[3];

        data[rowPtr + 4] = gamma[3] - beta[3];
        data[rowPtr + 5] = gamma[2] - beta[2];
        data[rowPtr + 6] = gamma[1] - beta[1];
        data[rowPtr + 7] = gamma[0] - beta[0];
    }

    // Down each column, with the stride written out.
    for (int column = 0; column < 8; ++column) {
        alpha[0] = c * data[16 + column];
        alpha[1] = f * data[16 + column];
        alpha[2] = c * data[48 + column];
        alpha[3] = f * data[48 + column];

        beta[0] = b * data[8 + column] + d * data[24 + column] + e * data[40 + column] +
                  g * data[56 + column];
        beta[1] = d * data[8 + column] - g * data[24 + column] - b * data[40 + column] -
                  e * data[56 + column];
        beta[2] = e * data[8 + column] - b * data[24 + column] + g * data[40 + column] +
                  d * data[56 + column];
        beta[3] = g * data[8 + column] - e * data[24 + column] + d * data[40 + column] -
                  b * data[56 + column];

        theta[0] = a * (data[column] + data[32 + column]);
        theta[3] = a * (data[column] - data[32 + column]);
        theta[1] = alpha[0] + alpha[3];
        theta[2] = alpha[1] - alpha[2];

        gamma[0] = theta[0] + theta[1];
        gamma[1] = theta[3] + theta[2];
        gamma[2] = theta[3] - theta[2];
        gamma[3] = theta[0] - theta[1];

        data[0 + column] = gamma[0] + beta[0];
        data[8 + column] = gamma[1] + beta[1];
        data[16 + column] = gamma[2] + beta[2];
        data[24 + column] = gamma[3] + beta[3];

        data[32 + column] = gamma[3] - beta[3];
        data[40 + column] = gamma[2] - beta[2];
        data[48 + column] = gamma[1] - beta[1];
        data[56 + column] = gamma[0] - beta[0];
    }
}

void EXRLoader::hufCanonicalCodeTable(std::vector<int>& hcode) {
    // One counter per possible length.
    std::vector<int> counts(static_cast<std::size_t>(kHufMaxCodeLength) + 1, 0);

    // The entries are lengths at this point, so they are counted as they are.
    for (const int length : hcode) {
        if (length >= 0 && length <= kHufMaxCodeLength) counts[static_cast<std::size_t>(length)] += 1;
    }

    // Walk down from the longest, giving each length the first code that fits after the ones
    // below it have taken their share.
    int available = 0;
    for (int length = kHufMaxCodeLength; length > 0; --length) {
        const int next = (available + counts[static_cast<std::size_t>(length)]) >> 1;
        counts[static_cast<std::size_t>(length)] = available;
        available = next;
    }

    // Pack the code above its length, taking each one in turn as it is handed out.
    for (int& entry : hcode) {
        if (entry > 0 && entry <= kHufMaxCodeLength) {
            entry = entry | (counts[static_cast<std::size_t>(entry)]++ << 6);
        }
    }
}

EXRLoader::AcScan EXRLoader::unRleAC(const std::vector<std::uint16_t>& acBuffer,
                                     std::size_t start) {
    AcScan scan;
    scan.values.assign(64, 0);
    int dctComp = 1;
    std::size_t current = start;

    while (dctComp < 64 && current < acBuffer.size()) {
        const std::uint16_t acValue = acBuffer[current];

        if (acValue == 0xff00) {
            // The end of the block, spelled out rather than implied.
            dctComp = 64;
        } else if ((acValue >> 8) == 0xff) {
            // A skip of that many coefficients.
            dctComp += acValue & 0xff;
        } else {
            // A coefficient, stored where the scan has reached.
            scan.values[static_cast<std::size_t>(dctComp)] = acValue;
            dctComp++;
        }

        current++;
        scan.wordsRead++;
    }
    scan.finalPosition = dctComp;
    return scan;
}