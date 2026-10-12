#include "materials/Material.h"
#include "math/MathUtils.h"
#include "math/Plane.h"
#include <iostream>
#include <limits>
#include "textures/Texture.h"
#include "core/NativeObject.h"
Material::Material() : uuid(MathUtils::generateUUID()) {}
namespace {
JsonValue fieldJSON(Material::PropertyReference property) {
    return std::visit([](auto* field) -> JsonValue {
        using T = std::remove_cvref_t<decltype(*field)>;
        if constexpr (std::is_same_v<T, Color>) return field->getHex();
        else if constexpr (std::is_same_v<T, Vector2>) return JsonValue::Array{field->x, field->y};
        else if constexpr (std::is_same_v<T, Euler>) return JsonValue::Array{field->x, field->y, field->z, field->getOrderString()};
        else if constexpr (std::is_same_v<T, Material::AlphaTest> || std::is_same_v<T, Material::Reflectivity>) return double(*field);
        else if constexpr (std::is_same_v<T, std::optional<int>> || std::is_same_v<T, std::optional<std::string>>) return *field ? JsonValue(**field) : JsonValue();
        else if constexpr (std::is_same_v<T, std::shared_ptr<Texture>>) return *field ? JsonValue((*field)->uuid) : JsonValue();
        else if constexpr (std::is_same_v<T, Material::ClippingPlanes>) {
            if (!*field) return nullptr;
            JsonValue::Array result;
            for (const auto& plane : **field) {
                if (!plane) throw JSTypeError("Cannot read properties of null (reading 'toJSON')");
                result.push_back(plane->toJSON());
            }
            return result;
        } else if constexpr (std::is_same_v<T, JSValue>) return field->toJSON().value_or(JsonValue());
        else if constexpr (std::is_same_v<T, JsonValue>) return field->sharedCopy();
        else return *field;
    }, property);
}
void assignJSON(Material::PropertyReference property, const JsonValue& value) {
    std::visit([&](auto* field) {
        using T = std::remove_cvref_t<decltype(*field)>;
        if constexpr (std::is_same_v<T, Color>) {
            if (std::holds_alternative<double>(value.storage())) field->setHex(value.get<double>());
            else if (std::holds_alternative<std::string>(value.storage())) field->setStyle(value.get<std::string>());
            // Color.set ignores values that are neither a number, string, nor Color.
        } else if constexpr (std::is_same_v<T, Vector2>) {
            throw JSTypeError("Material vector parameters require a native Vector2; use fromJSON for serialized arrays");
        } else if constexpr (std::is_same_v<T, Euler>) {
            throw JSTypeError("Material rotation parameters require a native Euler; use fromJSON for serialized arrays");
        } else if constexpr (std::is_same_v<T, std::shared_ptr<Texture>>) {
            if (!value.isNull()) throw JSTypeError("Material texture parameters require a native Texture");
            field->reset();
        } else if constexpr (std::is_same_v<T, Material::ClippingPlanes>) {
            if (!value.isNull()) throw JSTypeError("Material clippingPlanes requires native Plane objects; use fromJSON for serialized arrays");
            field->reset();
        } else if constexpr (std::is_same_v<T, std::optional<int>>) {
            *field = value.isNull() ? std::nullopt : std::optional<int>(int(value.get<double>()));
        } else if constexpr (std::is_same_v<T, std::optional<std::string>>) {
            *field = value.isNull() ? std::nullopt : std::optional<std::string>(value.get<std::string>());
        } else if constexpr (std::is_same_v<T, JSValue>) *field = JSValue::fromJSON(value);
        else if constexpr (std::is_same_v<T, JsonValue>) *field = value.sharedCopy();
        else if constexpr (std::is_same_v<T, bool>) *field = value.get<bool>();
        else if constexpr (std::is_same_v<T, std::string>) *field = value.get<std::string>();
        else if constexpr (std::is_same_v<T, Material::AlphaTest> || std::is_same_v<T, Material::Reflectivity>) *field = value.get<double>();
        else *field = static_cast<T>(value.get<double>());
    }, property);
}
}
Material::Reflectivity::operator double() const {
    return MathUtils::clamp(2.5 * (*ior - 1) / (*ior + 1), 0.0, 1.0);
}
Material::Reflectivity& Material::Reflectivity::operator=(double value) {
    *ior = (1 + 0.4 * value) / (1 - 0.4 * value);
    return *this;
}
void Material::setValues(const JsonValue& parameters) {
    if (parameters.isNull()) return;
    for (const auto& [key, value] : parameters.get<JsonValue::Object>()) {
        if (const auto field = property(key)) {
            const auto runtime = std::get_if<JSValue*>(&*field);
            if (runtime && (**runtime).isUndefined()) {
                std::cerr << "THREE.Material: '" << key << "' is not a property of THREE." << type << ".\n";
                continue;
            }
            assignJSON(*field, value);
        }
        else if (!extraProperties.get(key).isUndefined()) {
            const auto previous = extraProperties.get(key);
            if (previous.isHostObject("three.Color")) {
                const auto color = std::static_pointer_cast<Color>(previous.hostPayload("three.Color"));
                assignJSON(color.get(), value);
            } else extraProperties.set(key, JSValue::fromJSON(value));
        }
        else std::cerr << "THREE.Material: '" << key << "' is not a property of THREE." << type << ".\n";
    }
}
void Material::visitProperties(const PropertyVisitor& visitor) {
    visitor("uuid", &uuid);
    visitor("name", &name);
    visitor("type", &type);
    visitor("blending", &blending);
    visitor("side", &side);
    visitor("vertexColors", &vertexColors);
    visitor("opacity", &opacity);
    visitor("transparent", &transparent);
    visitor("alphaHash", &alphaHash);
    visitor("blendSrc", &blendSrc);
    visitor("blendDst", &blendDst);
    visitor("blendEquation", &blendEquation);
    visitor("blendSrcAlpha", &blendSrcAlpha);
    visitor("blendDstAlpha", &blendDstAlpha);
    visitor("blendEquationAlpha", &blendEquationAlpha);
    visitor("blendColor", &blendColor);
    visitor("blendAlpha", &blendAlpha);
    visitor("depthFunc", &depthFunc);
    visitor("depthTest", &depthTest);
    visitor("depthWrite", &depthWrite);
    visitor("stencilWriteMask", &stencilWriteMask);
    visitor("stencilFunc", &stencilFunc);
    visitor("stencilRef", &stencilRef);
    visitor("stencilFuncMask", &stencilFuncMask);
    visitor("stencilFail", &stencilFail);
    visitor("stencilZFail", &stencilZFail);
    visitor("stencilZPass", &stencilZPass);
    visitor("stencilWrite", &stencilWrite);
    visitor("clippingPlanes", &clippingPlanes);
    visitor("clipIntersection", &clipIntersection);
    visitor("clipShadows", &clipShadows);
    visitor("shadowSide", &shadowSide);
    visitor("colorWrite", &colorWrite);
    visitor("precision", &precision);
    visitor("polygonOffset", &polygonOffset);
    visitor("polygonOffsetFactor", &polygonOffsetFactor);
    visitor("polygonOffsetUnits", &polygonOffsetUnits);
    visitor("dithering", &dithering);
    visitor("alphaToCoverage", &alphaToCoverage);
    visitor("premultipliedAlpha", &premultipliedAlpha);
    visitor("forceSinglePass", &forceSinglePass);
    visitor("allowOverride", &allowOverride);
    visitor("visible", &visible);
    visitor("toneMapped", &toneMapped);
    visitor("userData", &userData);
    visitor("version", &version);
    visitor("_alphaTest", &alphaTest.value);
    visitor("alphaTest", &alphaTest);
}
std::optional<Material::PropertyReference> Material::property(std::string_view key) {
    std::optional<PropertyReference> result;
    visitProperties([&](std::string_view name, PropertyReference field) { if (name == key) result = field; });
    return result;
}
Material& Material::copy(const Material& s) {
    const auto sourcePlanes = s.clippingPlanes;
    name = s.name;
    blending = s.blending;
    side = s.side;
    vertexColors = s.vertexColors;
    opacity = s.opacity;
    transparent = s.transparent;
    blendSrc = s.blendSrc;
    blendDst = s.blendDst;
    blendEquation = s.blendEquation;
    blendSrcAlpha = s.blendSrcAlpha;
    blendDstAlpha = s.blendDstAlpha;
    blendEquationAlpha = s.blendEquationAlpha;
    blendColor.copy(s.blendColor);
    blendAlpha = s.blendAlpha;
    depthFunc = s.depthFunc;
    depthTest = s.depthTest;
    depthWrite = s.depthWrite;
    stencilWriteMask = s.stencilWriteMask;
    stencilFunc = s.stencilFunc;
    stencilRef = s.stencilRef;
    stencilFuncMask = s.stencilFuncMask;
    stencilFail = s.stencilFail;
    stencilZFail = s.stencilZFail;
    stencilZPass = s.stencilZPass;
    stencilWrite = s.stencilWrite;
    ClippingPlanes copiedPlanes;
    if (sourcePlanes) {
        copiedPlanes.emplace();
        for (const auto& plane : *sourcePlanes) {
            if (!plane)
                throw JSTypeError("Cannot read properties of null (reading 'clone')");
            copiedPlanes->push_back(std::make_shared<Plane>(plane->clone()));
        }
    }
    clippingPlanes = std::move(copiedPlanes);
    clipIntersection = s.clipIntersection;
    clipShadows = s.clipShadows;
    shadowSide = s.shadowSide;
    colorWrite = s.colorWrite;
    precision = s.precision;
    polygonOffset = s.polygonOffset;
    polygonOffsetFactor = s.polygonOffsetFactor;
    polygonOffsetUnits = s.polygonOffsetUnits;
    dithering = s.dithering;
    alphaTest = static_cast<double>(s.alphaTest);
    alphaHash = s.alphaHash;
    alphaToCoverage = s.alphaToCoverage;
    premultipliedAlpha = s.premultipliedAlpha;
    forceSinglePass = s.forceSinglePass;
    allowOverride = s.allowOverride;
    visible = s.visible;
    toneMapped = s.toneMapped;
    userData = s.userData.copyJSON();
    return *this;
}
std::shared_ptr<Material> Material::clone() const {
    auto result = std::make_shared<Material>();
    result->copy(*this);
    return result;
}
JsonValue Material::serializeCommonJSON() const {
    JsonValue result = JsonValue::Object{};
    result["metadata"] =
        JsonValue::Object{{"version", 4.7}, {"type", "Material"}, {"generator", "Material.toJSON"}};
    result["uuid"] = uuid;
    result["type"] = type;
    result["blending"] = blending;
    result["side"] = side;
    result["shadowSide"] = shadowSide ? JsonValue(*shadowSide) : JsonValue();
    result["vertexColors"] = vertexColors;
    result["opacity"] = opacity;
    result["transparent"] = transparent;
    result["blendSrc"] = blendSrc;
    result["blendDst"] = blendDst;
    result["blendEquation"] = blendEquation;
    result["blendSrcAlpha"] = blendSrcAlpha ? JsonValue(*blendSrcAlpha) : JsonValue();
    result["blendDstAlpha"] = blendDstAlpha ? JsonValue(*blendDstAlpha) : JsonValue();
    result["blendEquationAlpha"] =
        blendEquationAlpha ? JsonValue(*blendEquationAlpha) : JsonValue();
    result["blendColor"] = blendColor.getHex();
    result["blendAlpha"] = blendAlpha;
    result["depthFunc"] = depthFunc;
    result["depthTest"] = depthTest;
    result["depthWrite"] = depthWrite;
    result["colorWrite"] = colorWrite;
    result["clipIntersection"] = clipIntersection;
    result["clipShadows"] = clipShadows;
    result["stencilWriteMask"] = stencilWriteMask;
    result["stencilFunc"] = stencilFunc;
    result["stencilRef"] = stencilRef;
    result["stencilFuncMask"] = stencilFuncMask;
    result["stencilFail"] = stencilFail;
    result["stencilZFail"] = stencilZFail;
    result["stencilZPass"] = stencilZPass;
    result["stencilWrite"] = stencilWrite;
    result["polygonOffset"] = polygonOffset;
    result["polygonOffsetFactor"] = polygonOffsetFactor;
    result["polygonOffsetUnits"] = polygonOffsetUnits;
    result["dithering"] = dithering;
    result["alphaTest"] = static_cast<double>(alphaTest);
    result["alphaHash"] = alphaHash;
    result["alphaToCoverage"] = alphaToCoverage;
    result["premultipliedAlpha"] = premultipliedAlpha;
    result["forceSinglePass"] = forceSinglePass;
    result["allowOverride"] = allowOverride;
    result["visible"] = visible;
    result["toneMapped"] = toneMapped;
    result["name"] = name;
    return result;
}
void Material::serializeClippingPlanesJSON(JsonValue& result) const {
    if (clippingPlanes && !clippingPlanes->empty()) {
        JsonValue::Array planes;
        for (const auto& plane : *clippingPlanes) {
            if (!plane)
                throw JSTypeError("Cannot read properties of null (reading 'toJSON')");
            planes.push_back(plane->toJSON());
        }
        result["clippingPlanes"] = std::move(planes);
    }
}
void Material::serializeUserDataJSON(JsonValue& result) const {
    // Object.keys tests keys without evaluating the values. The native JSON
    // representation cannot carry accessors; string primitives still have keys.
    if (userData.isNull()) throw JSTypeError("Cannot convert undefined or null to object");
    const auto string = std::get_if<std::string>(&userData.storage());
    if (userData.size() || (string && !string->empty()))
        result["userData"] = userData.sharedCopy();
}
JsonValue Material::toJSON(TextureJSONContext* meta) const {
    const bool root = meta == nullptr;
    TextureJSONContext localMeta;
    if (root) meta = &localMeta;
    auto result = serializeCommonJSON();
    auto& self = *const_cast<Material*>(this);
    auto field = [&](const char* name) {
        if (const auto native = self.property(name)) result[name] = fieldJSON(*native);
        else {
            const auto value = extraProperties.get(name);
            if (!value.isUndefined()) {
                const std::string_view key(name);
                const bool color = key == "color" || key == "sheenColor" || key == "emissive" || key == "specular" || key == "specularColor";
                if (key == "attenuationColor" || (color && value.toBoolean() && value.get("isColor").toBoolean()))
                    result[name] = *value.get("getHex").call(value).toJSON();
                else if (!color) {
                    if (key == "envMapRotation" || key == "normalScale" || key == "clearcoatNormalScale")
                        result[name] = *value.get("toArray").call(value).toJSON();
                    else if (const auto json = value.toJSON()) result[name] = *json;
                }
            }
        }
    };
    auto texture = [&](const char* name) {
        const auto value = self.property(name);
        if (!value) {
            const auto property = extraProperties.get(name);
            if (!property.toBoolean() || !property.get("isTexture").toBoolean()) return false;
            if (property.isHostObject("three.Texture"))
                result[name] = std::static_pointer_cast<Texture>(property.hostPayload("three.Texture"))->toJSON(meta).at("uuid");
            else result[name] = *property.get("toJSON").call(property).get("uuid").toJSON();
            return true;
        }
        const auto slot = std::get_if<std::shared_ptr<Texture>*>(&*value);
        if (!slot || !**slot) return false;
        const auto retained = **slot;
        result[name] = retained->toJSON(meta).at("uuid");
        return true;
    };
    for (const auto* name : {"color", "roughness", "metalness", "sheen", "sheenColor", "sheenRoughness",
             "emissive", "emissiveIntensity", "specular", "specularIntensity", "specularColor",
             "shininess", "clearcoat", "clearcoatRoughness", "diffuseRoughness"}) field(name);
    for (const auto* name : {"diffuseRoughnessMap", "clearcoatMap", "clearcoatRoughnessMap"}) texture(name);
    if (texture("clearcoatNormalMap")) field("clearcoatNormalScale");
    texture("sheenColorMap"); texture("sheenRoughnessMap");
    for (const auto* name : {"dispersion", "retroreflectivity", "iridescence", "iridescenceIOR", "iridescenceThicknessRange"}) field(name);
    texture("iridescenceMap"); texture("iridescenceThicknessMap");
    field("anisotropy"); field("anisotropyRotation"); texture("anisotropyMap");
    texture("map"); texture("matcap"); texture("alphaMap");
    if (texture("lightMap")) field("lightMapIntensity");
    if (texture("aoMap")) field("aoMapIntensity");
    if (texture("bumpMap")) field("bumpScale");
    if (texture("normalMap")) { field("normalMapType"); field("normalScale"); }
    if (texture("displacementMap")) { field("displacementScale"); field("displacementBias"); }
    for (const auto* name : {"roughnessMap", "metalnessMap", "emissiveMap", "specularMap", "specularIntensityMap", "specularColorMap"}) texture(name);
    if (texture("envMap")) field("combine");
    for (const auto* name : {"envMapRotation", "envMapIntensity", "reflectivity", "refractionRatio"}) field(name);
    texture("gradientMap"); field("transmission"); texture("transmissionMap"); field("thickness"); texture("thicknessMap");
    for (const auto* name : {"attenuationDistance", "attenuationColor", "size", "sizeAttenuation"}) field(name);
    serializeClippingPlanesJSON(result);
    for (const auto* name : {"rotation", "depthPacking", "linewidth", "linecap", "linejoin", "dashSize", "gapSize", "scale",
             "wireframe", "wireframeLinewidth", "wireframeLinecap", "wireframeLinejoin", "flatShading", "fog"}) field(name);
    serializeUserDataJSON(result);
    if (root) {
        auto extract = [&](const char* name, OrderedMap<JsonValue>& cache) {
            JsonValue::Array values;
            for (auto& [key, value] : cache) {
                value.get<JsonValue::Object>().erase("metadata");
                values.push_back(value.sharedCopy());
            }
            if (!values.empty()) result[name] = std::move(values);
        };
        extract("textures", meta->textures); extract("images", meta->images);
    }
    return result;
}
Material& Material::fromJSON(const JsonValue& json, const OrderedMap<std::shared_ptr<Texture>>& textures) {
    if (json.isNull()) throw JSTypeError("Cannot read properties of null (reading 'uuid')");
    const auto assign = [&](const char* name) {
        if (!json.contains(name)) return;
        const auto slot = property(name);
        const auto& value = json.at(name);
        if (!slot) {
            const std::string key(name);
            if (key == "color" || key == "emissive" || key == "specular" || key == "specularColor" || key == "attenuationColor") {
                const auto current = extraProperties.get(name);
                if (current.isUndefined()) return;
                if (current.isHostObject("three.Color")) std::static_pointer_cast<Color>(current.hostPayload("three.Color"))->setHex(JSValue::fromJSON(value).toNumber());
                else current.get("setHex").call(current,{JSValue::fromJSON(value)});
            } else if (key == "sheenColor") {
                extraProperties.set(name,nativeColor(std::make_shared<Color>(JSValue::fromJSON(value).toNumber())));
            } else if (key == "normalScale" || key == "clearcoatNormalScale") {
                auto vector = nativeVector(std::make_shared<Vector2>());
                auto array = JSValue::fromJSON(value);
                if (key == "normalScale" && !value.isArray()) array = JSValue::fromJSON(JsonValue::Array{value,value});
                vector.get("fromArray").call(vector,{array});extraProperties.set(name,vector);
            } else if (key == "envMapRotation") {
                const auto current = extraProperties.get(name);
                current.get("fromArray").call(current,{JSValue::fromJSON(value)});
            } else if (key == "map" || key == "matcap" || (key.size()>3 && key.ends_with("Map"))) {
                const auto found=textures.find(JSValue::fromJSON(value).toString());
                extraProperties.set(name,found==textures.end()?JSValue(nullptr):nativeTexture(found->second));
            } else extraProperties.set(name,JSValue::fromJSON(value));
            return;
        }
        if (const auto texture = std::get_if<std::shared_ptr<Texture>*>(&*slot)) {
            const auto found = textures.find(JSValue::fromJSON(value).toString());
            **texture = found == textures.end() ? nullptr : found->second;
        } else if (const auto color = std::get_if<Color*>(&*slot)) {
            (*color)->setHex(JSValue::fromJSON(value).toNumber());
        } else if (const auto vector = std::get_if<Vector2*>(&*slot)) {
            if (value.isArray()) {
                const auto& a = value.get<JsonValue::Array>();
                (*vector)->set(a.size() > 0 ? JSValue::fromJSON(a[0]).toNumber() : std::numeric_limits<double>::quiet_NaN(),
                              a.size() > 1 ? JSValue::fromJSON(a[1]).toNumber() : std::numeric_limits<double>::quiet_NaN());
            } else if (std::string_view(name) == "normalScale") (*vector)->setScalar(JSValue::fromJSON(value).toNumber());
            else throw JSTypeError("Material clearcoatNormalScale requires an array");
        } else if (const auto euler = std::get_if<Euler*>(&*slot)) {
            std::vector<Euler::ArrayValue> array;
            for (const auto& item : value.get<JsonValue::Array>())
                if (std::holds_alternative<std::string>(item.storage())) array.emplace_back(item.get<std::string>());
                else array.emplace_back(JSValue::fromJSON(item).toNumber());
            (*euler)->fromArray(array);
        } else if (const auto planes = std::get_if<ClippingPlanes*>(&*slot)) {
            std::vector<std::shared_ptr<Plane>> result;
            for (const auto& item : value.get<JsonValue::Array>()) {
                auto plane = std::make_shared<Plane>(); plane->fromJSON(item); result.push_back(std::move(plane));
            }
            **planes = std::move(result);
        } else if (std::string_view(name) == "vertexColors" && std::holds_alternative<double>(value.storage())) {
            vertexColors = value.get<double>() > 0;
        } else assignJSON(*slot, value);
    };
    assign("uuid");
    assign("name");
    assign("color");
    assign("roughness");
    assign("metalness");
    assign("sheen");
    assign("sheenColor");
    assign("sheenRoughness");
    assign("emissive");
    assign("specular");
    assign("specularIntensity");
    assign("specularColor");
    assign("shininess");
    assign("clearcoat");
    assign("clearcoatRoughness");
    assign("diffuseRoughness");
    assign("dispersion");
    assign("retroreflectivity");
    assign("iridescence");
    assign("iridescenceIOR");
    assign("iridescenceThicknessRange");
    assign("transmission");
    assign("thickness");
    assign("attenuationDistance");
    assign("attenuationColor");
    assign("anisotropy");
    assign("anisotropyRotation");
    assign("fog");
    assign("flatShading");
    assign("blending");
    assign("combine");
    assign("side");
    assign("shadowSide");
    assign("opacity");
    assign("transparent");
    assign("alphaTest");
    assign("alphaHash");
    assign("depthFunc");
    assign("depthTest");
    assign("depthWrite");
    assign("colorWrite");
    assign("clippingPlanes");
    assign("clipIntersection");
    assign("clipShadows");
    assign("depthPacking");
    assign("blendSrc");
    assign("blendDst");
    assign("blendEquation");
    assign("blendSrcAlpha");
    assign("blendDstAlpha");
    assign("blendEquationAlpha");
    assign("blendColor");
    assign("blendAlpha");
    assign("stencilWriteMask");
    assign("stencilFunc");
    assign("stencilRef");
    assign("stencilFuncMask");
    assign("stencilFail");
    assign("stencilZFail");
    assign("stencilZPass");
    assign("stencilWrite");
    assign("wireframe");
    assign("wireframeLinewidth");
    assign("wireframeLinecap");
    assign("wireframeLinejoin");
    assign("rotation");
    assign("linewidth");
    assign("linecap");
    assign("linejoin");
    assign("dashSize");
    assign("gapSize");
    assign("scale");
    assign("polygonOffset");
    assign("polygonOffsetFactor");
    assign("polygonOffsetUnits");
    assign("dithering");
    assign("alphaToCoverage");
    assign("premultipliedAlpha");
    assign("forceSinglePass");
    assign("allowOverride");
    assign("visible");
    assign("toneMapped");
    assign("userData");
    assign("vertexColors");
    assign("size");
    assign("sizeAttenuation");
    assign("map");
    assign("matcap");
    assign("alphaMap");
    assign("bumpMap");
    assign("bumpScale");
    assign("normalMap");
    assign("normalMapType");
    assign("normalScale");
    assign("displacementMap");
    assign("displacementScale");
    assign("displacementBias");
    assign("roughnessMap");
    assign("metalnessMap");
    assign("emissiveMap");
    assign("emissiveIntensity");
    assign("specularMap");
    assign("specularIntensityMap");
    assign("specularColorMap");
    assign("envMap");
    assign("envMapRotation");
    assign("envMapIntensity");
    assign("reflectivity");
    assign("refractionRatio");
    assign("lightMap");
    assign("lightMapIntensity");
    assign("aoMap");
    assign("aoMapIntensity");
    assign("gradientMap");
    assign("clearcoatMap");
    assign("clearcoatRoughnessMap");
    assign("clearcoatNormalMap");
    assign("clearcoatNormalScale");
    assign("diffuseRoughnessMap");
    assign("iridescenceMap");
    assign("iridescenceThicknessMap");
    assign("transmissionMap");
    assign("thicknessMap");
    assign("anisotropyMap");
    assign("sheenColorMap");
    assign("sheenRoughnessMap");
    return *this;
}
void Material::dispose() {
    dispatchEvent(Event{"dispose"});
}
