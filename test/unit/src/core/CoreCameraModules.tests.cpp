#include "catch.hpp"
#include "core/Clock.h"
#include "core/Timer.h"
#include "core/InstancedBufferGeometry.h"
#include "core/NativeObject.h"
#include "cameras/ArrayCamera.h"
#include "cameras/StereoCamera.h"
#include "math/FrustumArray.h"
#include "objects/ClippingGroup.h"
#include "objects/Mesh.h"
#include "objects/Sprite.h"
#include <algorithm>

namespace {
NativeTimeSource clockSource(double& now, std::vector<std::string>& warnings) {
    return {[&]() -> JSValue { return now; }, [&](std::string_view text) { warnings.emplace_back(text); }};
}
struct Visibility final : NativeVisibilitySource {
    JSValue value = false;
    std::vector<Listener> listeners;
    std::size_t additions = 0, removals = 0;
    JSValue hidden() const override { return value; }
    void addVisibilityListener(const Listener& listener, bool capture) override {
        REQUIRE_FALSE(capture); ++additions; listeners.push_back(listener);
    }
    void removeVisibilityListener(const Listener& listener) override {
        ++removals; std::erase(listeners, listener);
    }
    void dispatch() { const auto snapshot = listeners; for (const auto& listener : snapshot) (*listener)(); }
};
}
TEST_CASE("Clock follows automatic start stop and explicit restart", "[core][clock]") {
    double now = 100; std::vector<std::string> warnings;
    Clock clock(true, clockSource(now, warnings));
    REQUIRE(warnings == std::vector<std::string>{"THREE.Clock: This module has been deprecated. Please use THREE.Timer instead."});
    REQUIRE(clock.getDelta() == 0); REQUIRE(clock.running.strictEquals(true)); REQUIRE(clock.startTime.toNumber() == 100);
    now = 223; REQUIRE(clock.getDelta() == Approx(.123));
    now = 323; REQUIRE(clock.getElapsedTime().toNumber() == Approx(.223));
    now = 500; clock.stop(); REQUIRE(clock.elapsedTime.toNumber() == Approx(.4));
    REQUIRE(clock.autoStart.strictEquals(false)); REQUIRE(clock.running.strictEquals(false));
    now = 1000; REQUIRE(clock.getDelta() == 0); REQUIRE(clock.getElapsedTime().toNumber() == Approx(.4));
    clock.start(); REQUIRE(clock.getElapsedTime().toNumber() == 0); REQUIRE(clock.startTime.toNumber() == 1000);
    now = 900; REQUIRE(clock.getDelta() == Approx(-.1));
}
TEST_CASE("Clock truthiness and accumulated runtime values match JavaScript", "[core][clock]") {
    double now = 0; std::vector<std::string> warnings;
    Clock clock("false", clockSource(now, warnings)); REQUIRE(clock.getDelta() == 0); REQUIRE(clock.running.toBoolean());
    clock.elapsedTime = "prefix"; now = 1000;
    REQUIRE(clock.getElapsedTime().toString() == "prefix1");
    clock.running = 0; REQUIRE(clock.getDelta() == 0); REQUIRE(clock.elapsedTime.strictEquals(0));
    Clock undefined(JSValue(), clockSource(now, warnings)); REQUIRE(undefined.autoStart.strictEquals(true));
    Clock stopped(false, clockSource(now, warnings)); REQUIRE(stopped.getDelta() == 0); REQUIRE(stopped.startTime.strictEquals(0));
    stopped.stop(); REQUIRE_FALSE(stopped.running.toBoolean()); REQUIRE(stopped.elapsedTime.strictEquals(0));
}
TEST_CASE("Timer updates once per frame supports signed scaling and reset", "[core][timer]") {
    double now = 100; std::vector<std::string> warnings; Timer timer(clockSource(now, warnings));
    REQUIRE(timer._startTime.toNumber() == 100); REQUIRE(timer.getDelta() == 0); REQUIRE(timer.getElapsed() == 0);
    now = 116; REQUIRE(&timer.update() == &timer); REQUIRE(timer.getDelta() == Approx(.016)); REQUIRE(timer.getDelta() == Approx(.016));
    REQUIRE(&timer.setTimescale(-2) == &timer); REQUIRE(timer.getTimescale().strictEquals(-2));
    timer.update(126); REQUIRE(timer.getDelta() == Approx(-.02)); REQUIRE(timer.getElapsed() == Approx(-.004));
    now = 1000; REQUIRE(&timer.reset() == &timer); REQUIRE(timer.getDelta() == Approx(-.02)); REQUIRE(timer.getElapsed() == Approx(-.004));
    timer.setTimescale(".5").update(1100); REQUIRE(timer.getDelta() == Approx(.05)); REQUIRE(timer.getElapsed() == Approx(.046));
    timer.update(nullptr); REQUIRE(timer.getDelta() == Approx(-.55)); REQUIRE(timer.getTimescale().toString() == ".5");
}
TEST_CASE("Timer visibility only suppresses literal true and resumes on literal false", "[core][timer]") {
    double now = 100; std::vector<std::string> warnings; Timer timer(clockSource(now, warnings)); auto source = std::make_shared<Visibility>();
    timer.connect(source); REQUIRE(source->additions == 1); REQUIRE(timer._pageVisibilityHandler == source->listeners[0]);
    now = 200; timer.update(); source->value = true; now = 1000; source->dispatch(); timer.update();
    REQUIRE(timer.getDelta() == 0); REQUIRE(timer.getElapsed() == Approx(.1)); REQUIRE(timer._currentTime.toNumber() == 100);
    source->value = "true"; timer.update(); REQUIRE(timer.getDelta() == Approx(.8));
    source->value = false; now = 2000; source->dispatch(); now = 2016; timer.update(); REQUIRE(timer.getDelta() == Approx(.016));
    timer.dispose(); REQUIRE(source->removals == 1); REQUIRE(source->listeners.empty()); REQUIRE_FALSE(timer._document); REQUIRE_FALSE(timer._pageVisibilityHandler);
    timer.dispose(); REQUIRE(source->removals == 1);
}
TEST_CASE("Timer repeated connect keeps prior listeners and latest document semantics", "[core][timer]") {
    double now = 0; std::vector<std::string> warnings; auto a = std::make_shared<Visibility>(), b = std::make_shared<Visibility>();
    {
        Timer timer(clockSource(now, warnings)); timer.connect(a); const auto first = timer._pageVisibilityHandler;
        timer.connect(b); REQUIRE(first != timer._pageVisibilityHandler); REQUIRE(a->listeners.size() == 1); REQUIRE(b->listeners.size() == 1);
        b->value = false; a->value = true; now = 200; a->dispatch(); REQUIRE(timer._currentTime.toNumber() == 200);
        timer.disconnect(); REQUIRE(a->listeners.size() == 1); REQUIRE(b->listeners.empty());
        REQUIRE_THROWS_AS(a->dispatch(), JSTypeError);
    }
    REQUIRE_NOTHROW(a->dispatch());
    Timer noAPI(clockSource(now, warnings)); a->value = JSValue(); noAPI.connect(a); REQUIRE_FALSE(noAPI._pageVisibilityHandler);
    a->value = true; now = 300; noAPI.update(); REQUIRE(noAPI.getDelta() == Approx(.1));
}
TEST_CASE("Native timer subscriptions retain identity through reentrant source callbacks", "[core][timer]") {
    struct ReentrantSource : NativeVisibilitySource {
        JSValue value=false; std::function<void()> onRead,onAdd,onRemove; Listener current;
        JSValue hidden() const override { const auto saved=value; if(onRead)onRead(); return saved; }
        void addVisibilityListener(const Listener& listener,bool) override {if(onAdd)onAdd();current=listener;}
        void removeVisibilityListener(const Listener& listener) override {REQUIRE(current==listener);if(onRemove)onRemove();current.reset();}
    };
    double now=100;std::vector<std::string>warnings;Timer timer(clockSource(now,warnings));
    auto first=std::make_shared<ReentrantSource>(),second=std::make_shared<ReentrantSource>();timer.connect(first);
    first->value=true;first->onRead=[&]{timer._document.reset();};now=200;
    REQUIRE_NOTHROW(timer.update());REQUIRE(timer.getDelta()==0);REQUIRE_FALSE(timer._document);
    first->onRead={};timer._document=first;first->onRemove=[&]{timer.connect(second);};timer.disconnect();
    REQUIRE_FALSE(timer._document);REQUIRE_FALSE(timer._pageVisibilityHandler);REQUIRE(second->current);
    REQUIRE_THROWS_AS((*second->current)(),JSTypeError);
    first->onRemove={};first->onAdd=[&]{timer.connect(second);};timer.connect(first);
    REQUIRE(first->current!=second->current);REQUIRE(timer._pageVisibilityHandler==second->current);REQUIRE(timer._document==second);
}
TEST_CASE("Timer and Clock dispatch overridable methods in upstream order", "[core][timer][clock]") {
    struct DerivedClock : Clock {
        int starts=0,elapsedQueries=0;
        using Clock::Clock;
        void start() override {++starts;Clock::start();}
        JSValue getElapsedTime() override {++elapsedQueries;return Clock::getElapsedTime();}
    };
    struct DerivedTimer : Timer {
        int resets=0;
        using Timer::Timer;
        Timer& reset() override {++resets;return Timer::reset();}
    };
    double now=100;std::vector<std::string>warnings;DerivedClock clock(true,clockSource(now,warnings));
    clock.stop();REQUIRE(clock.elapsedQueries==1);REQUIRE(clock.starts==1);REQUIRE(clock.running.strictEquals(false));
    DerivedTimer timer(clockSource(now,warnings));auto source=std::make_shared<Visibility>();timer.connect(source);
    now=200;source->dispatch();REQUIRE(timer.resets==1);REQUIRE(timer._currentTime.toNumber()==100);
}
TEST_CASE("InstancedBufferGeometry copies resource state and preserves runtime count identity", "[core][instancedgeometry]") {
    InstancedBufferGeometry source; REQUIRE(source.isInstancedBufferGeometry); REQUIRE(source.type == "InstancedBufferGeometry");
    REQUIRE(std::isinf(source.instanceCount.toNumber())); source.uuid = "geometry";
    auto position = std::make_shared<Float32BufferAttribute>(std::vector<float>{0,0,0,1,0,0,0,1,0},3);
    source.setAttribute("position", position); auto count = JSValue::object(); count.set("value", 3); source.instanceCount = count;
    auto copy = std::dynamic_pointer_cast<InstancedBufferGeometry>(source.clone()); REQUIRE(copy); REQUIRE(copy->instanceCount.sameValue(count));
    REQUIRE(copy->getAttribute("position") != position); REQUIRE(copy->getAttribute("position")->getCount() == 3);
    REQUIRE(copy->toJSON().at("isInstancedBufferGeometry").get<bool>());
    BufferGeometry plain; copy->copy(plain); REQUIRE(copy->instanceCount.isUndefined()); REQUIRE_FALSE(copy->toJSON().contains("instanceCount"));
    source.instanceCount = nullptr; REQUIRE(source.toJSON().at("instanceCount").isNull());
}
TEST_CASE("ArrayCamera aliases input camera arrays and inherits copy without copying extensions", "[cameras][arraycamera]") {
    auto camera = std::make_shared<PerspectiveCamera>(); SharedArray<std::shared_ptr<Camera>> cameras{camera}; ArrayCamera source(cameras);
    REQUIRE(source.isArrayCamera); REQUIRE_FALSE(source.isMultiViewCamera); REQUIRE(source.type == "PerspectiveCamera");
    REQUIRE(source.cameras.sameIdentity(cameras)); cameras.push_back(std::make_shared<Camera>()); REQUIRE(source.cameras.size() == 2);
    source.isMultiViewCamera = true; source.fov = 42; source.add(std::make_shared<Object3D>());
    auto clone = std::dynamic_pointer_cast<ArrayCamera>(source.clone()); REQUIRE(clone); REQUIRE(clone->cameras.empty()); REQUIRE_FALSE(clone->isMultiViewCamera);
    REQUIRE(clone->fov == 42); REQUIRE(clone->children.size() == 1);
    REQUIRE(source.clone(false)->children.size() == 1);
    ArrayCamera destination({camera}); destination.copy(source,false); REQUIRE(destination.cameras.size() == 1); REQUIRE(destination.children.empty());
    REQUIRE_FALSE(source.toJSON().at("object").contains("cameras"));
}
TEST_CASE("StereoCamera applies off-axis projection and leaves world and inverse matrices untouched", "[cameras][stereo]") {
    StereoCamera stereo; PerspectiveCamera camera(60,2,.2,500); camera.matrixWorld.makeTranslation(1,2,3);
    REQUIRE(stereo.type == "StereoCamera"); REQUIRE(stereo.cameraL->layers.mask == 3); REQUIRE(stereo.cameraR->layers.mask == 5);
    REQUIRE_FALSE(stereo.cameraL->matrixAutoUpdate); REQUIRE_FALSE(stereo.cameraR->matrixAutoUpdate);
    const auto inverse = stereo.cameraL->projectionMatrixInverse.clone(); stereo.update(camera);
    REQUIRE(stereo.cameraL->matrix.elements()[12] == Approx(.968)); REQUIRE(stereo.cameraR->matrix.elements()[12] == Approx(1.032));
    REQUIRE(stereo.cameraL->matrixWorld.elements()[12] == 0); REQUIRE(stereo.cameraL->matrixWorldNeedsUpdate);
    REQUIRE(stereo.cameraL->projectionMatrixInverse.equals(inverse));
    REQUIRE(stereo.cameraL->projectionMatrix.elements()[8] == Approx(-stereo.cameraR->projectionMatrix.elements()[8]));
    const auto projection = stereo.cameraL->projectionMatrix.clone(); camera.projectionMatrix.elements()[0] = 123;
    stereo.update(camera); REQUIRE(stereo.cameraL->projectionMatrix.equals(projection));
    stereo.eyeSep = .2; stereo.update(camera); REQUIRE(stereo.cameraL->matrix.elements()[12] == Approx(.9));
}
TEST_CASE("StereoCamera preserves shared eye transforms across instances on cache hits", "[cameras][stereo]") {
    PerspectiveCamera camera; StereoCamera a,b; a.eyeSep = .2; b.eyeSep = .8; a.update(camera); b.update(camera); a.update(camera);
    REQUIRE(a.cameraL->matrix.elements()[12] == Approx(-.4)); REQUIRE(a.cameraR->matrix.elements()[12] == Approx(.4));
    REQUIRE(a._cache.eyeSep.toNumber() == Approx(.2));
}
TEST_CASE("FrustumArray retains pooled frustums but queries only active cameras", "[math][frustumarray]") {
    auto left = std::make_shared<PerspectiveCamera>(60,1,.1,100), right = std::make_shared<PerspectiveCamera>(60,1,.1,100);
    left->position.x = -5; right->position.x = 5; left->updateMatrixWorld(); right->updateMatrixWorld();
    ArrayCamera cameras({left,right}); FrustumArray frustums;
    REQUIRE_FALSE(frustums.containsPoint({0,0,-5})); REQUIRE(&frustums.setFromArrayCamera(cameras) == &frustums);
    REQUIRE(frustums._count == 2); REQUIRE(frustums._frustums.size() == 2); REQUIRE(frustums.containsPoint({-5,0,-5})); REQUIRE(frustums.containsPoint({5,0,-5}));
    auto pool = frustums._frustums[1]; cameras.cameras = {left}; frustums.setFromArrayCamera(cameras);
    REQUIRE(frustums._count == 1); REQUIRE(frustums._frustums.size() == 2); REQUIRE(frustums._frustums[1] == pool); REQUIRE_FALSE(frustums.containsPoint({5,0,-5}));
    auto clone = frustums.clone(); REQUIRE(clone._frustums.size() == 1); REQUIRE(clone._frustums[0] != frustums._frustums[0]);
    cameras.cameras.clear(); frustums.setFromArrayCamera(cameras); REQUIRE(frustums._count == 0); REQUIRE_FALSE(frustums.intersectsSphere(Sphere({0,0,-5},1000)));
    frustums.copy(clone); REQUIRE(frustums._count == 1); REQUIRE(frustums._frustums[1] == pool);
}
TEST_CASE("ClippingGroup retains constructor defaults when cloned through inherited copy", "[objects][clippinggroup]") {
    ClippingGroup source; REQUIRE(source.isClippingGroup); REQUIRE(source.isGroup); REQUIRE(source.type == "Group");
    source.clippingPlanes.push_back(std::make_shared<Plane>()); source.enabled=false; source.clipIntersection=true; source.clipShadows=true; source.add(std::make_shared<Object3D>());
    auto clone=std::dynamic_pointer_cast<ClippingGroup>(source.clone()); REQUIRE(clone); REQUIRE(clone->clippingPlanes.empty()); REQUIRE(clone->enabled.strictEquals(true));
    REQUIRE(clone->clipIntersection.strictEquals(false)); REQUIRE(clone->clipShadows.strictEquals(false)); REQUIRE(clone->children.size()==1);
    ClippingGroup destination; destination.enabled="manual"; auto planes=destination.clippingPlanes; destination.copy(source,false);
    REQUIRE(destination.enabled.toString()=="manual"); REQUIRE(destination.clippingPlanes.sameIdentity(planes)); REQUIRE(destination.children.empty());
    const auto json=source.toJSON().at("object"); REQUIRE_FALSE(json.contains("clippingPlanes")); REQUIRE_FALSE(json.contains("enabled"));
}
TEST_CASE("New camera geometry and clipping wrappers write through live native fields", "[core][nativeobject][corecamera]") {
    auto camera=std::make_shared<ArrayCamera>();auto sub=std::make_shared<PerspectiveCamera>();auto wrapper=nativeObject(camera),array=wrapper.get("cameras");
    REQUIRE(wrapper.get("isArrayCamera").strictEquals(true));REQUIRE(wrapper.get("isMultiViewCamera").strictEquals(false));
    array.get("push").call(array,{nativeObject(sub)});REQUIRE(camera->cameras.size()==1);REQUIRE(camera->cameras[0]==sub);
    REQUIRE(array.sameValue(wrapper.get("cameras")));wrapper.set("isMultiViewCamera",true);REQUIRE(camera->isMultiViewCamera);
    auto group=std::make_shared<ClippingGroup>();auto plane=std::make_shared<Plane>(Vector3(1,2,3),4);group->clippingPlanes.push_back(plane);
    auto g=nativeObject(group),planes=g.get("clippingPlanes"),p=planes.get("0");
    REQUIRE(g.get("isClippingGroup").strictEquals(true));g.set("enabled","manual");g.set("clipIntersection",true);g.set("clipShadows",nullptr);
    REQUIRE(group->enabled.toString()=="manual");REQUIRE(group->clipIntersection.strictEquals(true));REQUIRE(group->clipShadows.isNull());
    p.get("normal").set("x",7);p.set("constant",9);REQUIRE(plane->normal.x==7);REQUIRE(plane->constant==9);
    planes.get("push").call(planes,{p});REQUIRE(group->clippingPlanes.size()==2);REQUIRE(group->clippingPlanes[0]==group->clippingPlanes[1]);
    auto geometry=std::make_shared<InstancedBufferGeometry>();auto mesh=std::make_shared<Mesh>(geometry);auto geometryWrapper=nativeObject(mesh).get("geometry");
    REQUIRE(geometryWrapper.get("isInstancedBufferGeometry").strictEquals(true));auto count=JSValue::object();geometryWrapper.set("instanceCount",count);
    REQUIRE(geometry->instanceCount.sameValue(count));geometry->instanceCount="7";REQUIRE(geometryWrapper.get("instanceCount").toString()=="7");
}
TEST_CASE("Native owner graph collects camera clipping and instanced geometry cycles", "[core][nativeobject][corecamera][gc]") {
    auto root=JSValue::object();
    {
        auto array=std::make_shared<ArrayCamera>();auto a=nativeObject(array);a.get("cameras");
        auto group=std::make_shared<ClippingGroup>();group->clippingPlanes.push_back(std::make_shared<Plane>());auto g=nativeObject(group);g.get("clippingPlanes").get("0").get("normal");
        auto mesh=std::make_shared<Mesh>(std::make_shared<InstancedBufferGeometry>());nativeObject(mesh).get("geometry");
    }
    root.collectGarbage();const auto baseline=root.heapObjectCount();std::weak_ptr<ArrayCamera> weakArray;std::weak_ptr<ClippingGroup> weakGroup;std::weak_ptr<InstancedBufferGeometry> weakGeometry;
    {
        auto array=std::make_shared<ArrayCamera>();weakArray=array;auto a=nativeObject(array);auto sub=std::make_shared<PerspectiveCamera>();array->cameras.push_back(sub);nativeObject(sub).set("owner",a);a.get("cameras").set("owner",a);
        auto group=std::make_shared<ClippingGroup>();weakGroup=group;auto g=nativeObject(group);group->enabled=g;group->clipIntersection=g;group->clipShadows=g;
        group->clippingPlanes.push_back(std::make_shared<Plane>());g.get("clippingPlanes").get("0").get("normal").set("owner",g);
        auto geometry=std::make_shared<InstancedBufferGeometry>();weakGeometry=geometry;auto mesh=std::make_shared<Mesh>(geometry);auto m=nativeObject(mesh);geometry->instanceCount=m;
        root.collectGarbage();REQUIRE_FALSE(weakArray.expired());REQUIRE_FALSE(weakGroup.expired());REQUIRE_FALSE(weakGeometry.expired());
    }
    REQUIRE(root.collectGarbage()>0);REQUIRE(weakArray.expired());REQUIRE(weakGroup.expired());REQUIRE(weakGeometry.expired());REQUIRE(root.heapObjectCount()==baseline);
}
TEST_CASE("Pure native camera array cycles retain their explicit C++ ownership", "[core][nativeobject][corecamera][gc]") {
    auto root=JSValue::object();std::weak_ptr<ArrayCamera> weak;
    {auto camera=std::make_shared<ArrayCamera>();weak=camera;camera->cameras.push_back(camera);auto wrapper=nativeObject(camera);wrapper.get("cameras").set("owner",wrapper);}
    root.collectGarbage();REQUIRE_FALSE(weak.expired());
    {auto camera=weak.lock();camera->cameras.clear();}
    root.collectGarbage();REQUIRE(weak.expired());
}
