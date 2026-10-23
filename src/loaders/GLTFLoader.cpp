#include "loaders/GLTFLoader.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace {
// The name of a component type as the file writes it, lower cased for comparison.
std::string lower(const std::string& text) {
    std::string out = text;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}
// Whether a uri ends in the given extension, with an optional query string after it.
bool endsWithExtension(const std::string& uri, const std::string& extension) {
    const std::string lowered = lower(uri);
    std::size_t end = lowered.size();
    // A query string may follow the extension.
    const std::size_t query = lowered.find('?');
    if (query != std::string::npos) end = query;
    if (end < extension.size()) return false;
    return lowered.compare(end - extension.size(), extension.size(), extension) == 0;
}
}  // namespace

std::string GLTFLoader::createAttributesKey(std::vector<Attribute> attributes) {
    // Sorting the names is what lets two primitives that list the same attributes in a
    // different order share a key.
    std::sort(attributes.begin(), attributes.end(),
              [](const Attribute& a, const Attribute& b) { return a.first < b.first; });
    std::string key;
    for (const auto& attribute : attributes) {
        key += attribute.first;
        key += ':';
        key += std::to_string(attribute.second);
        key += ';';
    }
    return key;
}

double GLTFLoader::normalizedComponentScale(const std::string& componentType) {
    // The scale is one over the largest value the type can hold.
    const std::string type = lower(componentType);
    if (type == "int8") return 1.0 / 127.0;
    if (type == "uint8") return 1.0 / 255.0;
    if (type == "int16") return 1.0 / 32767.0;
    if (type == "uint16") return 1.0 / 65535.0;
    throw std::runtime_error(
        "THREE.GLTFLoader: Unsupported normalized accessor component type.");
}

std::string GLTFLoader::imageMimeType(const std::string& uri) {
    // A data uri names its own type.
    if (uri.rfind("data:image/jpeg", 0) == 0) return "image/jpeg";
    if (uri.rfind("data:image/webp", 0) == 0) return "image/webp";
    if (uri.rfind("data:image/ktx2", 0) == 0) return "image/ktx2";
    // Otherwise the extension decides, and a query string does not hide it.
    if (endsWithExtension(uri, ".jpg") || endsWithExtension(uri, ".jpeg")) return "image/jpeg";
    if (endsWithExtension(uri, ".webp")) return "image/webp";
    if (endsWithExtension(uri, ".ktx2")) return "image/ktx2";
    // Anything else is assumed to be a png.
    return "image/png";
}

namespace {
std::uint32_t readLE32(const std::vector<std::uint8_t>& buffer, std::size_t at) {
    return static_cast<std::uint32_t>(buffer[at]) |
           (static_cast<std::uint32_t>(buffer[at + 1]) << 8) |
           (static_cast<std::uint32_t>(buffer[at + 2]) << 16) |
           (static_cast<std::uint32_t>(buffer[at + 3]) << 24);
}
}  // namespace

GLTFLoader::BinaryHeader GLTFLoader::readBinaryHeader(const std::vector<std::uint8_t>& buffer) {
    if (buffer.size() < static_cast<std::size_t>(kBinaryHeaderLength))
        throw std::runtime_error("THREE.GLTFLoader: Unsupported glTF-Binary header.");

    BinaryHeader header;
    // The magic is four characters rather than a number.
    header.magic.assign(reinterpret_cast<const char*>(buffer.data()), 4);
    header.version = readLE32(buffer, 4);
    header.length = readLE32(buffer, 8);

    // A file that does not begin with the magic is not a binary glTF at all.
    if (header.magic != kBinaryMagic)
        throw std::runtime_error("THREE.GLTFLoader: Unsupported glTF-Binary header.");
    // The first version of the format laid its chunks out differently.
    if (header.version < 2)
        throw std::runtime_error("THREE.GLTFLoader: Legacy binary file detected.");
    return header;
}

