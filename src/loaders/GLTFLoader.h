#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// Port of the key building and small lookups in examples/jsm/loaders/GLTFLoader.js at the
// pinned revision. A glTF file describes its primitives as plain JSON, and the loader caches
// geometry under a key it builds from the primitive, so the key has to be the same for two
// primitives that describe the same geometry. The parser itself needs a scene graph and a
// texture pipeline, so this part covers the key and the two small lookups.
class GLTFLoader {
  public:
    // The twelve bytes a binary glTF file begins with, the four character magic and the two
    // chunk kinds that follow the header.
    static constexpr const char* kBinaryMagic = "glTF";
    static constexpr int kBinaryHeaderLength = 12;
    static constexpr std::uint32_t kChunkJson = 0x4E4F534A;
    static constexpr std::uint32_t kChunkBin = 0x004E4942;

    // One attribute of a primitive: its name and the accessor it points at.
    using Attribute = std::pair<std::string, int>;

    // The key for a set of attributes. The names are sorted first so that two primitives that
    // list the same attributes in a different order give the same key, and each becomes
    // name, colon, accessor, semicolon.
    static std::string createAttributesKey(std::vector<Attribute> attributes);

    // The scale a normalised integer accessor needs: one over the largest value its type can
    // hold. A type that is not one of the four throws std::runtime_error.
    static double normalizedComponentScale(const std::string& componentType);

    // The mime type an image reference implies. A data uri names its own type, otherwise the
    // extension decides, and anything else is a png.
    static std::string imageMimeType(const std::string& uri);

    // The twelve byte header of a binary glTF file: four characters of magic, a version and the
    // total length, the last two little endian.
    struct BinaryHeader {
        std::string magic;
        std::uint32_t version = 0;
        std::uint32_t length = 0;
    };

    // Reads the header. Throws std::runtime_error when the magic is not the expected one or the
    // version is below two, both with the source message.
    static BinaryHeader readBinaryHeader(const std::vector<std::uint8_t>& buffer);

    // One chunk of a binary file. A chunk the loader does not know is reported with its type
    // so the caller can ignore it, which the format requires.
    struct BinaryChunk {
        std::uint32_t type = 0;
        std::size_t offset = 0;
        std::uint32_t length = 0;
    };

    // The chunks that follow the header, read in order. The loop stops when the declared file
    // length is reached rather than when the buffer ends.
    static std::vector<BinaryChunk> readBinaryChunks(const std::vector<std::uint8_t>& buffer,
                                                     const BinaryHeader& header);

    // The six component types the format allows, named the way the port names them.
    static std::string componentTypeName(std::uint32_t componentType);

    // How many bytes one component of a type takes. A type the format does not allow throws.
    static int componentTypeSize(std::uint32_t componentType);

    // The number of components an element of a shape holds, so a scalar is one and a four by
    // four matrix sixteen.
    static int typeSize(const std::string& type);

    // The attribute name a semantic maps to. A semantic not listed falls back to its own name
    // lower cased, which is what the parser does for the newer ones.
    static std::string attributeName(const std::string& semantic);

    // The filter a texture sampler names, as the port spells it. An unknown value throws.
    static std::string filterName(std::uint32_t filter);

    // The wrapping a sampler names. An unknown value throws.
    static std::string wrappingName(std::uint32_t wrapping);

    // The bytes a buffer view covers. A view with no length covers nothing, and a view with no
    // offset begins at the start of the buffer.
    static std::vector<std::uint8_t> sliceBufferView(const std::vector<std::uint8_t>& buffer,
                                                     std::size_t byteOffset, std::size_t byteLength);

    // How many components an accessor of the given count and shape holds. An accessor with no
    // view is still that long, filled entirely with zeroes.
    static std::size_t accessorComponentCount(std::size_t count, const std::string& type);

    // Whether a view's stride means its elements are interleaved with others. A stride equal to
    // the size of one element is the plain layout, and no stride at all means the same.
    static bool isInterleaved(std::size_t byteStride, std::size_t itemBytes);

