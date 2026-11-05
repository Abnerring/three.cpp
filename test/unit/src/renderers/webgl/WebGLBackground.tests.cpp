#include "catch.hpp"
#include "../../../utils/parity/WebGLBackgroundFixtures.h"
#include <exception>
#include <thread>

namespace {
using namespace WebGLBackgroundFixtures;
}
TEST_CASE("WebGLBackground exposes the pinned six argument factory and ordinary closure API", "[renderers][webgl][background]"){
    auto ctor=nativeWebGLBackgroundConstructor();
    REQUIRE(ctor.sameValue(nativeWebGLBackgroundConstructor()));
    REQUIRE(ctor.get("name").strictEquals("WebGLBackground"));
    REQUIRE(ctor.get("length").strictEquals(6));
    REQUIRE(ctor.isConstructible());
    REQUIRE(ctor.nativeFunctionSource().starts_with("function WebGLBackground( renderer, environments, state, objects, alpha, premultipliedAlpha )"));
    const auto value=ctor.call({},{});
    const std::vector<std::string> keys={"getClearColor","setClearColor","getClearAlpha","setClearAlpha","render","addToRenderList","dispose"};
    REQUIRE(value.keys()==keys);
    REQUIRE(call(value,"getClearColor").sameValue(call(value,"getClearColor")));
    REQUIRE(call(value,"getClearAlpha").strictEquals(1));
    for(const auto& key:keys){
        const auto descriptor=value.getOwnPropertyDescriptor(key);
        REQUIRE(descriptor->writable==true);
        REQUIRE(descriptor->enumerable==true);
        REQUIRE(descriptor->configurable==true);
        REQUIRE(value.get(key).isConstructible());
    }
}
TEST_CASE("WebGLBackground color forcing restores writable clear buffers in source order", "[renderers][webgl][background]"){
    auto request=object();request.set("autoClear",false);request.set("alpha",true);auto events=list();const auto c=context(request,events);
    WebGLBackground value(c.get("renderer"),c.get("environments"),c.get("state"),c.get("objects"),true,false);
    REQUIRE(value.getClearAlpha().strictEquals(0));
    REQUIRE(value.render(c.get("scene")).isUndefined());
    REQUIRE(events.length()==3);
    REQUIRE(events.get("0").get("0").strictEquals("target"));
    REQUIRE(events.get("1").get("0").strictEquals("setClear"));
    REQUIRE(events.get("2").get("0").strictEquals("xr"));
    auto scene=c.get("scene");scene.set("background",nativeColorConstructor().construct({0.1,0.4,0.7}));events.set("length",0);
    REQUIRE(value.render(scene).isUndefined());
    REQUIRE(events.length()==7);
    const std::vector<std::string> order={"target","setClear","xr","depth.setTest","depth.setMask","color.setMask","clear"};
    for(std::size_t i=0;i<order.size();++i)REQUIRE(events.get(JSValue(i)).get("0").strictEquals(order[i]));
    const auto original=value.getClearColor();
    REQUIRE(value.setClearColor("#ff0000",0.4).isUndefined());
    REQUIRE(value.getClearColor().sameValue(original));
    REQUIRE(value.getClearAlpha().strictEquals(0.4));
    REQUIRE(value.getClearColor().get("r").strictEquals(1));
    REQUIRE(value.setClearAlpha(0.8).isUndefined());
    REQUIRE(value.getClearAlpha().strictEquals(0.8));
}
TEST_CASE("WebGLBackground factory and cached real meshes release with their owning graph", "[renderers][webgl][background][gc]"){
    std::exception_ptr failure;
    std::thread worker([&]{try{
        REQUIRE_FALSE(JSValue::hasActiveHeap());
        JSWeakValue globalFactory;
        {
            auto keeper=object();globalFactory=JSWeakValue(nativeWebGLBackgroundConstructor());
            JSWeakValue contextWeak,factoryWeak,meshWeak;
            {
                auto events=list(),c=context(object(),events);contextWeak=JSWeakValue(c);factoryWeak=JSWeakValue(c.get("factory"));
                action(c,actionRecord("add",1));action(c,actionRecord("add",0));
                meshWeak=JSWeakValue(c.get("meshes").get("0"));
                REQUIRE(c.get("meshes").length()==2);
                REQUIRE(c.get("meshes").get("0").get("isMesh").strictEquals(true));
                REQUIRE(c.get("meshes").get("0").get("material").get("isShaderMaterial").strictEquals(true));
                call(c.get("factory"),"dispose");
            }
            keeper.collectGarbage();
            REQUIRE(contextWeak.expired());
            REQUIRE(factoryWeak.expired());
            REQUIRE(meshWeak.expired());
            REQUIRE_FALSE(globalFactory.expired());
        }
        REQUIRE(globalFactory.expired());
        REQUIRE_FALSE(JSValue::hasActiveHeap());
    }catch(...){failure=std::current_exception();}});
    worker.join();if(failure)std::rethrow_exception(failure);
}
