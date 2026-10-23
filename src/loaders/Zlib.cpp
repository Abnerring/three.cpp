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

Zlib::HuffmanTable Zlib::buildTable(const std::vector<int>& lengths) {
    HuffmanTable table;
    table.byLength.resize(16);
    // How many symbols use each length.
    int counts[16] = {};
    for (const int length : lengths) {
        if (length > 0 && length < 16) counts[length]++;
    }
    // The canonical first code of each length.
    std::uint32_t next[16] = {};
    std::uint32_t code = 0;
    counts[0] = 0;
    for (int bits = 1; bits < 16; ++bits) {
        code = (code + static_cast<std::uint32_t>(counts[bits - 1])) << 1;
        next[bits] = code;
        if (counts[bits] > 0 && bits > table.maxLength) table.maxLength = bits;
    }
    // Each symbol takes the next code of its length, turned round for the stream.
    for (std::size_t symbol = 0; symbol < lengths.size(); ++symbol) {
        const int length = lengths[symbol];
        if (length <= 0 || length >= 16) continue;
        const std::uint32_t reversed = reverseBits(next[length]++, length);
        table.byLength[static_cast<std::size_t>(length)].emplace_back(
            reversed, static_cast<int>(symbol));
    }
    return table;
}

Zlib::SymbolResult Zlib::decodeSymbol(BitReader& reader, const HuffmanTable& table) {
    SymbolResult result;
    std::uint32_t code = 0;
    // One bit at a time, each new one above those already taken.
    for (int length = 1; length <= table.maxLength; ++length) {
        // A stream that has run out cannot name anything, so the padding a read past the
        // end would give must not be allowed to match.
        if (!hasMoreBits(reader)) return result;
        const int bit = readBit(reader);
        code |= static_cast<std::uint32_t>(bit) << (length - 1);
        // The table for this length is checked as soon as it is reachable.
        const auto& row = table.byLength[static_cast<std::size_t>(length)];
        for (const auto& entry : row) {
            if (entry.first == code) {
                result.found = true;
                result.symbol = entry.second;
                result.bitsUsed = length;
                return result;
            }
        }
    }
    // Nothing matched within the longest code, so the stream is impossible.
    return result;
}

const std::array<int, 19>& Zlib::codeLengthOrder() {
    // The order the lengths arrive in, which is the order of how often each is expected.
    static const std::array<int, 19> order{16, 17, 18, 0, 8,  7, 9,  6, 10, 5,
                                           11, 4,  12, 3, 13, 2, 14, 1, 15};
    return order;
}

Zlib::DynamicCounts Zlib::dynamicCounts(std::uint32_t hlit, std::uint32_t hdist,
                                        std::uint32_t hclen) {
    DynamicCounts counts;
    // Each arrives with the bias the format adds to keep small values out of the field.
    counts.literalCount = static_cast<int>(hlit) + 257;
    counts.distanceCount = static_cast<int>(hdist) + 1;
    counts.codeLengthCount = static_cast<int>(hclen) + 4;
    return counts;
}

int Zlib::repeatCount(int code, std::uint32_t extra) {
    // Sixteen copies the previous length a few more times; the other two repeat zeros, over two
    // different ranges.
    if (code == 16) return static_cast<int>(extra) + 3;
    if (code == 17) return static_cast<int>(extra) + 3;
    if (code == 18) return static_cast<int>(extra) + 11;
    return 0;
}

std::vector<int> Zlib::readCodeLengthLengths(BitReader& reader, int count) {
    std::vector<int> lengths(19, 0);
    // The lengths arrive in their own order, three bits each, and only the first few are sent.
    for (int i = 0; i < count && i < 19; ++i) {
        lengths[static_cast<std::size_t>(codeLengthOrder()[static_cast<std::size_t>(i)])] =
            static_cast<int>(readBits(reader, 3));
    }
    return lengths;
}

