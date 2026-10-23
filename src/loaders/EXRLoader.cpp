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

bool EXRLoader::hufUsesSubTable(int length) {
    // Only a code longer than the first level reaches past it.
    return length > kHufDecodeBits;
}

int EXRLoader::hufSubTableIndex(int code, int length) {
    // A short code indexes the table with its whole value, which is the same as shifting by zero.
    if (length <= kHufDecodeBits) return code;
    // A long one keeps the bits above the first level.
    return code >> (length - kHufDecodeBits);
}

EXRLoader::BitState EXRLoader::getChar(BitState state, const std::vector<std::uint8_t>& bytes,
                                       std::size_t& offset) {
    // One byte goes in at the bottom, whether or not anything is read from the top.
    const std::uint8_t byte = offset < bytes.size() ? bytes[offset++] : std::uint8_t{0};
    state.accumulate = static_cast<std::int32_t>(
        (static_cast<std::uint32_t>(state.accumulate) << 8) | byte);
    state.liveBits += 8;
    return state;
}

EXRLoader::GetBitsResult EXRLoader::getBits(int nBits, BitState state,
                                            const std::vector<std::uint8_t>& bytes,
                                            std::size_t& offset) {
    // Fill until enough bits are live.
    while (state.liveBits < nBits) state = getChar(state, bytes, offset);

    // The bits just taken are no longer live.
    state.liveBits -= nBits;

    // Take them from the top, masked to the width asked for. The accumulate word is shifted as a
    // signed value, which is what the source does; the mask is what keeps the answer right.
    const int mask = (1 << nBits) - 1;
    GetBitsResult result;
    result.state = state;
    result.value = (state.accumulate >> state.liveBits) & mask;
    return result;
}

std::size_t EXRLoader::bitByteCount(int bits) {
    // Seven is added so a partial last byte still counts as one.
    if (bits <= 0) return 0;
    return static_cast<std::size_t>((bits + 7) / 8);
}

int EXRLoader::decodeTableIndex(std::int32_t accumulate, int liveBits) {
    // The top fourteen of what is live, masked so the sign the shift brings in never reaches it.
    if (liveBits < kHufDecodeBits) return 0;
    return (accumulate >> (liveBits - kHufDecodeBits)) & kHufDecodeMask;
}

bool EXRLoader::hufRangeValid(std::uint32_t im, std::uint32_t iM) {
    // Both ends have to fall inside the table the symbols are counted in.
    const auto limit = static_cast<std::uint32_t>(kHufEncSize);
    return im < limit && iM < limit;
}

bool EXRLoader::hufBitsFit(std::uint32_t nBits, std::size_t remainingBytes) {
    // Eight bits to a byte, so the run cannot want more than eight times what is left.
    return static_cast<std::uint64_t>(nBits) <= 8ull * static_cast<std::uint64_t>(remainingBytes);
}

std::string EXRLoader::hufRangeMessage() {
    return "THREE.EXRLoader: Something wrong with HUF_ENCSIZE";
}

std::string EXRLoader::hufBitsMessage() {
    return "THREE.EXRLoader: Something wrong with hufUncompress";
}

EXRLoader::ZeroRun EXRLoader::zeroRunKind(int symbol) {
    // The last value on its own, and the four before it as a group.
    if (symbol == kLongZeroCodeRun) return ZeroRun::Long;
    if (symbol >= kShortZeroCodeRun) return ZeroRun::Short;
    // Anything below the group is a real code length.
    return ZeroRun::None;
}

int EXRLoader::shortZeroRunLength(int symbol) {
    // The first value stands for two, not for one.
    return symbol - kShortZeroCodeRun + 2;
}

int EXRLoader::longZeroRunLength(int extra) {
    // The eight bits count from six rather than from zero.
    return extra + kShortestLongRun;
}

bool EXRLoader::looksLikeExr(const std::vector<std::uint8_t>& buffer) {
    if (buffer.size() < 4) return false;
    // The magic read little endian, which is the order the format writes it in.
    const std::uint32_t magic = static_cast<std::uint32_t>(buffer[0]) |
                                (static_cast<std::uint32_t>(buffer[1]) << 8) |
                                (static_cast<std::uint32_t>(buffer[2]) << 16) |
                                (static_cast<std::uint32_t>(buffer[3]) << 24);
    return magic == kMagic;
}

