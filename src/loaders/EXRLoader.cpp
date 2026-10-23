#include "loaders/EXRLoader.h"

#include "loaders/Zlib.h"

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

void EXRLoader::revertPredictor(std::vector<std::uint8_t>& data) {
    // The first byte is left as it is and each later one adds its predecessor, with a constant
    // taken off. The result is stored in a byte, so it wraps rather than being clamped.
    for (std::size_t t = 1; t < data.size(); ++t) {
        const int sum = static_cast<int>(data[t - 1]) + static_cast<int>(data[t]) - 128;
        data[t] = static_cast<std::uint8_t>(sum & 0xFF);
    }
}

std::size_t EXRLoader::interleaveSplit(std::size_t length) {
    // Halfway, rounded up: for an odd length the first half is the shorter one.
    return (length + 1) / 2;
}

std::vector<std::uint8_t> EXRLoader::interleaveScalar(const std::vector<std::uint8_t>& source) {
    std::vector<std::uint8_t> out(source.size(), 0);
    std::size_t t1 = 0;
    std::size_t t2 = interleaveSplit(source.size());
    std::size_t s = 0;
    // One from each half in turn, stopping as soon as the block is full.
    while (true) {
        if (s >= source.size()) break;
        out[s++] = t1 < source.size() ? source[t1++] : 0;
        if (s >= source.size()) break;
        out[s++] = t2 < source.size() ? source[t2++] : 0;
    }
    return out;
}

EXRLoader::ZipBlockResult EXRLoader::uncompressZIP(const std::vector<std::uint8_t>& block) {
    ZipBlockResult result;
    // The bytes are deflated first, so they are inflated first on the way back.
    const Zlib::UnzlibResult inflated = Zlib::unzlib(block);
    if (!inflated.valid) {
        result.error = inflated.error;
        return result;
    }
    // Then the predictor, then the halves put back together. The order matters: undoing the
    // predictor on interleaved bytes would predict from the wrong neighbour.
    std::vector<std::uint8_t> bytes = inflated.output;
    revertPredictor(bytes);
    result.bytes = interleaveScalar(bytes);
    result.valid = true;
    return result;
}

EXRLoader::RleBlockResult EXRLoader::uncompressRLE(const std::vector<std::uint8_t>& block) {
    RleBlockResult result;
    // The runs are expanded first, then the predictor is undone, then the halves are joined.
    std::vector<std::uint8_t> bytes = decodeRunLength(block);
    revertPredictor(bytes);
    result.bytes = interleaveScalar(bytes);
    result.valid = true;
    return result;
}

EXRLoader::CompressionKind EXRLoader::compressionKind(const std::string& name) {
    // Exact names, compared as strings. Anything else is unsupported rather than guessed.
    if (name == kNoCompression) return CompressionKind::None;
    if (name == kRleCompression) return CompressionKind::RunLength;
    if (name == kZipsCompression) return CompressionKind::Zip;
    return CompressionKind::Unsupported;
}

std::string EXRLoader::unsupportedDeepMessage(const std::string& name) {
    // The message names the compression it was given.
    return "THREE.EXRLoader: " + name + " is unsupported for deep data";
}

std::size_t EXRLoader::chunkPrefix(bool multiPart) {
    // A file with more than one part writes a number before each chunk; a single part one does
    // not.
    return multiPart ? static_cast<std::size_t>(kPartPrefixSize) : 0;
}

int EXRLoader::rowWithinWindow(int writtenRow, int windowFirstRow) {
    // Rows are counted from the first row of the window, not from zero of the file.
    return writtenRow - windowFirstRow;
}

std::uint32_t EXRLoader::totalSamples(const std::vector<std::uint32_t>& cumulativeCounts) {
    // The counts are cumulative, so the last one is the total for the whole row.
    if (cumulativeCounts.empty()) return 0;
    return cumulativeCounts.back();
}

std::uint32_t EXRLoader::cumulativeCount(std::uint32_t b0, std::uint32_t b1, std::uint32_t b2,
                                         std::uint32_t b3) {
    // Lowest byte first, unlike the rest of the file.
    return (b0 & 0xFFu) | ((b1 & 0xFFu) << 8) | ((b2 & 0xFFu) << 16) | ((b3 & 0xFFu) << 24);
}

