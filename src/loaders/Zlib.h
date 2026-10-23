#pragma once
#include <array>
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
    // The order the nineteen code-length lengths arrive in. It is not the order the symbols are
    // numbered in, and getting it wrong produces a table that decodes some streams and not
    // others, which is why it is written out here as one constant.
    static const std::array<int, 19>& codeLengthOrder();

    // The three counts at the head of a dynamic block. Each arrives with a bias, so the number of
    // literal and distance codes is the value plus two hundred and fifty seven and plus one, and
    // the number of code-length lengths is the value plus four. The largest of the first two is
    // two hundred and eighty six and thirty two.
    struct DynamicCounts {
        int literalCount = 0;
        int distanceCount = 0;
        int codeLengthCount = 0;
    };

    static DynamicCounts dynamicCounts(std::uint32_t hlit, std::uint32_t hdist, std::uint32_t hclen);

    // How many lengths a repeat code stands for. Code sixteen copies the previous length three
    // to six times and carries two bits; seventeen repeats a zero for three to ten and carries
    // three; eighteen repeats a zero for eleven to a hundred and thirty eight and carries seven.
    static int repeatCount(int code, std::uint32_t extra);
    // The nineteen code-length lengths, read three bits each in the order they arrive.
    static std::vector<int> readCodeLengthLengths(BitReader& reader, int count);

    // The lengths of the two alphabets, read through the code-length table. A repeat code that
    // copies the previous length needs one to exist, so the very first one may not be a sixteen,
    // and the run may not reach past the number of lengths wanted. The answer says whether it was
    // possible.
    struct LengthsResult {
        bool valid = false;
        std::vector<int> lengths;
        std::string error;
    };

    static LengthsResult expandLengths(BitReader& reader, const HuffmanTable& codeLengthTable,
                                       int total);

    // Splits the lengths into the two alphabets. The first literal count belong to the first and
    // the rest to the second, which is what ties the two tables to the counts read earlier.
    struct SplitLengths {
        std::vector<int> literal;
        std::vector<int> distance;
    };

    static SplitLengths splitLengths(const std::vector<int>& lengths, int literalCount,
                                     int distanceCount);

    // The two errors expanding can raise.
    static std::string repeatFirstError();
    static std::string overrunError();
    // The base length a length code stands for. The codes run from two hundred and fifty seven
    // to two hundred and eighty five in groups of four that double their reach, and the last one
    // is a special case: it has no extra bits at all and always means two hundred and fifty
    // eight. Codes above it name nothing.
    static int lengthBase(int code);

    // The extra bits a length code carries, which say how much above the base it reaches.
    static int lengthExtra(int code);

    // The base distance a distance code stands for. The thirty codes run in pairs, each pair
    // reaching four times as far as the one before.
    static int distanceBase(int code);

    // The extra bits a distance code carries.
    static int distanceExtra(int code);

    // How many length codes name something and how many distance codes there are.
    static constexpr int kLengthCodeCount = 29;
    static constexpr int kDistanceCodeCount = 30;
    // The table the fixed blocks use, built from the same lengths the literal alphabet uses.
    static HuffmanTable fixedLiteralTable();
    static HuffmanTable fixedDistanceTable();

    // Copies a run of bytes already written, which is how a match is expanded. The distance is
    // counted back from the end of the output, so the source and the destination may overlap: a
    // distance of one repeats the last byte.
    static void copyMatch(std::vector<std::uint8_t>& out, int distance, int length);

    // The result of unpacking a stream.
    struct InflateResult {
        bool valid = false;
        std::vector<std::uint8_t> output;
        std::string error;
    };

    // Unpacks a raw deflate stream, the blocks of which are the whole of the format's core. The
    // three kinds of block are all handled: a stored one copies its bytes, and the two others
    // read symbols from a table until the end marker at two hundred and fifty six.
    static InflateResult inflate(const std::vector<std::uint8_t>& input);

    // The errors unpacking can raise.
    static std::string blockTypeError();
    static std::string symbolError();
    static std::string matchError();
    // A checksum read back from four bytes. The zlib format writes it with the most significant
    // byte first, which is the opposite of the way every other number in a deflate stream is
    // written, so this one has to be assembled from the far end.
    static std::uint32_t adlerFromBigEndian(std::uint32_t b0, std::uint32_t b1, std::uint32_t b2,
                                            std::uint32_t b3);

    // The whole zlib format: a two byte header, the deflate stream, then the checksum of what
    // came out. The checksum is checked rather than merely skipped, since a stream whose data is
    // intact but whose checksum is not has been damaged and should say so.
    struct UnzlibResult {
        bool valid = false;
        std::vector<std::uint8_t> output;
        std::string error;
    };

    static UnzlibResult unzlib(const std::vector<std::uint8_t>& input);

    // The errors the wrapper can raise.
    static std::string shortError();
    static std::string checksumError();
    static std::string dictionaryError();
};
