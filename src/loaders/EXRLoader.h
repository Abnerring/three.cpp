#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
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
};
