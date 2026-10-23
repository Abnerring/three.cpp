#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Port of the small bit helpers in examples/jsm/loaders/EXRLoader.js at the pinned revision. The
// loader itself is one long function that reads a whole file, so the parts reached first are the
// pieces of bit arithmetic the Huffman decoding rests on.
class EXRLoader {
  public:
    // A Huffman table entry packs two numbers into one word: the low six bits hold the length of
    // the code and the rest hold the code itself.
    static int hufLength(int code);
    static int hufCode(int code);

    // Whether an entry is usable. A code that needs more bits than its length allows is invalid,
    // and so is a length of zero, since no code can be empty.
    static bool hufEntryValid(int code);

    // The four byte magic the format begins with, read little endian.
    static constexpr std::uint32_t kMagic = 20000630;
    // The version byte the format understands.
    static constexpr int kVersion = 2;
    // The order an eight by eight block is read in before its inverse transform. The values are
    // the positions the block's numbers are stored at, walking the square from its top left
    // corner in diagonals, which is why the sequence is not monotonic and why the last two
    // entries are the last two positions. The table is a permutation: every index from zero to
    // sixty three appears exactly once.
    static const std::array<int, 64>& unZigZagOrder();

    // The position the k-th number of a block is stored at.
    static int unZigZagIndex(int k);
    // The low sixteen bits of a value, and the same bits read as a signed number. The second is
    // what turns a stored unsigned half into a signed coefficient, so a value above the signed
    // maximum is pulled down by a whole range rather than clamped.
    static int uint16Bits(int value);
    static int int16Bits(int value);

    // One step of the inverse wavelet transform, which takes a low and a high coefficient and
    // gives back two neighbouring samples. The high coefficient carries the parity that tells
    // the pair apart, which is why half of it is added rather than all.
    struct WaveletPair {
        int a = 0;
        int b = 0;
    };

    static WaveletPair wdec14(int l, int h);

    // One pixel turned back from luminance and two colour differences. The three are read before
    // any is written, since the outputs overwrite the inputs; the coefficients are the ones the
    // source hard codes for the standard's conversion.
    struct YcbcrPixel {
        double y = 0;
        double cb = 0;
        double cr = 0;
    };

    static void csc709Inverse(YcbcrPixel& pixel);
    static void csc709InverseBlock(YcbcrPixel* pixels, std::size_t count);
    // A single precision number as the sixteen bit pattern of the nearest half. The rounding is
    // to nearest with ties going to the even pattern, which is what the note in the source is
    // about: truncating instead puts the output a few units in the last place away from the
    // reference implementation, and those units show up in the decoded image.
    //
    // The same three cases the source handles come up here. A number too large for a half becomes
    // infinity, and a pattern that is already not a number stays one. A number too small for a
    // normal half is nudged by half a step before the exponent is removed, which forces it onto
    // the coarser grid the subnormal patterns use. Everything else is rounded by adding half a
    // step less one and then the lowest bit of the result, so an exact tie lands on the even
    // pattern rather than always away from zero.
    static std::uint16_t floatToHalfBits(float value);
    static constexpr std::uint16_t kHalfInfinity = 0x7c00;
    static constexpr std::uint16_t kHalfNotANumber = 0x7e00;
    // The size of the value range a lookup table spans.
    static constexpr int kUshortRange = 1 << 16;

    // The reverse of a bitmap table, which lists the values whose bit is set. Two details are
    // easy to miss: the value zero is always listed whatever its bit says, and the count returned
    // is one less than the number listed, so it reads as the last index rather than a length.
    struct ReverseLut {
        std::vector<std::uint16_t> lut;
        int lastIndex = 0;
    };

    static ReverseLut reverseLutFromBitmap(const std::vector<std::uint8_t>& bitmap);

    // Replaces each value by the one the table gives for it, in place. A value outside the table
    // reads nothing and is left alone.
    static void applyLut(const std::vector<std::uint16_t>& lut,
                         std::vector<std::uint16_t>& data);

    // Undoes a differencing pass: each value becomes the running total of everything up to it,
    // with one hundred and twenty eight taken off for every step after the first. The pass has to
    // run forwards because each step reads what the last one wrote.
    static void predictor(std::vector<std::uint16_t>& source);