int EXRLoader::pizBitmapRunLength(int minNonZero, int maxNonZero) {
    // The block writes a byte for each position in the range, counting both ends. When the first
    // is past the second the range is empty and nothing is written.
    if (minNonZero > maxNonZero) return 0;
    return maxNonZero - minNonZero + 1;
}

std::size_t EXRLoader::pizChannelSize(std::uint32_t columns, std::uint32_t lines,
                                      std::uint32_t channelType) {
    // The whole window, times the channel's width in sixteen bit words.
    return static_cast<std::size_t>(columns) * lines * channelType;
}

std::vector<std::size_t> EXRLoader::pizChannelStarts(std::uint32_t columns, std::uint32_t lines,
                                                     std::uint32_t channelType,
                                                     std::size_t channelCount) {
    std::vector<std::size_t> starts;
    starts.reserve(channelCount);
    std::size_t end = 0;
    const std::size_t size = pizChannelSize(columns, lines, channelType);
    for (std::size_t i = 0; i < channelCount; ++i) {
        // Each channel begins where the one before it ended.
        starts.push_back(end);
        end += size;
    }
    return starts;
}

bool EXRLoader::pizBitmapFits(int maxNonZero) {
    // A second end at or past the bitmap's length cannot be read.
    return maxNonZero < kPizBitmapSize;
}

int EXRLoader::pizWaveletExtent(std::uint32_t nx, std::uint32_t ny) {
    // The smaller side, since the wavelet runs over the shorter of the two.
    return static_cast<int>(nx > ny ? ny : nx);
}

int EXRLoader::pizWaveletFirstStep(int extent) {
    // Double until past the extent, come back one place, then halve once more.
    int p = 1;
    while (p <= extent) p <<= 1;
    p >>= 1;
    p >>= 1;
    return p < 1 ? 0 : p;
}

bool EXRLoader::pizWaveletWide(int maxValue) {
    // Sixteen thousand three hundred and eighty four is where the wider arithmetic begins.
    return maxValue >= (1 << 14);
}

std::size_t EXRLoader::pizRowLength(std::uint32_t nx, std::uint32_t channelType) {
    // A row of a channel is its width times the channel's own width.
    return static_cast<std::size_t>(nx) * channelType;
}

std::vector<std::size_t> EXRLoader::pizReadOffsets(std::uint32_t columns, std::uint32_t lines,
                                                   std::uint32_t channelType,
                                                   std::size_t channelCount) {
    std::vector<std::size_t> offsets;
    offsets.reserve(static_cast<std::size_t>(lines) * channelCount);
    // Each channel's read position begins where its own samples begin.
    std::vector<std::size_t> ends = pizChannelStarts(columns, lines, channelType, channelCount);
    const std::size_t rowLength = pizRowLength(columns, channelType);
    // Rows outside, channels inside: a row is taken from every channel before the next row.
    for (std::uint32_t y = 0; y < lines; ++y) {
        for (std::size_t c = 0; c < channelCount; ++c) {
            offsets.push_back(ends[c]);
            ends[c] += rowLength;
        }
    }
    return offsets;
}

bool EXRLoader::pizHasOddColumn(std::uint32_t nx, int step) {
    // A bit test: the row must have the step's own bit set. A remainder test would be wrong.
    return (static_cast<int>(nx) & step) != 0;
}

EXRLoader::PizOffsets EXRLoader::pizOffsets(std::size_t px, std::size_t ox1, std::size_t oy1) {
    PizOffsets offsets;
    offsets.p00 = px;
    offsets.p01 = px + ox1;
    offsets.p10 = px + oy1;
    // One stride of each, which is the same as the second plus the first stride.
    offsets.p11 = offsets.p10 + ox1;
    return offsets;
}

const std::vector<int>& EXRLoader::pizWriteOrder() {
    // The four places, read and written in the same order.
    static const std::vector<int> order{0, 1, 2, 3};
    return order;
}

