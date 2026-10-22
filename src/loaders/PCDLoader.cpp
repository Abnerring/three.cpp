#include "loaders/PCDLoader.h"

#include <cctype>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace {
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string lower(std::string text) {
    for (auto& c : text) c = char(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

std::vector<std::string> splitWhitespace(const std::string& text) {
    std::vector<std::string> parts;
    std::istringstream stream(text);
    std::string token;
    while (stream >> token) parts.push_back(token);
    return parts;
}
}  // namespace

std::vector<std::uint8_t> PCDLoader::decompressLZF(const std::vector<std::uint8_t>& inData,
                                                    std::size_t outLength) {
    const std::size_t inLength = inData.size();
    std::vector<std::uint8_t> outData(outLength, 0);
    std::size_t inPtr = 0;
    std::size_t outPtr = 0;

    do {
        const std::uint8_t ctrl = inData[inPtr++];
        if (ctrl < (1u << 5)) {
            // A literal run: the control byte counts the bytes minus one.
            std::size_t count = static_cast<std::size_t>(ctrl) + 1;
            if (outPtr + count > outLength)
                throw std::runtime_error("THREE.PCDLoader: Output buffer is not large enough");
            if (inPtr + count > inLength)
                throw std::runtime_error("THREE.PCDLoader: Invalid compressed data");
            while (count--) outData[outPtr++] = inData[inPtr++];
        } else {
            // A back reference: length in the top three bits, offset in the low five.
            std::size_t length = ctrl >> 5;
            std::ptrdiff_t ref = static_cast<std::ptrdiff_t>(outPtr) -
                                 static_cast<std::ptrdiff_t>((ctrl & 0x1f) << 8) - 1;
            if (inPtr >= inLength)
                throw std::runtime_error("THREE.PCDLoader: Invalid compressed data");
            if (length == 7) {
                length += inData[inPtr++];
                if (inPtr >= inLength)
                    throw std::runtime_error("THREE.PCDLoader: Invalid compressed data");
            }
            ref -= inData[inPtr++];
            if (outPtr + length + 2 > outLength)
                throw std::runtime_error("THREE.PCDLoader: Output buffer is not large enough");
            if (ref < 0) throw std::runtime_error("THREE.PCDLoader: Invalid compressed data");
            if (static_cast<std::size_t>(ref) >= outPtr)
                throw std::runtime_error("THREE.PCDLoader: Invalid compressed data");
            // The run copies length plus two bytes.
            std::size_t remaining = length + 2;
            while (remaining--) outData[outPtr++] = outData[static_cast<std::size_t>(ref++)];
        }
    } while (inPtr < inLength);
    return outData;
}

PCDLoader::Header PCDLoader::parseHeader(const std::vector<std::uint8_t>& buffer) {
    Header header;
    std::string line;
    std::size_t i = 0;
    bool end = false;

    while (i < buffer.size() && !end) {
        const char character = static_cast<char>(buffer[i++]);
        if (character == '\n' || character == '\r') {
            const std::string text = trim(line);
            line.clear();
            if (text.empty()) continue;
            // The data line ends the header and names the storage.
            if (lower(text).rfind("data", 0) == 0) {
                header.data = trim(text.substr(4));
                end = true;
                break;
            }
            const auto tokens = splitWhitespace(text);
            if (tokens.empty()) continue;
            const std::string key = lower(tokens[0]);
            if (key == "fields") {
                for (std::size_t k = 1; k < tokens.size(); ++k)
                    header.fields.push_back(lower(tokens[k]));
            } else if (key == "size") {
                for (std::size_t k = 1; k < tokens.size(); ++k)
                    header.size.push_back(std::atoi(tokens[k].c_str()));
            } else if (key == "type") {
                for (std::size_t k = 1; k < tokens.size(); ++k)
                    header.type.push_back(lower(tokens[k]));
            } else if (key == "count") {
                for (std::size_t k = 1; k < tokens.size(); ++k)
                    header.count.push_back(std::atoi(tokens[k].c_str()));
            } else if (key == "width") {
                header.width = tokens.size() > 1 ? std::atoi(tokens[1].c_str()) : 0;
            } else if (key == "height") {
                header.height = tokens.size() > 1 ? std::atoi(tokens[1].c_str()) : 0;
            } else if (key == "points") {
                header.points = tokens.size() > 1 ? std::atoi(tokens[1].c_str()) : 0;
            }
        } else {
            line.push_back(character);
        }
    }
    header.headerLength = i;
    if (header.data.empty()) throw std::runtime_error("THREE.PCDLoader: Missing data declaration");
    return header;
}

PCDLoader::FieldLayout PCDLoader::computeOffsets(const Header& header) {
    FieldLayout layout;
    int sizeSum = 0;
    int countSum = 0;
    const bool ascii = header.data == "ascii";

    for (std::size_t i = 0; i < header.fields.size(); ++i) {
        const std::string& field = header.fields[i];
        const int fieldCount = i < header.count.size() ? header.count[i] : 1;
        if (ascii) {
            // Ascii offsets count tokens, so a field starts after the previous field's values.
            layout.offset[field] = countSum;
            countSum += fieldCount;
        } else {
            // Binary offsets count bytes, so a field starts after the previous field's storage.
            layout.offset[field] = sizeSum;
            const int fieldSize = i < header.size.size() ? header.size[i] : 1;
            sizeSum += fieldSize * fieldCount;
        }
    }
    // The row size only describes the binary forms.
    layout.rowSize = sizeSum;
    return layout;
}