    // Deals a scalar run into its output, taking alternately from the two halves of the source.
    // With an odd length the first half holds the extra one, which is why the second starts at
    // the size plus one halved.
    static void interleaveScalar(const std::vector<std::uint16_t>& source,
                                 std::vector<std::uint16_t>& out);
    // Expands a run length encoded stream. A signed header byte says which of the two forms
    // follows: negative means that many bytes are copied verbatim, the count being the header
    // negated, and non-negative means one byte is read and repeated one more time than the
    // header. The two counts therefore differ by one, and a header of zero still repeats its
    // value once rather than writing nothing. The stream ends when its declared size runs out
    // rather than when the buffer does.
    static std::vector<std::uint8_t> decodeRunLength(const std::vector<std::uint8_t>& source);
    // The inverse of the discrete cosine transform on an eight by eight block, done as one pass
    // along the rows and another down the columns.
    //
    // The constants are cosines of multiples of pi over sixteen, halved, but the pi they are
    // taken with is written to five places rather than to full precision, so the results differ
    // from a textbook transform in the last few bits. That is kept as it is, since the whole
    // point is to produce the same numbers the reference implementation produces.
    static void dctInverse(double data[64]);
    // The longest code the format's Huffman tables can hold, which is also the size of the table
    // the counts are kept in.
    static constexpr int kHufMaxCodeLength = 58;

    // Turns a list of code lengths into the packed entries the rest of the decoder reads, each
    // being the code above the length the way hufCode and hufLength read them.
    //
    // Canonical codes are assigned so that a shorter code sorts before every longer one and codes
    // of equal length follow each other in order. Counting the lengths and walking down from the
    // longest gives each length the first code available to it, and the halving is what makes a
    // code one bit shorter start where the codes below it would have run out. A length of zero
    // marks a symbol that does not appear and is left as it is.
    static void hufCanonicalCodeTable(std::vector<int>& hcode);
    // Expands the run length form the alternating coefficients are stored in. The scan starts at
    // the second coefficient, since the first is written separately, and three kinds of word can
    // appear: one that ends the block, one that says how many coefficients to skip, and one that
    // is a coefficient itself. The ending word has to be tested first, because it also looks like
    // a skip of zero once the top byte is looked at on its own.
    //
    // A skip may carry the position past the end, in which case the loop stops rather than
    // writing outside the block, and the position is left where the skip put it.
    struct AcScan {
        std::vector<std::uint16_t> values;
        std::size_t wordsRead = 0;
        int finalPosition = 1;
    };

    static AcScan unRleAC(const std::vector<std::uint16_t>& acBuffer, std::size_t start);
    // The size of the first level of the decoding table, in bits. A code no longer than this is
    // read from the table directly; a longer one uses its top bits to find a second table.
    static constexpr int kHufDecodeBits = 14;
    static constexpr int kHufDecodeSize = 1 << kHufDecodeBits;
    static constexpr int kHufDecodeMask = kHufDecodeSize - 1;

    // Whether a code of this length needs the second level.
    static bool hufUsesSubTable(int length);

    // Where in the first level a code of this length is found. A short code is looked up by the
    // whole of its value; a long one by the bits above the first level.
    static int hufSubTableIndex(int code, int length);
    // The state a bit stream is read through. The accumulate word is kept signed, and shifted
    // with the arithmetic shift, because that is what the source does: the word is filled eight
    // bits at a time without ever being masked, so its top bit can be set and a signed shift then
    // brings ones in above the bits being asked for. Masking after the shift is what makes the
    // answer right in spite of that, and reproducing both halves of the pair matters.
    struct BitState {
        std::int32_t accumulate = 0;
        int liveBits = 0;
    };

    // One byte read eight bits at a time, as the stream needs them.
    static BitState getChar(BitState state, const std::vector<std::uint8_t>& bytes,
                            std::size_t& offset);

    // A run of bits, taken from the top of what has been accumulated.
    struct GetBitsResult {
        BitState state;
        int value = 0;
    };

    static GetBitsResult getBits(int nBits, BitState state, const std::vector<std::uint8_t>& bytes,
                                 std::size_t& offset);
    // How many bytes a run of bits occupies. A run that does not fill its last byte still
    // occupies it, which is why seven is added before dividing.
    static std::size_t bitByteCount(int bits);

    // Where in the first level of the decoding table a word is looked up. The word is shifted
    // down until fourteen bits remain above the ones already consumed and then masked, so the
    // answer is the top fourteen of what is live.
    static int decodeTableIndex(std::int32_t accumulate, int liveBits);
    // The number of symbols a Huffman table may describe.
    static constexpr int kHufEncSize = 1 << 16;

    // Whether the two ends of the symbol range a table describes are usable. Both are read from
    // the file and both have to fall inside the table, which is what the first check is for.
    static bool hufRangeValid(std::uint32_t im, std::uint32_t iM);

    // Whether a run of bits fits in the bytes that are left. The check is made after the table
    // itself has been unpacked rather than before, since unpacking consumes bytes and the count
    // that remains is what the bits have to fit into.
    static bool hufBitsFit(std::uint32_t nBits, std::size_t remainingBytes);