EXRLoader::WaveletPair EXRLoader::wdec16(int l, int h) {
    // Both inputs are taken as unsigned sixteen bit words.
    const int m = l & kWaveletModMask;
    const int d = h & kWaveletModMask;
    // The difference is halved and taken back off, then the offset is added back in. Both
    // results wrap rather than being clamped.
    const int bb = (m - (d >> 1)) & kWaveletModMask;
    const int aa = (d + bb - kWaveletOffset) & kWaveletModMask;
    WaveletPair pair;
    pair.a = aa;
    pair.b = bb;
    return pair;
}

bool EXRLoader::pizHasOddRow(std::uint32_t ny, int step) {
    // The same bit test as the column one, on the other side.
    return (static_cast<int>(ny) & step) != 0;
}

EXRLoader::WaveletStart EXRLoader::pizWaveletStart(int extent) {
    WaveletStart start;
    int p = 1;
    while (p <= extent) p <<= 1;
    p >>= 1;
    start.previous = p;
    p >>= 1;
    start.step = p < 1 ? 0 : p;
    return start;
}

void EXRLoader::wav2Decode(std::vector<std::int32_t>& buffer, std::size_t j, std::uint32_t nx,
                           std::size_t ox, std::uint32_t ny, std::size_t oy, int maxValue) {
    const bool narrow = !pizWaveletWide(maxValue);
    const int extent = pizWaveletExtent(nx, ny);
    WaveletStart start = pizWaveletStart(extent);
    int p = start.step;
    int p2 = start.previous;
    while (p >= 1) {
        // The strides at this step and the one before it.
        const std::size_t oy1 = oy * static_cast<std::size_t>(p);
        const std::size_t oy2 = oy * static_cast<std::size_t>(p2);
        const std::size_t ox1 = ox * static_cast<std::size_t>(p);
        const std::size_t ox2 = ox * static_cast<std::size_t>(p2);
        // Rows whose squares are complete.
        std::size_t py = 0;
        const std::size_t ey = py + oy * static_cast<std::size_t>(ny - static_cast<std::uint32_t>(p2));
        for (; py <= ey; py += oy2) {
            std::size_t px = py;
            const std::size_t ex =
                py + ox * static_cast<std::size_t>(nx - static_cast<std::uint32_t>(p2));
            for (; px <= ex; px += ox2) {
                const PizOffsets at = pizOffsets(px, ox1, oy1);
                const auto step = [&](int a, int b) {
                    return narrow ? wdec16(a, b) : wdec14(a, b);
                };
                // The two pairs of the first pass, then the two of the second.
                const WaveletPair first = step(buffer[at.p00 + j], buffer[at.p10 + j]);
                const WaveletPair second = step(buffer[at.p01 + j], buffer[at.p11 + j]);
                const WaveletPair top = step(static_cast<int>(first.a), static_cast<int>(second.a));
                const WaveletPair bottom = step(static_cast<int>(first.b), static_cast<int>(second.b));
                buffer[at.p00 + j] = top.a;
                buffer[at.p01 + j] = top.b;
                buffer[at.p10 + j] = bottom.a;
                buffer[at.p11 + j] = bottom.b;
            }
            // A row that is not a multiple of the step keeps its last column.
            if (pizHasOddColumn(nx, p)) {
                const std::size_t p10 = px + oy1;
                const WaveletPair pair = narrow ? wdec16(buffer[px + j], buffer[p10 + j])
                                                : wdec14(buffer[px + j], buffer[p10 + j]);
                buffer[p10 + j] = pair.b;
                buffer[px + j] = pair.a;
            }
        }
        // A side that is not a multiple of the step keeps a whole row at the bottom.
        if (pizHasOddRow(ny, p)) {
            std::size_t px = py;
            const std::size_t ex =
                py + ox * static_cast<std::size_t>(nx - static_cast<std::uint32_t>(p2));
            for (; px <= ex; px += ox2) {
                const std::size_t p01 = px + ox1;
                const WaveletPair pair = narrow ? wdec16(buffer[px + j], buffer[p01 + j])
                                                : wdec14(buffer[px + j], buffer[p01 + j]);
                buffer[p01 + j] = pair.b;
                buffer[px + j] = pair.a;
            }
        }
        p2 = p;
        p >>= 1;
    }
}