EXRLoader::HeaderFlags EXRLoader::headerFlags(std::uint8_t spec) {
    HeaderFlags flags;
    // The four flags start at the second bit; the first is unused.
    flags.singleTile = (spec & 2) != 0;
    flags.longName = (spec & 4) != 0;
    flags.deepFormat = (spec & 8) != 0;
    flags.multiPart = (spec & 16) != 0;
    return flags;
}

std::string EXRLoader::notExrMessage() {
    return "THREE.EXRLoader: Provided file doesn't appear to be in OpenEXR format.";
}

std::string EXRLoader::noPartsMessage() {
    return "THREE.EXRLoader: No valid part headers found.";
}

std::string EXRLoader::readNullTerminated(const std::vector<std::uint8_t>& buffer,
                                          std::size_t& offset) {
    std::string text;
    // Read until the null, which is one past the last character and is consumed as well.
    while (offset < buffer.size() && buffer[offset] != 0) {
        text.push_back(static_cast<char>(buffer[offset++]));
    }
    if (offset < buffer.size()) offset++;
    return text;
}

bool EXRLoader::partHasAttributes(std::size_t attributeCount) {
    // A part that listed nothing is not a part.
    return attributeCount > 0;
}

std::string EXRLoader::unknownAttributeWarning(const std::string& type) {
    // The type is quoted, so a name with a space in it stays readable.
    return "THREE.EXRLoader: Skipped unknown header attribute type '" + type + "'.";
}

EXRLoader::AttributeKind EXRLoader::attributeKind(const std::string& type) {
    // Three names share the fixed string handler.
    if (type == "string" || type == "stringvector" || type == "iccProfile") {
        return AttributeKind::FixedString;
    }
    if (type == "chlist") return AttributeKind::ChannelList;
    if (type == "chromaticities") return AttributeKind::Chromaticities;
    if (type == "compression") return AttributeKind::Compression;
    if (type == "box2i") return AttributeKind::Box;
    if (type == "envmap") return AttributeKind::EnvironmentMap;
    if (type == "tiledesc") return AttributeKind::TileDescription;
    if (type == "lineOrder") return AttributeKind::LineOrder;
    if (type == "float") return AttributeKind::Float;
    if (type == "v2f") return AttributeKind::Vector2;
    if (type == "v3f") return AttributeKind::Vector3;
    if (type == "int") return AttributeKind::Integer;
    if (type == "rational") return AttributeKind::Rational;
    // Anything else is reported and skipped.
    return AttributeKind::Unknown;
}

bool EXRLoader::attributeKindKnown(AttributeKind kind) {
    return kind != AttributeKind::Unknown;
}

std::string EXRLoader::compressionName(std::uint8_t code) {
    // Ten codes, in the order the format numbers them.
    static const char* names[] = {
        "NO_COMPRESSION",  "RLE_COMPRESSION",  "ZIPS_COMPRESSION", "ZIP_COMPRESSION",
        "PIZ_COMPRESSION", "PXR24_COMPRESSION", "B44_COMPRESSION", "B44A_COMPRESSION",
        "DWAA_COMPRESSION", "DWAB_COMPRESSION",
    };
    // A code past the table names nothing rather than reading past it.
    if (code >= 10) return std::string();
    return names[code];
}

std::string EXRLoader::lineOrderName(std::uint8_t code) {
    static const char* names[] = {"INCREASING_Y", "DECREASING_Y", "RANDOM_Y"};
    if (code >= 3) return std::string();
    return names[code];
}

std::string EXRLoader::envmapName(std::uint8_t code) {
    static const char* names[] = {"ENVMAP_LATLONG", "ENVMAP_CUBE"};
    if (code >= 2) return std::string();
    return names[code];
}

EXRLoader::Box2i EXRLoader::boxFromNumbers(std::int32_t first, std::int32_t second,
                                           std::int32_t third, std::int32_t fourth) {
    // The first two are the minima of the two axes and the last two their maxima.
    Box2i box;
    box.xMin = first;
    box.yMin = second;
    box.xMax = third;
    box.yMax = fourth;
    return box;
}

std::size_t EXRLoader::channelEntrySize(std::size_t nameLength) {
    // The name, its terminator, four bytes of type, one of flag, three reserved and two of
    // sampling: seventeen bytes beyond the name itself.
    return nameLength + 1 + 4 + 1 + kChannelReserved + 4 + 4;
}

bool EXRLoader::channelEntryInRange(std::size_t position, std::size_t start, std::size_t size) {
    // The last byte of the list is its terminator and is not an entry, hence the one short.
    if (size == 0) return false;
    return position < (start + size - 1);
}

