#pragma once
#include <array>
#include <map>
#include <optional>
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

    // A texture reference in an MTL file can carry trailing options: -bm for bump scale, -mm
    // for displacement bias and scale, -s for repeat and -o for offset. Whatever is left
    // after removing them is the url.
    struct TextureParams {
        double scaleU = 1, scaleV = 1;
        double offsetU = 0, offsetV = 0;
        std::string url;
        // Set only when the corresponding option was present.
        std::optional<double> bumpScale;
        std::optional<double> displacementBias;
        std::optional<double> displacementScale;
    };

    // Splits a texture line into its options and its url. Upstream also writes bumpScale,
    // displacementBias and displacementScale into the material params, which the three
    // optionals carry back instead.
    static TextureParams getTextureParams(const std::string& value);
};
