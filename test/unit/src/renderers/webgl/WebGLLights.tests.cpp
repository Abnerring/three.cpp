#include "catch.hpp"
#include "renderers/webgl/WebGLLights.h"
#include "core/NativeObject.h"
#include "lights/DirectionalLight.h"
#include "lights/LightProbe.h"
#include "lights/RectAreaLight.h"
#include "cameras/PerspectiveCamera.h"
#include "renderers/shaders/UniformsLib.h"
namespace {using Args=std::vector<JSValue>;JSValue lightsArray(Args values={}){auto a=JSValue::array();for(std::size_t i=0;i<values.size();++i)a.set(std::to_string(i),values[i]);return a;}JSValue extension(JSValue result=true){auto e=JSValue::object();e.set("has",JSValue::function([](const JSValue&,const Args&,const Args& c){return c[0];},{result}));return e;}}
TEST_CASE("WebGLLights caches native light uniform identities and structural versions", "[renderers][webgl][lights]"){
 WebGLLights lights(extension());auto l=std::make_shared<DirectionalLight>();l->position.set(1,2,3);l->updateMatrixWorld(true);auto a=lightsArray({nativeObject(l)});REQUIRE(lights.setup(a).isUndefined());auto state=lights.state.value(),uniform=state.get("directional").get("0");const auto version=state.get("version").number();
 l->intensity=3;lights.setup(a);REQUIRE(state.get("version").number()==version);REQUIRE(state.get("directional").get("0").sameValue(uniform));REQUIRE(uniform.get("color").get("r").number()==3);
 auto c=std::make_shared<PerspectiveCamera>();c->updateMatrixWorld(true);lights.setupView(a,nativeObject(c));REQUIRE(uniform.get("direction").get("x").number()==Approx(1/std::sqrt(14.)));
 l->castShadow=true;lights.setup(a);REQUIRE(state.get("version").number()>version);REQUIRE(state.get("directionalShadow").length()==1);REQUIRE(state.get("directionalShadow").get("0").get("shadowMapSize").sameValue(nativeObject(l).get("shadow").get("mapSize")));
 lights.setup(JSValue::array());REQUIRE(state.get("directional").length()==0);lights.setup(a);REQUIRE(state.get("directional").get("0").sameValue(uniform));
}
TEST_CASE("WebGLLights LTC extension detection uses strict true", "[renderers][webgl][lights]"){
 const JSValue library = UniformsLib; // Retain the source module realm across primitive configuration writes.
 auto old1=UniformsLib.get("LTC_FLOAT_1"),old2=UniformsLib.get("LTC_FLOAT_2"),old3=UniformsLib.get("LTC_HALF_1"),old4=UniformsLib.get("LTC_HALF_2");UniformsLib.set("LTC_FLOAT_1","float1");UniformsLib.set("LTC_FLOAT_2","float2");UniformsLib.set("LTC_HALF_1","half1");UniformsLib.set("LTC_HALF_2","half2");
 for(auto available:Args{true,false,1}){WebGLLights lights(extension(available));auto l=std::make_shared<RectAreaLight>();lights.setup(lightsArray({nativeObject(l)}));REQUIRE(lights.state.get("rectAreaLTC1").strictEquals(available.strictEquals(true)?JSValue("float1"):JSValue("half1")));}
 UniformsLib.set("LTC_FLOAT_1",old1);UniformsLib.set("LTC_FLOAT_2",old2);UniformsLib.set("LTC_HALF_1",old3);UniformsLib.set("LTC_HALF_2",old4);
}
TEST_CASE("WebGLLights detached closures preserve private state and ordinary constructibility", "[renderers][webgl][lights][construct]"){
 WebGLLights lights(extension());auto state=lights.state.value();lights.state=nullptr;auto setup=lights.object().get("setup"),view=lights.object().get("setupView");REQUIRE(setup.isConstructible());REQUIRE(view.isConstructible());REQUIRE(setup.get("length").number()==1);REQUIRE(view.get("length").number()==2);auto p=JSValue::object();setup.set("prototype",p);auto result=setup.construct({JSValue::array()});REQUIRE(result.getPrototype().sameValue(p));REQUIRE(state.get("hash").get("sunLength").number()==0);
}
TEST_CASE("Native light shadow camera and harmonic aliases participate in managed collection", "[renderers][webgl][lights][gc]"){
 auto anchor=JSValue::object();JSWeakValue lightWeak,shadowWeak,cameraWeak,coeffWeak;std::weak_ptr<DirectionalLight> nativeWeak;
 {auto l=std::make_shared<DirectionalLight>();nativeWeak=l;auto object=nativeObject(l),shadow=object.get("shadow"),camera=shadow.get("camera");lightWeak=JSWeakValue(object);shadowWeak=JSWeakValue(shadow);cameraWeak=JSWeakValue(camera);camera.set("light",object);object.set("cycle",shadow);shadow.get("matrix").set("cycle",camera);auto p=std::make_shared<LightProbe>();auto probe=nativeObject(p),coefficient=probe.get("sh").get("coefficients").get("0");coeffWeak=JSWeakValue(coefficient);coefficient.set("probe",probe);anchor.collectGarbage();REQUIRE_FALSE(lightWeak.expired());}
 anchor.collectGarbage();REQUIRE(lightWeak.expired());REQUIRE(shadowWeak.expired());REQUIRE(cameraWeak.expired());REQUIRE(coeffWeak.expired());REQUIRE(nativeWeak.expired());
}
