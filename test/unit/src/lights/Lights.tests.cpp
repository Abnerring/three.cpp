#include "catch.hpp"
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
#include <cmath>
#include <numbers>

TEST_CASE("Light constructors preserve default positions power and probe identity", "[lights]") {
    AmbientLight ambient; DirectionalLight directional; HemisphereLight hemisphere; PointLight point; SpotLight spot; RectAreaLight area;
    REQUIRE(ambient.type == "AmbientLight"); REQUIRE(ambient.isLight); REQUIRE(ambient.color.getHex() == 0xffffff);
    REQUIRE(directional.position.equals(Object3D::DEFAULT_UP)); REQUIRE(hemisphere.position.equals(Object3D::DEFAULT_UP)); REQUIRE(spot.position.equals(Object3D::DEFAULT_UP));
    REQUIRE(point.getPower() == Approx(4 * std::numbers::pi)); point.power = 80; REQUIRE(point.intensity == Approx(80 / (4 * std::numbers::pi)));
    spot.power = 25; REQUIRE(spot.intensity == Approx(25 / std::numbers::pi));
    area.width = -2; area.height = 3; area.power = 12; REQUIRE(area.intensity == Approx(-2 / std::numbers::pi)); area.width = 0; area.power = 1; REQUIRE(std::isinf(area.intensity));
    auto coefficients = std::make_shared<SphericalHarmonics3>(); LightProbe probe(coefficients); REQUIRE(probe.sh == coefficients);
    coefficients->coefficients[2].x = 9; REQUIRE(probe.sh->coefficients[2].x == 9);
    auto cloned = std::dynamic_pointer_cast<LightProbe>(probe.clone()); REQUIRE(cloned->sh != coefficients); REQUIRE(cloned->sh->equals(*coefficients));
}

TEST_CASE("Light copy follows upstream recursive argument and resource ownership", "[lights]") {
    DirectionalLight directional; directional.add(std::make_shared<Object3D>()); directional.target->add(std::make_shared<Object3D>());
    auto copy = std::dynamic_pointer_cast<DirectionalLight>(directional.clone(false)); REQUIRE(copy->children.size() == 1); REQUIRE(copy->target != directional.target); REQUIRE(copy->target->children.size() == 1);
    SpotLight spot; spot.add(std::make_shared<Object3D>()); spot.map = std::make_shared<Texture>(); spot.shadow->map = std::make_shared<RenderTarget>(); spot.shadow->mapType = FloatType;
    spot.shadow->biasNode = JSValue::object(); spot.shadow->focus = 0.25; auto cloned = std::dynamic_pointer_cast<SpotLight>(spot.clone(false));
    REQUIRE(cloned->children.empty()); REQUIRE(cloned->map == spot.map); REQUIRE(cloned->shadow != spot.shadow); REQUIRE(cloned->shadow->camera != spot.shadow->camera);
    REQUIRE_FALSE(cloned->shadow->map); REQUIRE(cloned->shadow->mapType == UnsignedByteType); REQUIRE(cloned->shadow->biasNode.sameValue(spot.shadow->biasNode)); REQUIRE(cloned->shadow->focus == 0.25);
    auto retainedMap = std::make_shared<RenderTarget>(); cloned->shadow->map = retainedMap; cloned->shadow->mapType = HalfFloatType; cloned->shadow->copy(*spot.shadow);
    REQUIRE(cloned->shadow->map == retainedMap); REQUIRE(cloned->shadow->mapType == HalfFloatType);
    IESSpotLight ies; ies.iesMap = std::make_shared<Texture>(); REQUIRE(std::dynamic_pointer_cast<IESSpotLight>(ies.clone())->iesMap == ies.iesMap);
    ProjectorLight projector; projector.aspect = 1.3; REQUIRE(std::dynamic_pointer_cast<ProjectorLight>(projector.clone())->aspect == 1.3); REQUIRE(projector.type == "SpotLight");
}

TEST_CASE("Shadow disposal retains references and observes reentrant resource replacement", "[lights]") {
    SpotLight light; std::vector<std::string> events;
    auto map = std::make_shared<RenderTarget>(), pass = std::make_shared<RenderTarget>(), replacement = std::make_shared<RenderTarget>();
    light.shadow->map = map; light.shadow->mapPass = pass;
    light.addEventListener("dispose", [&](Event&) { events.push_back("light"); });
    map->addEventListener("dispose", [&](Event&) { events.push_back("map"); light.shadow->mapPass = replacement; });
    pass->addEventListener("dispose", [&](Event&) { events.push_back("oldPass"); });
    replacement->addEventListener("dispose", [&](Event&) { events.push_back("replacement"); });
    Object3D& base = light; base.dispose();
    REQUIRE(events == std::vector<std::string>{"light", "map", "replacement"}); REQUIRE(light.shadow->map == map); REQUIRE(light.shadow->mapPass == replacement);
    events.clear(); base.dispose(); REQUIRE(events == std::vector<std::string>{"light", "map", "replacement"});
}

