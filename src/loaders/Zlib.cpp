#include "loaders/Zlib.h"

Zlib::HeaderResult Zlib::readHeader(std::uint8_t cmf, std::uint8_t flg) {
    HeaderResult result;
    // The two bytes must together be a multiple of thirty one.
    const unsigned combined = (static_cast<unsigned>(cmf) << 8) | static_cast<unsigned>(flg);
    if (combined % 31u != 0) {
        result.error = checkError();
        return result;
    }
    // The low nibble of the first byte names the method, and it must be deflate's.
    const int method = cmf & 0x0f;
    if (method != 8) {
        result.error = methodError();
        return result;
    }
    result.valid = true;
    result.header.method = method;
    // The high nibble is the window exponent, one more than the power it stands for.
    result.header.windowBits = (cmf >> 4) + 8;
    // The sixth bit of the second byte says a dictionary is needed.
    result.header.needsDictionary = (flg & 0x20) != 0;
    // The top two bits are the level hint.
    result.header.levelHint = flg >> 6;
    return result;
}

std::uint32_t Zlib::adler32(const std::vector<std::uint8_t>& data) {
    return adler32Update(1, data);
}

std::uint32_t Zlib::adler32Update(std::uint32_t running, const std::vector<std::uint8_t>& data) {
    // The running value carries both sums, one in each half.
    std::uint32_t a = running & 0xffffu;
    std::uint32_t b = (running >> 16) & 0xffffu;
    for (const std::uint8_t byte : data) {
        a = (a + byte) % kAdlerBase;
        b = (b + a) % kAdlerBase;
    }
    return (b << 16) | a;
}

std::size_t Zlib::windowSize(int windowBits) {
    // The exponent is at least eight and at most fifteen.
    if (windowBits < 8) return 0;
    return static_cast<std::size_t>(1) << windowBits;
}

std::string Zlib::methodError() {
    return "Zlib: the compression method is not deflate.";
}

std::string Zlib::checkError() {
    return "Zlib: the header check failed.";
}

bool Zlib::hasMoreBits(const BitReader& reader) {
    // Any bit left in the current byte, or any byte left after it.
    if (reader.bytes == nullptr) return false;
    return reader.used < 8 ? reader.offset < reader.bytes->size()
                           : reader.offset + 1 < reader.bytes->size();
}

int Zlib::readBit(BitReader& reader) {
    if (reader.bytes == nullptr || reader.offset >= reader.bytes->size()) return 0;
    // The bit is taken from the low end of the byte and the position moves up.
    const int bit = ((*reader.bytes)[reader.offset] >> reader.used) & 1;
    reader.used += 1;
    if (reader.used == 8) {
        reader.used = 0;
        reader.offset += 1;
    }
    return bit;
}

std::uint32_t Zlib::readBits(BitReader& reader, int count) {
    // Bits arrive lowest first, so each new one goes above the ones already taken.
    std::uint32_t value = 0;
    for (int i = 0; i < count; ++i) {
        value |= static_cast<std::uint32_t>(readBit(reader)) << i;
    }
    return value;
}

void Zlib::alignToByte(BitReader& reader) {
    // Whatever is left of the current byte is dropped.
    if (reader.used == 0) return;
    reader.used = 0;
    reader.offset += 1;
}

Zlib::BlockHeader Zlib::readBlockHeader(BitReader& reader) {
    BlockHeader header;
    header.last = readBit(reader) != 0;
    header.typeBits = static_cast<int>(readBits(reader, 2));
    // The three kinds by their bits; the fourth value is reserved and cannot be read.
    if (header.typeBits == 0) header.type = BlockType::Stored;
    else if (header.typeBits == 1) header.type = BlockType::Fixed;
    else if (header.typeBits == 2) header.type = BlockType::Dynamic;
    else header.type = BlockType::Reserved;
    return header;
}

Zlib::StoredHeader Zlib::readStoredHeader(BitReader& reader) {
    // The two lengths are written low byte first, and the second inverts the first.
    StoredHeader header;
    header.length = readBits(reader, 16);
    header.inverse = readBits(reader, 16);
    header.valid = (header.inverse == (~header.length & 0xFFFFu));
    return header;
}

int Zlib::fixedLiteralLength(int symbol) {
    // Outside the alphabet there is no length at all.
    if (symbol < 0 || symbol >= 288) return 0;
    // The four runs, in the order the alphabet is numbered.
    if (symbol <= 143) return 8;
    if (symbol <= 255) return 9;
    if (symbol <= 279) return 7;
    return 8;
}

std::uint32_t Zlib::reverseBits(std::uint32_t value, int count) {
    std::uint32_t out = 0;
    for (int i = 0; i < count; ++i) {
        out = (out << 1) | ((value >> i) & 1u);
    }
    return out;
}

std::vector<std::uint32_t> Zlib::fixedLiteralCodes() {
    // The codes are assigned in the canonical way: the first of a length gets the value carried
    // over from the length above, shifted up, and each next one counts on from there.
    std::vector<std::uint32_t> codes(288, 0);
    std::uint32_t next[16] = {};
    std::uint32_t code = 0;
    // A count of how many symbols use each length, then the usual walk down.
    int counts[16] = {};
    for (int symbol = 0; symbol < 288; ++symbol) counts[fixedLiteralLength(symbol)]++;
    counts[0] = 0;
    for (int bits = 1; bits < 16; ++bits) {
        code = (code + static_cast<std::uint32_t>(counts[bits - 1])) << 1;
        next[bits] = code;
    }
    // Each symbol takes the next code of its length, turned round for the stream.
    for (int symbol = 0; symbol < 288; ++symbol) {
        const int length = fixedLiteralLength(symbol);
        if (length == 0) continue;
        codes[static_cast<std::size_t>(symbol)] = reverseBits(next[length]++, length);
    }
    return codes;
}

std::vector<std::uint32_t> Zlib::fixedDistanceCodes() {
    // All thirty codes use five bits, so they simply count from zero.
    std::vector<std::uint32_t> codes(30, 0);
    for (int symbol = 0; symbol < 30; ++symbol) {
        codes[static_cast<std::size_t>(symbol)] =
            reverseBits(static_cast<std::uint32_t>(symbol), kFixedDistanceLength);
    }
    return codes;
}