    // Which element of the stride a byte offset falls in, counted from the start of the view.
    static std::size_t interleavedSlice(std::size_t byteOffset, std::size_t byteStride);

    // How far into one element the attribute begins, counted in components rather than bytes.
    static std::size_t interleavedAttributeOffset(std::size_t byteOffset, std::size_t byteStride,
                                                  std::size_t elementBytes);

    // The stride counted in components, which is what an interleaved buffer is told.
    static std::size_t interleavedStride(std::size_t byteStride, std::size_t elementBytes);

    // Writes a sparse accessor's overrides into the array that already holds its default
    // values. Each entry of the index list names a whole element, and the values run on in the
    // element order the indices name. An index past the end of the array is ignored, and a
    // short value run leaves the remaining components as they were.
    static void applySparse(const std::vector<std::uint32_t>& indices,
                            const std::vector<double>& values, std::size_t itemSize,
                            std::vector<double>& target);

    // Whether an accessor filled by a sparse override reports itself as normalised. The source
    // turns the flag off for these, because the values copied in are already in their final
    // form.
    static bool sparseNormalized(bool normalized);
    // How a node is placed. A node either carries a full matrix or a translation, a rotation
    // and a scale, and the two forms are exclusive: a node that has a matrix ignores the three
    // even when they are present. Each of the three may be left out on its own, in which case
    // it keeps the value a fresh object has.
    struct NodeTransform {
        // True when the matrix was given; the three below are then unset and unused.
        bool hasMatrix = false;
        std::array<double, 16> matrix{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        std::array<double, 3> translation{0, 0, 0};
        // A quaternion in x, y, z, w order, which is the identity as written.
        std::array<double, 4> rotation{0, 0, 0, 1};
        std::array<double, 3> scale{1, 1, 1};
    };

    // Builds the transform a node declares. The matrix wins when present; otherwise each of
    // the three is taken when present and left at its default when not.
    static NodeTransform nodeTransform(bool hasMatrix, const std::array<double, 16>& matrix,
                                       bool hasTranslation,
                                       const std::array<double, 3>& translation, bool hasRotation,
                                       const std::array<double, 4>& rotation, bool hasScale,
                                       const std::array<double, 3>& scale);
    // The property an animation channel drives. Three of the four are renamed on the way in,
    // which is why a channel path cannot simply be copied. A path the format does not allow
    // throws.
    static std::string pathProperty(const std::string& path);

    // Whether an alpha mode is one of the three the format allows. The mode is also the value
    // the port stores, so this check is all that reading one needs.
    static bool isValidAlphaMode(const std::string& mode);

    // The interpolation a sampler names. A cubic spline is not one of the port's own
    // interpolants and comes back as its own name for the caller to handle, while the other two
    // map onto theirs.
    static std::string interpolationName(const std::string& interpolation);
    // A colour with its alpha, as a material holds one.
    struct MaterialColour {
        double r = 1;
        double g = 1;
        double b = 1;
        double a = 1;
    };

    // What a material starts as when the file says nothing: white and fully opaque.
    static MaterialColour materialDefaults();

    // A material with a base colour factor read into it. The four numbers are taken as they
    // stand, because the factor is written in the linear space the port works in; a factor with
    // fewer than four numbers leaves the missing pieces alone.
    static MaterialColour applyBaseColorFactor(MaterialColour colour,
                                               const std::vector<double>& factor);

    // The space a base colour factor is read in, and the space its texture is read in. The two
    // differ: the numbers are already linear while the image is display referred, which is why
    // they cannot share one rule.
    static constexpr const char* kBaseColorFactorSpace = "srgb-linear";
    static constexpr const char* kBaseColorTextureSpace = "srgb";
    // The four settings a sampler gives, after the defaults are applied. The two filters have
    // different defaults, which is why they cannot share one fallback.
    struct SamplerSettings {
        std::string magFilter;
        std::string minFilter;
        std::string wrapS;
        std::string wrapT;
    };

