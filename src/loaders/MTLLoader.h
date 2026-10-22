#pragma once
#include <array>
#include <map>
#include <optional>
#include <string>
#include "constants.h"
#include "math/Color.h"
#include <memory>
#include <unordered_map>
class MeshPhongMaterial;
#include <variant>

// Port of the parsing half of examples/jsm/loaders/MTLLoader.js at the pinned revision. The
// text becomes one record per material; building actual materials from those records is the
// MaterialCreator half and is not part of this slice.
class MTLLoader {
  public:
    // Either the raw string value or, for the four colour keys, three numbers.
    using ColorValue = std::array<double, 3>;
    using Value = std::variant<std::string, ColorValue>;
    using MaterialInfo = std::map<std::string, Value>;
    // Keyed by material name, in insertion order upstream; a map keeps lookup deterministic.
    using MaterialsInfo = std::map<std::string, MaterialInfo>;

    // Upstream documents these four options; the two colour ones change convert().
    struct MaterialOptions {
        double side = FrontSide;
        double wrap = RepeatWrapping;
        bool normalizeRGB = false;
        bool ignoreZeroRGBs = false;
        // Inverts the tr value, an option upstream documents but does not set by default.
        bool invertTrProperty = false;
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

    // The material parameters upstream assembles before constructing a MeshPhongMaterial.
    // Texture slots hold the resolved url, because loading a texture is host work; every
    // other field mirrors what the material would receive.
    struct MaterialParams {
        std::string name;
        ::Color color, specular, emissive;
        bool hasColor = false, hasSpecular = false, hasEmissive = false;
        double shininess = 30;
        double opacity = 1;
        bool transparent = false;
        std::string map, specularMap, emissiveMap, normalMap, bumpMap, displacementMap, alphaMap;
        std::optional<double> bumpScale, displacementBias, displacementScale;
    };

    // Mirrors createMaterial_'s switch. Empty values are skipped, colour keys are converted
    // from sRGB to the working space, d below one and a positive tr both set opacity and the
    // transparent flag, and the first texture seen for a slot is the one kept.
    static MaterialParams createMaterialParams(const MaterialInfo& materialInfo,
                                               const MaterialOptions& options);

    // Builds one material from an already converted record. Texture slots stay empty because
    // loading them is host work; the urls remain in the params for a caller that wants them.
    std::shared_ptr<MeshPhongMaterial> create(const std::string& materialName,
                                              const MaterialsInfo& converted,
                                              const MaterialOptions& options);

    // Builds every material, remembering the index of each name.
    std::vector<std::shared_ptr<MeshPhongMaterial>> getAsArray(const MaterialsInfo& converted,
                                                               const MaterialOptions& options);

    // The index recorded by getAsArray, or -1 when the name was never built.
    int getIndex(const std::string& materialName) const;

  private:
    std::unordered_map<std::string, std::shared_ptr<MeshPhongMaterial>> _materials;
    std::unordered_map<std::string, int> _nameLookup;
};
