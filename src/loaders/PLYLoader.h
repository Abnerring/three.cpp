#pragma once
#include <map>
#include <optional>
#include <string>
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
};
