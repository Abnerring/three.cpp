#pragma once
#include "materials/Material.h"

class ShaderMaterial : public Material {
  public:
    bool isShaderMaterial = true;
    JSValue defines = JSValue::object();
    JSValue uniforms = JSValue::object();
    JSValue uniformsGroups = JSValue::array();
    std::string vertexShader, fragmentShader;
    double linewidth = 1;
    bool wireframe = false;
    double wireframeLinewidth = 1;
    bool fog = false, lights = false, clipping = false;
    JSValue extensions, defaultAttributeValues;
    JSValue index0AttributeName;
    bool uniformsNeedUpdate = false;
    JSValue glslVersion = nullptr;

    explicit ShaderMaterial(const JsonValue& parameters = JsonValue());
    void visitProperties(const PropertyVisitor& visitor) override;
    Material& copy(const Material& source) override;
    std::shared_ptr<Material> clone() const override;
    JsonValue toJSON(TextureJSONContext* meta = nullptr) const override;
    Material& fromJSON(const JsonValue& json,
        const OrderedMap<std::shared_ptr<Texture>>& textures = {}) override;
};