    // The two messages the wrapper can raise.
    static std::string hufRangeMessage();
    static std::string hufBitsMessage();
    // The six bit symbols that stand for runs of absent codes rather than for a length. The five
    // values from the first of them to the last name runs of two to five, and the last alone
    // means a run long enough to need eight more bits.
    static constexpr int kShortZeroCodeRun = 59;
    static constexpr int kLongZeroCodeRun = 63;
    static constexpr int kShortestLongRun = 2 + kLongZeroCodeRun - kShortZeroCodeRun;

    // What a six bit symbol means.
    enum class ZeroRun { None, Short, Long };

    static ZeroRun zeroRunKind(int symbol);

    // How many absent codes a short run stands for. The first of the five values means two, so
    // the count is the symbol less the first value plus two.
    static int shortZeroRunLength(int symbol);

    // How many absent codes a long run stands for, given the eight bits that follow it.
    static int longZeroRunLength(int extra);
    // Where the header begins, after the four byte magic and the two bytes of version and
    // specification.
    static constexpr std::size_t kHeaderStart = 8;

    // Whether a buffer begins with the magic. The check looks at the first four bytes only, so a
    // file cut short is reported by the length elsewhere rather than here.
    static bool looksLikeExr(const std::vector<std::uint8_t>& buffer);

    // What the specification byte says about the file. The four flags start at the second bit;
    // the first is not used by any of them.
    struct HeaderFlags {
        bool singleTile = false;
        bool longName = false;
        bool deepFormat = false;
        bool multiPart = false;
    };

    static HeaderFlags headerFlags(std::uint8_t spec);

    // The two messages the header can raise.
    static std::string notExrMessage();
    static std::string noPartsMessage();
    // A string read up to a null byte, which is how the format writes the names and types in a
    // header. The offset is left just past the null, so the next field begins where this one
    // ended.
    static std::string readNullTerminated(const std::vector<std::uint8_t>& buffer,
                                          std::size_t& offset);

    // Whether a part header is worth returning. A part that listed no attribute at all is not,
    // and the answer is null rather than an empty object. Note that a part whose attributes were
    // all of an unknown type still counts as having listed one, so it comes back as an empty
    // object instead.
    static bool partHasAttributes(std::size_t attributeCount);

    // The warning an attribute of a type the reader does not know produces. Such an attribute is
    // skipped and the parser carries on, so a file with something new in it still loads.
    static std::string unknownAttributeWarning(const std::string& type);
    // The kinds of value a header attribute can hold. Three of the type names share the first
    // kind, since a fixed length string is what all three of them are on disk.
    enum class AttributeKind {
        FixedString,
        ChannelList,
        Chromaticities,
        Compression,
        Box,
        EnvironmentMap,
        TileDescription,
        LineOrder,
        Float,
        Vector2,
        Vector3,
        Integer,
        Rational,
        Unknown,
    };

    // The kind a type name stands for. The comparison is exact, and a name that is not one of
    // the thirteen gives Unknown, which the caller reports and skips.
    static AttributeKind attributeKind(const std::string& type);

    // Whether a kind names something the reader can handle. Only Unknown cannot.
    static bool attributeKindKnown(AttributeKind kind);
    // The names a compression code stands for, in code order.
    static std::string compressionName(std::uint8_t code);

    // The names a line order stands for. There are three.
    static std::string lineOrderName(std::uint8_t code);

    // The names an environment map stands for. There are two.
    static std::string envmapName(std::uint8_t code);

    // A box read from four signed numbers. The order they arrive in is the minimum of each axis
    // followed by the maximum of each, not the two corners one after the other, which is the easy
    // thing to get wrong.
    struct Box2i {
        std::int32_t xMin = 0;
        std::int32_t yMin = 0;
        std::int32_t xMax = 0;
        std::int32_t yMax = 0;
    };

    // Fills a box from four numbers already read in that order.
    static Box2i boxFromNumbers(std::int32_t first, std::int32_t second, std::int32_t third,
                                std::int32_t fourth);
    // One entry of a channel list.
    struct Channel {
        std::string name;
        std::int32_t pixelType = 0;
        std::uint8_t pLinear = 0;
        std::int32_t xSampling = 0;
        std::int32_t ySampling = 0;
    };

    // How many bytes an entry of a channel list takes. After the name and its terminator come a
    // four byte type, one byte of a flag, three reserved bytes and two four byte sampling rates,
    // which is seventeen bytes that do not depend on the name.
    static std::size_t channelEntrySize(std::size_t nameLength);

