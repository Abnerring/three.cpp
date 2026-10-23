#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// A decoder for the zlib format, which wraps a deflate stream in a two byte header and a four
// byte checksum. The port has no library for this and several loaders need one, so the pieces are
// being written here: this part covers the header and the checksum, which can be done and checked
// without any of the decompression itself.
class Zlib {
  public:
    // The header of a zlib stream.
    struct Header {
        // The compression method, which must be eight for deflate.
        int method = 0;
        // The window size, written as a power of two less eight, so a value of seven means a
        // window of thirty two thousand bytes.
        int windowBits = 0;
        // Whether a dictionary is required before the stream can be read.
        bool needsDictionary = false;
        // The level hint, which the format carries but decoders may ignore.
        int levelHint = 0;
    };

    // Reads the two byte header. The two bytes hold a checksum of their own, so a header that
    // fails it is refused; the flag says whether that happened.
    struct HeaderResult {
        bool valid = false;
        Header header;
        std::string error;
    };

    static HeaderResult readHeader(std::uint8_t cmf, std::uint8_t flg);

    // The checksum a stream ends with, computed over the data it produced. Two running sums are
    // kept modulo the largest prime below sixty five thousand five hundred and thirty six, and
    // the answer packs the second above the first.
    static constexpr std::uint32_t kAdlerBase = 65521;

    static std::uint32_t adler32(const std::vector<std::uint8_t>& data);
    static std::uint32_t adler32Update(std::uint32_t running, const std::vector<std::uint8_t>& data);

    // The window size a header's exponent names.
    static std::size_t windowSize(int windowBits);

    // The two errors the header can raise.
    static std::string methodError();
    static std::string checkError();
    // A bit reader for a deflate stream. The bits of a byte come out lowest first, which is the
    // opposite of how the EXR loader reads its stream, and the two must not be confused: the same
    // bytes read the other way give different values.
    struct BitReader {
        const std::vector<std::uint8_t>* bytes = nullptr;
        std::size_t offset = 0;
        // Bits taken from the current byte, from zero to seven.
        int used = 0;
    };

    // One bit, then up to sixteen at a time, taken from the low end of each byte.
    static int readBit(BitReader& reader);
    static std::uint32_t readBits(BitReader& reader, int count);

    // Drops the bits left in the current byte so the next read begins on a byte boundary.
    static void alignToByte(BitReader& reader);

    // Whether the reader has anything left at all.
    static bool hasMoreBits(const BitReader& reader);

    // The header of a block: whether it is the last and which of the three kinds it is.
    enum class BlockType { Stored, Fixed, Dynamic, Reserved };

    struct BlockHeader {
        bool last = false;
        BlockType type = BlockType::Reserved;
        // The two kind bits as they were read, which the caller may find useful.
        int typeBits = 0;
    };

    static BlockHeader readBlockHeader(BitReader& reader);

    // A stored block's two lengths. The second is the first with every bit inverted, so a block
    // whose lengths do not agree is corrupt; the flag says whether they agree.
    struct StoredHeader {
        std::uint32_t length = 0;
        std::uint32_t inverse = 0;
        bool valid = false;
    };

    static StoredHeader readStoredHeader(BitReader& reader);
    // The number of bits the fixed literal codes use for a symbol. The alphabet is split into
    // four runs: the first hundred and forty four use eight bits, the next hundred and twelve use
    // nine, the next twenty four use seven and the last eight use eight again.
    static int fixedLiteralLength(int symbol);

    // The fixed distance codes are all five bits long.
    static constexpr int kFixedDistanceLength = 5;

    // A code with its bits turned round. A code is written in the specification most significant
    // bit first, while the stream carries the least significant bit first, so every code has to
    // be reversed before it is compared with what the stream gives.
    static std::uint32_t reverseBits(std::uint32_t value, int count);

    // The fixed literal codes, one per symbol, as the stream carries them.
    static std::vector<std::uint32_t> fixedLiteralCodes();

    // The fixed distance codes, one per symbol, likewise.
    static std::vector<std::uint32_t> fixedDistanceCodes();
    // A table that turns what the stream gives back into a symbol. It is built from the lengths
    // only, so the codes follow the canonical assignment and are reversed for the stream.
    struct HuffmanTable {
        // The longest code in the table, which bounds the search.
        int maxLength = 0;
        // For each length, a map from the code as the stream carries it to the symbol it names.
        // A flat vector of pairs keeps this small; the tables here have at most a few hundred
        // entries.
        std::vector<std::vector<std::pair<std::uint32_t, int>>> byLength;
    };

    // Builds a table from an alphabet's code lengths. A length of zero means the symbol does not
    // appear and is not entered.
    static HuffmanTable buildTable(const std::vector<int>& lengths);

    // Reads bits until they name a symbol. The bits are taken one at a time, each new one going
    // above the ones already taken, because the stream hands out the lowest first while a code
    // reads from its own lowest end. The answer says whether a symbol was found; an impossible
    // stream gives none rather than looping for ever.
    struct SymbolResult {
        bool found = false;
        int symbol = -1;
        int bitsUsed = 0;
    };

    static SymbolResult decodeSymbol(BitReader& reader, const HuffmanTable& table);
};