    // Builds the settings from the values a sampler names, or from nothing when it names none.
    // An absent filter falls back to the linear one for magnification and to the trilinear one
    // for minification; an absent wrapping falls back to repeating.
    static SamplerSettings samplerSettings(const std::string& magFilter, const std::string& minFilter,
                                           const std::string& wrapS, const std::string& wrapT);

    // Whether a texture made with these settings generates mipmaps. A compressed texture never
    // does, and neither does one whose minification filter is a single level, since neither
    // nearest nor linear looks at more than one level.
    static bool needsMipmaps(bool compressed, const std::string& minFilter);
    // The key a primitive's geometry is cached under. A primitive compressed by the draco
    // extension is keyed by its compressed buffer, its index accessor and its attributes; an
    // ordinary one by its index accessor, its attributes and its mode. Each morph target then
    // adds its own attributes, so two primitives that differ only in a target do not share a key.
    static std::string createPrimitiveKey(bool hasDraco, int dracoBufferView, int dracoIndices,
                                          std::vector<Attribute> dracoAttributes, int indices,
                                          std::vector<Attribute> attributes, int mode,
                                          const std::vector<std::vector<Attribute>>& targets);
    // The extension names the loader knows, in the order the source lists them. Each is both
    // the key the file writes and the value the loader uses, so the list holds one copy of each.
    static const std::vector<std::string>& knownExtensions();

    // Whether a name is one of them.
    static bool isKnownExtension(const std::string& name);

    // How many there are, which the tests check against the source list.
    static std::size_t knownExtensionCount();
    // The three kinds of light a punctual extension may place.
    static bool isLightType(const std::string& type);

    // A light's range, which is zero when the file leaves it out rather than an error. A range
    // of zero means the light reaches without limit.
    static double lightRange(bool hasRange, double range);

    // A light's colour, which is white when the file leaves it out and is taken as it stands
    // when it does not, because the numbers are already in the linear space the port uses.
    static std::array<double, 3> lightColour(bool hasColour, const std::vector<double>& colour);

    // Where a directional light points by default, which is straight down the negative z axis
    // with its target attached to the light itself.
    static std::array<double, 3> directionalTarget();
    // Whether a material names an extension. A material with no extensions object at all names
    // none, which is what the source's optional chaining amounts to.
    static bool hasMaterialExtension(const std::vector<std::string>& extensionNames,
                                     const std::string& name);

    // The material a material definition makes. Only the unlit extension changes it, and then
    // to the basic material, which ignores lighting; every other definition makes the standard
    // one. The name is returned rather than the class because the port has no material here.
    static std::string materialTypeName(bool unlit);

    // The emissive intensity an emissive strength extension asks for. The strength is copied
    // straight across, and a material without the extension keeps whatever intensity it has,
    // so the flag says whether the value should be applied at all.
    struct EmissiveIntensity {
        bool present = false;
        double value = 1;
    };

    static EmissiveIntensity emissiveIntensity(bool hasStrength, double strength);
    // The material parameter an extension field fills. The names differ in three ways: a
    // factor loses its suffix, a texture gains a map suffix, and the normal scale is one number
    // written twice. A field not in the table names nothing.
    static std::string materialParamForField(const std::string& field);

    // The material a clearcoat extension asks for. A material that carries it needs the
    // physical material, which is the only one with a clearcoat; one that does not is left to
    // whatever the rest of the definition asks for, which is reported as an empty name.
    static std::string materialTypeForClearcoat(bool hasExtension);

    // The two numbers a normal scale takes: the one the file wrote, on both axes.
    static std::array<double, 2> normalScaleFrom(double scale);
    // What a sheen extension starts from. Unlike a base colour, which starts white, the sheen
    // colour starts black and the roughness at zero, while the sheen itself is turned all the
    // way up so the extension's own values are what the material shows.
    struct SheenDefaults {
        std::array<double, 3> colour{0, 0, 0};
        double roughness = 0;
        double sheen = 1;
    };

    static SheenDefaults sheenDefaults();

