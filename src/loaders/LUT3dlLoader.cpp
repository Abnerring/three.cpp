#include "loaders/LUT3dlLoader.h"

#include "constants.h"

#include <cmath>
#include <cstdint>
#include <regex>
#include <stdexcept>
#include <vector>

LUT3dlLoader::Result LUT3dlLoader::parse(const std::string& input) const {
    static const std::regex regExpGridInfo("^[\\d ]+$", std::regex::multiline);
    static const std::regex regExpDataPoints(
        "^([\\d.e+-]+) +([\\d.e+-]+) +([\\d.e+-]+) *$", std::regex::multiline);

    std::smatch match;
    if (!std::regex_search(input, match, regExpGridInfo))
        throw std::runtime_error("THREE.LUT3dlLoader: Missing grid information");

    // The grid line lists the sample positions, so its length is the LUT size and the gap
    // between neighbours has to be constant.
    std::vector<double> gridLines;
    {
        const std::string line = match[0].str();
        static const std::regex separator("\\s+");
        for (std::sregex_token_iterator it(line.begin(), line.end(), separator, -1), end; it != end;
             ++it) {
            const std::string token = *it;
            if (!token.empty()) gridLines.push_back(std::stod(token));
        }
    }
    if (gridLines.size() < 2)
        throw std::runtime_error("THREE.LUT3dlLoader: Missing grid information");

    const double gridStep = gridLines[1] - gridLines[0];
    const std::size_t size = gridLines.size();
    const std::size_t sizeSq = size * size;

    for (std::size_t i = 1; i < gridLines.size(); ++i)
        if (gridStep != gridLines[i] - gridLines[i - 1])
            throw std::runtime_error("THREE.LUT3dlLoader: Inconsistent grid size");

    std::vector<double> dataFloat(size * size * size * 4, 0.0);
    double maxValue = 0.0;
    std::size_t index = 0;

    for (std::sregex_iterator it(input.begin(), input.end(), regExpDataPoints), end; it != end;
         ++it) {
        const double r = std::stod((*it)[1].str());
        const double g = std::stod((*it)[2].str());
        const double b = std::stod((*it)[3].str());
        maxValue = std::max(maxValue, std::max(r, std::max(g, b)));

        const std::size_t bLayer = index % size;
        const std::size_t gLayer = (index / size) % size;
        const std::size_t rLayer = (index / sizeSq) % size;
        // Blue grows first, then green, then red.
        const std::size_t d4 = (bLayer * sizeSq + gLayer * size + rLayer) * 4;
        if (d4 + 3 >= dataFloat.size()) break;
        dataFloat[d4 + 0] = r;
        dataFloat[d4 + 1] = g;
        dataFloat[d4 + 2] = b;
        ++index;
    }

    // The stored values are scaled by the next power of two above the largest sample.
    const double bits = std::max(0.0, std::ceil(std::log2(maxValue)));
    const double maxBitValue = std::pow(2.0, bits);
    const bool isByteType = _type == UnsignedByteType;
    const double scale = isByteType ? 255.0 : 1.0;

    std::vector<std::uint8_t> bytes;
    std::vector<float> floats;
    if (isByteType) bytes.assign(dataFloat.size(), 0);
    else floats.assign(dataFloat.size(), 0.0f);

    for (std::size_t i = 0; i < dataFloat.size(); i += 4) {
        const double r = dataFloat[i + 0] / maxBitValue * scale;
        const double g = dataFloat[i + 1] / maxBitValue * scale;
        const double b = dataFloat[i + 2] / maxBitValue * scale;
        if (isByteType) {
            bytes[i + 0] = static_cast<std::uint8_t>(r);
            bytes[i + 1] = static_cast<std::uint8_t>(g);
            bytes[i + 2] = static_cast<std::uint8_t>(b);
            bytes[i + 3] = static_cast<std::uint8_t>(scale);
        } else {
            floats[i + 0] = static_cast<float>(r);
            floats[i + 1] = static_cast<float>(g);
            floats[i + 2] = static_cast<float>(b);
            floats[i + 3] = static_cast<float>(scale);
        }
    }

    Result result;
    result.size = static_cast<double>(size);
    if (isByteType)
        result.texture3D = std::make_shared<Data3DTexture>(Uint8Array(bytes), result.size,
                                                          result.size, result.size);
    else
        result.texture3D = std::make_shared<Data3DTexture>(Float32Array(floats), result.size,
                                                          result.size, result.size);
    result.texture3D->format = RGBAFormat;
    result.texture3D->type = _type;
    result.texture3D->magFilter = LinearFilter;
    result.texture3D->minFilter = LinearFilter;
    result.texture3D->wrapS = ClampToEdgeWrapping;
    result.texture3D->wrapT = ClampToEdgeWrapping;
    result.texture3D->wrapR = ClampToEdgeWrapping;
    result.texture3D->generateMipmaps = false;
    result.texture3D->needsUpdate = true;
    return result;
}