    // How far the entry loop runs. The list ends with a null byte, which the loop does not read:
    // the bound is one short of the declared size, and the null is stepped over afterwards.
    static bool channelEntryInRange(std::size_t position, std::size_t start, std::size_t size);

    // The step taken after the loop, which is the terminator the bound kept out.
    static constexpr std::size_t kChlistTerminator = 1;
    // The three reserved bytes inside an entry.
    static constexpr std::size_t kChannelReserved = 3;
    // The four points that describe a colour space, each as a pair of coordinates. The pairs
    // arrive together, red then green then blue then white, rather than as four x values and
    // then four y values.
    struct Chromaticities {
        double redX = 0, redY = 0;
        double greenX = 0, greenY = 0;
        double blueX = 0, blueY = 0;
        double whiteX = 0, whiteY = 0;
    };

    static Chromaticities chromaticitiesFromNumbers(const double numbers[8]);

    // The three ways a file may describe its levels.
    static std::string levelModeName(int code);

    // The two ways it may round a tile size.
    static std::string roundingModeName(int code);

    // A tile description packs both of those into one byte: the low four bits are the level mode
    // and the high four the rounding mode.
    struct TileDescription {
        std::uint32_t xSize = 0;
        std::uint32_t ySize = 0;
        std::string levelMode;
        std::string roundingMode;
    };

    static TileDescription tileDescription(std::uint32_t xSize, std::uint32_t ySize,
                                           std::uint8_t modes);
    // How many times a run length symbol repeats. The bits are shifted down to the live ones and
    // then cut to a byte, so a count that would not fit is taken modulo two hundred and fifty
    // six rather than refused, which is what putting it through a one byte array does in the
    // source.
    static int runLengthCount(std::int32_t accumulate, int liveBits);

    // Whether a run can be written without reaching past the end of the output. A run of zero is
    // always allowed, since it writes nothing.
    static bool runFits(std::size_t outOffset, int count, std::size_t outEnd);

    // Writes a byte the given number of times. The byte written is the one already at the end of
    // the output, so a run of zero does not read it at all and an empty output has nothing to
    // repeat.
    static void writeRun(std::vector<std::uint8_t>& out, int count);
    // The size of a float and of an int in the file.
    static constexpr std::size_t kFloatSize = 4;
    static constexpr std::size_t kIntSize = 4;

    // A count and a divisor. The two are read with different signs: the first as a signed number
    // and the second as an unsigned one, so a rational with a negative count is possible while a
    // negative divisor is not.
    struct Rational {
        std::int32_t numerator = 0;
        std::uint32_t denominator = 0;
    };

    static Rational rationalFrom(std::int32_t numerator, std::uint32_t denominator);

    // A string of a stated length. Unlike the names in the header this one does not stop at a
    // zero byte: exactly the number of bytes the attribute declares are taken, so an embedded
    // zero stays in the middle of the result rather than ending it.
    static std::string fixedLengthString(const std::vector<std::uint8_t>& buffer,
                                         std::size_t offset, std::size_t size);

    // How far an offset moves after reading a float or an int.
    static std::size_t advanceF32(std::size_t offset);
    static std::size_t advanceU32(std::size_t offset);
    // The two ways a block of coefficients may itself be compressed.
    static constexpr int kStaticHuffman = 0;
    static constexpr int kDeflate = 1;

    // What a block says about its own compression. A code outside the two leaves the reader with
    // nothing to unpack, which the source allows to happen rather than refusing the file, since
    // its switch has no default.
    enum class BlockCompression { None, Huffman, Deflate };

    static BlockCompression blockCompression(int code);

    // Whether a block of the three is present. The three are read independently, each gated on
    // its own size rather than on a count, so a file may carry any mixture of them.
    static bool blockPresent(std::uint32_t size);

    // How far the offset moves after a block. The source advances by a different amount for the
    // two kinds of the first block: by the compressed size for the huffman one and by the
    // uncompressed count for the deflate one, which is a discrepancy kept as it is.
    static std::size_t advanceAfterAc(BlockCompression kind, std::uint32_t compressedSize,
                                      std::uint32_t uncompressedCount);
    // One entry of the first level of the decoding table. An entry either names a symbol directly,
    // with a length and a count of one, or holds no length and instead names a run of candidate
    // symbols that have to be tried in turn. The two uses never share an entry, which is why a
    // second attempt to fill one with a length is an error.
    struct DecodeEntry {
        int length = 0;
        int literal = 0;
        // Whether a run of candidates is held rather than a single symbol.
        bool hasCandidates = false;
        std::vector<int> candidates;
    };

    // Whether an entry is free to take a code of this length. An entry already holding a length
    // cannot take another, whether it names one symbol or a run.
    static bool decodeEntryFree(const DecodeEntry& entry);

