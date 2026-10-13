#include "Three.h"
#include "core/JSWeakMap.h"
#include "core/JSJobs.h"
#include "math/Plane.h"
#include <iostream>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
unsigned checks = 0;
void check(bool value, const char* why) { ++checks; if (!value) throw std::runtime_error(why); }
JSValue planes(std::initializer_list<std::shared_ptr<Plane>> values) {
    auto result = JSValue::array(); unsigned i = 0;
    for (const auto& value : values) result.set(std::to_string(i++), nativePlane(value));
    return result;
}
struct Restore {
    JSValue object, original; std::string name;
    ~Restore() { object.set(name, original); }
};
struct Fixture {
    VulkanRenderer renderer{{32, 32, false}};
    Scene scene;
    OrthographicCamera camera{-1, 1, 1, -1, .1, 20};
    std::shared_ptr<MeshBasicMaterial> material = std::make_shared<MeshBasicMaterial>(Color(1,1,1));
    std::shared_ptr<Plane> plane = std::make_shared<Plane>(Vector3(1,0,0), 0);
    Fixture() {
        camera.position.z = 5; renderer.srgbOutput = false; renderer.localClippingEnabled = true;
        material->clippingPlanes.emplace(); material->clippingPlanes->push_back(plane);
        auto mesh = std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(4,4), material);
        mesh->frustumCulled = false; scene.add(mesh);
    }
};
void disposeReentry() {
    Fixture fixture; fixture.renderer.render(fixture.scene, fixture.camera);
    auto prototype = nativeWeakMapConstructor().get("prototype"), original = prototype.get("delete");
    Restore restore{prototype, original, "delete"};
    struct State { Fixture* fixture; unsigned deletes = 0; bool reenter = true; };
    auto state = std::make_shared<State>(); state->fixture = &fixture;
    auto token = JSValue::hostObject(state, state.get(), "audit.ClippingDispose");
    auto material = nativeMaterial(fixture.material);
    prototype.set("delete", JSValue::function(+[](const JSValue& self, const auto& args, const auto& captures)->JSValue {
        auto result = captures[0].call(self, args);
        if (!args.empty() && args[0].sameValue(captures[2])) {
            const auto state = std::static_pointer_cast<State>(captures[1].hostPayload("audit.ClippingDispose"));
            ++state->deletes;
            if (state->reenter) {
                state->reenter = false;
                state->fixture->renderer.render(state->fixture->scene, state->fixture->camera);
            }
        }
        return result;
    }, {original, token, material}));
    fixture.material->dispose();
    check(state->deletes == 1, "first dispose did not delete material properties");
    fixture.material->dispose();
    std::cout << "dispose delete calls: " << state->deletes << "\n";
    check(state->deletes == 2, "dispose delete -> render failed to register the replacement material listener");
}
void disposeThrowBefore() {
    Fixture fixture; fixture.renderer.render(fixture.scene, fixture.camera);
    auto prototype = nativeWeakMapConstructor().get("prototype"), original = prototype.get("delete");
    Restore restore{prototype, original, "delete"};
    struct State { unsigned deletes = 0; };
    auto state = std::make_shared<State>();
    auto token = JSValue::hostObject(state, state.get(), "audit.ThrowDelete");
    auto material = nativeMaterial(fixture.material);
    prototype.set("delete", JSValue::function(+[](const JSValue& self, const auto& args, const auto& captures)->JSValue {
        if (!args.empty() && args[0].sameValue(captures[2])) {
            ++std::static_pointer_cast<State>(captures[1].hostPayload("audit.ThrowDelete"))->deletes;
            throw std::runtime_error("audit-before-delete");
        }
        return captures[0].call(self, args);
    }, {original, token, material}));
    bool threw = false;
    try { fixture.material->dispose(); } catch (const std::runtime_error& e) { threw = std::string(e.what()) == "audit-before-delete"; }
    check(threw && state->deletes == 1, "pre-delete exception not propagated");
    fixture.material->dispose();
    check(state->deletes == 1, "removed listener ran again after failed delete");
    fixture.renderer.render(fixture.scene, fixture.camera);
    fixture.material->dispose();
    check(state->deletes == 1, "unchanged properties re-registered listener after failed delete");
}
void propertiesGetReentry() {
    Fixture fixture; fixture.renderer.render(fixture.scene, fixture.camera);
    auto prototype = nativeWeakMapConstructor().get("prototype"), originalGet = prototype.get("get"), originalDelete = prototype.get("delete");
    Restore restoreGet{prototype, originalGet, "get"}, restoreDelete{prototype, originalDelete, "delete"};
    struct State { Material* material; unsigned gets = 0, deletes = 0; };
    auto state = std::make_shared<State>(); state->material = fixture.material.get();
    auto token = JSValue::hostObject(state, state.get(), "audit.GetReentry"), material = nativeMaterial(fixture.material);
    prototype.set("get", JSValue::function(+[](const JSValue& self, const auto& args, const auto& captures)->JSValue {
        if (!args.empty() && args[0].sameValue(captures[2])) {
            const auto state = std::static_pointer_cast<State>(captures[1].hostPayload("audit.GetReentry"));
            ++state->gets;
            if (state->gets == 1) return {};
            if (state->gets == 3) state->material->dispose();
        }
        return captures[0].call(self, args);
    }, {originalGet, token, material}));
    prototype.set("delete", JSValue::function(+[](const JSValue& self, const auto& args, const auto& captures)->JSValue {
        if (!args.empty() && args[0].sameValue(captures[2]))
            ++std::static_pointer_cast<State>(captures[1].hostPayload("audit.GetReentry"))->deletes;
        return captures[0].call(self, args);
    }, {originalDelete, token, material}));
    fixture.renderer.render(fixture.scene, fixture.camera);
    check(state->gets >= 3 && state->deletes == 1, "registration get did not reenter material disposal");
    fixture.material->dispose();
    check(state->deletes == 2, "registration failed after get -> dispose reentry");
}
void disposeDestroyRenderer() {
    Scene scene; OrthographicCamera camera(-1,1,1,-1,.1,20); camera.position.z = 5;
    auto material = std::make_shared<MeshBasicMaterial>(Color(1,1,1));
    material->clippingPlanes.emplace(); material->clippingPlanes->push_back(std::make_shared<Plane>(Vector3(1,0,0), 0));
    scene.add(std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(4,4), material));
    auto renderer = std::make_unique<VulkanRenderer>(VulkanRendererParameters{32,32,false});
    renderer->localClippingEnabled = true; renderer->render(scene, camera);
    auto prototype = nativeWeakMapConstructor().get("prototype"), original = prototype.get("delete");
    Restore restore{prototype, original, "delete"};
    struct State { std::unique_ptr<VulkanRenderer>* renderer; unsigned deletes = 0; };
    auto state = std::make_shared<State>(); state->renderer = &renderer;
    auto token = JSValue::hostObject(state, state.get(), "audit.DestroyRenderer");
    prototype.set("delete", JSValue::function(+[](const JSValue& self, const auto& args, const auto& captures)->JSValue {
        const auto result = captures[0].call(self, args);
        if (!args.empty() && args[0].sameValue(captures[2])) {
            const auto state = std::static_pointer_cast<State>(captures[1].hostPayload("audit.DestroyRenderer"));
            ++state->deletes;
            state->renderer->reset();
            std::cout << "renderer destroyed within material delete\n" << std::flush;
            captures[1].collectGarbage();
        }
        return result;
    }, {original, token, nativeMaterial(material)}));
    material->dispose();
    check(!renderer && state->deletes == 1, "renderer destroy did not finish");
    material->dispose();
    check(state->deletes == 1, "destroyed renderer listener ran again");
}
void borrowLifetimes() {
    Fixture fixture;
    auto state = JSValue::object(); state.set("fail", false); state.set("escaped", nullptr);
    auto prototype = nativePlanePrototype(), original = prototype.get("applyMatrix4");
    Restore restore{prototype, original, "applyMatrix4"};
    prototype.set("applyMatrix4", JSValue::function(+[](const JSValue& self, const auto& args, const auto& captures)->JSValue {
        auto state = captures[0], view = args[0], elements = view.get("elements");
        state.set("escaped", view); state.set("elements", elements);
        const auto getter = elements.getOwnPropertyDescriptor("12")->getter.value();
        const auto setter = elements.getOwnPropertyDescriptor("12")->setter.value();
        state.set("getter", getter); state.set("setter", setter);
        auto coercion = JSValue::object();
        coercion.setPrimitiveConverter(JSValue::function(+[](const JSValue&, const auto& args, const auto& captures)->JSValue {
            check(args[0].strictEquals("number"), "matrix setter conversion hint");
            captures[0].collectGarbage();
            check(captures[1].get("12").isObject() == false, "matrix disappeared during setter coercion GC");
            return .125;
        }, {state, elements}));
        elements.set("12", coercion);
        check(elements.get("12").strictEquals(.125), "borrowed matrix setter did not survive coercion GC");
        state.collectGarbage();
        if (state.get("fail").toBoolean()) throw std::runtime_error("audit-project");
        return captures[1].call(self, args);
    }, {state, original}));
    auto expired = [&](auto&& action) { bool threw = false; try { action(); } catch (const JSTypeError&) { threw = true; } check(threw, "escaped camera borrow remained live"); };
    fixture.renderer.render(fixture.scene, fixture.camera);
    check(fixture.camera.matrixWorldInverse.elements()[12] == .125, "camera storage not mutated");
    expired([&] { state.get("escaped").get("elements"); });
    expired([&] { state.get("elements").get("12"); });
    expired([&] { state.get("getter").call(); });
    expired([&] { state.get("setter").call({}, {4}); });
    bool converted = false;
    struct Counter { bool* converted; }; auto counter = std::make_shared<Counter>(); counter->converted = &converted;
    auto marker = JSValue::hostObject(counter, counter.get(), "audit.Counter"), input = JSValue::object();
    input.setPrimitiveConverter(JSValue::function(+[](const JSValue&, const auto&, const auto& captures)->JSValue {
        *std::static_pointer_cast<Counter>(captures[0].hostPayload("audit.Counter"))->converted = true; return 4;
    }, {marker}));
    expired([&] { state.get("setter").call({}, {input}); }); check(!converted, "expired setter coerced user input");
    state.set("fail", true); bool threw = false;
    try { fixture.renderer.render(fixture.scene, fixture.camera); } catch (const std::runtime_error& e) { threw = std::string(e.what()) == "audit-project"; }
    check(threw, "projection exception was not propagated");
    expired([&] { state.get("elements").get("12"); });
    expired([&] { state.get("setter").call({}, {3}); });
    JSWeakValue weakView(state.get("escaped")), weakElements(state.get("elements")), weakGetter(state.get("getter")), weakSetter(state.get("setter"));
    for (const auto* key : {"escaped", "elements", "getter", "setter"}) state.set(key, nullptr);
    state.collectGarbage();
    check(weakView.expired() && weakElements.expired() && weakGetter.expired() && weakSetter.expired(), "retired borrow managed graph leaked");
}
void ownedCameraGraph() {
    Fixture fixture; auto state = JSValue::object(); state.set("escaped", nullptr);
    auto prototype = nativePlanePrototype(), original = prototype.get("applyMatrix4");
    Restore restore{prototype, original, "applyMatrix4"};
    prototype.set("applyMatrix4", JSValue::function(+[](const JSValue& self, const auto& args, const auto& captures)->JSValue {
        auto state = captures[0]; state.set("escaped", args[0]); state.collectGarbage();
        return captures[1].call(self, args);
    }, {state, original}));
    auto camera = std::make_shared<OrthographicCamera>(-1,1,1,-1,.1,20); camera->position.z = 5;
    auto native = nativeObject(camera); JSWeakValue weakNative(native); std::weak_ptr<Camera> weakCamera = camera;
    fixture.renderer.render(fixture.scene, *camera);
    check(state.get("escaped").sameValue(native.get("matrixWorldInverse")), "owned matrix identity differs");
    native = {}; camera.reset(); state.collectGarbage();
    check(!weakCamera.expired() && !weakNative.expired(), "escaped owned matrix lost its native camera");
    state.get("escaped").get("elements").set("12", 23);
    check(weakCamera.lock()->matrixWorldInverse.elements()[12] == 23, "escaped owned matrix no longer writes to owner");
    JSWeakValue weakMatrix(state.get("escaped")); state.set("escaped", nullptr); state.collectGarbage();
    check(weakMatrix.expired() && weakCamera.expired() && weakNative.expired(), "owned camera native/managed graph leaked");
}
void exceptionalCameraOwner() {
    Fixture fixture;
    auto camera = std::make_shared<OrthographicCamera>(-1,1,1,-1,.1,20); camera->position.z = 5;
    auto native = nativeObject(camera); JSWeakValue weakNative(native); std::weak_ptr<Camera> weakCamera = camera;
    fixture.scene.children.front()->onAfterRender=[](auto&,auto&,auto&,auto*,auto*,auto*) {
        throw std::runtime_error("audit-after-draw");
    };
    bool threw=false;
    try { fixture.renderer.render(fixture.scene,*camera); }
    catch(const std::runtime_error& error) { threw=std::string(error.what())=="audit-after-draw"; }
    check(threw,"after-draw exception not propagated");
    native={};camera.reset();fixture.renderer.info.collectGarbage();
    check(!weakNative.expired()&&!weakCamera.expired(),"exceptional render did not retain current managed camera identity");
    fixture.scene.children.front()->onAfterRender={};
    fixture.renderer.render(fixture.scene,fixture.camera);fixture.renderer.info.collectGarbage();
    check(weakNative.expired()&&weakCamera.expired(),"successful render did not release previous exceptional camera");
}

