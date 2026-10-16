#include "catch.hpp"
#include "extras/TextureUtils.h"
#include <exception>
#include <thread>

namespace {
void onFreshThread(void (*body)()) {
    std::exception_ptr failure;
    std::thread thread([&]{try{body();}catch(...){failure=std::current_exception();}});
    thread.join();if(failure)std::rethrow_exception(failure);
}
JSValue texture() {
    auto value=JSValue::object(),image=JSValue::object(),repeat=JSValue::object(),offset=JSValue::object();
    image.set("width",4);image.set("height",2);repeat.set("x",1);repeat.set("y",1);offset.set("x",0);offset.set("y",0);
    value.set("image",image);value.set("repeat",repeat);value.set("offset",offset);return value;
}
}
TEST_CASE("TextureUtils module identities and mutations survive GC within a live realm") {
    onFreshThread([]{
        auto keeper=JSValue::object();auto ctor=nativeTextureUtils();JSWeakValue weak(ctor);
        ctor.set("marker",42);ctor.get("prototype").set("marker",19);
        auto original=nativeTextureContain();JSWeakValue originalWeak(original);
        ctor.set("contain",17);ctor={};original={};keeper.collectGarbage();
        REQUIRE(weak.lock());REQUIRE(originalWeak.lock());
        REQUIRE(nativeTextureUtils().get("marker").strictEquals(42));
        REQUIRE(TextureUtils::prototype().get("marker").strictEquals(19));
        REQUIRE(nativeTextureUtils().get("contain").strictEquals(17));
        auto input=texture();REQUIRE(nativeTextureContain().call({}, {input,1}).sameValue(input));
        REQUIRE(input.get("repeat").get("y").strictEquals(2));
        REQUIRE(TextureUtils::contain(input,4).sameValue(input));
        REQUIRE(input.get("repeat").get("x").strictEquals(2));
        const TextureUtils instance;REQUIRE(JSValue(instance).getPrototype().sameValue(TextureUtils::prototype()));
    });
}
TEST_CASE("TextureUtils module roots release with the last realm root and a later realm resets exports") {
    onFreshThread([]{
        REQUIRE_FALSE(JSValue::hasActiveHeap());JSWeakValue ctor,method;
        {
            auto keeper=JSValue::object();auto value=nativeTextureUtils();value.set("marker",42);
            ctor=JSWeakValue(value);method=JSWeakValue(nativeTextureContain());
            REQUIRE(ctor.lock());REQUIRE(method.lock());
        }
        REQUIRE_FALSE(ctor.lock());REQUIRE_FALSE(method.lock());REQUIRE_FALSE(JSValue::hasActiveHeap());
        auto keeper=JSValue::object();REQUIRE(nativeTextureUtils().get("marker").isUndefined());
    });
}