EXRLoader::Chromaticities EXRLoader::chromaticitiesFromNumbers(const double numbers[8]) {
    // Each pair in turn.
    Chromaticities points;
    points.redX = numbers[0];
    points.redY = numbers[1];
    points.greenX = numbers[2];
    points.greenY = numbers[3];
    points.blueX = numbers[4];
    points.blueY = numbers[5];
    points.whiteX = numbers[6];
    points.whiteY = numbers[7];
    return points;
}

std::string EXRLoader::levelModeName(int code) {
    static const char* names[] = {"ONE_LEVEL", "MIPMAP_LEVELS", "RIPMAP_LEVELS"};
    if (code >= 3 || code < 0) return std::string();
    return names[code];
}

std::string EXRLoader::roundingModeName(int code) {
    static const char* names[] = {"ROUND_DOWN", "ROUND_UP"};
    if (code >= 2 || code < 0) return std::string();
    return names[code];
}

EXRLoader::TileDescription EXRLoader::tileDescription(std::uint32_t xSize, std::uint32_t ySize,
                                                       std::uint8_t modes) {
    TileDescription tile;
    tile.xSize = xSize;
    tile.ySize = ySize;
    // The low nibble is the level mode and the high one the rounding mode.
    tile.levelMode = levelModeName(modes & 0xf);
    tile.roundingMode = roundingModeName(modes >> 4);
    return tile;
}

int EXRLoader::runLengthCount(std::int32_t accumulate, int liveBits) {
    // Shift down to the live bits and keep the low byte, which is what a one byte array does.
    if (liveBits < 0) return 0;
    const std::int32_t shifted = accumulate >> liveBits;
    return static_cast<int>(static_cast<std::uint8_t>(shifted & 0xFF));
}

bool EXRLoader::runFits(std::size_t outOffset, int count, std::size_t outEnd) {
    // A count of zero writes nothing and always fits.
    if (count <= 0) return true;
    return outOffset + static_cast<std::size_t>(count) <= outEnd;
}

void EXRLoader::writeRun(std::vector<std::uint8_t>& out, int count) {
    // Nothing to repeat and nothing written.
    if (count <= 0 || out.empty()) return;
    const std::uint8_t value = out.back();
    for (int i = 0; i < count; ++i) out.push_back(value);
}

EXRLoader::Rational EXRLoader::rationalFrom(std::int32_t numerator, std::uint32_t denominator) {
    Rational rational;
    rational.numerator = numerator;
    rational.denominator = denominator;
    return rational;
}

std::string EXRLoader::fixedLengthString(const std::vector<std::uint8_t>& buffer,
                                         std::size_t offset, std::size_t size) {
    std::string text;
    // Exactly the stated number of bytes, or as many as are there if fewer.
    for (std::size_t i = 0; i < size && offset + i < buffer.size(); ++i) {
        text.push_back(static_cast<char>(buffer[offset + i]));
    }
    return text;
}

std::size_t EXRLoader::advanceF32(std::size_t offset) { return offset + kFloatSize; }

std::size_t EXRLoader::advanceU32(std::size_t offset) { return offset + kIntSize; }

EXRLoader::BlockCompression EXRLoader::blockCompression(int code) {
    // Only two kinds are named; anything else leaves nothing to unpack.
    if (code == kStaticHuffman) return BlockCompression::Huffman;
    if (code == kDeflate) return BlockCompression::Deflate;
    return BlockCompression::None;
}

bool EXRLoader::blockPresent(std::uint32_t size) {
    // Each block says whether it is there by its own size.
    return size > 0;
}

std::size_t EXRLoader::advanceAfterAc(BlockCompression kind, std::uint32_t compressedSize,
                                      std::uint32_t uncompressedCount) {
    // The two kinds advance by different amounts, which the source does without a comment.
    if (kind == BlockCompression::Deflate) return uncompressedCount;
    if (kind == BlockCompression::Huffman) return compressedSize;
    return 0;
}

bool EXRLoader::decodeEntryFree(const DecodeEntry& entry) {
    // A length anywhere in the entry means it is spoken for.
    return entry.length == 0 && !entry.hasCandidates && entry.candidates.empty();
}

bool EXRLoader::decodeEntryWantsCandidates(int codeLength) {
    // Only a code too long for the first level goes into a run.
    return hufUsesSubTable(codeLength);
}

std::vector<EXRLoader::DecodeEntry> EXRLoader::emptyDecodeTable() {
    // One entry per slot of the first level, all of them free.
    return std::vector<DecodeEntry>(static_cast<std::size_t>(kHufDecodeSize), DecodeEntry{});
}