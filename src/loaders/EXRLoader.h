#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

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
};