std::vector<GLTFLoader::BinaryChunk> GLTFLoader::readBinaryChunks(
    const std::vector<std::uint8_t>& buffer, const BinaryHeader& header) {
    std::vector<BinaryChunk> chunks;
    // The chunks fill whatever the header length leaves.
    if (header.length < static_cast<std::uint32_t>(kBinaryHeaderLength)) return chunks;
    std::size_t contentsLength =
        static_cast<std::size_t>(header.length) - static_cast<std::size_t>(kBinaryHeaderLength);
    std::size_t index = 0;
    const std::size_t base = static_cast<std::size_t>(kBinaryHeaderLength);

    // Each chunk is a length and a type followed by that many bytes.
    while (index + 8 <= contentsLength && base + index + 8 <= buffer.size()) {
        const std::uint32_t chunkLength = readLE32(buffer, base + index);
        index += 4;
        const std::uint32_t chunkType = readLE32(buffer, base + index);
        index += 4;

        BinaryChunk chunk;
        chunk.type = chunkType;
        chunk.offset = base + index;
        chunk.length = chunkLength;
        chunks.push_back(chunk);

        // A chunk the caller does not know is skipped rather than refused.
        index += chunkLength;
    }
    return chunks;
}

std::string GLTFLoader::componentTypeName(std::uint32_t componentType) {
    // The six types the format allows, by the number the file writes.
    switch (componentType) {
        case 5120: return "int8";
        case 5121: return "uint8";
        case 5122: return "int16";
        case 5123: return "uint16";
        case 5125: return "uint32";
        case 5126: return "float32";
        default:
            throw std::runtime_error("THREE.GLTFLoader: Unsupported component type.");
    }
}

int GLTFLoader::componentTypeSize(std::uint32_t componentType) {
    // The width in bytes of one component.
    switch (componentType) {
        case 5120: return 1;
        case 5121: return 1;
        case 5122: return 2;
        case 5123: return 2;
        case 5125: return 4;
        case 5126: return 4;
        default:
            throw std::runtime_error("THREE.GLTFLoader: Unsupported component type.");
    }
}

int GLTFLoader::typeSize(const std::string& type) {
    // A matrix holds as many components as its rows times its columns.
    if (type == "SCALAR") return 1;
    if (type == "VEC2") return 2;
    if (type == "VEC3") return 3;
    if (type == "VEC4") return 4;
    if (type == "MAT2") return 4;
    if (type == "MAT3") return 9;
    if (type == "MAT4") return 16;
    throw std::runtime_error("THREE.GLTFLoader: Unsupported accessor type.");
}

