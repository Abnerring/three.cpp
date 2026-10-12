#include "core/Clock.h"
#include "core/Timer.h"
#include "core/InstancedBufferGeometry.h"
#include "cameras/ArrayCamera.h"
#include "cameras/StereoCamera.h"
#include "cameras/OrthographicCamera.h"
#include "math/FrustumArray.h"
#include "objects/ClippingGroup.h"
#include "objects/Mesh.h"
#include "objects/Sprite.h"
#include <algorithm>
#include <cmath>
#include <iostream>
using A = JsonValue::Array; using O = JsonValue::Object;
JsonValue value(JSValue v) {
    if (v.isUndefined()) return "undefined";
    if (v.type() == JSValue::Type::Number) {
        const auto n = v.toNumber();
        if (std::isnan(n)) return "NaN";
        if (std::isinf(n)) return n < 0 ? "-Infinity" : "Infinity";
        if (n == 0 && std::signbit(n)) return "-0";
    }
    return v.toJSON().value_or(JsonValue());
}
JsonValue matrix(const Matrix4& m) { A a; for (const auto n : m.elements()) a.push_back(value(n)); return a; }
NativeTimeSource source(double& now, A& trace) {
    return {[&]() -> JSValue { trace.emplace_back(A{"now",value(now)}); return now; },
            [&](std::string_view text) { trace.emplace_back(A{"warn",std::string(text)}); }};
}
JsonValue clockState(const Clock& c, JSValue result) {
    return A{value(result),value(c.autoStart),value(c.startTime),value(c.oldTime),value(c.elapsedTime),value(c.running)};
}
JsonValue clockCase(int mode) {
    double now = 100; A trace, frames;
    const std::vector<JSValue> starts{true,false,0,1,"","false",nullptr,JSValue()};
    Clock c(starts[mode % starts.size()], source(now,trace));
    frames.push_back(clockState(c,c.getDelta())); now=116; frames.push_back(clockState(c,c.getElapsedTime()));
    if(mode>=8) c.elapsedTime="elapsed";
    now=216; frames.push_back(clockState(c,c.getDelta())); now=250; c.stop(); frames.push_back(clockState(c,JSValue()));
    now=1000; frames.push_back(clockState(c,c.getElapsedTime())); c.start(); frames.push_back(clockState(c,JSValue()));
    now=900; frames.push_back(clockState(c,c.getDelta())); now=1001; frames.push_back(clockState(c,c.getElapsedTime()));
    return O{{"kind","clock"},{"mode",mode},{"frames",frames},{"trace",trace}};
}
struct Visibility final : NativeVisibilitySource {
    JSValue state = false; std::string name; A& trace; std::vector<Listener> listeners;
    explicit Visibility(std::string name,A& trace): name(std::move(name)),trace(trace) {}
    JSValue hidden() const override { trace.emplace_back(A{"hidden",name,value(state)}); return state; }
    void addVisibilityListener(const Listener& listener,bool capture) override {
        trace.emplace_back(A{"add",name,capture}); listeners.push_back(listener);
    }
    void removeVisibilityListener(const Listener& listener) override {
        trace.emplace_back(A{"remove",name,std::find(listeners.begin(),listeners.end(),listener)!=listeners.end()});
        std::erase(listeners,listener);
    }
    void dispatch(){const auto snapshot=listeners;for(const auto& listener:snapshot)(*listener)();}
};
JsonValue timerState(const Timer& t) {
    return A{value(t._previousTime),value(t._currentTime),value(t._startTime),value(t._delta),value(t._elapsed),
             value(t.getTimescale()),value(t.getDelta()),value(t.getElapsed()),bool(t._pageVisibilityHandler),bool(t._document)};
}
JsonValue timerCase(int mode) {
    double now=100; A trace,frames; Timer t(source(now,trace));
    const std::vector<JSValue> scales{1,0,-1,2,.5,NAN,INFINITY,"2",nullptr,false,JSValue(),"bad"};
    t.setTimescale(scales[mode%scales.size()]); frames.push_back(timerState(t));
    now=116; t.update();frames.push_back(timerState(t)); t.update(150);frames.push_back(timerState(t));
    now=500;t.reset();frames.push_back(timerState(t));t.update("550");frames.push_back(timerState(t));
    t.update(nullptr);frames.push_back(timerState(t)); if(mode>=12)t._elapsed="total";
    t.update(100);frames.push_back(timerState(t));t.dispose();frames.push_back(timerState(t));
    return O{{"kind","timer"},{"mode",mode},{"frames",frames},{"trace",trace}};
}
JsonValue visibilityCase(int mode) {
    double now=100; A trace,frames; Timer t(source(now,trace));auto a=std::make_shared<Visibility>("a",trace),b=std::make_shared<Visibility>("b",trace);
    const std::vector<JSValue> values{false,true,JSValue(),"true",0,nullptr};a->state=values[mode%values.size()];
    t.connect(a);now=200;t.update();frames.push_back(timerState(t));
    a->state=true;now=1000;a->dispatch();t.update(1100);frames.push_back(timerState(t));
    a->state=false;now=1200;a->dispatch();now=1216;t.update();frames.push_back(timerState(t));
    if(mode>=6){if(mode>=12)b->state=JSValue();t.connect(b);now=2000;a->dispatch();frames.push_back(timerState(t));}
    t.disconnect();frames.push_back(timerState(t));now=2100;t.update();frames.push_back(timerState(t));t.dispose();
    return O{{"kind","visibility"},{"mode",mode},{"frames",frames},{"trace",trace},{"listeners",A{a->listeners.size(),b->listeners.size()}}};
}
JsonValue runtimeTimerCase(int mode) {
    double now=100;A trace;auto timer=std::make_shared<Timer>(source(now,trace));
    auto log=JSValue::array(),wrapper=JSValue::hostObject(timer,timer.get(),"probe.Timer");
    auto object=JSValue::object();object.set("valueOf",JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>& captures)->JSValue {
        auto t=std::static_pointer_cast<Timer>(captures[0].hostPayload("probe.Timer"));const auto mode=captures[1].toNumber();
        auto log=captures[2];log.get("push").call(log,{"coerce"});
        if(mode==0){t->_startTime=999;return 200;}
        if(mode==1){t->_currentTime=999;return 25;}
        if(mode==2){t->_delta=99;t->_elapsed="x";return 2;}
        if(mode==3){t->_delta=777;return 10;}
        if(mode==4){t->_timescale=9;return 100;}
        throw JSTypeError("coercion");
    },{wrapper,mode,log}));
    std::string error;
    if(mode==1)timer->_currentTime=object;
    if(mode==2)timer->_timescale=object;
    if(mode==3)timer->_elapsed=object;
    if(mode==4)timer->_startTime=object;
    try {timer->update(mode==0||mode==5?object:JSValue(200));}catch(const JSTypeError&){error="TypeError";}
    // A runtime object's JSON value is {}, just as in the upstream snapshot.
    const auto state=timerState(*timer);const auto result=O{{"kind","runtimeTimer"},{"mode",mode},{"state",state},{"trace",trace},{"coercions",*log.toJSON()},{"error",error}};
    timer->_currentTime=0;timer->_previousTime=0;timer->_startTime=0;timer->_delta=0;timer->_elapsed=0;timer->_timescale=1;
    return result;
}
JsonValue stereoState(const StereoCamera& s) {
    return O{{"left",matrix(s.cameraL->projectionMatrix)},{"right",matrix(s.cameraR->projectionMatrix)},
        {"matrixL",matrix(s.cameraL->matrix)},{"matrixR",matrix(s.cameraR->matrix)},
        {"worldL",matrix(s.cameraL->matrixWorld)},{"inverseL",matrix(s.cameraL->projectionMatrixInverse)},
        {"dirty",A{s.cameraL->matrixWorldNeedsUpdate,s.cameraR->matrixWorldNeedsUpdate}},
        {"cache",A{value(s._cache.focus),value(s._cache.fov),value(s._cache.aspect),value(s._cache.near),value(s._cache.far),value(s._cache.zoom),value(s._cache.eyeSep)}}};
}
JsonValue stereoCase(int mode) {
    PerspectiveCamera c(35+mode, .5+(mode%5)*.4, .01+(mode%3)*.1, 10+mode*100);
    c.focus=mode==16?0:2+mode; c.zoom=mode==17?0:mode==18?-1:1+(mode%3)*.5;
    c.coordinateSystem=mode%2?WebGPUCoordinateSystem:WebGLCoordinateSystem;c.reversedDepth=mode%4==3;
    c.filmOffset=(mode%3)-1;if(mode%3==1)c.setViewOffset(1200,800,200,100,600,400);c.updateProjectionMatrix();
    c.position.set(mode-5,mode%3,2);c.rotation.set(.1,.2,.3);c.scale.set(2,1,3);c.updateMatrixWorld();
    StereoCamera s;s.aspect=mode%2?.5:1;s.eyeSep=mode==19?-.5:.064;A frames;
    s.update(c);frames.push_back(stereoState(s));c.projectionMatrix.elements()[5]=123;c.matrixWorld.elements()[12]+=10;
    s.update(c);frames.push_back(stereoState(s));s.eyeSep=.2;s.update(c);frames.push_back(stereoState(s));
    return O{{"kind","stereo"},{"mode",mode},{"frames",frames}};
}
JsonValue stereoShared() {
    PerspectiveCamera c;StereoCamera a,b;a.eyeSep=.2;b.eyeSep=.8;a.update(c);b.update(c);a.update(c);
    return O{{"kind","stereoShared"},{"state",stereoState(a)}};
}
JsonValue frustumState(const FrustumArray& f) {
    A planes;for(const auto& frustum:f._frustums){A group;for(const auto& plane:frustum->planes)group.emplace_back(A{value(plane.normal.x),value(plane.normal.y),value(plane.normal.z),value(plane.constant)});planes.emplace_back(group);}
    return O{{"count",f._count},{"coordinateSystem",f.coordinateSystem},{"planes",planes}};
}
JsonValue frustumCase(int mode) {
    auto a=std::make_shared<PerspectiveCamera>(60,1,.1,50);auto b=std::make_shared<OrthographicCamera>(-3,3,3,-3,.2,30);
    a->coordinateSystem=b->coordinateSystem=mode%2?WebGPUCoordinateSystem:WebGLCoordinateSystem;
    a->reversedDepth=b->reversedDepth=mode%4>=2;a->position.set(-5,0,1);b->position.set(5,0,2);
    a->rotation.y=(mode/4)*.1;b->rotation.y=-(mode/4)*.1;a->updateProjectionMatrix();b->updateProjectionMatrix();a->updateMatrixWorld();b->updateMatrixWorld();
    ArrayCamera cameras({a,b});FrustumArray f;f.coordinateSystem=9000;f.setFromArrayCamera(cameras);A queries;
    auto geometry=std::make_shared<BufferGeometry>();geometry->setAttribute("position",std::make_shared<Float32BufferAttribute>(std::vector<float>{-1,-1,0,1,1,0,0,0,1},3));Mesh mesh(geometry);Sprite sprite;
    for(double x:{-10.,-5.,0.,5.,10.})for(double y:{-4.,0.,4.})for(double z:{2.,0.,-5.,-20.,-60.}){
        Vector3 p(x,y,z);Sphere sphere(p,.5);Box3 box(Vector3(x-.5,y-.5,z-.5),Vector3(x+.5,y+.5,z+.5));
        mesh.matrixWorld.makeTranslation(x,y,z);sprite.matrixWorld.makeTranslation(x,y,z);sprite.center.set(.2,.8);
        queries.emplace_back(A{f.containsPoint(p),f.intersectsSphere(sphere),f.intersectsBox(box),f.intersectsObject(mesh),f.intersectsSprite(sprite)});
    }
    A states{frustumState(f)};const auto original=f._frustums[1];cameras.cameras={a};f.setFromArrayCamera(cameras);states.push_back(frustumState(f));
    auto clone=f.clone();states.push_back(frustumState(clone));cameras.cameras.clear();f.setFromArrayCamera(cameras);states.push_back(frustumState(f));
    const bool empty=f.intersectsSphere(Sphere(Vector3(),100));f.copy(clone);states.push_back(frustumState(f));
    return O{{"kind","frustum"},{"mode",mode},{"queries",queries},{"states",states},{"empty",empty},{"poolIdentity",f._frustums[1]==original},{"copyIdentity",f._frustums[0]==clone._frustums[0]}};
}
JsonValue geometryCase(int mode) {
    InstancedBufferGeometry g;g.uuid="geometry";g.name="instanced";
    g.setAttribute("position",std::make_shared<Float32BufferAttribute>(std::vector<float>{1,2,3,4,5,6},3));g.setIndex(std::vector<std::uint32_t>{0,1});g.addGroup(0,2,3);g.computeBoundingSphere();
    const std::vector<JSValue> counts{INFINITY,3,1.5,0,-1,"7",nullptr,JSValue(),NAN,false};g.instanceCount=counts[mode];
    auto copy=std::dynamic_pointer_cast<InstancedBufferGeometry>(g.clone());copy->uuid="copy";const auto before=copy->toJSON();
    BufferGeometry plain;copy->copy(plain);
    return O{{"kind","geometry"},{"mode",mode},{"count",value(g.instanceCount)},{"json",g.toJSON()},{"clone",before},{"plain",copy->toJSON()},{"undefined",copy->instanceCount.isUndefined()}};
}
void stamp(Object3D& object,const std::string& name) {object.uuid=name;for(std::size_t i=0;i<object.children.size();++i)stamp(*object.children[i],name+"/"+std::to_string(i));}
JsonValue containersCase(int mode) {
    auto c=std::make_shared<PerspectiveCamera>();SharedArray<std::shared_ptr<Camera>> array{c};ArrayCamera a(array);a.isMultiViewCamera=true;a.name="array";a.fov=42;array.push_back(c);a.add(std::make_shared<Object3D>());
    auto ac=std::dynamic_pointer_cast<ArrayCamera>(a.clone(mode%2==0));ArrayCamera target({c});target.copy(a,false);stamp(a,"array");stamp(*ac,"arrayClone");
    ClippingGroup g;g.enabled=false;g.clipIntersection=true;g.clipShadows=true;g.clippingPlanes.push_back(std::make_shared<Plane>(Vector3(1,2,3),4));g.add(std::make_shared<Object3D>());
    auto gc=std::dynamic_pointer_cast<ClippingGroup>(g.clone(mode%2==0));ClippingGroup gt;gt.enabled="manual";gt.copy(g,false);stamp(g,"group");stamp(*gc,"groupClone");
    return O{{"kind","containers"},{"mode",mode},{"array",A{a.type,a.isArrayCamera,a.isMultiViewCamera,a.cameras.size(),a.cameras.sameIdentity(array),ac->type,ac->isArrayCamera,ac->isMultiViewCamera,ac->cameras.size(),target.cameras.size()}},
        {"group",A{g.type,g.isGroup,g.isClippingGroup,g.clippingPlanes.size(),gc->type,gc->isClippingGroup,gc->clippingPlanes.size(),value(gc->enabled),value(gc->clipIntersection),value(gc->clipShadows),value(gt.enabled)}},
        {"json",A{a.toJSON(),ac->toJSON(),g.toJSON(),gc->toJSON()}}};
}
JsonValue cameraCloneCase(int kind) {
    std::shared_ptr<Camera> camera=kind==0?std::make_shared<Camera>():kind==1?std::static_pointer_cast<Camera>(std::make_shared<PerspectiveCamera>()):std::static_pointer_cast<Camera>(std::make_shared<OrthographicCamera>());
    camera->name="camera";auto child=std::make_shared<Object3D>();child->add(std::make_shared<Object3D>());camera->add(child);
    auto clone=camera->clone(false);stamp(*clone,"clone");auto shallow=camera->clone();shallow->clear();shallow->copy(*camera,false);
    return O{{"kind","cameraClone"},{"camera",kind},{"children",clone->children.size()},{"nested",clone->children.empty()?0:clone->children[0]->children.size()},{"shallow",shallow->children.size()},{"json",clone->toJSON()}};
}
int main(){A result;for(int i=0;i<16;++i)result.push_back(clockCase(i));for(int i=0;i<24;++i)result.push_back(timerCase(i));for(int i=0;i<18;++i)result.push_back(visibilityCase(i));for(int i=0;i<20;++i)result.push_back(stereoCase(i));result.push_back(stereoShared());for(int i=0;i<8;++i)result.push_back(frustumCase(i));for(int i=0;i<10;++i)result.push_back(geometryCase(i));for(int i=0;i<2;++i)result.push_back(containersCase(i));for(int i=0;i<6;++i)result.push_back(runtimeTimerCase(i));for(int i=0;i<3;++i)result.push_back(cameraCloneCase(i));std::cout<<JsonValue(result).dump()<<'\n';}