    // Whether an entry now needs a run of candidates, which is what a code longer than the first
    // level puts there.
    static bool decodeEntryWantsCandidates(int codeLength);

    // A fresh table, every entry empty.
    static std::vector<DecodeEntry> emptyDecodeTable();
    // Undoes the byte predictor a compressed block was written with. Each byte becomes the sum of
    // itself, the one before it and a constant, and because the block holds bytes the sum is kept
    // to eight bits: it wraps rather than saturating. The first byte is left alone.
    static void revertPredictor(std::vector<std::uint8_t>& data);

    // Puts the two halves of a block back into their original order, taking one byte from the
    // first half and then one from the second, until the block is filled. The second half begins
    // at the halfway point rounded up, so an odd length gives a shorter first half.
    static std::vector<std::uint8_t> interleaveScalar(const std::vector<std::uint8_t>& source);

    // The offset the second half of a block begins at.
    static std::size_t interleaveSplit(std::size_t length);
    // A block compressed the way a zip file compresses one: the bytes are deflated, then a
    // predictor is applied to the raw bytes, then the two halves are separated so that like
    // bytes lie together before deflating does its work. Reading it back undoes those in the
    // opposite order, and the answer is the block as the file meant it.
    struct ZipBlockResult {
        bool valid = false;
        std::vector<std::uint8_t> bytes;
        std::string error;
    };

    static ZipBlockResult uncompressZIP(const std::vector<std::uint8_t>& block);
    // A block compressed with runs: a negative count introduces that many bytes as they are,
    // and a count of zero or more introduces that many plus one copies of the byte after it. The
    // predictor and the interleave then undo exactly as they do for a zipped block.
    struct RleBlockResult {
        bool valid = false;
        std::vector<std::uint8_t> bytes;
        std::string error;
    };

    static RleBlockResult uncompressRLE(const std::vector<std::uint8_t>& block);
    // The compression a block was written with, named rather than numbered. The three the deep
    // path can read are these; the rest are turned away with a message naming the one asked for.
    static constexpr const char* kNoCompression = "NO_COMPRESSION";
    static constexpr const char* kRleCompression = "RLE_COMPRESSION";
    static constexpr const char* kZipsCompression = "ZIPS_COMPRESSION";

    // Which of the three kinds a name stands for. A name outside them is Unsupported rather
    // than being guessed at, since a wrong guess would produce pixels rather than an error.
    enum class CompressionKind { None, RunLength, Zip, Unsupported };

    static CompressionKind compressionKind(const std::string& name);

    // The message the deep path raises for a compression it cannot read, which names the one it
    // was given. Note that the single and multi scanline zips are different names, so a block
    // that says ZIP is not read here.
    static std::string unsupportedDeepMessage(const std::string& name);
    // A multipart file writes a four byte part number before each chunk, and the reader steps
    // over it rather than reading it, so the part is implied by the order of the chunks rather
    // than by anything in them.
    static constexpr int kPartPrefixSize = 4;

    // How far a chunk's reader starts past the first of the chunk's own data: four bytes when
    // the file has more than one part and nothing when it has one.
    static std::size_t chunkPrefix(bool multiPart);

    // The image row a chunk begins at. The chunk writes the absolute row and the reader takes
    // the window's own first row off it, so a window that does not start at zero still gives
    // rows counted from zero.
    static int rowWithinWindow(int writtenRow, int windowFirstRow);

    // The number of samples a row holds, read from the last of the cumulative counts. An empty
    // row gives zero and its data block is stepped over rather than read.
    static std::uint32_t totalSamples(const std::vector<std::uint32_t>& cumulativeCounts);

    // The cumulative count table is written with each count in the machine's own byte order,
    // which is the little one, while the rest of the file is the other way round. Reading it the
    // file's usual way would turn a small count into a huge one.
    static std::uint32_t cumulativeCount(std::uint32_t b0, std::uint32_t b1, std::uint32_t b2,
                                         std::uint32_t b3);
    // The wavelet compression keeps a bitmap of which of the sixty five thousand and thirty six
    // possible byte values a block actually uses. The bitmap is stored as bits, so it is eight
    // thousand one hundred and ninety two bytes long.
    static constexpr int kPizBitmapSize = 8192;

    // How many bytes of the bitmap the block writes. The two ends are read as sixteen bit
    // values; when the first is past the second the block writes nothing at all, which is how a
    // block that uses none of the values is stored rather than by writing a range of zeroes.
    static int pizBitmapRunLength(int minNonZero, int maxNonZero);