std::string EXRLoader::compressionNameOf(int code) {
    // The ten the format defines, by their own numbers rather than by the order of the names.
    switch (code) {
        case kNoCompressionCode: return kNoCompression;
        case kRleCompressionCode: return kRleCompression;
        case kZipsCompressionCode: return kZipsCompression;
        case kZipCompressionCode: return "ZIP_COMPRESSION";
        case kPizCompressionCode: return "PIZ_COMPRESSION";
        case kPxr24CompressionCode: return "PXR24_COMPRESSION";
        case kB44CompressionCode: return "B44_COMPRESSION";
        case kB44aCompressionCode: return "B44A_COMPRESSION";
        case kDwaaCompressionCode: return "DWAA_COMPRESSION";
        case kDwabCompressionCode: return "DWAB_COMPRESSION";
        default: return std::string();
    }
}

bool EXRLoader::tiledFlag(int flags) { return (flags & 0x1) != 0; }
bool EXRLoader::hasLongNames(int flags) { return (flags & 0x4) != 0; }
bool EXRLoader::hasDeepData(int flags) { return (flags & 0x8) != 0; }

bool EXRLoader::unusedFlagSet(int flags) {
    // The bits the format leaves unset are all but these four.
    return (flags & ~0x1Fu) != 0;
}

std::string EXRLoader::hufTableMessage() {
    return "THREE.EXRLoader: Something wrong with hufUnpackEncTable";
}

EXRLoader::EncTableResult EXRLoader::hufUnpackEncTable(const std::vector<std::uint8_t>& data,
                                                       std::size_t& byteOffset, int& liveBits,
                                                       std::uint32_t& accumulator,
                                                       std::size_t compressedSize, std::uint32_t im,
                                                       std::uint32_t iM) {
    EncTableResult result;
    result.lengths.assign(kHufEncSize, 0);
    const std::size_t start = byteOffset;
    // Six bits per symbol, with two special lengths standing for runs.
    const auto take = [&](int count) {
        std::uint32_t value = 0;
        for (int i = 0; i < count; ++i) {
            if (liveBits == 0) {
                // A new byte is taken from the stream and its bits are all still to come.
                if (byteOffset >= data.size()) return -1;
                accumulator = data[byteOffset++];
                liveBits = 8;
            }
            value = (value << 1) | ((accumulator >> (liveBits - 1)) & 1u);
            liveBits -= 1;
        }
        return static_cast<int>(value);
    };
    for (std::uint32_t symbol = im; symbol <= iM; ++symbol) {
        // The stream must still have room for what is about to be read.
        if (byteOffset - start > compressedSize) return result;
        const int length = take(6);
        if (length < 0) return result;
        result.lengths[symbol] = length;
        if (length == 63) {
            // The long run: eight more bits say how long it is.
            if (byteOffset - start > compressedSize) {
                result.error = hufTableMessage();
                return result;
            }
            const int extra = take(8);
            if (extra < 0) {
                result.error = hufTableMessage();
                return result;
            }
            const int run = longZeroRunLength(static_cast<std::uint32_t>(extra));
            if (symbol + static_cast<std::uint32_t>(run) > iM + 1) {
                result.error = hufTableMessage();
                return result;
            }
            for (int i = 0; i < run; ++i) result.lengths[symbol++] = 0;
            symbol -= 1;
        } else if (length >= 59) {
            // The short run: the length itself says how long.
            const int run = length - 59 + 2;
            if (symbol + static_cast<std::uint32_t>(run) > iM + 1) {
                result.error = hufTableMessage();
                return result;
            }
            for (int i = 0; i < run; ++i) result.lengths[symbol++] = 0;
            symbol -= 1;
        }
    }
    result.valid = true;
    return result;
}

std::string EXRLoader::hufEntryMessage() { return "THREE.EXRLoader: Invalid table entry"; }