    // The material parameter a sheen field fills. A factor loses its suffix, so the colour
    // factor fills the colour and the roughness factor fills the roughness; nothing else is
    // recognised here because the extension has only those two.
    static std::string sheenParamForField(const std::string& field);
    // What a volume extension starts from. The thickness is zero when absent, and the
    // attenuation colour is white.
    struct VolumeDefaults {
        double thickness = 0;
        double attenuationDistance = 0;
        std::array<double, 3> attenuationColour{1, 1, 1};
    };

    // The attenuation distance a volume extension asks for. The source writes
    // `attenuationDistance || Infinity`, so a distance of zero reads as no limit at all
    // rather than as zero: the two cannot be told apart in the file. An absent value is the
    // same, and anything else is taken as written. The result says whether it is infinite
    // rather than using a special number for it.
    struct Attenuation {
        bool infinite = true;
        double distance = 0;
    };

    static Attenuation attenuationDistance(bool hasDistance, double distance);

    // The thickness the extension asks for, which is zero when absent rather than the
    // material's own value.
    static double volumeThickness(bool hasThickness, double thickness);
    // The index of refraction an ior extension asks for. The default is one and a half, the
    // index of glass, rather than one, which would be no bending at all; a written zero is kept
    // because the test is against undefined rather than against a falsy value.
    static double indexOfRefraction(bool hasIor, double ior);

    // The specular intensity, which is one by default.
    static double specularIntensity(bool hasFactor, double factor);

    // The specular colour, which is white by default and is taken as written otherwise, in the
    // same linear space as the other factors.
    static std::array<double, 3> specularColour(bool hasColour, const std::vector<double>& colour);
    // The material parameter an iridescence field fills. Here the renames follow the same three
    // rules as clearcoat, with one addition: the index of refraction field is spelled with a
    // lower case o and the parameter with an upper case one, since the port spells the acronym
    // out in full on its own side. A field not in the table names nothing.
    static std::string iridescenceParamForField(const std::string& field);

    // The value an iridescence field that was written fills, or a flag saying it was not
    // written. Unlike the clearcoat factors, these are only set when the file names them, so a
    // material keeps whatever the physical material already has.
    struct OptionalNumber {
        bool present = false;
        double value = 0;
    };

    static OptionalNumber iridescenceValue(bool hasFactor, double factor);
    // The material parameter an anisotropy field fills. The strength is the one field in all of
    // these extensions whose suffix is not  factor , so a rule that drops that word alone would
    // leave it unmapped; the rotation keeps its name and the texture gains a map suffix.
    static std::string anisotropyParamForField(const std::string& field);

    // The three fields are all set only when written, so the same flag structure the
    // iridescence extension uses serves here too.
    static OptionalNumber anisotropyValue(bool hasValue, double value);
    // The material parameter a texture transform field fills, in the cases where it renames.
    // The coordinate becomes the channel and the scale becomes the repeat, while the offset and
    // the rotation keep their names.
    static std::string textureTransformField(const std::string& field);

    // Whether a transform asks for anything at all. A transform whose coordinate matches the
    // texture's own channel and which carries no offset, rotation or scale leaves the texture
    // alone, so the caller can keep the same object instead of a copy.
    static bool textureTransformApplies(bool hasTexCoord, int texCoord, int channel,
                                           bool hasOffset, bool hasRotation, bool hasScale);

    // The matrix glTF's order implies, which differs from the port's own order. Both orders
    // start with the translation but glTF rotates before scaling and the port scales before
    // rotating, so the matrix is written out here rather than left to the texture's own fields.
    // The six numbers are the first two columns of a three by three, in column order: the first
    // pair is the image of the x axis, the second the image of the y axis, and the third the
    // translation. The rotation is in radians and the negative sign on the second column's first
    // number is what makes the turn go the way glTF's does.
    struct TextureMatrix {
        double a = 1, b = 0, c = 0, d = 1, e = 0, f = 0;
    };

    static TextureMatrix textureTransformMatrix(std::array<double, 2> repeat,
                                                std::array<double, 2> offset, double rotation);
};