    // How much room a channel's samples take. Each channel of the block is the whole window
    // again, so the size grows by the window's area times the channel's width in sixteen bit
    // words.
    static std::size_t pizChannelSize(std::uint32_t columns, std::uint32_t lines,
                                      std::uint32_t channelType);

    // Where each channel's samples begin, accumulated in the order the channels are written.
    static std::vector<std::size_t> pizChannelStarts(std::uint32_t columns, std::uint32_t lines,
                                                     std::uint32_t channelType,
                                                     std::size_t channelCount);

    // Whether the two ends name a bitmap that fits. A second end at or past the bitmap's own
    // length is refused rather than read.
    static bool pizBitmapFits(int maxNonZero);
    // The wavelet step works on the smaller side of the window, so a window wider than it is
    // tall is stepped along its height and the other way about.
    static int pizWaveletExtent(std::uint32_t nx, std::uint32_t ny);

    // The first step the wavelet takes. It begins at one and doubles until it passes the extent,
    // comes back down one place, and is then halved again, so it lands on half of the largest
    // power of two that does not exceed the extent. An extent of one leaves no step to take.
    static int pizWaveletFirstStep(int extent);

    // Whether the block's largest value needs the wider arithmetic. Values below sixteen
    // thousand three hundred and eighty four are handled without it.
    static bool pizWaveletWide(int maxValue);

    // The number of sixteen bit words a row of a channel occupies.
    static std::size_t pizRowLength(std::uint32_t nx, std::uint32_t channelType);
    // The order the samples are read back out of the wavelet buffer, as the index each read
    // begins at. The outer run is the rows and the inner one the channels, so a row is taken
    // from every channel before the next row is touched, and each channel's own read position
    // moves down one row's worth at a time. The result holds one entry per row and channel.
    static std::vector<std::size_t> pizReadOffsets(std::uint32_t columns, std::uint32_t lines,
                                                   std::uint32_t channelType,
                                                   std::size_t channelCount);
    // Whether a row has an odd sample left over at this step. The test is a bit test rather than
    // a remainder: the row must have the step's own bit set, so for a step of four only rows of
    // four or more with that bit count, not every row that is odd.
    static bool pizHasOddColumn(std::uint32_t nx, int step);

    // The four places one step of the wavelet touches, given the place it starts at and the two
    // strides. The fourth is one stride of each away, which is the same as the second plus the
    // first stride.
    struct PizOffsets {
        std::size_t p00 = 0;
        std::size_t p01 = 0;
        std::size_t p10 = 0;
        std::size_t p11 = 0;
    };

    static PizOffsets pizOffsets(std::size_t px, std::size_t ox1, std::size_t oy1);

    // The order the four places are written back in, which is the same order they were read, so
    // a reader can tell that no place is written before it has been read. The pairs are the two
    // of the first pass and then the two of the second.
    static const std::vector<int>& pizWriteOrder();
    // The narrow form of the wavelet step. It works on the values as unsigned sixteen bit words
    // and takes everything modulo the same, so a value that would go below zero comes back round
    // to the top rather than being clamped. The wide form above works on signed ones.
    static constexpr int kWaveletModMask = 0xffff;
    static constexpr int kWaveletOffset = 1 << 15;

    static WaveletPair wdec16(int l, int h);

    // Whether a row or a column has a leftover at this step. Both tests are bit tests, and both
    // are needed: the column one closes the row and the row one closes the block.
    static bool pizHasOddRow(std::uint32_t ny, int step);
    // The wavelet itself, over one channel of a block. The buffer holds the block as sixteen bit
    // words, j picks which word of each sample this pass works on, and the two strides say how
    // far apart neighbouring samples lie. The step halves from the largest power of two the
    // smaller side allows down to one, and each step touches the four corners of every square of
    // its own size, with two leftovers for when a side is not a multiple of it.
    static void wav2Decode(std::vector<std::int32_t>& buffer, std::size_t j, std::uint32_t nx,
                           std::size_t ox, std::uint32_t ny, std::size_t oy, int maxValue);

    // The two step counts a pass begins with: the last step taken and the one before it. The
    // second is the largest power of two the side allows and the first is half of that.
    struct WaveletStart {
        int step = 0;
        int previous = 0;
    };

    static WaveletStart pizWaveletStart(int extent);
    // The number a file writes for each compression, and the name the loader gives it. The
    // numbers are the format's own and do not follow the order the names are listed in: the two
    // zips are two and three, the two B44s six and seven, and the two deep ones eight and nine.
    static constexpr int kNoCompressionCode = 0;
    static constexpr int kRleCompressionCode = 1;
    static constexpr int kZipsCompressionCode = 2;
    static constexpr int kZipCompressionCode = 3;
    static constexpr int kPizCompressionCode = 4;
    static constexpr int kPxr24CompressionCode = 5;
    static constexpr int kB44CompressionCode = 6;
    static constexpr int kB44aCompressionCode = 7;
    static constexpr int kDwaaCompressionCode = 8;
    static constexpr int kDwabCompressionCode = 9;