EXRLoader::DecTableResult EXRLoader::hufBuildDecTable(const std::vector<int>& codes,
                                                      std::uint32_t im, std::uint32_t iM) {
    DecTableResult result;
    result.entries = emptyDecodeTable();
    for (std::uint32_t symbol = im; symbol <= iM; ++symbol) {
        const int packed = codes[symbol];
        const int code = hufCode(packed);
        const int length = hufLength(packed);
        // A code that does not fit inside its own length cannot name anything.
        if (length > 0 && (code >> length) != 0) {
            result.error = hufEntryMessage();
            return result;
        }
        if (length > kHufDecodeBits) {
            // Too long for the first level: the slot its leading bits reach holds a list.
            const std::size_t slot =
                static_cast<std::size_t>(code >> (length - kHufDecodeBits));
            if (slot >= result.entries.size()) {
                result.error = hufEntryMessage();
                return result;
            }
            DecodeEntry& entry = result.entries[slot];
            if (entry.length != 0) {
                result.error = hufEntryMessage();
                return result;
            }
            entry.hasCandidates = true;
            entry.candidates.push_back(static_cast<int>(symbol));
        } else if (length > 0) {
            // Short enough: every slot the code's bits can reach names it.
            const int span = 1 << (kHufDecodeBits - length);
            const std::size_t base = static_cast<std::size_t>(code) << (kHufDecodeBits - length);
            for (int i = 0; i < span; ++i) {
                const std::size_t slot = base + static_cast<std::size_t>(i);
                if (slot >= result.entries.size()) {
                    result.error = hufEntryMessage();
                    return result;
                }
                DecodeEntry& entry = result.entries[slot];
                if (entry.length != 0 || !entry.candidates.empty()) {
                    result.error = hufEntryMessage();
                    return result;
                }
                entry.length = length;
                entry.literal = static_cast<int>(symbol);
            }
        }
    }
    result.valid = true;
    return result;
}

int EXRLoader::hufPadBits(std::uint32_t usedBits) {
    // The block says how many bits it uses; whatever the last byte holds beyond that is padding.
    return static_cast<int>((8u - usedBits) & 7u);
}

std::size_t EXRLoader::hufFlushIndex(std::uint32_t accumulator, int liveBits) {
    // The held bits are moved up to the table's width, so a symbol shorter than it lands in the
    // slot its leading bits name, exactly as a short code was stored.
    const std::uint32_t moved = accumulator << (kHufDecodeBits - liveBits);
    return static_cast<std::size_t>(moved & static_cast<std::uint32_t>(kHufDecodeMask));
}

bool EXRLoader::hufCandidateMatches(int code, int length, std::uint32_t accumulator,
                                    int liveBits) {
    // Only the code's own length is compared, taken from the top of what is held.
    if (length <= 0 || liveBits < length) return false;
    const std::uint32_t mask = (1u << length) - 1u;
    return static_cast<std::uint32_t>(code) == ((accumulator >> (liveBits - length)) & mask);
}

std::string EXRLoader::hufDecodeMessage() { return "THREE.EXRLoader: hufDecode issues"; }

bool EXRLoader::hufIsRunMarker(int symbol, int marker) {
    // The marker is compared as an integer like any other symbol.
    return symbol == marker;
}

int EXRLoader::hufRunLength(std::uint32_t rawBits) {
    // Kept to one byte, which is what putting the number into a byte sized array does upstream.
    return static_cast<int>(rawBits & 0xFFu);
}

bool EXRLoader::hufRoomFor(std::size_t written, std::size_t wanted, std::size_t room) {
    // A run that would reach past the room is reported rather than clipped.
    return written + wanted <= room;
}

