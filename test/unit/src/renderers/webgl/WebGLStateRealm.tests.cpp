#include "catch.hpp"
#include "WebGLStateFixture.h"
#include <thread>

TEST_CASE("WebGLState escaped methods retain their cache and release the application realm", "[webgl][state][runtime][gc]") {
    bool inactiveAtEntry=false, survived=false, callable=false, retired=false;
    std::thread worker([&] {
        inactiveAtEntry=!JSValue::hasActiveHeap();
        JSWeakValue ctorWeak,managerWeak,glWeak;
        JSValue escaped;
        {
            StateFixture::Fixture fixture(JSValue::object());
            ctorWeak=JSWeakValue(nativeWebGLStateConstructor());managerWeak=JSWeakValue(fixture.manager);glWeak=JSWeakValue(fixture.gl);
            escaped=fixture.manager.get("useProgram");
            escaped.call(nullptr,{1});
        }
        survived=!ctorWeak.expired()&&!managerWeak.expired()&&!glWeak.expired();
        callable=escaped.call({}, {1}).strictEquals(false)&&escaped.call({}, {2}).strictEquals(true);
        escaped={};JSValue::nativeSafepoint();
        retired=ctorWeak.expired()&&managerWeak.expired()&&glWeak.expired()&&!JSValue::hasActiveHeap();
    });worker.join();
    REQUIRE(inactiveAtEntry);REQUIRE(survived);REQUIRE(callable);REQUIRE(retired);
}

TEST_CASE("WebGLState constructor is stable within each realm and expires between realms", "[webgl][state][runtime][gc]") {
    bool stable=true,metadata=true,expired=true;
    std::thread worker([&] {
        for(unsigned cycle=0;cycle<4;++cycle) {
            JSWeakValue weak;
            {
                auto keeper=JSValue::object();auto constructor=nativeWebGLStateConstructor();weak=JSWeakValue(constructor);
                StateFixture::property(constructor,"realmMarker",cycle);
                const auto again=nativeWebGLStateConstructor();
                stable=stable&&again.sameValue(constructor)&&again.get("realmMarker").strictEquals(cycle);
                metadata=metadata&&again.get("name").strictEquals("WebGLState")&&again.get("length").strictEquals(2)&&again.isConstructible();
                keeper.collectGarbage();stable=stable&&!weak.expired();
            }
            JSValue::nativeSafepoint();expired=expired&&weak.expired()&&!JSValue::hasActiveHeap();
        }
    });worker.join();REQUIRE(stable);REQUIRE(metadata);REQUIRE(expired);
}

TEST_CASE("WebGLState rejects foreign runtime command handles before taking ownership", "[webgl][state][runtime][thread]") {
    auto keeper=JSValue::object();StateFixture::Fixture fixture(JSValue::object());
    bool constructorRejected=false,methodRejected=false;
    std::thread worker([&] {
        try { WebGLState state(fixture.gl,fixture.extensions); }catch(const JSTypeError&){constructorRejected=true;}
        try { fixture.manager.get("reset").call(fixture.manager); }catch(const JSTypeError&){methodRejected=true;}
    });worker.join();REQUIRE(constructorRejected);REQUIRE(methodRejected);
}

TEST_CASE("native math callback instance edges preserve the owner and retire after escape", "[webgl][state][runtime][gc]") {
    std::vector<bool> evidence;
    std::thread worker([&] {
        auto verify=[&](auto native, auto wrapper) {
            unsigned calls=0;native->_onChange([&]{++calls;});
            std::weak_ptr<void> owner=native;
            JSWeakValue weakObject,weakFunction;JSValue escaped;
            {
                auto object=wrapper(native);weakObject=JSWeakValue(object);
                escaped=object.get("_onChangeCallback");weakFunction=JSWeakValue(escaped);
                object.getPrototype().get("_onChangeCallback").call(object,{});
                evidence.push_back(calls==0&&object.hasOwn("_onChangeCallback"));
                native.reset();
            }
            JSValue::nativeSafepoint();
            evidence.push_back(!owner.expired()&&!weakObject.expired());
            escaped.call(nullptr,{});evidence.push_back(calls==1);
            escaped={};JSValue::nativeSafepoint();
            evidence.push_back(owner.expired()&&weakObject.expired()&&weakFunction.expired());
            evidence.push_back(!JSValue::hasActiveHeap());
        };
        verify(std::make_shared<Quaternion>(),[](auto q){return nativeQuaternion(q);});
        verify(std::make_shared<Euler>(),[](auto q){return nativeEuler(q);});
    });worker.join();
    REQUIRE(evidence.size()==10);
    for(bool value:evidence)REQUIRE(value);
}
