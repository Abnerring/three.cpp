#pragma once
#include <array>
#include <map>
#include <string>
#include "constants.h"
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

    // Upstream documents these four options; the two colour ones change convert().
    struct MaterialOptions {
        double side = FrontSide;
        double wrap = RepeatWrapping;
        bool normalizeRGB = false;
        bool ignoreZeroRGBs = false;
    };

    // Parses the text. Keys are lower cased, blank lines and comments are skipped, and the
    // first space separates the key from the value.
    MaterialsInfo parse(const std::string& text) const;

    // Normalises a parsed record set for material construction: colour keys are divided by
    // two hundred and fifty five when normalizeRGB is set, and all zero colours are dropped
    // when ignoreZeroRGBs is set. Output keys are lower cased, as upstream does.
    static MaterialsInfo convert(const MaterialsInfo& materialsInfo, const MaterialOptions& options);
};
