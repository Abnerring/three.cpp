#include "loaders/HDRLoader.h"

#include <cmath>
#include <cstdlib>
#include <regex>
#include <stdexcept>

namespace {
// Reads one line and advances the cursor past its newline.
bool fgets(const std::vector<std::uint8_t>& buffer, std::size_t& pos, std::string& line) {
    if (pos >= buffer.size()) return false;
    const std::size_t start = pos;
    while (pos < buffer.size() && buffer[pos] != '\n') ++pos;
    line.assign(reinterpret_cast<const char*>(buffer.data() + start), pos - start);
    if (pos < buffer.size()) ++pos;  // consume the newline
    return true;
}

[[noreturn]] void fail(const char* message) {
    throw std::runtime_error(std::string("THREE.HDRLoader: ") + message);
}
}  // namespace

void HDRLoader::rgbeToFloat(const std::uint8_t* source, std::size_t offset, float* dest,
                            std::size_t destOffset) {
    const double e = source[offset + 3];
    const double scale = std::pow(2.0, e - 128.0) / 255.0;
    dest[destOffset + 0] = static_cast<float>(source[offset + 0] * scale);
    dest[destOffset + 1] = static_cast<float>(source[offset + 1] * scale);
    dest[destOffset + 2] = static_cast<float>(source[offset + 2] * scale);
    dest[destOffset + 3] = 1.0f;
}

HDRLoader::Result HDRLoader::parse(const std::vector<std::uint8_t>& buffer) const {
    static const std::regex magicToken("^#\\?(\\S+)");
    static const std::regex gammaRe("^\\s*GAMMA\\s*=\\s*(\\d+(\\.\\d+)?)\\s*$");
    static const std::regex exposureRe("^\\s*EXPOSURE\\s*=\\s*(\\d+(\\.\\d+)?)\\s*$");
    static const std::regex formatRe("^\\s*FORMAT=(\\S+)\\s*$");
    static const std::regex dimensionsRe("^\\s*-Y\\s+(\\d+)\\s+\\+X\\s+(\\d+)\\s*$");

    Result result;
    Header& header = result.header;
    std::size_t pos = 0;
    std::string line;
    std::smatch match;

    if (!fgets(buffer, pos, line) || !std::regex_search(line, match, magicToken))
        fail("bad initial token");
    header.hasProgramType = true;
    header.programtype = match[1].str();

    while (fgets(buffer, pos, line)) {
        if (!line.empty() && line[0] == '#') {
            header.comments += line + "\n";
            continue;
        }
        if (std::regex_match(line, match, gammaRe)) header.gamma = std::stod(match[1].str());
        if (std::regex_match(line, match, exposureRe)) header.exposure = std::stod(match[1].str());
        if (std::regex_match(line, match, formatRe)) {
            header.hasFormat = true;
            header.format = match[1].str();
        }
        if (std::regex_match(line, match, dimensionsRe)) {
            header.hasDimensions = true;
            header.height = std::stoi(match[1].str());
            header.width = std::stoi(match[2].str());
        }
        // Upstream stops as soon as both required fields have been seen.
        if (header.hasFormat && header.hasDimensions) break;
    }

    if (!header.hasFormat) fail("missing format specifier");
    if (!header.hasDimensions) fail("missing image size specifier");

    const int w = header.width;
    const int h = header.height;
    std::vector<std::uint8_t> scanline;
    std::vector<std::uint8_t> rgbe;  // four channel planes for one scanline
    result.data.assign(std::size_t(w) * h * 4, 0.0f);

    // Run length encoding is only used for widths in the documented range and when the
    // scanline marker is present; otherwise the pixels are stored flat.
    const bool useRle = !(w < 8 || w > 0x7fff) && pos + 4 <= buffer.size() &&
                        buffer[pos] == 2 && buffer[pos + 1] == 2 && (buffer[pos + 2] & 0x80) == 0;

    if (!useRle) {
        const std::size_t need = std::size_t(w) * h * 4;
        if (pos + need > buffer.size()) fail("unexpected end of pixel data");
        for (std::size_t i = 0; i < std::size_t(w) * h; ++i)
            rgbeToFloat(buffer.data(), pos + i * 4, result.data.data(), i * 4);
        return result;
    }

    rgbe.assign(std::size_t(w) * 4, 0);
    int numScanlines = h;
    while (numScanlines > 0 && pos < buffer.size()) {
        if (pos + 4 > buffer.size()) fail("unexpected end of pixel data");
        const std::uint8_t b0 = buffer[pos++];
        const std::uint8_t b1 = buffer[pos++];
        const std::uint8_t b2 = buffer[pos++];
        const std::uint8_t b3 = buffer[pos++];
        if (b0 != 2 || b1 != 2 || ((b2 << 8) | b3) != w) fail("bad rgbe scanline format");

        const std::size_t ptrEnd = std::size_t(w) * 4;
        std::size_t ptr = 0;
        while (ptr < ptrEnd && pos < buffer.size()) {
            std::size_t count = buffer[pos++];
            const bool isEncodedRun = count > 128;
            if (isEncodedRun) count -= 128;
            if (count == 0) continue;
            if (isEncodedRun) {
                if (pos >= buffer.size()) fail("unexpected end of pixel data");
                const std::uint8_t value = buffer[pos++];
                for (std::size_t i = 0; i < count && ptr < ptrEnd; ++i) rgbe[ptr++] = value;
            } else {
                for (std::size_t i = 0; i < count && ptr < ptrEnd; ++i) rgbe[ptr++] = buffer[pos++];
            }
        }

        const std::size_t row = std::size_t(h - numScanlines);
        for (int x = 0; x < w; ++x) {
            const std::uint8_t pixel[4] = {rgbe[x], rgbe[w + x], rgbe[2 * w + x], rgbe[3 * w + x]};
            rgbeToFloat(pixel, 0, result.data.data(), (row * std::size_t(w) + x) * 4);
        }
        --numScanlines;
    }
    return result;
}
