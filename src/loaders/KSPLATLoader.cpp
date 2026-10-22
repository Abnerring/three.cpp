#include "loaders/KSPLATLoader.h"

#include "loaders/SPZLoader.h"
#include <cstring>
#include <stdexcept>
#include <string>

namespace {
// The component order per degree, flattened from the three bands.
const int kBandIndex[3][21] = {
    {0, 3, 6, 1, 4, 7, 2, 5, 8},
    {9, 14, 19, 10, 15, 20, 11, 16, 21, 12, 17, 22, 13, 18, 23},
    {24, 31, 38, 25, 32, 39, 26, 33, 40, 27, 34, 41, 28, 35, 42, 29, 36, 43, 30, 37, 44}};
}  // namespace

KSPLATLoader::Compression KSPLATLoader::compression(int level) {
    Compression result;
    // Level zero stores everything as floats, so a coordinate's range is one unit and there is
    // no implied scaling.
    if (level == 0) {
        result.bytesPerCenter = 12;
        result.bytesPerScale = 12;
        result.bytesPerRotation = 16;
        result.bytesPerColor = 4;
        result.bytesPerHarmonicComponent = 4;
        result.scaleOffset = 12;
        result.rotationOffset = 24;
        result.colorOffset = 40;
        result.scaleRange = 1;
        return result;
    }
    // Both compressed levels use six byte centres, six byte scales and eight byte rotations,
    // differing only in how wide a harmonic component is.
    if (level == 1 || level == 2) {
        result.bytesPerCenter = 6;
        result.bytesPerScale = 6;
        result.bytesPerRotation = 8;
        result.bytesPerColor = 4;
        result.bytesPerHarmonicComponent = level == 1 ? 2 : 1;
        result.scaleOffset = 6;
        result.rotationOffset = 12;
        result.colorOffset = 20;
        result.scaleRange = 32767;
    }
    return result;
}

int KSPLATLoader::harmonicIndex(int degree, int component) {
    if (degree < 1 || degree > 3) return -1;
    const int count = kDegreeComponents[degree];
    if (component < 0 || component >= count) return -1;
    return kBandIndex[degree - 1][component];
}

int KSPLATLoader::splatStride(int level, int degree) {
    const Compression layout = compression(level);
    if (layout.bytesPerCenter == 0 || degree < 0 || degree > 3) return 0;
    // The four fixed fields, then every harmonic component the degree asks for.
    return layout.bytesPerCenter + layout.bytesPerScale + layout.bytesPerRotation +
           layout.bytesPerColor +
           kDegreeComponents[degree] * layout.bytesPerHarmonicComponent;
}

