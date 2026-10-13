#include "catch.hpp"
#include "renderers/webgl/WebGLRenderStates.h"
namespace {using Args=std::vector<JSValue>;JSValue stateLight(bool grid=false){auto l=JSValue::object();l.set("isLightProbeGrid",grid);return l;}}
TEST_CASE("WebGLRenderStates preserves depth append semantics and disposal", "[renderers][webgl]"){
 WebGLRenderStates states;auto scene=JSValue::object(),zero=states.get(scene,5);REQUIRE(states.get(scene).sameValue(zero));auto second=states.get(scene,5);REQUIRE_FALSE(second.sameValue(zero));REQUIRE(states.get(scene,1).sameValue(second));REQUIRE(states.get(scene,2).isObject());REQUIRE(states.get(scene,-1).isUndefined());REQUIRE(states.get(scene,"length").number()==3);states.dispose();REQUIRE_FALSE(states.get(scene).sameValue(zero));
}
TEST_CASE("WebGLRenderState captured arrays remain live after public replacement", "[renderers][webgl]"){
 WebGLRenderState state;auto publicState=state.state.value(),a=publicState.get("lightsArray"),s=publicState.get("shadowsArray"),g=publicState.get("lightProbeGridArray");publicState.set("lightsArray",JSValue::array());publicState.set("shadowsArray",nullptr);publicState.set("lightProbeGridArray",nullptr);state.pushLight(stateLight());state.pushLight(stateLight(true));state.pushShadow(stateLight());REQUIRE(a.length()==1);REQUIRE(s.length()==1);REQUIRE(g.length()==1);auto camera=JSValue::object();state.init(camera);REQUIRE(a.length()==0);REQUIRE(s.length()==0);REQUIRE(g.length()==0);REQUIRE(publicState.get("camera").sameValue(camera));
}
TEST_CASE("WebGLRenderStates scene cache uses weak ephemeron identities", "[renderers][webgl][gc]"){
 WebGLRenderStates states;auto anchor=JSValue::object();JSWeakValue sw,rw;{auto scene=JSValue::object();auto state=states.get(scene);sw=JSWeakValue(scene);rw=JSWeakValue(state);state.set("scene",scene);anchor.collectGarbage();REQUIRE_FALSE(sw.expired());}anchor.collectGarbage();REQUIRE(sw.expired());REQUIRE(rw.expired());
}
TEST_CASE("WebGLRenderState method constructors use captured data and alternate newTarget", "[renderers][webgl][construct]"){
 WebGLRenderState state;WebGLRenderStates cache;auto scene=JSValue::object();auto target=JSValue::constructor([](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("not invoked");},{},"Target",0,JSValue::ConstructorKind::Callable);auto p=JSValue::object();target.set("prototype",p);
 for(const auto* name:{"init","pushLight","pushShadow","setupLights","setupLightsView","get","dispose"}){auto fn=(std::string(name)=="get"||std::string(name)=="dispose"?cache.object():state.object()).get(name);REQUIRE(fn.isConstructible());Args args;if(std::string(name)=="get"||std::string(name)=="init")args={scene};if(std::string(name)=="pushLight"||std::string(name)=="pushShadow")args={stateLight()};if(std::string(name)=="setupLightsView"){auto camera=JSValue::object();camera.set("matrixWorldInverse",JSValue::object());args={camera};}if(std::string(name)=="setupLights"){state.init(scene);}auto result=fn.construct(args,target);if(std::string(name)=="get")REQUIRE(result.sameValue(cache.get(scene)));else REQUIRE(result.getPrototype().sameValue(p));}
}
