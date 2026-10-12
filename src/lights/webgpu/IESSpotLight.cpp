#include "lights/webgpu/IESSpotLight.h"
#include <stdexcept>
void IESSpotLight::registerNode(const JSValue& nodeClass) { registerNodeFor("IESSpotLight", nodeClass); }
JSValue IESSpotLight::getLightNode() const { auto node = registeredNodeFor("IESSpotLight"); return node ? *node : SpotLight::getLightNode(); }
IESSpotLight& IESSpotLight::copy(const Object3D& source, bool recursive) {
    const auto* light = dynamic_cast<const IESSpotLight*>(&source);
    if (!light) throw std::invalid_argument("IESSpotLight::copy requires a IESSpotLight");
    SpotLight::copy(source, recursive); iesMap = light->iesMap; return *this;
}
std::shared_ptr<Object3D> IESSpotLight::clone(bool recursive) const { auto result = std::make_shared<IESSpotLight>(); result->copy(*this, recursive); return result; }
