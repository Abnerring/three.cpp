#include "catch.hpp"
#include "renderers/webgl/WebGLGeometries.h"
#include "renderers/webgl/WebGLObjects.h"
#include "core/EventDispatcher.h"
#include "core/JSJobs.h"
#include "core/JSWeakMap.h"
#include "core/JSFinalizationRegistry.h"
#include "core/NativeObject.h"
#include "core/BufferGeometry.h"
#include "core/InstancedBufferGeometry.h"
#include "objects/InstancedMesh.h"
#include "objects/SkinnedMesh.h"
#include <limits>

namespace {
using Args = std::vector<JSValue>;
JSPropertyDescriptor data(JSValue value, bool enumerable = true) { JSPropertyDescriptor d; d.value = value; d.writable = d.configurable = true; d.enumerable = enumerable; return d; }
void push(JSValue list, JSValue value) { list.get("push").call(list, {value}); }
JSValue tuple(const Args& values) { auto result = JSValue::array(); result.get("push").call(result, values); return result; }
struct Fixture {
    JSValue log = JSValue::array(), gl = JSValue::object(), attributes = JSValue::object(), info = JSValue::object(), binding = JSValue::object();
    Fixture() {
        gl.set("ARRAY_BUFFER", 34962); auto memory = JSValue::object(); memory.set("geometries", 0); info.set("memory", memory);
        auto render = JSValue::object(); render.set("frame", 0); info.set("render", render);
        for (const char* name : {"update", "remove"}) attributes.set(name, JSValue::function(+[](const JSValue& self, const Args& args, const Args& cap)->JSValue {
            REQUIRE(self.sameValue(cap[1])); push(cap[0], tuple({cap[2], args[0]})); return {};
        }, {log, attributes, name}));
        for (const char* name : {"releaseStatesOfGeometry", "releaseStatesOfObject"}) binding.set(name, JSValue::function(+[](const JSValue& self, const Args& args, const Args& cap)->JSValue {
            REQUIRE(self.sameValue(cap[1])); push(cap[0], tuple({cap[2], args[0]})); return {};
        }, {log, binding, name}));
    }
    double memory() const { return info.get("memory").get("geometries").toNumber(); }
    void frame(JSValue value) { auto render = info.get("render"); render.set("frame", value); }
};
JSValue geometry(unsigned id = 1, unsigned vertices = 3) {
    auto result = EventDispatcher::runtimeObject(); result.set("id", id); result.set("index", nullptr);
    auto map = JSValue::object(); auto position = nativeAttribute(std::make_shared<Float32BufferAttribute>(double(vertices) * 3, 3)); map.set("position", position);
    result.set("attributes", map); return result;
}
void dispose(JSValue object) { auto event = JSValue::object(); event.set("type", "dispose"); object.get("dispatchEvent").call(object, {event}); }
JSValue instance(JSValue geometry) {
    auto result = EventDispatcher::runtimeObject(); result.set("geometry", geometry); result.set("isInstancedMesh", true);
    result.set("instanceMatrix", JSValue::object()); result.set("instanceColor", JSValue::object()); return result;
}
}
TEST_CASE("Geometry registration and explicit disposal preserve shared identities and full removal order", "[webgl][geometries]") {
    Fixture f; WebGLGeometries geometries(f.gl, f.attributes, f.info, f.binding); auto g = geometry();
    auto index = nativeAttribute(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0,1,2}, 1)); g.set("index", index);
    g.set("isInstancedBufferGeometry", true); g.set("_maxInstanceCount", 4);
    REQUIRE(geometries.get({}, g).sameValue(g)); REQUIRE(geometries.get({}, g).sameValue(g)); REQUIRE(f.memory() == 1);
    auto wire = geometries.getWireframeAttribute(g); REQUIRE(wire.get("array").get("length").number() == 6);
    dispose(g); REQUIRE(f.memory() == 0); REQUIRE_FALSE(g.hasOwn("_maxInstanceCount"));
    REQUIRE(f.log.length() == 4); REQUIRE(f.log.get("0").get("1").sameValue(index)); REQUIRE(f.log.get("2").get("1").sameValue(wire));
    REQUIRE(f.log.get("3").get("0").strictEquals("releaseStatesOfGeometry"));
    geometries.get({}, g); REQUIRE(f.memory() == 1); geometries.dispose(); REQUIRE(f.memory() == 0);
}
TEST_CASE("Wireframe generation keeps pinned indexed and nonindexed invalidation behavior", "[webgl][geometries]") {
    Fixture f; WebGLGeometries geometries(f.gl, f.attributes, f.info, f.binding); auto g = geometry();
    auto index = nativeAttribute(std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{3,4,5,1,2}, 1)); index.set("version", "2"); g.set("index", index);
    auto wire = geometries.getWireframeAttribute(g); auto array = wire.get("array");
    const std::vector<double> expected{3,4,4,5,5,3,1,2,2,0,0,1}; for(unsigned i=0;i<expected.size();++i) REQUIRE(array.get(std::to_string(i)).number()==expected[i]);
    index.set("version", "10"); REQUIRE(geometries.getWireframeAttribute(g).sameValue(wire));
    index.set("version", 3); auto next = geometries.getWireframeAttribute(g); REQUIRE_FALSE(next.sameValue(wire)); REQUIRE(f.log.get("0").get("1").sameValue(wire));
    g.set("index", nullptr); auto position = g.get("attributes").get("position"); position.set("version", 100); REQUIRE(geometries.getWireframeAttribute(g).sameValue(next));
    auto wide = geometry(2, 65535); REQUIRE(geometries.getWireframeAttribute(wide).get("array").get("constructor").get("name").strictEquals("Uint32Array"));
    auto missing = geometry(3); auto map = missing.get("attributes"); map.erase("position"); REQUIRE(geometries.getWireframeAttribute(missing).isUndefined());
}
TEST_CASE("Geometry attribute traversal includes inherited keys and nonenumerable shadows", "[webgl][geometries]") {
    Fixture f; WebGLGeometries geometries(f.gl, f.attributes, f.info, f.binding); auto g=geometry(), parent=JSValue::object(), map=JSValue::object(parent);
    auto inherited=JSValue::object(); parent.set("inherited", inherited); parent.set("hidden", 1); map.defineProperty("hidden", data(2, false)); map.set("own", 3); g.set("attributes", map);
    geometries.update(g); REQUIRE(f.log.length()==2); REQUIRE(f.log.get("0").get("1").strictEquals(3)); REQUIRE(f.log.get("1").get("1").sameValue(inherited));
}
TEST_CASE("Failed geometry registry registration preserves prior weak reference assignment", "[webgl][geometries]") {
    Fixture f; WebGLGeometries geometries(f.gl,f.attributes,f.info,f.binding); auto g=geometry();
    auto prototype=nativeFinalizationRegistryConstructor().get("prototype"); const auto old=*prototype.getOwnPropertyDescriptor("register");
    prototype.defineProperty("register",data(JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue {throw JSTypeError("register");}),false));
    REQUIRE_THROWS_AS(geometries.get({},g),JSTypeError); REQUIRE(f.memory()==0); REQUIRE(geometries.get({},g).sameValue(g)); REQUIRE(f.memory()==0);
    prototype.defineProperty("register",old); geometries.dispose(); REQUIRE(f.memory()==-1);
}
TEST_CASE("Object updates deduplicate geometry instance and skeleton separately by strict frame", "[webgl][objects]") {
    Fixture f; WebGLGeometries geometries(f.gl,f.attributes,f.info,f.binding); WebGLObjects objects(f.gl,geometries,f.attributes,f.binding,f.info);
    auto g=geometry(), object=instance(g), skeleton=JSValue::object(); object.set("isSkinnedMesh",true); object.set("skeleton",skeleton);
    skeleton.set("update",JSValue::function(+[](const JSValue& self,const Args&,const Args& cap)->JSValue {REQUIRE(self.sameValue(cap[1]));push(cap[0],"skeleton");return {};},{f.log,skeleton}));
    REQUIRE(objects.update(object).sameValue(g)); REQUIRE(f.log.length()==4); objects.update(object); REQUIRE(f.log.length()==4);
    f.frame("0"); objects.update(object); REQUIRE(f.log.length()==8); objects.dispose(); objects.update(object); REQUIRE(f.log.length()==12);
    f.frame(JSValue{}); objects.dispose(); objects.update(object); REQUIRE(f.log.length()==12);
}
TEST_CASE("Instanced dispose releases current resources and does not invalidate same-frame updateMap", "[webgl][objects]") {
    Fixture f; WebGLGeometries geometries(f.gl,f.attributes,f.info,f.binding); WebGLObjects objects(f.gl,geometries,f.attributes,f.binding,f.info); auto object=instance(geometry());
    objects.update(object); auto matrix=object.get("instanceMatrix"); dispose(object); REQUIRE(f.log.length()==6); REQUIRE(f.log.get("3").get("0").strictEquals("releaseStatesOfObject"));
    REQUIRE(f.log.get("4").get("1").sameValue(matrix)); objects.update(object); REQUIRE(f.log.length()==6);
    f.frame(1); objects.update(object); REQUIRE(f.log.length()==9);
}
TEST_CASE("Returned renderer methods remain ordinary constructible closures with captured receiver ignored", "[webgl][geometries][objects]") {
    Fixture f; WebGLGeometries geometries(f.gl,f.attributes,f.info,f.binding); WebGLObjects objects(f.gl,geometries,f.attributes,f.binding,f.info); auto g=geometry();
    for(auto api:{geometries.object(),objects.object()}) for(const auto& name:api.keys()) {auto fn=api.get(name); REQUIRE(fn.isConstructible()); REQUIRE(fn.get("name").strictEquals(name)); REQUIRE(fn.ownPropertyNames()==std::vector<std::string>{"length","name","prototype"});}
    REQUIRE(geometries.object().get("get").construct({{},g}).sameValue(g));
    auto target=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{return {};}, {}, "Target", 0, JSValue::ConstructorKind::Callable); auto prototype=JSValue::object(); target.set("prototype",prototype);
    auto result=geometries.object().get("update").construct({g},target); REQUIRE(result.getPrototype().sameValue(prototype));
    REQUIRE(objects.object().get("update").call(JSValue::object(),{instance(g)}).sameValue(g));
}
TEST_CASE("Native geometry rendering fields expose one live attribute map and real disposal", "[webgl][native][geometries]") {
    Fixture f; WebGLGeometries geometries(f.gl,f.attributes,f.info,f.binding); auto geometry=std::make_shared<InstancedBufferGeometry>();
    auto position=std::make_shared<Float32BufferAttribute>(std::vector<float>{0,0,0,1,0,0,0,1,0},3); geometry->setAttribute("position",position); auto object=nativeGeometry(geometry),map=object.get("attributes");
    REQUIRE(object.get("id").number()==geometry->id); REQUIRE(object.get("attributes").sameValue(map)); REQUIRE(map.get("position").sameValue(nativeAttribute(position)));
    auto index=std::make_shared<Uint16BufferAttribute>(std::vector<std::uint16_t>{0,1,2},1); object.set("index",nativeAttribute(index)); REQUIRE(geometry->index==index);
    auto normal=std::make_shared<Float32BufferAttribute>(std::vector<float>{0,0,1},3); map.set("normal",nativeAttribute(normal)); REQUIRE(geometry->getAttribute("normal")==normal); REQUIRE(map.erase("normal")); REQUIRE_FALSE(geometry->hasAttribute("normal"));
    geometry->setAttribute("uv",normal); REQUIRE(map.get("uv").sameValue(nativeAttribute(normal)));
    object.set("_maxInstanceCount",3); geometries.get({},object); geometries.update(object); REQUIRE(f.memory()==1); auto wire=geometries.getWireframeAttribute(object); REQUIRE(wire.isObject());
    geometry->dispose(); REQUIRE(f.memory()==0); REQUIRE_FALSE(object.hasOwn("_maxInstanceCount"));
}
TEST_CASE("Native instance uploads and native disposal use the managed renderer listener", "[webgl][native][objects]") {
    Fixture f; WebGLGeometries geometries(f.gl,f.attributes,f.info,f.binding); WebGLObjects objects(f.gl,geometries,f.attributes,f.binding,f.info);
    auto mesh=std::make_shared<InstancedMesh>(std::make_shared<BufferGeometry>(),std::make_shared<MeshBasicMaterial>(),1); auto object=nativeObject(mesh);
    REQUIRE(object.get("instanceMatrix").sameValue(nativeAttribute(mesh->instanceMatrix))); REQUIRE(object.get("instanceColor").isNull());
    objects.update(object); REQUIRE(f.log.length()==1); mesh->dispose(); REQUIRE(f.log.length()==3); REQUIRE(f.log.get("1").get("0").strictEquals("releaseStatesOfObject"));
    REQUIRE(f.log.get("2").get("1").sameValue(nativeAttribute(mesh->instanceMatrix)));
    auto other=nativeObject(std::make_shared<Mesh>());
    REQUIRE_THROWS_AS(object.get("dispose").call(other),JSTypeError);
    const auto descriptor=*object.getOwnPropertyDescriptor("instanceMatrix");
    REQUIRE_THROWS_AS(descriptor.getter->call(other),JSTypeError);
    REQUIRE_THROWS_AS(descriptor.setter->call(other,{nullptr}),JSTypeError);
}
TEST_CASE("Geometry finalization deletes its ID without decrementing source memory accounting", "[webgl][geometries][gc]") {
    Fixture f; WebGLGeometries geometries(f.gl,f.attributes,f.info,f.binding); auto witness=JSValue::object(); JSWeakValue weak;
    {auto g=geometry(99); geometries.get({},g); weak=JSWeakValue(g);}
    endJSJob(); witness.collectGarbage(); REQUIRE(weak.expired()); REQUIRE(f.memory()==1); REQUIRE(hasPendingFinalizationJobs()); drainFinalizationJobs();
    auto replacement=geometry(99); geometries.get({},replacement); REQUIRE(f.memory()==2); geometries.dispose(); REQUIRE(f.memory()==1);
}
