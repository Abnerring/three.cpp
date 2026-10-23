#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// A reader for the parts of the zip format a loader needs: the end record that says where the
// directory is, the entries of that directory, and the compressed bytes each entry points at.
// Nothing here knows what the files mean.
class Zip {
  public:
    // The four byte signatures, read little endian.
    static constexpr std::uint32_t kLocalHeader = 0x04034b50;
    static constexpr std::uint32_t kCentralHeader = 0x02014b50;
    static constexpr std::uint32_t kEndRecord = 0x06054b50;

    // The end record, which sits at the back of the archive inside a comment that may be up to
    // sixty five thousand bytes long. It says how many entries there are and where the directory
    // begins, and its own length is what the search backwards leans on.
    struct EndRecord {
        bool found = false;
        std::uint16_t entryCount = 0;
        std::uint32_t directoryOffset = 0;
        std::uint32_t directorySize = 0;
        // Where the record itself begins, so a caller may look at the comment.
        std::size_t offset = 0;
    };

    static EndRecord findEndRecord(const std::vector<std::uint8_t>& archive);

    // One file in the directory. A size of zero in the directory means the real one follows the
    // local header in a small extra field, which is marked by a flag.
    struct Entry {
        std::string name;
        std::uint16_t method = 0;
        std::uint32_t compressedSize = 0;
        std::uint32_t uncompressedSize = 0;
        std::uint32_t crc = 0;
        std::uint32_t localHeaderOffset = 0;
        // Whether the two sizes were written after the local header instead of here.
        bool sizesInTrailer = false;
    };

    // The entries of the directory, in order. Only what the loaders need is read: the names, the
    // method, the sizes and where each file's own header sits.
    static std::vector<Entry> readDirectory(const std::vector<std::uint8_t>& archive,
                                            const EndRecord& end);

    // Where a file's bytes begin, which is past its own header and the two strings that follow.
    static std::size_t dataOffset(const std::vector<std::uint8_t>& archive, const Entry& entry);

    // Every entry as a pair of name and contents, unpacking the two methods that appear in the
    // files a loader meets: stored and deflated.
    struct File {
        std::string name;
        std::vector<std::uint8_t> contents;
    };

    struct ReadResult {
        bool valid = false;
        std::vector<File> files;
        std::string error;
    };

    static ReadResult readAll(const std::vector<std::uint8_t>& archive);

    // The errors the reader can raise.
    static std::string noEndError();
    static std::string methodError();
};