Zlib::LengthsResult Zlib::expandLengths(BitReader& reader, const HuffmanTable& codeLengthTable,
                                        int total) {
    LengthsResult result;
    while (static_cast<int>(result.lengths.size()) < total) {
        // A symbol is either a length or a run.
        const auto symbol = decodeSymbol(reader, codeLengthTable);
        if (!symbol.found) {
            result.error = "Zlib: the lengths ran out.";
            return result;
        }
        if (symbol.symbol < 16) {
            result.lengths.push_back(symbol.symbol);
            continue;
        }
        // A run needs something to repeat, so the very first symbol may not be one of these.
        if (result.lengths.empty() && symbol.symbol == 16) {
            result.error = repeatFirstError();
            return result;
        }
        // The extra bits say how long the run is.
        const int extraBits = symbol.symbol == 16 ? 2 : (symbol.symbol == 17 ? 3 : 7);
        const std::uint32_t extra = readBits(reader, extraBits);
        const int run = repeatCount(symbol.symbol, extra);
        // A run may not reach past the number of lengths wanted.
        if (static_cast<int>(result.lengths.size()) + run > total) {
            result.error = overrunError();
            return result;
        }
        // Sixteen copies the length before it; the other two write zeros.
        const int value = symbol.symbol == 16 ? result.lengths.back() : 0;
        for (int i = 0; i < run; ++i) result.lengths.push_back(value);
    }
    result.valid = true;
    return result;
}

Zlib::SplitLengths Zlib::splitLengths(const std::vector<int>& lengths, int literalCount,
                                      int distanceCount) {
    SplitLengths split;
    // The first count gives the literal table and the next gives the distance one. A vector that
    // is short simply gives a shorter second half rather than reading past its end.
    for (int i = 0; i < literalCount && i < static_cast<int>(lengths.size()); ++i) {
        split.literal.push_back(lengths[static_cast<std::size_t>(i)]);
    }
    for (int i = 0; i < distanceCount && literalCount + i < static_cast<int>(lengths.size()); ++i) {
        split.distance.push_back(lengths[static_cast<std::size_t>(literalCount + i)]);
    }
    return split;
}

std::string Zlib::repeatFirstError() {
    return "Zlib: a length run began before any length.";
}

std::string Zlib::overrunError() {
    return "Zlib: a length run reached past the count.";
}

int Zlib::lengthBase(int code) {
    // The codes start at two hundred and fifty seven; the first eight are three to ten.
    static const int base[] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17, 19, 23, 27,
                               31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    const int index = code - 257;
    if (index < 0 || index >= kLengthCodeCount) return 0;
    return base[index];
}

int Zlib::lengthExtra(int code) {
    // The first eight carry none; then each four carries one more, up to five.
    const int index = code - 257;
    if (index < 0 || index >= kLengthCodeCount) return 0;
    if (index < 8) return 0;
    // The last code is the special case and carries nothing either.
    if (index == 28) return 0;
    return (index - 8) / 4 + 1;
}

