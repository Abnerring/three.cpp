#pragma once
#include <array>
#include <map>
#include <string>
#include <variant>

// Port of the parsing half of examples/jsm/loaders/MTLLoader.js at the pinned revision. The
// text becomes one record per material; building actual materials from those records is the
// MaterialCreator half and is not part of this slice.
class MTLLoader {
  public:
    // Either the raw string value or, for the four colour keys, three numbers.
    using Color = std::array<double, 3>;
    using Value = std::variant<std::string, Color>;
    using MaterialInfo = std::map<std::string, Value>;
    // Keyed by material name, in insertion order upstream; a map keeps lookup deterministic.
    using MaterialsInfo = std::map<std::string, MaterialInfo>;

    // Parses the text. Keys are lower cased, blank lines and comments are skipped, and the
    // first space separates the key from the value.
    MaterialsInfo parse(const std::string& text) const;
};