    // The name a number stands for, or an empty string when the number is not one the format
    // defines.
    static std::string compressionNameOf(int code);

    // Seven bit of the header's flags are unused and must be zero. The first is the tiled flag.
    static bool tiledFlag(int flags);
    static bool hasLongNames(int flags);
    static bool hasDeepData(int flags);
    static bool unusedFlagSet(int flags);
    // The encoded table a block carries: for each symbol between the two ends, six bits saying
    // how long its code is, with two special lengths standing for runs of symbols that do not
    // appear at all. A run of the short kind is written in the six bits themselves; a run of the
    // long kind follows with eight more bits saying how long it is.
    struct EncTableResult {
        bool valid = false;
        std::vector<int> lengths;
        std::string error;
    };

    static EncTableResult hufUnpackEncTable(const std::vector<std::uint8_t>& data,
                                            std::size_t& byteOffset, int& liveBits,
                                            std::uint32_t& accumulator, std::size_t compressedSize,
                                            std::uint32_t im, std::uint32_t iM);

    // The error the unpacker raises when a run would reach past the far end.
    static std::string hufTableMessage();
    // Builds the decoding table from the lengths the block carried. A code of fourteen bits or
    // fewer fills every slot its bits can reach, so the rest of the table is a wildcard, while a
    // longer code puts a list of candidates in the one slot its leading bits reach and leaves
    // the rest to be sorted out as the bits arrive. Both refuse a slot that is already taken,
    // and a code that does not fit inside its own length is refused outright.
    struct DecTableResult {
        bool valid = false;
        std::vector<DecodeEntry> entries;
        std::string error;
    };

    static DecTableResult hufBuildDecTable(const std::vector<int>& codes, std::uint32_t im,
                                           std::uint32_t iM);

    // The error building the table raises.
    static std::string hufEntryMessage();
    // How many bits of padding the last byte of a block carries. The block says how many bits of
    // it are used, and the rest are dropped before the final symbols are read.
    static int hufPadBits(std::uint32_t usedBits);

    // Where the bits still held are looked up once the block has run out. They are moved up to
    // the top of the table's width so that the slots they can reach are the ones the leading
    // bits name, which is the same arrangement the short codes fill.
    static std::size_t hufFlushIndex(std::uint32_t accumulator, int liveBits);

    // Whether a long code is the one the bits spell. Its own length says how many of the held
    // bits to compare, and only those.
    static bool hufCandidateMatches(int code, int length, std::uint32_t accumulator, int liveBits);

    // The error the decode loop raises when a slot's candidates all disagree.
    static std::string hufDecodeMessage();
    // A symbol equal to the block's run marker does not name a value of its own. Instead the
    // next eight bits say how many copies of the byte just written to follow, and the run is
    // written by repeating that byte. Everything else names a value directly.
    static bool hufIsRunMarker(int symbol, int marker);

    // How many copies a run marker stands for. The bits are taken as a whole number and then
    // kept to one byte, so a count that would need more than eight bits comes back truncated
    // rather than being refused.
    static int hufRunLength(std::uint32_t rawBits);

    // Whether a run or a single value still fits in the room the caller left. A block that would
    // overrun is reported by the function returning false rather than by raising anything.
    static bool hufRoomFor(std::size_t written, std::size_t wanted, std::size_t room);
    // The decode loop. Bits are gathered a byte at a time and fourteen at a time are looked up
    // in the table; a slot naming one symbol is taken at once, while a slot holding candidates
    // is searched one by one over each candidate's own length. Once the block runs out the bits
    // left are moved up to the table's width and read the same way until none remain.
    struct DecodeResult {
        bool valid = false;
        std::size_t written = 0;
        std::string error;
    };

    static DecodeResult hufDecode(const std::vector<int>& codes,
                                  const std::vector<DecodeEntry>& table,
                                  const std::vector<std::uint8_t>& data, std::size_t byteOffset,
                                  int liveBits, std::uint32_t accumulator, std::uint32_t usedBits,
                                  int runMarker, std::vector<std::int32_t>& out, std::size_t room);
    // The whole of the huffman stage: two counts naming the range of symbols the block carries,
    // then how many bits of code follow, then the encoded lengths and the codes themselves. The
    // two counts must lie inside the alphabet and the bit count must fit the bytes that remain,
    // both being refused before any work is done.
    struct HuffResult {
        bool valid = false;
        std::vector<std::int32_t> values;
        std::string error;
    };

