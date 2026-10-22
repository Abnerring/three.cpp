#pragma once
#include <cstdint>
#include <vector>

// Port of the layout tables in examples/jsm/loaders/KSPLATLoader.js at the pinned revision. A
// KSPLAT file is a splat cloud with three selectable compression levels, each fixing the byte
// width of every field and where in a splat record it sits. The harmonic components are stored
// band by band but out of their natural order, which a fixed index table puts back.
class KSPLATLoader {
  public:
    static constexpr int kHeaderSizeBytes = 4096;
    static constexpr int kSectionHeaderSizeBytes = 1024;
    static constexpr int kVersionMajor = 0;
    static constexpr int kVersionMinor = 1;
    static constexpr int kMaxSplats = 10000000;
    // How many harmonic components each degree holds, the same table the other splat loaders
    // use.
    static constexpr int kDegreeComponents[4] = {0, 9, 24, 45};

    // The byte layout one compression level fixes.
    struct Compression {
        int bytesPerCenter = 0;
        int bytesPerScale = 0;
        int bytesPerRotation = 0;
        int bytesPerColor = 0;
        int bytesPerHarmonicComponent = 0;
        int scaleOffset = 0;
        int rotationOffset = 0;
        int colorOffset = 0;
        // Coordinates are stored as integers over this range, or one when they are floats.
        int scaleRange = 1;
    };

    // The layout for a level, or a zeroed one for a level the format does not define.
    static Compression compression(int level);

    // Where a harmonic component is stored for a degree. The bands are written out of their
    // natural order, so component index maps to a slot; a degree of zero has no entries.
    static int harmonicIndex(int degree, int component);

    // How many bytes one splat occupies at a level and degree: the four fixed fields plus three
    // components for the base colour and the rest of the band.
    static int splatStride(int level, int degree);

    // The file header, which is four kilobytes with the fields at fixed offsets.
    struct Header {
        int versionMajor = 0;
        int versionMinor = 0;
        std::uint32_t maxSectionCount = 0;
        std::uint32_t sectionCount = 0;
        std::uint32_t maxSplatCount = 0;
        std::uint32_t splatCount = 0;
        int compressionLevel = 0;
        // The harmonic coefficients are kept in a range; upstream falls back to minus one and
        // a half and one and a half when a stored zero is found, because a zero is falsy there.
        double minHarmonicCoefficient = -1.5;
        double maxHarmonicCoefficient = 1.5;
    };

    // One section header, a kilobyte each, at a given offset.
    struct SectionHeader {
        std::uint32_t splatCount = 0;
        std::uint32_t maxSplatCount = 0;
        std::uint32_t bucketSize = 0;
        std::uint32_t bucketCount = 0;
        double bucketBlockSize = 0;
        int bucketStorageSizeBytes = 0;
        std::uint32_t compressionScaleRange = 1;
        std::uint32_t fullBucketCount = 0;
        std::uint32_t partiallyFilledBucketCount = 0;
        int sphericalHarmonicsDegree = 0;
    };

    // Reads the file header. Throws when the buffer is shorter than the fixed fields.
    static Header parseHeader(const std::vector<std::uint8_t>& buffer);

    // Reads one section header at the given offset, taking the scale range from the level when
    // the stored value is zero.
    static SectionHeader parseSectionHeader(const std::vector<std::uint8_t>& buffer,
                                            std::size_t offset, const Compression& layout);

    // The step one stored unit covers: half a bucket block divided by the range a coordinate is
    // allowed to span. A block size of four over a range of 32767 makes a full range span two
    // units.
    static double compressionScaleFactor(const SectionHeader& section);

    // Decodes one compressed centre component. The stored unsigned value is measured from the
    // middle of the range, and the bucket supplies the offset that brings it back into place.
    static double decodeCenter(std::uint32_t stored, const SectionHeader& section,
                               double bucketCoordinate);

    // Reads one scale or rotation component. A vector of twelve or sixteen bytes is stored as
    // plain floats; the compressed levels use half floats, which are two bytes each.
    static double readCompressedFloat(const std::vector<std::uint8_t>& buffer, std::size_t offset,
                                      int bytesPerVector);

    // Reads one harmonic component. Four bytes is a float, two a half float, and one a byte
    // scaled linearly between the range the header declares.
    static double readCompressedHarmonic(const std::vector<std::uint8_t>& buffer,
                                         std::size_t offset, int bytesPerComponent,
                                         double minCoefficient, double maxCoefficient);

    // Which bucket a splat falls in. With no buckets everything is bucket zero; while the full
    // buckets last, the index is the splat divided by the bucket size; after that the partial
    // bucket lengths are walked in order.
    struct BucketPosition {
        std::uint32_t value = 0;
        std::uint32_t partialIndex = 0;
        std::uint32_t partialBase = 0;
    };
    static BucketPosition bucketIndex(std::uint32_t splatIndex, const SectionHeader& section,
                                      std::uint32_t fullBucketSplats,
                                      const std::vector<std::uint32_t>& partialLengths);

    // Writes one band of harmonic components for one splat. The source components are stored
    // out of order, so each is read at the slot the index table gives it, then scaled from the
    // minus one to one range into a byte by multiplying by one hundred and twenty eight and
    // adding the same. A component above one overflows the byte, which is the upstream
    // behaviour and the reason the declared range matters.
    static void writeHarmonicBand(std::vector<std::uint8_t>& target, std::size_t splatIndex,
                                  int bandComponents, int byteStride,
                                  const std::vector<int>& componentIndexes,
                                  const std::vector<std::uint8_t>& source,
                                  std::size_t rowOffset, int bytesPerComponent,
                                  double minCoefficient, double maxCoefficient);

    // The byte budget one section needs, derived from its header.
    struct SectionLayout {
        int bytesPerSplat = 0;
        std::uint32_t bucketsMetaDataSize = 0;
        std::uint32_t bucketsStorageSize = 0;
        std::uint32_t splatDataSize = 0;
        std::uint32_t storageSize = 0;
    };

    // Checks the file as a whole: the header length, the version, the compression level, the
    // splat cap, and whether the section header table fits. Throws with the upstream message
    // for each.
    static void validateFile(const std::vector<std::uint8_t>& buffer, const Header& header);

    // Works out how many bytes one section occupies. The bucket storage counts the optional
    // per bucket coordinates as well as the buckets themselves.
    static SectionLayout sectionLayout(const SectionHeader& section, const Compression& layout);
};
