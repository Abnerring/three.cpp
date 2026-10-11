#include "scenes/Scene.h"
#include "materials/Material.h"
#include <stdexcept>

std::optional<JsonValue> Scene::propertyValue(const std::string& property) const {
    if (property == "isScene")
        return isScene;
    if (property == "backgroundBlurriness")
        return backgroundBlurriness;
    if (property == "backgroundIntensity")
        return backgroundIntensity;
    if (property == "environmentIntensity")
        return environmentIntensity;
    return Object3D::propertyValue(property);
}
Scene& Scene::copy(const Object3D& source, bool recursive) {
    const auto* scene = dynamic_cast<const Scene*>(&source);
    if (!scene)
        throw std::invalid_argument("Scene::copy requires a Scene");
    if (scene->environment)
        throw std::logic_error("Scene environment cloning requires the unported Texture module");
    Object3D::copy(source, recursive);
    background = scene->background;
    environment.reset();
    fog = scene->fog ? scene->fog->clone() : nullptr;
    backgroundBlurriness = scene->backgroundBlurriness;
    backgroundIntensity = scene->backgroundIntensity;
    backgroundRotation.copy(scene->backgroundRotation);
    environmentIntensity = scene->environmentIntensity;
    environmentRotation.copy(scene->environmentRotation);
    overrideMaterial = scene->overrideMaterial ? scene->overrideMaterial->clone() : nullptr;
    return *this;
}
std::shared_ptr<Object3D> Scene::clone(bool recursive) const {
    auto result = std::make_shared<Scene>();
    result->copy(*this, recursive);
    return result;
}
void Scene::serializeObjectResources(Object3DJSONContext&, JsonValue& object) const {
    if (environment)
        throw std::logic_error("Scene environment JSON requires the unported Texture module");
    if (background)
        object["background"] = background->getHex();
}
JsonValue Scene::toJSON(Object3DJSONContext* meta) const {
    auto output = Object3D::toJSON(meta);
    auto& object = output["object"];
    if (fog)
        object["fog"] = fog->toJSON();
    object["backgroundBlurriness"] = backgroundBlurriness;
    object["backgroundIntensity"] = backgroundIntensity;
    object["backgroundRotation"] =
        JsonValue::Array{backgroundRotation.x(), backgroundRotation.y(), backgroundRotation.z(),
                         backgroundRotation.getOrderString()};
    object["environmentIntensity"] = environmentIntensity;
    object["environmentRotation"] =
        JsonValue::Array{environmentRotation.x(), environmentRotation.y(), environmentRotation.z(),
                         environmentRotation.getOrderString()};
    return output;
}