    static HuffResult hufUncompress(const std::vector<std::uint8_t>& data,
                                    std::size_t byteOffset, std::uint32_t compressedSize,
                                    std::size_t room);
    // The wavelet compression, end to end: the bitmap of values used, the table built from it,
    // the huffman codes, the wavelet over each channel and the table applied back. The values
    // are worked through as thirty two bit numbers because one of the two wavelet forms is
    // signed, and are cut back to sixteen for the table.
    struct PizResult {
        bool valid = false;
        std::vector<std::uint16_t> values;
        std::string error;
    };

    static PizResult uncompressPIZ(const std::vector<std::uint8_t>& data, std::size_t byteOffset,
                                   std::uint32_t compressedSize, std::uint32_t columns,
                                   std::uint32_t lines, std::uint32_t channelType,
                                   std::size_t channelCount);
    // The seven ways a block can be compressed, which the ten numbers in a file reduce to: the
    // two zips share one reader, the two B44s another, and the two deep ones a third, so a file
    // naming any of ten numbers lands on one of seven. When the same file mixed parts of
    // different kinds it would still be safe, since the reader is chosen by the name rather than
    // by the part.
    enum class Decompresser { None, RunLength, Zip, Wavelet, Pxr, B44, Deep };

    static Decompresser decompresserFor(int code);
    // The pixel rate compression keeps each channel's values as separate planes of one byte per
    // pixel, one plane for the top byte, one for the next and so on, and writes the difference
    // between a value and the one before it rather than the value. A half precision channel
    // therefore takes two planes and a full precision one three, the lowest byte of a full
    // precision value never being kept at all, which is what makes the compression lossy.
    static int pxrPlaneCount(int pixelType);

    // The difference one pixel contributes, taken from a byte of each plane. A half precision
    // channel gives sixteen bits and a full precision one twenty four, the bottom eight being
    // left as zero.
    static std::uint32_t pxrDifference(int pixelType, std::uint32_t plane0, std::uint32_t plane1,
                                       std::uint32_t plane2);

    // Where each plane of one row begins, given where the row's planes begin.
    static std::vector<std::size_t> pxrPlaneStarts(std::size_t rowStart, std::uint32_t columns,
                                                   int pixelType);

    // How far the planes for one row move the read position on.
    static std::size_t pxrRowBytes(std::uint32_t columns, int pixelType);
    // The pixel rate compression end to end: the block is a deflated run of byte planes, and
    // each row's channel is a difference from the value before it, summed as it is read and
    // written back little endian. A difference is the running total's own step, so the sum
    // starts again at zero for each row of each channel rather than running across the block.
    struct PxrResult {
        bool valid = false;
        std::vector<std::uint32_t> values;
        std::string error;
    };

    static PxrResult uncompressPXR(const std::vector<std::uint8_t>& block,
                                   const std::vector<int>& pixelTypes, std::uint32_t columns,
                                   std::uint32_t lines);
    // The block compression keeps half precision channels as four by four blocks of fourteen
    // bytes, each block holding its own range, so no two blocks need agree. Every other channel
    // is not compressed at all but stored as it lies, one scanline after another.
    static constexpr int kB44BlockExtent = 4;
    static constexpr int kB44BlockBytes = 14;

    // The number of bytes one sample of a channel takes. The format counts a half as one and a
    // full precision value as two, so the bytes are that count doubled.
    static int b44PixelSize(int pixelType);

    // The size a channel reaches once its sampling is applied. A channel sampled every second
    // pixel has half as many samples, rounded up.
    static std::uint32_t b44ChannelExtent(std::uint32_t size, int sampling);

    // Whether a channel is sampled at every pixel in both directions, which is the only case in
    // which its samples are written to the output rather than stepped over.
    static bool b44IsFullResolution(int xSampling, int ySampling);

    // Whether a channel is compressed at all. Only the half precision ones are; the rest are
    // stored as they lie.
    static bool b44IsCompressed(int pixelType);

    // How many blocks a side of a channel needs, rounded up.
    static std::uint32_t b44BlockCount(std::uint32_t extent);

    // Whether a block may be written flat, which only the second of the two block kinds allows.
    static bool b44AllowsFlat(bool isB44a);
    // A block stores its sixteen samples in an order that puts negative numbers below positive
    // ones rather than round a sign bit, so that neighbouring values differ in few bits whichever
    // side of zero they lie on. Turning one back into a half puts the sign where it belongs: a
    // value with its top bit set is positive and loses that bit, and one without it is negative
    // and has every bit turned over.
    static std::uint16_t b44OrderedToHalf(std::uint16_t ordered);
};
