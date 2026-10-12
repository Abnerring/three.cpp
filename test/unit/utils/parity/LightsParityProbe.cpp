#include "lights/AmbientLight.h"
#include "lights/DirectionalLight.h"
#include "lights/HemisphereLight.h"
#include "lights/PointLight.h"
#include "lights/SpotLight.h"
#include "lights/RectAreaLight.h"
#include "lights/LightProbe.h"
#include "lights/webgpu/IESSpotLight.h"
#include "lights/webgpu/ProjectorLight.h"
#include "core/RenderTarget.h"
#include "cameras/PerspectiveCamera.h"
#include "cameras/OrthographicCamera.h"
#include <iostream>
#include <limits>

namespace {
template <class T> JsonValue array(const T& values) { JsonValue::Array result; for (const auto value : values) result.emplace_back(value); return result; }
JsonValue sanitize(JsonValue value) {
    if (value.isObject()) { value.get<JsonValue::Object>().erase("uuid"); for (auto& [key, child] : value.get<JsonValue::Object>()) { if (key == "target") child = "<target>"; else child = sanitize(std::move(child)); } }
    else if (value.isArray()) for (auto& child : value.get<JsonValue::Array>()) child = sanitize(std::move(child));
    return value;
}
std::shared_ptr<Light> create(int type, double intensity = 1) {
    switch (type) {
    case 0: return std::make_shared<Light>(Color(), intensity);
    case 1: return std::make_shared<AmbientLight>(Color(), intensity);
    case 2: return std::make_shared<DirectionalLight>(Color(), intensity);
    case 3: return std::make_shared<HemisphereLight>(Color(), Color(), intensity);
    case 4: return std::make_shared<PointLight>(Color(), intensity);
    case 5: return std::make_shared<SpotLight>(Color(), intensity);
    case 6: return std::make_shared<RectAreaLight>(Color(), intensity);
    case 7: return std::make_shared<LightProbe>(std::make_shared<SphericalHarmonics3>(), intensity);
    case 8: return std::make_shared<IESSpotLight>(Color(), intensity);
    default: return std::make_shared<ProjectorLight>(Color(), intensity);
    }
}
std::shared_ptr<LightShadow> shadowOf(Light& light) {
    if (auto* directional = dynamic_cast<DirectionalLight*>(&light)) return directional->shadow;
    if (auto* point = dynamic_cast<PointLight*>(&light)) return point->shadow;
    if (auto* spot = dynamic_cast<SpotLight*>(&light)) return spot->shadow;
    return {};
}
JsonValue shadowState(LightShadow& shadow) {
    JsonValue::Array planes; for (const auto& plane : shadow._frustum.planes) planes.emplace_back(JsonValue::Array{plane.normal.x, plane.normal.y, plane.normal.z, plane.constant});
    JsonValue::Array viewports; for (const auto& viewport : shadow._viewports) viewports.emplace_back(array(viewport.toArray()));
    return JsonValue::Object{{"json", sanitize(shadow.toJSON())}, {"matrix", array(shadow.matrix.elements())}, {"planes", planes}, {"viewports", viewports},
        {"frameExtents", array(shadow._frameExtents.toArray())}, {"count", shadow.getViewportCount()}, {"cameraSame", shadow.getCamera(37) == shadow.camera},
        {"frustumSame", &shadow.getFrustum() == &shadow._frustum}, {"viewportSame", &shadow.getViewport(0) == &shadow._viewports[0]}, {"extentsSame", &shadow.getFrameExtents() == &shadow._frameExtents},
        {"mapType", shadow.mapType}, {"autoUpdate", shadow.autoUpdate}, {"needsUpdate", shadow.needsUpdate}, {"mapNull", !shadow.map}, {"mapPassNull", !shadow.mapPass},
        {"position", array(shadow.camera->position.toArray())}, {"world", array(shadow.camera->matrixWorld.elements())}, {"projection", array(shadow.camera->projectionMatrix.elements())}};
}
JsonValue state(Light& light) {
    JsonValue result = JsonValue::Object{{"json", sanitize(light.toJSON())}, {"position", array(light.position.toArray())}, {"matrix", array(light.matrix.elements())},
        {"color", array(light.color.toArray())}, {"isLight", light.isLight}};
    if (auto shadow = shadowOf(light)) result["shadow"] = shadowState(*shadow);
    if (auto* point = dynamic_cast<PointLight*>(&light)) result["power"] = point->getPower();
    if (auto* spot = dynamic_cast<SpotLight*>(&light)) result["power"] = spot->getPower();
    if (auto* area = dynamic_cast<RectAreaLight*>(&light)) result["power"] = area->getPower();
    if (auto* ies = dynamic_cast<IESSpotLight*>(&light)) result["iesMapNull"] = !ies->iesMap;
    if (auto* projector = dynamic_cast<ProjectorLight*>(&light)) result["aspect"] = projector->aspect ? JsonValue(*projector->aspect) : JsonValue();
    return result;
}
JsonValue copyCase(int type, bool recursive) {
    auto source = create(type, 2.75); source->color.setHex(0x339977); source->position.set(3, -2, 7); source->name = "sample"; source->add(std::make_shared<Object3D>());
    if (auto* light = dynamic_cast<HemisphereLight*>(source.get())) light->groundColor.setHex(0xaaff55);
    if (auto* light = dynamic_cast<PointLight*>(source.get())) { light->distance = 7; light->decay = -3; }
    if (auto* light = dynamic_cast<SpotLight*>(source.get())) { light->distance = 23; light->angle = 0.3; light->decay = 0; light->penumbra = 0.4; light->map = std::make_shared<Texture>(); light->map->uuid = "map-id"; }
    if (auto* light = dynamic_cast<RectAreaLight*>(source.get())) { light->width = -3; light->height = 0; }
    if (auto* light = dynamic_cast<LightProbe*>(source.get())) light->sh->coefficients[3].set(2, 5, -1);
    if (auto* light = dynamic_cast<IESSpotLight*>(source.get())) light->iesMap = std::make_shared<Texture>();
    if (auto* light = dynamic_cast<ProjectorLight*>(source.get())) light->aspect = 1.7;
    if (auto* target = source->shadowTarget()) { target->name = "target"; target->position.set(-1, 4, 9); target->add(std::make_shared<Object3D>()); }
    auto sourceShadow = shadowOf(*source);
    if (sourceShadow) { sourceShadow->intensity = 0.7; sourceShadow->bias = -0.1; sourceShadow->normalBias = 0.2; sourceShadow->radius = 4; sourceShadow->blurSamples = 13;
        sourceShadow->autoUpdate = false; sourceShadow->needsUpdate = true; sourceShadow->mapSize.set(256, 1024); sourceShadow->mapType = FloatType;
        sourceShadow->biasNode = JSValue::object(); sourceShadow->map = std::make_shared<RenderTarget>(); sourceShadow->mapPass = std::make_shared<RenderTarget>();
        sourceShadow->matrix.makeTranslation(9, 8, 7); sourceShadow->_frameExtents.set(4, 2); sourceShadow->camera->position.set(5, 6, 7);
        if (auto* spot = dynamic_cast<SpotLightShadow*>(sourceShadow.get())) { spot->focus = 0.7; spot->aspect = 1.8; }
    }
    auto clone = std::dynamic_pointer_cast<Light>(source->clone(recursive));
    JsonValue result = JsonValue::Object{{"state", state(*clone)}, {"children", clone->children.size()}, {"uuidDifferent", source->uuid != clone->uuid}};
    if (sourceShadow) { auto clonedShadow = shadowOf(*clone); result["shadowDifferent"] = clonedShadow != sourceShadow; result["cameraDifferent"] = clonedShadow->camera != sourceShadow->camera; result["biasNodeSame"] = clonedShadow->biasNode.sameValue(sourceShadow->biasNode); }
    if (source->shadowTarget()) { result["targetDifferent"] = source->shadowTarget() != clone->shadowTarget(); result["targetChildren"] = clone->shadowTarget()->children.size(); result["targetPosition"] = array(clone->shadowTarget()->position.toArray()); }
    if (auto* light = dynamic_cast<SpotLight*>(source.get())) result["mapSame"] = dynamic_cast<SpotLight&>(*clone).map == light->map;
    if (auto* light = dynamic_cast<LightProbe*>(source.get())) { auto& clonedProbe = dynamic_cast<LightProbe&>(*clone); result["shDifferent"] = clonedProbe.sh != light->sh; result["shEqual"] = clonedProbe.sh->equals(*light->sh); }
    if (auto* light = dynamic_cast<IESSpotLight*>(source.get())) result["iesMapSame"] = dynamic_cast<IESSpotLight&>(*clone).iesMap == light->iesMap;
    return result;
}
JsonValue matrixCase(int type, int coordinate, bool reversed, int scenario) {
    auto light = create(type); auto shadow = shadowOf(*light); auto target = light->shadowTarget();
    auto parent = std::make_shared<Object3D>(), targetParent = std::make_shared<Object3D>(); parent->add(light);
    if (auto* directional = dynamic_cast<DirectionalLight*>(light.get())) targetParent->add(directional->target); else targetParent->add(dynamic_cast<SpotLight&>(*light).target);
    light->position.set(3 - scenario, 4 + scenario * 0.5, -2); target->position.set(-1, 2, 3 + scenario);
    if (scenario & 1) { parent->position.set(9, -4, 3); parent->rotation.set(0.2, -0.3, 0.5); parent->scale.set(2, 1.5, 0.7); targetParent->position.set(-2, 1, -5); targetParent->rotateY(0.3); }
    if (scenario == 2) { light->position.set(0, 0, 0); target->position.set(0, 0, 0); }
    parent->updateMatrixWorld(); targetParent->updateMatrixWorld(); shadow->camera->coordinateSystem = coordinate; shadow->camera->reversedDepth = reversed;
    if (auto* camera = dynamic_cast<PerspectiveCamera*>(shadow->camera.get())) camera->updateProjectionMatrix(); else dynamic_cast<OrthographicCamera&>(*shadow->camera).updateProjectionMatrix();
    if (auto* spot = dynamic_cast<SpotLight*>(light.get())) { spot->angle = 0.3 + scenario * 0.1; spot->distance = scenario == 3 ? -2 : scenario == 4 ? std::numeric_limits<double>::quiet_NaN() : scenario * 8;
        spot->shadow->focus = 0.8; spot->shadow->aspect = 1.3; spot->shadow->mapSize.set(1024, 256); }
    shadow->updateMatrices(*light); auto result = shadowState(*shadow);
    shadow->_frameExtents.set(4, 2); Vector4 viewport(2, 1, 1, 1); Matrix4 matrix; Frustum frustum;
    shadow->_updateMatrix(*shadow->camera, matrix, frustum, &viewport); result["atlas"] = array(matrix.elements());
    return result;
}
JsonValue disposeCase(int type, int scenario) {
    auto light = create(type); auto shadow = shadowOf(*light); auto first = std::make_shared<RenderTarget>(), second = std::make_shared<RenderTarget>(), replacement = std::make_shared<RenderTarget>();
    shadow->map = first; shadow->mapPass = scenario == 1 ? first : second;
    JsonValue::Array trace;
    first->addEventListener("dispose", [&](Event&) { trace.emplace_back("map"); if (scenario == 2) shadow->mapPass = replacement; if (scenario == 5) throw std::runtime_error("stop map"); });
    second->addEventListener("dispose", [&](Event&) { trace.emplace_back("mapPass"); });
    replacement->addEventListener("dispose", [&](Event&) { trace.emplace_back("replacement"); });
    light->addEventListener("dispose", [&](Event&) { trace.emplace_back("light"); if (scenario == 3) throw std::runtime_error("stop"); if (scenario == 4) { auto next = std::make_shared<SpotLightShadow>(); next->map = replacement; dynamic_cast<SpotLight&>(*light).shadow = next; } });
    try { light->dispose(); if (scenario == 0) light->dispose(); } catch (const std::exception&) { trace.emplace_back("error"); }
    return JsonValue::Object{{"trace", trace}, {"mapRetained", shadow->map == first}, {"mapPassRetained", shadow->mapPass == (scenario == 1 ? first : scenario == 2 ? replacement : second)}};
}
JsonValue harmonics(int index) {
    SphericalHarmonics3 a, b; std::vector<double> input{123,456};
    for (int i = 0; i < 27; ++i) input.push_back(((index + 3) * (i + 7) % 97 - 43) * 0.125);
    a.fromArray(input, 2); b.copy(a).scale(-0.75); Vector3 normal(index % 5 - 2, (index * 3) % 7 - 3, 0.1 + (index % 11)); normal.normalize(); Vector3 radiance, irradiance;
    a.getAt(normal, radiance); a.getIrradianceAt(normal, irradiance); std::vector<double> basis(12, -9); SphericalHarmonics3::getBasisAt(normal, basis);
    auto clone = a.clone(); a.add(b).addScaledSH(b, 0.3).lerp(clone, 0.6); std::vector<double> output{4, 5}; a.toArray(output, 2);
    JsonValue result = JsonValue::Object{{"radiance", array(radiance.toArray())}, {"irradiance", array(irradiance.toArray())}, {"basis", array(basis)}, {"values", array(output)}, {"equalClone", clone.equals(clone)}, {"equalOther", clone.equals(b)}};
    auto alias = clone.clone(); alias.getAt(normal, alias.coefficients[index % 9]); result["coefficientAlias"] = array(alias.toArray());
    alias.copy(clone).getIrradianceAt(normal, alias.coefficients[(index + 2) % 9]); result["irradianceAlias"] = array(alias.toArray());
    alias.copy(clone).add(alias).addScaledSH(alias, 0.2).lerp(alias, 1.7); result["self"] = array(alias.toArray());
    normal.set(1, 2, 3); clone.getAt(normal, normal); result["normalAlias"] = array(normal.toArray()); clone.zero(); result["zero"] = array(clone.toArray());
    return result;
}
}
int main() {
    JsonValue result = JsonValue::Object{};
    for (int type = 0; type < 10; ++type) { auto light = create(type); result["default:" + std::to_string(type)] = state(*light); for (bool recursive : {false, true}) result["copy:" + std::to_string(type) + ":" + std::to_string(recursive)] = copyCase(type, recursive); }
    for (int type : {2, 5}) for (int coordinate : {WebGLCoordinateSystem, WebGPUCoordinateSystem}) for (bool reversed : {false, true}) for (int scenario = 0; scenario < 5; ++scenario)
        result["matrix:" + std::to_string(type) + ":" + std::to_string(coordinate) + ":" + std::to_string(reversed) + ":" + std::to_string(scenario)] = matrixCase(type, coordinate, reversed, scenario);
    for (int type : {2, 4, 5}) for (int scenario = 0; scenario < (type == 5 ? 6 : 4); ++scenario) result["dispose:" + std::to_string(type) + ":" + std::to_string(scenario)] = disposeCase(type, scenario);
    for (int i = 0; i < 128; ++i) result["sh:" + std::to_string(i)] = harmonics(i);
    for (int type : {4, 5, 6}) for (double value : {-50.0, -0.0, 0.25, 100.0, 1e30}) { auto light = create(type); if (auto* p = dynamic_cast<PointLight*>(light.get())) p->power = value; else if (auto* s = dynamic_cast<SpotLight*>(light.get())) s->power = value; else { auto& a = dynamic_cast<RectAreaLight&>(*light); a.width = -3; a.height = 7; a.power = value; } result["power:" + std::to_string(type) + ":" + JSValue(value).toString()] = JsonValue::Object{{"intensity", light->intensity}, {"state", state(*light)}}; }
    auto sh = std::make_shared<SphericalHarmonics3>(); LightProbe probe(sh); sh->coefficients[0].x = 9; result["shIdentity"] = JsonValue::Object{{"same", probe.sh == sh}, {"value", probe.sh->coefficients[0].x}};
    PointLight point; point.position.set(2, 3, 4); point.updateMatrixWorld(); bool pointError = false; try { point.shadow->updateMatrices(point); } catch (const JSTypeError&) { pointError = true; }
    result["pointUpdate"] = JsonValue::Object{{"error", pointError}, {"position", array(point.shadow->camera->position.toArray())}};
    const auto inheritedNode = JSValue::object(), ownNode = JSValue::object(); IESSpotLight ies; Light::registerNode(inheritedNode); result["nodeBase"] = ies.getLightNode().sameValue(inheritedNode); SpotLight::registerNode(ownNode); result["nodeInherited"] = ies.getLightNode().sameValue(ownNode); IESSpotLight::registerNode(JSValue(nullptr)); result["nodeOwnNull"] = ies.getLightNode().isNull();
    auto parent = std::make_shared<Object3D>(); auto first = std::make_shared<SpotLight>(), second = std::make_shared<SpotLight>();
    first->map = second->map = std::make_shared<Texture>(); first->map->uuid = "shared-map"; first->map->source->uuid = "shared-image"; parent->add(first, second);
    result["nestedJSON"] = sanitize(parent->toJSON()); Object3DJSONContext meta; first->toJSON(&meta); second->toJSON(&meta); result["sharedMetaCount"] = meta.textures.size();
    LightShadow plain(std::make_shared<Camera>()); auto cloned = plain.clone(); result["plainShadow"] = JsonValue::Object{{"state", shadowState(*cloned)}, {"cameraDifferent", cloned->camera != plain.camera}};
    std::cout << result.dump() << '\n';
}
