#include "lights/webgpu/ProjectorLight.h"
#include <stdexcept>
void ProjectorLight::registerNode(const JSValue& nodeClass) { registerNodeFor("ProjectorLight", nodeClass); }
JSValue ProjectorLight::getLightNode() const { auto node = registeredNodeFor("ProjectorLight"); return node ? *node : SpotLight::getLightNode(); }
ProjectorLight& ProjectorLight::copy(const Object3D& source, bool recursive) {
    const auto* light = dynamic_cast<const ProjectorLight*>(&source);
    if (!light) throw std::invalid_argument("ProjectorLight::copy requires a ProjectorLight");
    SpotLight::copy(source, recursive); aspect = light->aspect; return *this;
}
std::shared_ptr<Object3D> ProjectorLight::clone(bool recursive) const { auto result = std::make_shared<ProjectorLight>(); result->copy(*this, recursive); return result; }
