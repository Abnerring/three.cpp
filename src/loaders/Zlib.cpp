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
