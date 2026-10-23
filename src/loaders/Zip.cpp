#include "loaders/Zip.h"

#include "loaders/Zlib.h"

namespace {
std::uint16_t readU16(const std::vector<std::uint8_t>& data, std::size_t at) {
    if (at + 2 > data.size()) return 0;
    return static_cast<std::uint16_t>(data[at] | (data[at + 1] << 8));
}
std::uint32_t readU32(const std::vector<std::uint8_t>& data, std::size_t at) {
    if (at + 4 > data.size()) return 0;
    return static_cast<std::uint32_t>(data[at]) |
           (static_cast<std::uint32_t>(data[at + 1]) << 8) |
           (static_cast<std::uint32_t>(data[at + 2]) << 16) |
           (static_cast<std::uint32_t>(data[at + 3]) << 24);
}
std::string readName(const std::vector<std::uint8_t>& data, std::size_t at, std::size_t length) {
    std::string name;
    for (std::size_t i = 0; i < length && at + i < data.size(); ++i) {
        name.push_back(static_cast<char>(data[at + i]));
    }
    return name;
}
}  // namespace

std::string Zip::noEndError() { return "Zip: no end record was found."; }
std::string Zip::methodError() { return "Zip: an entry used a method that is not supported."; }

Zip::EndRecord Zip::findEndRecord(const std::vector<std::uint8_t>& archive) {
    EndRecord end;
    // The record is twenty two bytes and may be followed by a comment, so the search runs back
    // from the end over the longest a comment can be.
    if (archive.size() < 22) return end;
    const std::size_t farthest = archive.size() >= 22 + 65535 ? archive.size() - 22 - 65535 : 0;
    for (std::size_t at = archive.size() - 22;; --at) {
        if (readU32(archive, at) == kEndRecord) {
            end.found = true;
            end.offset = at;
            end.entryCount = readU16(archive, at + 10);
            end.directorySize = readU32(archive, at + 12);
            end.directoryOffset = readU32(archive, at + 16);
            return end;
        }
        if (at == farthest) break;
    }
    return end;
}

std::vector<Zip::Entry> Zip::readDirectory(const std::vector<std::uint8_t>& archive,
                                           const EndRecord& end) {
    std::vector<Entry> entries;
    std::size_t at = end.directoryOffset;
    // Each entry is forty six bytes of fixed fields and then two strings.
    for (std::uint16_t i = 0; i < end.entryCount; ++i) {
        if (readU32(archive, at) != kCentralHeader) break;
        Entry entry;
        const std::uint16_t flags = readU16(archive, at + 8);
        entry.method = readU16(archive, at + 10);
        entry.crc = readU32(archive, at + 16);
        entry.compressedSize = readU32(archive, at + 20);
        entry.uncompressedSize = readU32(archive, at + 24);
        const std::uint16_t nameLength = readU16(archive, at + 28);
        const std::uint16_t extraLength = readU16(archive, at + 30);
        const std::uint16_t commentLength = readU16(archive, at + 32);
        entry.localHeaderOffset = readU32(archive, at + 42);
        entry.name = readName(archive, at + 46, nameLength);
        // The flag says the sizes are not here but after the local header.
        entry.sizesInTrailer = (flags & 0x08) != 0;
        entries.push_back(entry);
        at += 46 + nameLength + extraLength + commentLength;
    }
    return entries;
}

std::size_t Zip::dataOffset(const std::vector<std::uint8_t>& archive, const Entry& entry) {
    // The local header is thirty bytes and then the name and the extra field it declares.
    const std::size_t at = entry.localHeaderOffset;
    const std::uint16_t nameLength = readU16(archive, at + 26);
    const std::uint16_t extraLength = readU16(archive, at + 28);
    return at + 30 + nameLength + extraLength;
}

Zip::ReadResult Zip::readAll(const std::vector<std::uint8_t>& archive) {
    ReadResult result;
    const EndRecord end = findEndRecord(archive);
    if (!end.found) {
        result.error = noEndError();
        return result;
    }
    for (const Entry& entry : readDirectory(archive, end)) {
        File file;
        file.name = entry.name;
        const std::size_t start = dataOffset(archive, entry);
        // A name ending in a slash is a directory and holds nothing.
        if (!entry.name.empty() && entry.name.back() == '/') {
            result.files.push_back(file);
            continue;
        }
        const std::size_t stop = start + entry.compressedSize;
        if (stop > archive.size()) {
            result.error = noEndError();
            return result;
        }
        const std::vector<std::uint8_t> compressed(archive.begin() + static_cast<std::ptrdiff_t>(start),
                                                   archive.begin() + static_cast<std::ptrdiff_t>(stop));
        if (entry.method == 0) {
            // Stored: the bytes are already what they claim to be.
            file.contents = compressed;
        } else if (entry.method == 8) {
            // Deflated: the raw stream needs no header or checksum of its own here.
            const Zlib::InflateResult inflated = Zlib::inflate(compressed);
            if (!inflated.valid) {
                result.error = inflated.error;
                return result;
            }
            file.contents = inflated.output;
        } else {
            result.error = methodError();
            return result;
        }
        result.files.push_back(file);
    }
    result.valid = true;
    return result;
}