namespace {
std::uint32_t readUint32(const std::vector<std::uint8_t>& data, std::size_t at) {
    return static_cast<std::uint32_t>(data[at]) | (static_cast<std::uint32_t>(data[at + 1]) << 8) |
           (static_cast<std::uint32_t>(data[at + 2]) << 16) |
           (static_cast<std::uint32_t>(data[at + 3]) << 24);
}
std::uint32_t readUint16(const std::vector<std::uint8_t>& data, std::size_t at) {
    return static_cast<std::uint32_t>(data[at]) | (static_cast<std::uint32_t>(data[at + 1]) << 8);
}
float readFloat32(const std::vector<std::uint8_t>& data, std::size_t at) {
    const std::uint32_t bits = readUint32(data, at);
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
}  // namespace

KSPLATLoader::Header KSPLATLoader::parseHeader(const std::vector<std::uint8_t>& buffer) {
    // The fields run to offset forty four, well inside the four kilobyte header.
    if (buffer.size() < 44) throw std::runtime_error("THREE.KSPLATLoader: Header is too short");

    Header header;
    header.versionMajor = buffer[0];
    header.versionMinor = buffer[1];
    header.maxSectionCount = readUint32(buffer, 4);
    header.sectionCount = readUint32(buffer, 8);
    header.maxSplatCount = readUint32(buffer, 12);
    header.splatCount = readUint32(buffer, 16);
    header.compressionLevel = static_cast<int>(readUint16(buffer, 20));

    // A stored zero means the writer left the field out, which upstream takes as falsy and
    // replaces with its default rather than keeping.
    const float minCoefficient = readFloat32(buffer, 36);
    const float maxCoefficient = readFloat32(buffer, 40);
    header.minHarmonicCoefficient = minCoefficient != 0.0f ? minCoefficient : -1.5;
    header.maxHarmonicCoefficient = maxCoefficient != 0.0f ? maxCoefficient : 1.5;
    return header;
}

KSPLATLoader::SectionHeader KSPLATLoader::parseSectionHeader(
    const std::vector<std::uint8_t>& buffer, std::size_t offset, const Compression& layout) {
    if (offset + 42 > buffer.size())
        throw std::runtime_error("THREE.KSPLATLoader: Section header is too short");

    SectionHeader header;
    header.splatCount = readUint32(buffer, offset + 0);
    header.maxSplatCount = readUint32(buffer, offset + 4);
    header.bucketSize = readUint32(buffer, offset + 8);
    header.bucketCount = readUint32(buffer, offset + 12);
    header.bucketBlockSize = readFloat32(buffer, offset + 16);
    header.bucketStorageSizeBytes = static_cast<int>(readUint16(buffer, offset + 20));
    // The same falsy rule applies here: a zero takes the level's range.
    const std::uint32_t storedRange = readUint32(buffer, offset + 24);
    header.compressionScaleRange =
        storedRange != 0 ? storedRange : static_cast<std::uint32_t>(layout.scaleRange);
    header.fullBucketCount = readUint32(buffer, offset + 32);
    header.partiallyFilledBucketCount = readUint32(buffer, offset + 36);
    header.sphericalHarmonicsDegree = static_cast<int>(readUint16(buffer, offset + 40));
    return header;
}

double KSPLATLoader::compressionScaleFactor(const SectionHeader& section) {
    // A range of zero would divide by nothing, so the caller is protected here rather than at
    // every call site.
    if (section.compressionScaleRange == 0) return 0.0;
    return section.bucketBlockSize / 2.0 / static_cast<double>(section.compressionScaleRange);
}

double KSPLATLoader::decodeCenter(std::uint32_t stored, const SectionHeader& section,
                                  double bucketCoordinate) {
    // Shifting by the range puts zero at the middle of the block, then the step scales it and
    // the bucket's own coordinate places the block.
    const double offset =
        static_cast<double>(static_cast<std::int64_t>(stored) -
                            static_cast<std::int64_t>(section.compressionScaleRange));
    return offset * compressionScaleFactor(section) + bucketCoordinate;
}

double KSPLATLoader::readCompressedFloat(const std::vector<std::uint8_t>& buffer,
                                         std::size_t offset, int bytesPerVector) {
    if (offset + 2 > buffer.size()) return 0.0;
    // A whole float vector is stored as it is.
    if (bytesPerVector == 12 || bytesPerVector == 16) {
        if (offset + 4 > buffer.size()) return 0.0;
        return readFloat32(buffer, offset);
    }
    // Anything narrower is a half float, which the splat loader already decodes.
    return SPZLoader::fromHalfFloat(static_cast<std::uint16_t>(readUint16(buffer, offset)));
}

double KSPLATLoader::readCompressedHarmonic(const std::vector<std::uint8_t>& buffer,
                                            std::size_t offset, int bytesPerComponent,
                                            double minCoefficient, double maxCoefficient) {
    if (offset >= buffer.size()) return 0.0;
    if (bytesPerComponent == 4) {
        if (offset + 4 > buffer.size()) return 0.0;
        return readFloat32(buffer, offset);
    }
    if (bytesPerComponent == 2) {
        if (offset + 2 > buffer.size()) return 0.0;
        return SPZLoader::fromHalfFloat(static_cast<std::uint16_t>(readUint16(buffer, offset)));
    }
    // A single byte is a fraction of the declared range.
    const double t = buffer[offset] / 255.0;
    return minCoefficient + t * (maxCoefficient - minCoefficient);
}

KSPLATLoader::BucketPosition KSPLATLoader::bucketIndex(
    std::uint32_t splatIndex, const SectionHeader& section, std::uint32_t fullBucketSplats,
    const std::vector<std::uint32_t>& partialLengths) {
    BucketPosition position;
    // Without buckets everything belongs to the first one.
    if (section.bucketCount == 0) return position;

    // While the full buckets last, the splat divides evenly across them.
    if (splatIndex < fullBucketSplats) {
        if (section.bucketSize != 0) position.value = splatIndex / section.bucketSize;
        return position;
    }

    // Past those, the partial bucket lengths are walked in order.
    std::uint32_t bucket = section.fullBucketCount;
    std::uint32_t base = fullBucketSplats;
    std::size_t at = 0;
    while (bucket < section.bucketCount) {
        const std::uint32_t length = at < partialLengths.size() ? partialLengths[at++] : 0;
        if (splatIndex < base + length) {
            position.value = bucket;
            position.partialIndex = bucket;
            position.partialBase = base;
            return position;
        }
        ++bucket;
        base += length;
    }
    // Running out of buckets means the file disagrees with itself.
    throw std::runtime_error("THREE.KSPLATLoader: Invalid KSPLAT bucket data.");
}

void KSPLATLoader::writeHarmonicBand(std::vector<std::uint8_t>& target, std::size_t splatIndex,
                                     int bandComponents, int byteStride,
                                     const std::vector<int>& componentIndexes,
                                     const std::vector<std::uint8_t>& source,
                                     std::size_t rowOffset, int bytesPerComponent,
                                     double minCoefficient, double maxCoefficient) {
    const std::size_t targetOffset = splatIndex * static_cast<std::size_t>(byteStride);
    if (target.size() < targetOffset + static_cast<std::size_t>(bandComponents)) return;

    for (int i = 0; i < bandComponents; ++i) {
        // The component index table says which slot of the row holds this component.
        const int slot = i < static_cast<int>(componentIndexes.size()) ? componentIndexes[i] : -1;
        if (slot < 0) continue;
        const std::size_t at = rowOffset + static_cast<std::size_t>(slot) *
                                               static_cast<std::size_t>(bytesPerComponent);
        const double value = readCompressedHarmonic(source, at, bytesPerComponent, minCoefficient,
                                                    maxCoefficient);
        // The byte holds the value shifted from minus one through one into zero through two
        // hundred and fifty six, truncated to a byte.
        target[targetOffset + static_cast<std::size_t>(i)] =
            static_cast<std::uint8_t>(value * 128.0 + 128.0);
    }
}

namespace {
// The component order per degree, matching harmonicIndex.
std::vector<int> bandIndexes(int degree) {
    static const int kIndex[3][21] = {
        {0, 3, 6, 1, 4, 7, 2, 5, 8},
        {9, 14, 19, 10, 15, 20, 11, 16, 21, 12, 17, 22, 13, 18, 23},
        {24, 31, 38, 25, 32, 39, 26, 33, 40, 27, 34, 41, 28, 35, 42, 29, 36, 43, 30, 37, 44}};
    std::vector<int> out;
    if (degree < 1 || degree > 3) return out;
    const int count = KSPLATLoader::kDegreeComponents[degree];
    for (int i = 0; i < count; ++i) out.push_back(kIndex[degree - 1][i]);
    return out;
}
}  // namespace

void KSPLATLoader::validateFile(const std::vector<std::uint8_t>& buffer, const Header& header) {
    // The header is a fixed four kilobytes.
    if (buffer.size() < static_cast<std::size_t>(kHeaderSizeBytes))
        throw std::runtime_error("THREE.KSPLATLoader: Invalid KSPLAT header.");

    // The major version must match and the minor may not be older than the one the format
    // started at.
    if (header.versionMajor != kVersionMajor || header.versionMinor < kVersionMinor)
        throw std::runtime_error("THREE.KSPLATLoader: Unsupported KSPLAT version " +
                                 std::to_string(header.versionMajor) + "." +
                                 std::to_string(header.versionMinor) + ".");

    if (header.compressionLevel < 0 || header.compressionLevel > 2)
        throw std::runtime_error("THREE.KSPLATLoader: Unsupported KSPLAT compression level " +
                                 std::to_string(header.compressionLevel) + ".");

    // A cap so a corrupt count cannot ask for a huge allocation.
    if (header.splatCount > static_cast<std::uint32_t>(kMaxSplats))
        throw std::runtime_error("THREE.KSPLATLoader: KSPLAT file contains too many splats (" +
                                 std::to_string(header.splatCount) + ").");

    // Every possible section header sits between the file header and the section data.
    const std::size_t sectionDataOffset =
        static_cast<std::size_t>(kHeaderSizeBytes) +
        static_cast<std::size_t>(header.maxSectionCount) * kSectionHeaderSizeBytes;
    if (buffer.size() < sectionDataOffset)
        throw std::runtime_error("THREE.KSPLATLoader: Invalid KSPLAT section headers.");
}

KSPLATLoader::SectionLayout KSPLATLoader::sectionLayout(const SectionHeader& section,
                                                        const Compression& layout) {
    SectionLayout result;
    const int components = section.sphericalHarmonicsDegree >= 0 &&
                                   section.sphericalHarmonicsDegree < 4
                               ? kDegreeComponents[section.sphericalHarmonicsDegree]
                               : -1;
    if (components < 0)
        throw std::runtime_error(
            "THREE.KSPLATLoader: Unsupported KSPLAT spherical harmonics degree " +
            std::to_string(section.sphericalHarmonicsDegree) + ".");

    // One splat is its four fixed fields plus every harmonic component the degree asks for.
    result.bytesPerSplat = layout.bytesPerCenter + layout.bytesPerScale +
                           layout.bytesPerRotation + layout.bytesPerColor +
                           components * layout.bytesPerHarmonicComponent;
    // The partial bucket lengths live in front of the bucket storage, and the splat data after
    // both.
    result.bucketsMetaDataSize = section.partiallyFilledBucketCount * 4;
    result.bucketsStorageSize = section.bucketStorageSizeBytes * section.bucketCount +
                                result.bucketsMetaDataSize;
    result.splatDataSize = result.bytesPerSplat * section.maxSplatCount;
    result.storageSize = result.bucketsStorageSize + result.splatDataSize;
    return result;
}

KSPLATLoader::Row KSPLATLoader::decodeRow(const std::vector<std::uint8_t>& buffer,
                                          std::size_t rowOffset, const SectionHeader& section,
                                          const Compression& layout,
                                          double bucketCoordinate[3]) {
    Row row;
    if (rowOffset + static_cast<std::size_t>(layout.bytesPerCenter) > buffer.size()) return row;

    if (layout.bytesPerCenter == 12) {
        // Uncompressed: three plain floats, with no bucket to add.
        for (int axis = 0; axis < 3; ++axis)
            row.center[axis] = readFloat32(buffer, rowOffset + static_cast<std::size_t>(axis) * 4);
    } else {
        // Compressed: three unsigned shorts measured from the middle of the range, each placed
        // by its own bucket coordinate.
        for (int axis = 0; axis < 3; ++axis) {
            const std::uint32_t stored =
                readUint16(buffer, rowOffset + static_cast<std::size_t>(axis) * 2);
            row.center[axis] = decodeCenter(stored, section, bucketCoordinate[axis]);
        }
    }

    // Three scales and four rotation components, packed tight at their own offsets.
    for (int axis = 0; axis < 3; ++axis) {
        const std::size_t at = rowOffset + static_cast<std::size_t>(layout.scaleOffset) +
                               static_cast<std::size_t>(axis) *
                                   static_cast<std::size_t>(layout.bytesPerScale / 3);
        row.scale[axis] = readCompressedFloat(buffer, at, layout.bytesPerScale);
    }
    for (int component = 0; component < 4; ++component) {
        const std::size_t at = rowOffset + static_cast<std::size_t>(layout.rotationOffset) +
                               static_cast<std::size_t>(component) *
                                   static_cast<std::size_t>(layout.bytesPerRotation / 4);
        row.rotation[component] = readCompressedFloat(buffer, at, layout.bytesPerRotation);
    }

    // The colour is four bytes as they are.
    const std::size_t colorAt = rowOffset + static_cast<std::size_t>(layout.colorOffset);
    for (int i = 0; i < 4; ++i)
        if (colorAt + static_cast<std::size_t>(i) < buffer.size())
            row.color[i] = buffer[colorAt + static_cast<std::size_t>(i)];
    return row;
}

std::vector<KSPLATLoader::Row> KSPLATLoader::decodeSection(
    const std::vector<std::uint8_t>& buffer, const SectionHeader& section,
    const Compression& layout, std::size_t sectionBase) {
    std::vector<Row> rows;
    const SectionLayout sizes = sectionLayout(section, layout);
    if (sizes.bytesPerSplat <= 0) return rows;

    // The partial bucket lengths come first, then the bucket records, then the rows.
    const std::size_t bucketsBase = sectionBase + sizes.bucketsMetaDataSize;
    const std::size_t dataBase = sectionBase + sizes.bucketsStorageSize;
    const std::uint32_t fullBucketSplats = section.fullBucketCount * section.bucketSize;

    // The partial lengths are read once so the walk does not touch the buffer per splat.
    std::vector<std::uint32_t> partialLengths;
    for (std::uint32_t i = 0; i < section.partiallyFilledBucketCount; ++i) {
        const std::size_t at = sectionBase + static_cast<std::size_t>(i) * 4;
        partialLengths.push_back(at + 4 <= buffer.size() ? readUint32(buffer, at) : 0);
    }

    rows.reserve(section.splatCount);
    for (std::uint32_t i = 0; i < section.splatCount; ++i) {
        const BucketPosition bucket =
            bucketIndex(i, section, fullBucketSplats, partialLengths);

        // A compressed centre is placed by the bucket's own coordinate, read from the record.
        double bucketCoordinate[3] = {0, 0, 0};
        if (layout.bytesPerCenter != 12 && section.bucketCount > 0) {
            const std::size_t at =
                bucketsBase + static_cast<std::size_t>(bucket.value) *
                                  static_cast<std::size_t>(section.bucketStorageSizeBytes);
            for (int axis = 0; axis < 3; ++axis) {
                const std::size_t componentAt = at + static_cast<std::size_t>(axis) * 4;
                if (componentAt + 4 <= buffer.size())
                    bucketCoordinate[axis] = readFloat32(buffer, componentAt);
            }
        }

        const std::size_t rowOffset = dataBase + static_cast<std::size_t>(i) *
                                                    static_cast<std::size_t>(sizes.bytesPerSplat);
        rows.push_back(decodeRow(buffer, rowOffset, section, layout, bucketCoordinate));
    }
    return rows;
}