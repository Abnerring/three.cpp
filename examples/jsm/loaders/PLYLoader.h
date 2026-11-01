#pragma once
#include <cstdint>
#include <map>
#include <variant>
#include <optional>
#include <memory>
#include <string>
class BufferGeometry;
#include <vector>

// Port of the header half of examples/jsm/loaders/PLYLoader.js at the pinned revision. The
// body readers (ascii and binary), the attribute building and the geometry are the layers
// above and are not part of this slice.
class PLYLoader {
  public:
    struct Property {
        // "list" for array properties, otherwise the scalar type name.
        std::string type;
        std::string name;
        // Only meaningful for a list property.
        std::string countType;
        std::string itemType;
        bool isList() const { return type == "list"; }
    };

    struct Element {
        std::string name;
        int count = 0;
        std::vector<Property> properties;
    };

    struct Header {
        std::optional<std::string> format;
        std::optional<std::string> version;
        std::vector<std::string> comments;
        std::vector<Element> elements;
        std::string objInfo;
        // Bytes consumed up to and including the end_header newline, when known.
        std::size_t headerLength = 0;
    };

    // Renames properties as they are read, matching upstream's propertyNameMapping.
    std::map<std::string, std::string> propertyNameMapping;

    // Reads the text between the ply magic and end_header. Throws std::runtime_error when
    // that block is missing.
    Header parseHeader(const std::string& text) const;

    // The typed buffer attribute class upstream picks for a property type, as the class name
    // it returns. An unknown type yields an empty string, matching upstream returning
    // undefined.
    static std::string bufferAttributeClass(const std::string& type);

    // Colour channels are scaled by the storage width: bytes by 255, shorts by 65535, floats
    // by one, and anything unrecognised falls back to 255 as upstream does.
    static double colorScale(const std::string& type);

    // Reads one ascii token with the integer or floating parser the type calls for.
    static double parseASCIINumber(const std::string& text, const std::string& type);

    // Whether a type is one of the integer spellings.
    static bool isIntegerType(const std::string& type);

    // A property value is one number, or a list of them for a list property.
    using Value = std::variant<double, std::vector<double>>;
    using ElementRecord = std::map<std::string, Value>;

    // Consumes one element worth of whitespace separated tokens. Returns nothing when the
    // stream runs out part way, which upstream signals by returning null.
    static std::optional<ElementRecord> parseASCIIElement(const std::vector<Property>& properties,
                                                    const std::vector<std::string>& tokens,
                                                    std::size_t& cursor);

    // The byte width of a property type, or zero when it is unknown.
    static int binaryTypeSize(const std::string& type);

    // Reads one binary element starting at at. Returns the record and how many bytes it
    // consumed, or nothing when the data ends early. Single byte types ignore the endian
    // flag exactly as upstream does.
    static std::optional<std::pair<ElementRecord, std::size_t>> binaryReadElement(
        const std::vector<std::uint8_t>& data, std::size_t at,
        const std::vector<Property>& properties, bool littleEndian);

    // A vertex record contributes to whichever streams the descriptor names.
    struct AttributeDescriptor {
        bool positionUsage = false, normalUsage = false, uvUsage = false, colorUsage = false;
        std::string positionNames[3];
        std::string normalNames[3];
        std::string uvNames[2];
        std::string colorNames[3];
        std::string colorType = "uchar";
    };

    // The flat streams a PLY file is accumulated into before the geometry is built.
    struct Buffer {
        std::vector<double> vertices;
        std::vector<double> normals;
        std::vector<double> uvs;
        std::vector<double> colors;
        std::vector<std::uint32_t> indices;
    };

    // Appends one vertex record to the streams the descriptor enables. Colours are scaled
    // to one, converted out of sRGB, then rounded back to the original storage width, which
    // is what upstream does so integer colours keep their channel values.
    static void handleVertex(Buffer& buffer, const ElementRecord& element,
                             const AttributeDescriptor& descriptor);

    // Appends triangles from a face record. Three indices make one triangle and four make
    // two by the spec rule 0,1,2 and 0,2,3; other counts contribute nothing.
    static void handleFace(Buffer& buffer, const std::vector<double>& vertexIndices);

    // Whether a type is one of the floating spellings.
    static bool isFloatType(const std::string& type);

    // Turns the accumulated streams into a geometry: the index first, then each attribute
    // the file supplied. Upstream does not compute missing normals, so a file without them
    // yields a geometry without a normal attribute.
    static std::shared_ptr<BufferGeometry> buildGeometry(const Buffer& buffer);
};