TEST_CASE("Shadow matrices use world transforms coordinate range and atlas offsets", "[lights]") {
    DirectionalLight light; light.position.set(2, 4, 6); light.target->position.set(0, 0, 0); light.updateMatrixWorld(); light.target->updateMatrixWorld(); light.shadow->updateMatrices(light);
    REQUIRE(light.shadow->camera->position.equals(light.position)); REQUIRE(light.shadow->getCamera(20) == light.shadow->camera); REQUIRE(light.shadow->getViewportCount() == 1);
    REQUIRE(&light.shadow->getFrustum() == &light.shadow->_frustum); REQUIRE(&light.shadow->getViewport(0) == &light.shadow->_viewports[0]);
    Vector3 origin; origin.applyMatrix4(light.shadow->matrix); REQUIRE(origin.x == Approx(0.5)); REQUIRE(origin.y == Approx(0.5));
    light.shadow->_frameExtents.set(4, 2); Vector4 viewport(2, 1, 1, 1); Matrix4 atlas; Frustum frustum;
    light.shadow->_updateMatrix(*light.shadow->camera, atlas, frustum, &viewport); Vector3 atlasOrigin; atlasOrigin.applyMatrix4(atlas); REQUIRE(atlasOrigin.x == Approx(0.625)); REQUIRE(atlasOrigin.y == Approx(0.75));
    SpotLight spot; spot.distance = 30; spot.shadow->mapSize.set(1024, 256); spot.shadow->aspect = 1.5; spot.shadow->focus = .5; spot.updateMatrixWorld(); spot.target->updateMatrixWorld(); spot.shadow->updateMatrices(spot);
    auto camera = std::dynamic_pointer_cast<PerspectiveCamera>(spot.shadow->camera); REQUIRE(camera->fov == Approx(60)); REQUIRE(camera->aspect == Approx(6)); REQUIRE(camera->far == 30);
    spot.distance = 0; spot.shadow->updateMatrices(spot); REQUIRE(camera->far == 30);
    PointLight point; point.position.set(1, 2, 3); point.updateMatrixWorld(); REQUIRE_THROWS_AS(point.shadow->updateMatrices(point), JSTypeError); REQUIRE(point.shadow->camera->position.equals(point.position));
}

TEST_CASE("Shadow serialization omits runtime GPU resources and camera matrix", "[lights]") {
    SpotLight light; light.shadow->map = std::make_shared<RenderTarget>(); light.shadow->focus = .75; light.shadow->aspect = 2;
    const auto json = light.toJSON(); const auto& shadow = json.at("object").at("shadow");
    REQUIRE(shadow.at("focus") == JsonValue(.75)); REQUIRE(shadow.at("aspect") == JsonValue(2)); REQUIRE_FALSE(shadow.contains("map")); REQUIRE_FALSE(shadow.at("camera").contains("matrix"));
    REQUIRE(json.at("object").at("target") == JsonValue(light.target->uuid)); REQUIRE_FALSE(json.contains("textures"));
}

TEST_CASE("Spherical harmonics keep coefficients and evaluate aliased targets", "[lights][math]") {
    SphericalHarmonics3 sh; REQUIRE(sh.toArray() == std::vector<double>(27, 0)); sh.coefficients[0].set(1, 2, 3);
    Vector3 value; REQUIRE(&sh.getAt(Vector3(0, 0, 1), value) == &value); REQUIRE(value.x == Approx(.282095));
    sh.getIrradianceAt(Vector3(0, 0, 1), value); REQUIRE(value.z == Approx(3 * .886227));
    auto* coefficient = &sh.coefficients[0]; auto clone = sh.clone(); sh.copy(clone); REQUIRE(&sh.coefficients[0] == coefficient); REQUIRE(sh.equals(clone));
    sh.add(sh); REQUIRE(sh.coefficients[0].x == 2); sh.zero(); REQUIRE(sh.toArray() == std::vector<double>(27, 0));
    std::vector<double> basis(11, 7); SphericalHarmonics3::getBasisAt(Vector3(1, 0, 0), basis); REQUIRE(basis[3] == Approx(.488603)); REQUIRE(basis[9] == 7);
    sh.fromArray({1, 2, 3}); REQUIRE(sh.coefficients[0].x == 1); REQUIRE(std::isnan(sh.coefficients[1].x));
    REQUIRE_THROWS_AS(sh.set(std::vector<Vector3>{Vector3(9, 8, 7)}), JSTypeError); REQUIRE(sh.coefficients[0].x == 9);
}
