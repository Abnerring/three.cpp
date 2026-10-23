#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
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
};
