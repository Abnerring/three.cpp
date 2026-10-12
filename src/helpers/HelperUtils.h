#pragma once
#include "core/NativeObject.h"
#include "materials/LineBasicMaterial.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"
#include "objects/Line.h"

// C++ argument adapter: primitive colors keep their original JavaScript value,
// and shared Color inputs retain identity for helpers with a color override.
class HelperColor {
  public:
    JSValue value;
    HelperColor() = default;
    HelperColor(JSValue value) : value(std::move(value)) {}
    HelperColor(const Color& color) : value(nativeColor(std::make_shared<Color>(color))) {}
    HelperColor(std::shared_ptr<Color> color) : value(nativeColor(std::move(color))) {}
    HelperColor(const char* value) : value(value) {}
    HelperColor(const std::string& value) : value(value) {}
    template<class T> requires std::is_arithmetic_v<T>
    HelperColor(T value) : value(value) {}
};
namespace Helper_detail {
inline void setColor(Color& target,JSValue value) {
    if(value.toBoolean() && value.get("isColor").toBoolean()) {
        target.r=value.get("r").toNumber(); target.g=value.get("g").toNumber(); target.b=value.get("b").toNumber();
    } else if(value.type()==JSValue::Type::Number) target.setHex(value.number());
    else if(value.type()==JSValue::Type::String) target.setStyle(value.string());
}
inline Color color(const JSValue& value) { Color result;setColor(result,value);return result; }
inline std::shared_ptr<LineBasicMaterial> lineMaterial(const JSValue& color=JSValue(),bool vertexColors=false) {
    auto result=std::make_shared<LineBasicMaterial>();setColor(result->color,color);result->vertexColors=vertexColors;result->toneMapped=false;return result;
}
inline std::shared_ptr<MeshBasicMaterial> meshMaterial(const JSValue& color=JSValue()) {
    auto result=std::make_shared<MeshBasicMaterial>();setColor(result->color,color);result->toneMapped=false;return result;
}
inline std::shared_ptr<BufferGeometry> geometry(const std::vector<double>& positions) {
    auto result=std::make_shared<BufferGeometry>();result->setAttribute("position",std::make_shared<Float32BufferAttribute>(positions,3));return result;
}
inline Color& lineColor(const std::shared_ptr<Material>& material) {
    const auto line=std::dynamic_pointer_cast<LineBasicMaterial>(material);
    if(!line)throw JSTypeError("Cannot read properties of material.color");return line->color;
}
inline Color& meshColor(const std::shared_ptr<Material>& material) {
    const auto mesh=std::dynamic_pointer_cast<MeshBasicMaterial>(material);
    if(!mesh)throw JSTypeError("Cannot read properties of material.color");return mesh->color;
}
inline void setLineColor(std::shared_ptr<Material> material,JSValue value) {setColor(lineColor(material),std::move(value));}
inline void setMeshColor(std::shared_ptr<Material> material,JSValue value) {setColor(meshColor(material),std::move(value));}
inline void disposeGeometry(const std::shared_ptr<BufferGeometry> geometry) {if(!geometry)throw JSTypeError("geometry is undefined");geometry->dispose();}
inline void disposeMaterial(const std::shared_ptr<Material> material) {if(!material)throw JSTypeError("material is undefined");material->dispose();}
inline std::shared_ptr<Mesh> childMesh(const Object3D& object) {
    if(object.children.empty())throw JSTypeError("helper child is undefined");
    const auto mesh=std::dynamic_pointer_cast<Mesh>(object.children[0]);if(!mesh)throw JSTypeError("helper child has no geometry");return mesh;
}
}
