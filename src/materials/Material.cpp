#include "materials/Material.h"
#include "math/MathUtils.h"
#include "math/Plane.h"
#include <iostream>
Material::Material() : uuid(MathUtils::generateUUID()) {}
void Material::setValues(const JsonValue& parameters) {
    if (parameters.isNull())
        return;
    for (const auto& [key, value] : parameters.get<JsonValue::Object>()) {
        if (key == "name")
            name = value.get<std::string>();
        else if (key == "userData")
            userData = value;
        else if (key == "alphaTest")
            alphaTest = value.get<double>();
#define THREE_SET_NUMBER(field)                                                                    \
    else if (key == #field) field = static_cast<decltype(field)>(value.get<double>());
#define THREE_SET_BOOL(field) else if (key == #field) field = value.get<bool>();
        THREE_SET_NUMBER(blending)
        THREE_SET_NUMBER(side)
        THREE_SET_NUMBER(opacity)
        THREE_SET_NUMBER(blendSrc) THREE_SET_NUMBER(blendDst) THREE_SET_NUMBER(blendEquation)
            THREE_SET_NUMBER(blendAlpha) THREE_SET_NUMBER(depthFunc) THREE_SET_NUMBER(
                stencilWriteMask) THREE_SET_NUMBER(stencilFunc) THREE_SET_NUMBER(stencilRef)
                THREE_SET_NUMBER(stencilFuncMask) THREE_SET_NUMBER(stencilFail)
                    THREE_SET_NUMBER(stencilZFail) THREE_SET_NUMBER(stencilZPass)
                        THREE_SET_NUMBER(polygonOffsetFactor) THREE_SET_NUMBER(polygonOffsetUnits)
                            THREE_SET_BOOL(vertexColors) THREE_SET_BOOL(transparent) THREE_SET_BOOL(
                                alphaHash) THREE_SET_BOOL(depthTest) THREE_SET_BOOL(depthWrite)
                                THREE_SET_BOOL(stencilWrite) THREE_SET_BOOL(clipIntersection)
                                    THREE_SET_BOOL(clipShadows) THREE_SET_BOOL(colorWrite)
                                        THREE_SET_BOOL(polygonOffset) THREE_SET_BOOL(dithering)
                                            THREE_SET_BOOL(alphaToCoverage) THREE_SET_BOOL(
                                                premultipliedAlpha) THREE_SET_BOOL(forceSinglePass)
                                                THREE_SET_BOOL(allowOverride)
                                                    THREE_SET_BOOL(visible)
                                                        THREE_SET_BOOL(toneMapped)
#undef THREE_SET_NUMBER
#undef THREE_SET_BOOL
                                                            else if (key == "blendSrcAlpha" ||
                                                                     key == "blendDstAlpha" ||
                                                                     key == "blendEquationAlpha" ||
                                                                     key == "shadowSide") {
            auto* target = key == "blendSrcAlpha"        ? &blendSrcAlpha
                           : key == "blendDstAlpha"      ? &blendDstAlpha
                           : key == "blendEquationAlpha" ? &blendEquationAlpha
                                                         : &shadowSide;
            *target = value.isNull() ? std::nullopt
                                     : std::optional<int>(static_cast<int>(value.get<double>()));
        }
        else if (key == "precision") precision =
            value.isNull() ? std::nullopt : std::optional<std::string>(value.get<std::string>());
        else if (key == "blendColor") {
            if (std::holds_alternative<double>(value.storage()))
                blendColor.setHex(static_cast<int>(value.get<double>()));
            else
                blendColor.copy(Color(value.get<std::string>()));
        }
        else std::cerr << "THREE.Material: '" << key << "' is not a property of " << type << ".\n";
    }
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
    clippingPlanes.reset();
    if (sourcePlanes) {
        clippingPlanes.emplace();
        for (const auto& plane : *sourcePlanes) {
            if (!plane)
                throw std::logic_error("Material clippingPlanes requires non-null Plane");
            clippingPlanes->push_back(std::make_shared<Plane>(plane->clone()));
        }
    }
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
JsonValue Material::toJSON() const {
    JsonValue result = JsonValue::Object{};
    result["metadata"] =
        JsonValue::Object{{"version", 4.7}, {"type", "Material"}, {"generator", "Material.toJSON"}};
    result["uuid"] = uuid;
    result["type"] = type;
    result["name"] = name;
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
    if (clippingPlanes && !clippingPlanes->empty()) {
        JsonValue::Array planes;
        for (const auto& plane : *clippingPlanes) {
            if (!plane)
                throw std::logic_error("Material clippingPlanes requires non-null Plane");
            planes.push_back(plane->toJSON());
        }
        result["clippingPlanes"] = std::move(planes);
    }
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
    if (userData.size())
        result["userData"] = userData;
    return result;
}
void Material::dispose() {
    dispatchEvent(Event{"dispose"});
}