void reconstructedStackCamera() {
    Fixture fixture;
    alignas(OrthographicCamera) std::byte storage[sizeof(OrthographicCamera)];
    auto* camera=std::construct_at(reinterpret_cast<OrthographicCamera*>(storage),-1,1,1,-1,.1,20);
    struct Lifetime { OrthographicCamera*& camera; ~Lifetime(){if(camera)std::destroy_at(camera);} } lifetime{camera};
    camera->position.z=5;
    const auto firstId=camera->id;
    auto projected=JSValue::array();
    auto prototype=nativePlanePrototype(),original=prototype.get("applyMatrix4");
    Restore restore{prototype,original,"applyMatrix4"};
    prototype.set("applyMatrix4",JSValue::function(+[](const JSValue& self,const auto& args,const auto& captures)->JSValue {
        auto result=captures[0].call(self,args);
        auto values=captures[1];const auto index=values.get("length").toNumber();
        values.set(std::to_string(unsigned(index)),self.get("constant"));
        return result;
    },{original,projected}));
    fixture.scene.children.front()->onAfterRender=[](auto&,auto&,auto&,auto*,auto*,auto*) {
        throw std::runtime_error("audit-reconstructed-camera");
    };
    bool threw=false;
    try{fixture.renderer.render(fixture.scene,*camera);}
    catch(const std::runtime_error& error){threw=std::string(error.what())=="audit-reconstructed-camera";}
    check(threw,"first reconstructed-camera render did not throw");
    std::destroy_at(camera);camera=nullptr;
    camera=std::construct_at(reinterpret_cast<OrthographicCamera*>(storage),-1,1,1,-1,.1,20);
    camera->position.set(.5,0,5);
    check(camera->id!=firstId,"distinct camera reused its immutable native identity");
    fixture.scene.children.front()->onAfterRender={};
    fixture.renderer.render(fixture.scene,*camera);
    const auto rgba=fixture.renderer.readPixels();const auto offset=(16*32+8)*4;
    std::cout<<"{\"projected\":[";
    for(unsigned i=0;i<unsigned(projected.get("length").toNumber());++i){if(i)std::cout<<',';std::cout<<projected.get(std::to_string(i)).toNumber();}
    std::cout<<"],\"pixel\":["<<unsigned(rgba[offset])<<','<<unsigned(rgba[offset+1])<<','<<unsigned(rgba[offset+2])<<','<<unsigned(rgba[offset+3])<<"]}\n";
    check(projected.get("length").strictEquals(2),"reconstructed camera at the same address reused obsolete clipping projection");
    check(projected.get("0").strictEquals(0)&&projected.get("1").strictEquals(.5),"reconstructed camera projection differs from its new view transform");
    check(rgba[offset]==255&&rgba[offset+1]==255&&rgba[offset+2]==255&&rgba[offset+3]==255,"reconstructed camera inherited the previous camera's clipped pixels");
}

}
int main(int argc, char** argv) {
    try {
        const std::string mode = argc > 1 ? argv[1] : "all";
        if (mode == "all") { borrowLifetimes(); ownedCameraGraph(); disposeReentry(); disposeThrowBefore(); propertiesGetReentry(); disposeDestroyRenderer(); exceptionalCameraOwner(); reconstructedStackCamera(); }
        else if (mode == "dispose") disposeReentry();
        else if (mode == "throw") disposeThrowBefore();
        else if (mode == "get") propertiesGetReentry();
        else if (mode == "destroy") disposeDestroyRenderer();
        else if (mode == "owned") ownedCameraGraph();
        else if (mode == "exceptional-owner") exceptionalCameraOwner();
        else if (mode == "reconstructed") reconstructedStackCamera();
        else if (mode == "borrow") borrowLifetimes();
        else throw std::runtime_error("Unknown lifetime test mode");
        std::cout << mode << ": " << checks << " checks passed\n"; return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