EXRLoader::DecodeResult EXRLoader::hufDecode(const std::vector<int>& codes,
                                             const std::vector<DecodeEntry>& table,
                                             const std::vector<std::uint8_t>& data,
                                             std::size_t byteOffset, int liveBitsIn,
                                             std::uint32_t accumulatorIn, std::uint32_t usedBits,
                                             int runMarker, std::vector<std::int32_t>& out,
                                             std::size_t room) {
    DecodeResult result;
    std::size_t written = 0;
    std::uint32_t c = accumulatorIn;
    int lc = liveBitsIn;
    std::size_t at = byteOffset;
    // The block ends where its own bit count says, rounded up to a whole byte.
    const std::size_t end = byteOffset + (usedBits + 7) / 8;
    // One symbol is written, with the run marker repeating the byte just written instead.
    const auto emit = [&](int symbol) {
        if (hufIsRunMarker(symbol, runMarker)) {
            const int count = hufRunLength(c);
            if (!hufRoomFor(written, static_cast<std::size_t>(count), room)) return false;
            const std::int32_t last = written > 0 ? out[written - 1] : 0;
            for (int i = 0; i < count; ++i) out[written++] = last;
            return true;
        }
        if (!hufRoomFor(written, 1, room)) return false;
        out[written++] = symbol;
        return true;
    };
    while (at < end) {
        // A byte is gathered, its bits going below the ones already held.
        c = (c << 8) | data[at++];
        lc += 8;
        while (lc >= kHufDecodeBits) {
            const std::size_t index =
                static_cast<std::size_t>((c >> (lc - kHufDecodeBits)) & kHufDecodeMask);
            if (index >= table.size()) {
                result.error = hufDecodeMessage();
                return result;
            }
            const DecodeEntry& entry = table[index];
            if (entry.length != 0) {
                // One symbol, taken at once.
                lc -= entry.length;
                if (!emit(entry.literal)) {
                    result.error = hufDecodeMessage();
                    return result;
                }
            } else {
                // Candidates, tried in the order the table kept them.
                if (entry.candidates.empty()) {
                    result.error = hufDecodeMessage();
                    return result;
                }
                bool matched = false;
                for (const int symbol : entry.candidates) {
                    const int packed = codes[static_cast<std::size_t>(symbol)];
                    const int length = hufLength(packed);
                    if (hufCandidateMatches(hufCode(packed), length, c, lc)) {
                        lc -= length;
                        if (!emit(symbol)) {
                            result.error = hufDecodeMessage();
                            return result;
                        }
                        matched = true;
                        break;
                    }
                }
                if (!matched) {
                    result.error = hufDecodeMessage();
                    return result;
                }
            }
        }
    }
    // The padding at the end of the last byte is dropped before the rest is read.
    const int pad = hufPadBits(usedBits);
    c >>= pad;
    lc -= pad;
    while (lc > 0) {
        const std::size_t index = hufFlushIndex(c, lc);
        if (index >= table.size()) break;
        const DecodeEntry& entry = table[index];
        if (entry.length == 0 || entry.length > lc) break;
        lc -= entry.length;
        if (!emit(entry.literal)) {
            result.error = hufDecodeMessage();
            return result;
        }
    }
    result.valid = true;
    result.written = written;
    return result;
}

EXRLoader::HuffResult EXRLoader::hufUncompress(const std::vector<std::uint8_t>& data,
                                               std::size_t byteOffset, std::uint32_t compressedSize,
                                               std::size_t room) {
    HuffResult result;
    const std::size_t start = byteOffset;
    // Each of the three numbers is four bytes, high first, and each is padded out by four.
    const auto readWord = [&](std::size_t at) {
        std::uint32_t value = 0;
        for (int i = 0; i < 4; ++i) {
            if (at + static_cast<std::size_t>(i) >= data.size()) return value;
            value = (value << 8) | data[at + static_cast<std::size_t>(i)];
        }
        return value;
    };
    const std::uint32_t im = readWord(byteOffset);
    const std::uint32_t iM = readWord(byteOffset + 4);
    const std::uint32_t nBits = readWord(byteOffset + 12);
    byteOffset += 16;
    // Both counts must lie inside the alphabet.
    if (!hufRangeValid(im, iM)) {
        result.error = hufRangeMessage();
        return result;
    }
    // The bit count must fit the bytes that remain after the header.
    const std::size_t used = byteOffset - start;
    const std::size_t remaining = compressedSize > used ? compressedSize - used : 0;
    if (!hufBitsFit(nBits, remaining)) {
        result.error = hufBitsMessage();
        return result;
    }
    // The encoded lengths, then the table built from them, then the codes themselves.
    const std::size_t tableBytes = compressedSize > used ? compressedSize - used : 0;
    std::size_t offset = byteOffset;
    int liveBits = 0;
    std::uint32_t accumulator = 0;
    const EncTableResult lengths =
        hufUnpackEncTable(data, offset, liveBits, accumulator, tableBytes, im, iM);
    if (!lengths.valid) {
        result.error = lengths.error.empty() ? hufTableMessage() : lengths.error;
        return result;
    }
    const DecTableResult table = hufBuildDecTable(lengths.lengths, im, iM);
    if (!table.valid) {
        result.error = table.error;
        return result;
    }
    // The run marker is the far end of the range the block named.
    result.values.assign(room, 0);
    const DecodeResult decoded = hufDecode(lengths.lengths, table.entries, data, offset, liveBits,
                                           accumulator, nBits, static_cast<int>(iM),
                                           result.values, room);
    if (!decoded.valid) {
        result.error = decoded.error;
        return result;
    }
    result.values.resize(decoded.written);
    result.valid = true;
    return result;
}