std::string GLTFLoader::attributeName(const std::string& semantic) {
    // The ten the parser names explicitly.
    if (semantic == "POSITION") return "position";
    if (semantic == "NORMAL") return "normal";
    if (semantic == "TANGENT") return "tangent";
    if (semantic == "TEXCOORD_0") return "uv";
    if (semantic == "TEXCOORD_1") return "uv1";
    if (semantic == "TEXCOORD_2") return "uv2";
    if (semantic == "TEXCOORD_3") return "uv3";
    if (semantic == "COLOR_0") return "color";
    if (semantic == "WEIGHTS_0") return "skinWeight";
    if (semantic == "JOINTS_0") return "skinIndex";
    // Anything else is its own name lower cased.
    std::string name = semantic;
    for (char& c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return name;
}

std::string GLTFLoader::filterName(std::uint32_t filter) {
    switch (filter) {
        case 9728: return "NearestFilter";
        case 9729: return "LinearFilter";
        case 9984: return "NearestMipmapNearestFilter";
        case 9985: return "LinearMipmapNearestFilter";
        case 9986: return "NearestMipmapLinearFilter";
        case 9987: return "LinearMipmapLinearFilter";
        default:
            throw std::runtime_error("THREE.GLTFLoader: Unknown filter.");
    }
}

std::string GLTFLoader::wrappingName(std::uint32_t wrapping) {
    switch (wrapping) {
        case 33071: return "ClampToEdgeWrapping";
        case 33648: return "MirroredRepeatWrapping";
        case 10497: return "RepeatWrapping";
        default:
            throw std::runtime_error("THREE.GLTFLoader: Unknown wrapping.");
    }
}

std::vector<std::uint8_t> GLTFLoader::sliceBufferView(const std::vector<std::uint8_t>& buffer,
                                                      std::size_t byteOffset,
                                                      std::size_t byteLength) {
    // A view that reaches past the buffer is cut at its end rather than refused, which is what
    // taking a slice does.
    if (byteOffset >= buffer.size() || byteLength == 0) return {};
    const std::size_t end = std::min(buffer.size(), byteOffset + byteLength);
    return std::vector<std::uint8_t>(buffer.begin() + static_cast<std::ptrdiff_t>(byteOffset),
                                     buffer.begin() + static_cast<std::ptrdiff_t>(end));
}

std::size_t GLTFLoader::accessorComponentCount(std::size_t count, const std::string& type) {
    // Each element holds as many components as its shape says.
    return count * static_cast<std::size_t>(typeSize(type));
}

bool GLTFLoader::isInterleaved(std::size_t byteStride, std::size_t itemBytes) {
    // No stride, or one that matches the element size, is the plain layout.
    return byteStride != 0 && byteStride != itemBytes;
}

std::size_t GLTFLoader::interleavedSlice(std::size_t byteOffset, std::size_t byteStride) {
    // Which run of elements the offset begins in.
    if (byteStride == 0) return 0;
    return byteOffset / byteStride;
}

std::size_t GLTFLoader::interleavedAttributeOffset(std::size_t byteOffset, std::size_t byteStride,
                                                   std::size_t elementBytes) {
    // The attribute begins part way into each element, counted in components.
    if (byteStride == 0 || elementBytes == 0) return 0;
    return (byteOffset % byteStride) / elementBytes;
}

std::size_t GLTFLoader::interleavedStride(std::size_t byteStride, std::size_t elementBytes) {
    // The buffer counts its stride in components rather than bytes.
    if (elementBytes == 0) return 0;
    return byteStride / elementBytes;
}

void GLTFLoader::applySparse(const std::vector<std::uint32_t>& indices,
                             const std::vector<double>& values, std::size_t itemSize,
                             std::vector<double>& target) {
    if (itemSize == 0) return;
    // Each index names one element and the values run on in that order.
    for (std::size_t entry = 0; entry < indices.size(); ++entry) {
        const std::size_t base = static_cast<std::size_t>(indices[entry]) * itemSize;
        for (std::size_t component = 0; component < itemSize; ++component) {
            // An element past the end, or a value run that stopped short, leaves what was there.
            if (base + component >= target.size()) break;
            const std::size_t source = entry * itemSize + component;
            if (source >= values.size()) break;
            target[base + component] = values[source];
        }
    }
}

bool GLTFLoader::sparseNormalized(bool /*normalized*/) {
    // Whatever the accessor declared, a sparse override reports itself as not normalised.
    return false;
}

GLTFLoader::NodeTransform GLTFLoader::nodeTransform(
    bool hasMatrix, const std::array<double, 16>& matrix, bool hasTranslation,
    const std::array<double, 3>& translation, bool hasRotation,
    const std::array<double, 4>& rotation, bool hasScale, const std::array<double, 3>& scale) {
    NodeTransform transform;
    // A matrix replaces the three entirely rather than combining with them.
    if (hasMatrix) {
        transform.hasMatrix = true;
        transform.matrix = matrix;
        return transform;
    }
    // Each of the three is taken on its own and left alone when it was not given.
    if (hasTranslation) transform.translation = translation;
    if (hasRotation) transform.rotation = rotation;
    if (hasScale) transform.scale = scale;
    return transform;
}

std::string GLTFLoader::pathProperty(const std::string& path) {
    // Three of the four are renamed on the way in.
    if (path == "scale") return "scale";
    if (path == "translation") return "position";
    if (path == "rotation") return "quaternion";
    if (path == "weights") return "morphTargetInfluences";
    throw std::runtime_error("THREE.GLTFLoader: Unsupported animation path.");
}

bool GLTFLoader::isValidAlphaMode(const std::string& mode) {
    // The mode is stored as it is written, so only the three words are checked.
    return mode == "OPAQUE" || mode == "MASK" || mode == "BLEND";
}

std::string GLTFLoader::interpolationName(const std::string& interpolation) {
    // A cubic spline has no interpolant of its own here, so its name is passed through.
    if (interpolation == "CUBICSPLINE") return "CUBICSPLINE";
    if (interpolation == "LINEAR") return "InterpolateLinear";
    if (interpolation == "STEP") return "InterpolateDiscrete";
    throw std::runtime_error("THREE.GLTFLoader: Unsupported interpolation.");
}

GLTFLoader::MaterialColour GLTFLoader::materialDefaults() {
    // White and fully opaque.
    MaterialColour colour;
    colour.r = 1.0;
    colour.g = 1.0;
    colour.b = 1.0;
    colour.a = 1.0;
    return colour;
}

GLTFLoader::MaterialColour GLTFLoader::applyBaseColorFactor(MaterialColour colour,
                                                            const std::vector<double>& factor) {
    // The first three are the colour and the fourth the alpha; anything missing is left alone.
    if (factor.size() >= 1) colour.r = factor[0];
    if (factor.size() >= 2) colour.g = factor[1];
    if (factor.size() >= 3) colour.b = factor[2];
    if (factor.size() >= 4) colour.a = factor[3];
    return colour;
}

GLTFLoader::SamplerSettings GLTFLoader::samplerSettings(const std::string& magFilter,
                                                        const std::string& minFilter,
                                                        const std::string& wrapS,
                                                        const std::string& wrapT) {
    SamplerSettings settings;
    // The two filters fall back differently: magnification to the plain linear filter and
    // minification to the one that also blends between levels.
    settings.magFilter = magFilter.empty() ? "LinearFilter" : magFilter;
    settings.minFilter = minFilter.empty() ? "LinearMipmapLinearFilter" : minFilter;
    // Both axes fall back to repeating.
    settings.wrapS = wrapS.empty() ? "RepeatWrapping" : wrapS;
    settings.wrapT = wrapT.empty() ? "RepeatWrapping" : wrapT;
    return settings;
}

bool GLTFLoader::needsMipmaps(bool compressed, const std::string& minFilter) {
    // A compressed texture carries its levels already, and a single level filter never asks for
    // them.
    if (compressed) return false;
    if (minFilter == "NearestFilter") return false;
    if (minFilter == "LinearFilter") return false;
    return true;
}

std::string GLTFLoader::createPrimitiveKey(bool hasDraco, int dracoBufferView, int dracoIndices,
                                           std::vector<Attribute> dracoAttributes, int indices,
                                           std::vector<Attribute> attributes, int mode,
                                           const std::vector<std::vector<Attribute>>& targets) {
    std::string key;
    if (hasDraco) {
        // A compressed primitive is keyed by what the extension names.
        key = "draco:" + std::to_string(dracoBufferView) + ":" + std::to_string(dracoIndices) +
              ":" + createAttributesKey(std::move(dracoAttributes));
    } else {
        // An ordinary one by its index accessor, its attributes and its mode.
        key = std::to_string(indices) + ":" + createAttributesKey(std::move(attributes)) + ":" +
              std::to_string(mode);
    }
    // Each morph target adds its own attributes.
    for (const auto& target : targets) {
        key += ":" + createAttributesKey(target);
    }
    return key;
}

const std::vector<std::string>& GLTFLoader::knownExtensions() {
    // The twenty four names the source lists, in its order.
    static const std::vector<std::string> names{
        "KHR_binary_glTF",
        "KHR_draco_mesh_compression",
        "KHR_lights_punctual",
        "KHR_materials_clearcoat",
        "KHR_materials_diffuse_roughness",
        "KHR_materials_dispersion",
        "KHR_materials_ior",
        "KHR_materials_sheen",
        "KHR_materials_specular",
        "KHR_materials_transmission",
        "KHR_materials_iridescence",
        "KHR_materials_anisotropy",
        "KHR_materials_unlit",
        "KHR_materials_volume",
        "KHR_texture_basisu",
        "KHR_texture_transform",
        "KHR_mesh_quantization",
        "KHR_materials_emissive_strength",
        "EXT_materials_bump",
        "EXT_texture_webp",
        "EXT_texture_avif",
        "EXT_meshopt_compression",
        "KHR_meshopt_compression",
        "EXT_mesh_gpu_instancing",
    };
    return names;
}

bool GLTFLoader::isKnownExtension(const std::string& name) {
    const auto& names = knownExtensions();
    return std::find(names.begin(), names.end(), name) != names.end();
}

std::size_t GLTFLoader::knownExtensionCount() { return knownExtensions().size(); }

bool GLTFLoader::isLightType(const std::string& type) {
    // The three the extension allows.
    return type == "directional" || type == "point" || type == "spot";
}

double GLTFLoader::lightRange(bool hasRange, double range) {
    // An absent range reads as zero, which stands for no limit.
    return hasRange ? range : 0.0;
}

std::array<double, 3> GLTFLoader::lightColour(bool hasColour,
                                              const std::vector<double>& colour) {
    // White when nothing is written.
    if (!hasColour || colour.size() < 3) return {1.0, 1.0, 1.0};
    // Otherwise the three numbers as they stand, with no conversion.
    return {colour[0], colour[1], colour[2]};
}

std::array<double, 3> GLTFLoader::directionalTarget() {
    // A directional light points down the negative z axis until it is turned.
    return {0.0, 0.0, -1.0};
}

bool GLTFLoader::hasMaterialExtension(const std::vector<std::string>& extensionNames,
                                      const std::string& name) {
    // A missing extensions object names nothing, so only the listed names count.
    return std::find(extensionNames.begin(), extensionNames.end(), name) != extensionNames.end();
}

std::string GLTFLoader::materialTypeName(bool unlit) {
    // Only the unlit extension changes the material type, and then to one that ignores lights.
    return unlit ? "MeshBasicMaterial" : "MeshStandardMaterial";
}

GLTFLoader::EmissiveIntensity GLTFLoader::emissiveIntensity(bool hasStrength, double strength) {
    EmissiveIntensity result;
    // The strength is copied only when the extension actually wrote one.
    if (hasStrength) {
        result.present = true;
        result.value = strength;
    }
    return result;
}

std::string GLTFLoader::materialParamForField(const std::string& field) {
    // A factor drops its suffix; a texture gains a map suffix; the normal scale is one number.
    if (field == "clearcoatFactor") return "clearcoat";
    if (field == "clearcoatTexture") return "clearcoatMap";
    if (field == "clearcoatRoughnessFactor") return "clearcoatRoughness";
    if (field == "clearcoatRoughnessTexture") return "clearcoatRoughnessMap";
    if (field == "clearcoatNormalTexture") return "clearcoatNormalMap";
    if (field == "clearcoatNormalTexture.scale") return "clearcoatNormalScale";
    // Anything else names no parameter.
    return std::string();
}

std::string GLTFLoader::materialTypeForClearcoat(bool hasExtension) {
    // Only the physical material carries a clearcoat.
    return hasExtension ? "MeshPhysicalMaterial" : std::string();
}

std::array<double, 2> GLTFLoader::normalScaleFrom(double scale) {
    // One number on both axes.
    return {scale, scale};
}

GLTFLoader::SheenDefaults GLTFLoader::sheenDefaults() {
    // Black, smooth, and turned all the way up.
    SheenDefaults defaults;
    defaults.colour = {0.0, 0.0, 0.0};
    defaults.roughness = 0.0;
    defaults.sheen = 1.0;
    return defaults;
}

std::string GLTFLoader::sheenParamForField(const std::string& field) {
    // A factor loses its suffix.
    if (field == "sheenColorFactor") return "sheenColor";
    if (field == "sheenRoughnessFactor") return "sheenRoughness";
    // The extension has only those two, so anything else names nothing.
    return std::string();
}

GLTFLoader::Attenuation GLTFLoader::attenuationDistance(bool hasDistance, double distance) {
    Attenuation result;
    // A missing distance and a distance of zero both mean no limit, because the source's `||`
    // treats zero as if it were not there.
    if (!hasDistance || distance == 0.0) {
        result.infinite = true;
        result.distance = 0.0;
        return result;
    }
    result.infinite = false;
    result.distance = distance;
    return result;
}

double GLTFLoader::volumeThickness(bool hasThickness, double thickness) {
    // The thickness is zero when the file leaves it out, not the material's own value.
    return hasThickness ? thickness : 0.0;
}

double GLTFLoader::indexOfRefraction(bool hasIor, double ior) {
    // Glass by default.
    if (!hasIor) return 1.5;
    // A written zero is kept: the source tests against undefined, not against falsiness.
    return ior;
}

double GLTFLoader::specularIntensity(bool hasFactor, double factor) {
    // Fully specular by default.
    return hasFactor ? factor : 1.0;
}

std::array<double, 3> GLTFLoader::specularColour(bool hasColour,
                                                 const std::vector<double>& colour) {
    // White by default.
    if (!hasColour || colour.size() < 3) return {1.0, 1.0, 1.0};
    // Otherwise as written, with no conversion.
    return {colour[0], colour[1], colour[2]};
}

std::string GLTFLoader::iridescenceParamForField(const std::string& field) {
    // A factor drops its suffix and a texture gains a map suffix.
    if (field == "iridescenceFactor") return "iridescence";
    if (field == "iridescenceTexture") return "iridescenceMap";
    // The index of refraction is spelled with a lower case o in the file and an upper case one
    // on the port's side.
    if (field == "iridescenceIor") return "iridescenceIOR";
    // Anything else names no parameter.
    return std::string();
}

GLTFLoader::OptionalNumber GLTFLoader::iridescenceValue(bool hasFactor, double factor) {
    // The value is only set when the file names it, so a written zero is kept and a missing
    // one leaves the material alone.
    OptionalNumber result;
    if (hasFactor) {
        result.present = true;
        result.value = factor;
    }
    return result;
}

std::string GLTFLoader::anisotropyParamForField(const std::string& field) {
    // The strength drops a suffix that is not the one the other extensions use.
    if (field == "anisotropyStrength") return "anisotropy";
    // The rotation keeps its name.
    if (field == "anisotropyRotation") return "anisotropyRotation";
    // A texture gains a map suffix.
    if (field == "anisotropyTexture") return "anisotropyMap";
    // Anything else names no parameter.
    return std::string();
}

GLTFLoader::OptionalNumber GLTFLoader::anisotropyValue(bool hasValue, double value) {
    // Set only when written, so a written zero is kept.
    OptionalNumber result;
    if (hasValue) {
        result.present = true;
        result.value = value;
    }
    return result;
}

std::string GLTFLoader::textureTransformField(const std::string& field) {
    // The coordinate becomes the channel and the scale becomes the repeat.
    if (field == "texCoord") return "channel";
    if (field == "scale") return "repeat";
    // The offset and the rotation keep their names.
    if (field == "offset") return "offset";
    if (field == "rotation") return "rotation";
    return std::string();
}


GLTFLoader::TextureMatrix GLTFLoader::textureTransformMatrix(std::array<double, 2> repeat,
                                                             std::array<double, 2> offset,
                                                             double rotation) {
    const double cosine = std::cos(rotation);
    const double sine = std::sin(rotation);
    TextureMatrix matrix;
    // The first column scales and turns the x axis.
    matrix.a = repeat[0] * cosine;
    matrix.b = -repeat[0] * sine;
    // The second scales and turns the y axis, with its own factors rather than the x one's.
    matrix.c = repeat[1] * sine;
    matrix.d = repeat[1] * cosine;
    // The translation comes last.
    matrix.e = offset[0];
    matrix.f = offset[1];
    return matrix;
}
bool GLTFLoader::textureTransformApplies(bool hasTexCoord, int texCoord, int channel,
                                           bool hasOffset, bool hasRotation, bool hasScale) {
    // A transform that only restates the texture's own channel and carries no offset,
    // rotation or scale asks for nothing, so the texture need not be copied.
    const bool channelAgrees = !hasTexCoord || texCoord == channel;
    const bool nothingElse = !hasOffset && !hasRotation && !hasScale;
    return !(channelAgrees && nothingElse);
}

int GLTFLoader::defaultPrimitiveMode() {
    // Triangles when nothing is written.
    return kTriangles;
}

bool GLTFLoader::supportsInstancing(bool hasMode, int mode) {
    // A missing mode is the default and therefore a triangle mode.
    if (!hasMode) return true;
    // Of the seven modes only the three that build triangles can be instanced here.
    return mode == kTriangles || mode == kTriangleStrip || mode == kTriangleFan;
}

bool GLTFLoader::hasInstanceAttributes(std::size_t attributeCount) {
    // Nothing to place the copies by.
    return attributeCount >= 1;
}

std::string GLTFLoader::diffuseRoughnessParamForField(const std::string& field) {
    // The factor drops its suffix and the texture gains a map suffix.
    if (field == "diffuseRoughnessFactor") return "diffuseRoughness";
    if (field == "diffuseRoughnessTexture") return "diffuseRoughnessMap";
    // The value's own name is a field of the dispersion extension, not of this one.
    if (field == "diffuseRoughness") return std::string();
    return std::string();
}

double GLTFLoader::diffuseRoughness(bool hasFactor, double factor) {
    // Always written, so an absent factor means zero rather than the material's own value.
    return hasFactor ? factor : 0.0;
}

double GLTFLoader::dispersion(bool hasValue, double value) {
    // Written the same way the roughness factor is.
    return hasValue ? value : 0.0;
}

GLTFLoader::DefaultMaterialSettings GLTFLoader::defaultMaterialSettings() {
    DefaultMaterialSettings settings;
    // Fully metallic and fully rough, which is the value the port starts from rather than a
    // halfway material.
    settings.metalness = 1.0;
    settings.roughness = 1.0;
    return settings;
}

std::vector<std::string> GLTFLoader::unknownExtensions(const std::vector<std::string>& present,
                                                       const std::vector<std::string>& known) {
    std::vector<std::string> unknown;
    for (const auto& name : present) {
        // A name the loader knows is handled elsewhere; the rest are kept as they are.
        if (std::find(known.begin(), known.end(), name) == known.end()) unknown.push_back(name);
    }
    return unknown;
}

GLTFLoader::ExtrasDisposition GLTFLoader::extrasDisposition(bool hasExtras,
                                                            const std::string& typeName) {
    // Nothing to do when there are no extras at all.
    if (!hasExtras) return ExtrasDisposition::None;
    // Only the object type merges, and null and arrays both report themselves as objects.
    if (typeName == "object") return ExtrasDisposition::Assign;
    // Anything else is reported and dropped.
    return ExtrasDisposition::Warn;
}

std::string GLTFLoader::extrasWarning(const std::string& valueText) {
    // The message the source prints, with the value appended as it would be converted to text.
    return "THREE.GLTFLoader: Ignoring primitive type .extras, " + valueText;
}

bool GLTFLoader::materialExtensionPresent(bool hasExtensions, bool valueIsTruthy) {
    // A missing extensions object carries nothing, and a value that is not true counts as absent.
    return hasExtensions && valueIsTruthy;
}

bool GLTFLoader::targetNamesApply(bool isArray, std::size_t nameCount,
                                  std::size_t influenceCount) {
    // The names are only usable when they are an array of exactly the right length.
    return isArray && nameCount == influenceCount;
}

std::size_t GLTFLoader::weightsWritten(std::size_t weightCount, std::size_t influenceCount) {
    // The shorter of the two, since the source writes weight by weight into the influences.
    return weightCount < influenceCount ? weightCount : influenceCount;
}

GLTFLoader::MorphFlags GLTFLoader::morphFlags(const std::vector<bool>& hasPosition,
                                              const std::vector<bool>& hasNormal,
                                              const std::vector<bool>& hasColour) {
    MorphFlags flags;
    // Any single target that names an attribute is enough to turn that attribute on.
    for (std::size_t i = 0; i < hasPosition.size(); ++i) {
        if (hasPosition[i]) flags.position = true;
    }
    for (std::size_t i = 0; i < hasNormal.size(); ++i) {
        if (hasNormal[i]) flags.normal = true;
    }
    for (std::size_t i = 0; i < hasColour.size(); ++i) {
        if (hasColour[i]) flags.colour = true;
    }
    return flags;
}

bool GLTFLoader::morphsAnything(const MorphFlags& flags) {
    // All three off means the geometry is left exactly as it came in.
    return flags.position || flags.normal || flags.colour;
}

GLTFLoader::MorphSource GLTFLoader::morphSource(bool flagSet, bool targetHasAttribute) {
    // The attribute is only read when its flag is on, and then from the target when the target
    // names one and from the mesh itself when it does not.
    if (!flagSet) return MorphSource::Base;
    return targetHasAttribute ? MorphSource::Target : MorphSource::Base;
}

GLTFLoader::BoundsResult GLTFLoader::boundsFromAccessor(bool hasMin, bool hasMax,
                                                        std::array<double, 3> min,
                                                        std::array<double, 3> max, bool normalized,
                                                        const std::string& componentType) {
    BoundsResult result;
    // Both corners are required; without them no box can be built.
    if (!hasMin || !hasMax) return result;
    result.present = true;
    result.bounds.min = min;
    result.bounds.max = max;
    // A normalised accessor stores integers, so both corners scale by the same factor.
    if (normalized) {
        const double scale = normalizedComponentScale(componentType);
        for (int axis = 0; axis < 3; ++axis) {
            result.bounds.min[axis] *= scale;
            result.bounds.max[axis] *= scale;
        }
    }
    return result;
}

std::string GLTFLoader::boundsWarning() {
    return "THREE.GLTFLoader: Missing min/max properties for accessor POSITION.";
}

std::array<double, 3> GLTFLoader::morphDisplacement(std::array<double, 3> min,
                                                    std::array<double, 3> max) {
    std::array<double, 3> reach{};
    // The larger magnitude of the two corners on each axis, since the weight can be negative.
    for (int axis = 0; axis < 3; ++axis) {
        reach[axis] = std::max(std::abs(min[axis]), std::abs(max[axis]));
    }
    return reach;
}

bool GLTFLoader::shouldSetAttribute(bool alreadyPresent) {
    // An attribute the geometry already has is left as it is.
    return !alreadyPresent;
}

bool GLTFLoader::shouldSetIndex(bool hasIndices, bool alreadyIndexed) {
    // Only when the file names an index accessor and the geometry has no index yet.
    return hasIndices && !alreadyIndexed;
}

std::string GLTFLoader::colourSpaceWarning(const std::string& workingSpace,
                                           bool hasVertexColour) {
    // Nothing to report when there are no vertex colours or the spaces already agree.
    if (!hasVertexColour || workingSpace == "srgb-linear") return std::string();
    return "THREE.GLTFLoader: Converting vertex colors from \"srgb-linear\" to \"" + workingSpace +
           "\" not supported.";
}