int Zlib::distanceBase(int code) {
    // The first four are one to four, then pairs reaching four times as far each time.
    static const int base[] = {1,    2,    3,    4,    5,    7,     9,     13,    17,   25,
                               33,   49,   65,   97,   129,  193,   257,   385,   513,  769,
                               1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
    if (code < 0 || code >= kDistanceCodeCount) return 0;
    return base[code];
}

int Zlib::distanceExtra(int code) {
    // The first four carry none and each pair after that carries one more.
    if (code < 0 || code >= kDistanceCodeCount) return 0;
    if (code < 4) return 0;
    return (code - 4) / 2 + 1;
}

Zlib::HuffmanTable Zlib::fixedLiteralTable() {
    std::vector<int> lengths(288, 0);
    for (int symbol = 0; symbol < 288; ++symbol) {
        lengths[static_cast<std::size_t>(symbol)] = fixedLiteralLength(symbol);
    }
    return buildTable(lengths);
}

Zlib::HuffmanTable Zlib::fixedDistanceTable() {
    return buildTable(std::vector<int>(30, kFixedDistanceLength));
}

void Zlib::copyMatch(std::vector<std::uint8_t>& out, int distance, int length) {
    // The source moves forward as bytes are written, so an overlap repeats what was just added.
    for (int i = 0; i < length; ++i) {
        const std::size_t from = out.size() - static_cast<std::size_t>(distance);
        out.push_back(out[from]);
    }
}

std::string Zlib::blockTypeError() { return "Zlib: a block named a reserved kind."; }
std::string Zlib::symbolError() { return "Zlib: the stream ended before its end marker."; }
std::string Zlib::matchError() { return "Zlib: a match reached before the start."; }

Zlib::InflateResult Zlib::inflate(const std::vector<std::uint8_t>& input) {
    InflateResult result;
    BitReader reader{&input, 0, 0};
    bool last = false;

    while (!last) {
        const BlockHeader header = readBlockHeader(reader);
        last = header.last;
        if (header.type == BlockType::Reserved) {
            result.error = blockTypeError();
            return result;
        }

        if (header.type == BlockType::Stored) {
            // A stored block lines up to a byte and copies its bytes as they are.
            alignToByte(reader);
            const StoredHeader stored = readStoredHeader(reader);
            if (!stored.valid) {
                result.error = "Zlib: a stored block's lengths did not agree.";
                return result;
            }
            for (std::uint32_t i = 0; i < stored.length; ++i) {
                result.output.push_back(static_cast<std::uint8_t>(readBits(reader, 8)));
            }
            continue;
        }

        // The two Huffman kinds differ only in where their tables come from.
        HuffmanTable literals;
        HuffmanTable distances;
        if (header.type == BlockType::Fixed) {
            literals = fixedLiteralTable();
            distances = fixedDistanceTable();
        } else {
            // The dynamic header: three counts, the code length lengths, then the lengths.
            const std::uint32_t hlit = readBits(reader, 5);
            const std::uint32_t hdist = readBits(reader, 5);
            const std::uint32_t hclen = readBits(reader, 4);
            const DynamicCounts counts = dynamicCounts(hlit, hdist, hclen);
            const auto codeLengths = readCodeLengthLengths(reader, counts.codeLengthCount);
            const HuffmanTable codeTable = buildTable(codeLengths);
            const auto expanded = expandLengths(reader, codeTable,
                                                counts.literalCount + counts.distanceCount);
            if (!expanded.valid) {
                result.error = expanded.error;
                return result;
            }
            const SplitLengths split =
                splitLengths(expanded.lengths, counts.literalCount, counts.distanceCount);
            literals = buildTable(split.literal);
            distances = buildTable(split.distance);
        }

        // Read symbols until the end marker.
        while (true) {
            const SymbolResult symbol = decodeSymbol(reader, literals);
            if (!symbol.found) {
                result.error = symbolError();
                return result;
            }
            if (symbol.symbol < 256) {
                result.output.push_back(static_cast<std::uint8_t>(symbol.symbol));
                continue;
            }
            if (symbol.symbol == 256) break;
            // A length and a distance give a run copied from what is already there.
            const int length = lengthBase(symbol.symbol) + static_cast<int>(
                readBits(reader, lengthExtra(symbol.symbol)));
            const SymbolResult distanceSymbol = decodeSymbol(reader, distances);
            if (!distanceSymbol.found) {
                result.error = symbolError();
                return result;
            }
            const int distance = distanceBase(distanceSymbol.symbol) + static_cast<int>(
                readBits(reader, distanceExtra(distanceSymbol.symbol)));
            // A distance reaching before the output would read nothing.
            if (distance <= 0 || static_cast<std::size_t>(distance) > result.output.size()) {
                result.error = matchError();
                return result;
            }
            copyMatch(result.output, distance, length);
        }
    }
    result.valid = true;
    return result;
}

std::uint32_t Zlib::adlerFromBigEndian(std::uint32_t b0, std::uint32_t b1, std::uint32_t b2,
                                       std::uint32_t b3) {
    // The first byte is the most significant, unlike every other number in the stream.
    return ((b0 & 0xFFu) << 24) | ((b1 & 0xFFu) << 16) | ((b2 & 0xFFu) << 8) | (b3 & 0xFFu);
}

std::string Zlib::shortError() { return "Zlib: the stream is too short."; }
std::string Zlib::checksumError() { return "Zlib: the checksum does not match."; }
std::string Zlib::dictionaryError() { return "Zlib: a dictionary is required."; }

Zlib::UnzlibResult Zlib::unzlib(const std::vector<std::uint8_t>& input) {
    UnzlibResult result;
    // The two byte header comes first.
    if (input.size() < 6) {
        result.error = shortError();
        return result;
    }
    const HeaderResult header = readHeader(input[0], input[1]);
    if (!header.valid) {
        result.error = header.error;
        return result;
    }
    // A stream that wants a dictionary cannot be read without one, and none is offered here.
    if (header.header.needsDictionary) {
        result.error = dictionaryError();
        return result;
    }

    // The deflate stream is everything between the header and the four byte checksum.
    const std::vector<std::uint8_t> body(input.begin() + 2, input.end() - 4);
    const InflateResult inflated = inflate(body);
    if (!inflated.valid) {
        result.error = inflated.error;
        return result;
    }

    // The checksum covers what came out, not what went in.
    const std::uint32_t stored = adlerFromBigEndian(input[input.size() - 4], input[input.size() - 3],
                                                    input[input.size() - 2], input[input.size() - 1]);
    if (adler32(inflated.output) != stored) {
        result.error = checksumError();
        return result;
    }
    result.valid = true;
    result.output = inflated.output;
    return result;
}