EXRLoader::PizResult EXRLoader::uncompressPIZ(const std::vector<std::uint8_t>& data,
                                              std::size_t byteOffset, std::uint32_t compressedSize,
                                              std::uint32_t columns, std::uint32_t lines,
                                              std::uint32_t channelType, std::size_t channelCount) {
    PizResult result;
    const std::size_t start = byteOffset;
    // The two ends of the bitmap range come first, each two bytes high first.
    const auto readWord16 = [&](std::size_t at) {
        std::uint32_t value = 0;
        for (int i = 0; i < 2; ++i) {
            if (at + static_cast<std::size_t>(i) >= data.size()) return value;
            value = (value << 8) | data[at + static_cast<std::size_t>(i)];
        }
        return value;
    };
    const int minNonZero = static_cast<int>(readWord16(byteOffset));
    const int maxNonZero = static_cast<int>(readWord16(byteOffset + 2));
    byteOffset += 4;
    if (!pizBitmapFits(maxNonZero)) {
        result.error = "THREE.EXRLoader: Something is wrong with PIZ_COMPRESSION BITMAP_SIZE";
        return result;
    }
    // The bitmap follows, one byte per position in the range.
    std::vector<std::uint8_t> bitmap(static_cast<std::size_t>(kPizBitmapSize), 0);
    const int run = pizBitmapRunLength(minNonZero, maxNonZero);
    for (int i = 0; i < run; ++i) {
        const std::size_t slot = byteOffset++;
        if (slot >= data.size()) {
            result.error = "THREE.EXRLoader: Something is wrong with PIZ_COMPRESSION BITMAP_SIZE";
            return result;
        }
        bitmap[static_cast<std::size_t>(minNonZero + i)] = data[slot];
    }
    // The table from the bitmap, and the largest value it holds.
    const ReverseLut reversed = reverseLutFromBitmap(bitmap);
    const int maxValue = reversed.lastIndex;
    // Then four bytes saying how long the encoded table is, and the huffman stage itself.
    std::uint32_t tableBytes = 0;
    for (int i = 0; i < 4; ++i) {
        if (byteOffset < data.size()) tableBytes = (tableBytes << 8) | data[byteOffset++];
    }
    const std::size_t total = static_cast<std::size_t>(columns) * lines * channelType * channelCount;
    const HuffResult packed =
        hufUncompress(data, byteOffset, compressedSize - static_cast<std::uint32_t>(byteOffset - start), total);
    if (!packed.valid) {
        result.error = packed.error;
        return result;
    }
    // The values arrive as thirty two bit numbers so the signed wavelet can work on them.
    std::vector<std::int32_t> working(packed.values.begin(), packed.values.end());
    working.resize(total, 0);
    // The wavelet runs over each channel's own words, one pass per word of the channel.
    const std::size_t rowWords = pizRowLength(columns, channelType);
    const std::vector<std::size_t> starts = pizChannelStarts(columns, lines, channelType, channelCount);
    for (std::size_t channel = 0; channel < channelCount; ++channel) {
        for (std::uint32_t word = 0; word < channelType; ++word) {
            wav2Decode(working, starts[channel] + word, columns, channelType, lines, rowWords,
                       maxValue);
        }
    }
    // The table is applied back over what the wavelet produced.
    std::vector<std::uint16_t> values(working.size(), 0);
    for (std::size_t i = 0; i < working.size(); ++i) {
        values[i] = static_cast<std::uint16_t>(working[i] & 0xFFFF);
    }
    applyLut(reversed.lut, values);
    result.values = values;
    result.valid = true;
    return result;
}