#include "catch.hpp"
#include "loaders/FileLoader.h"
#include "loaders/ImageLoader.h"
#include "loaders/LoadingManager.h"
#include "loaders/BufferGeometryLoader.h"
#include <thread>

namespace {
using Constructor=JSValue(*)();
struct Entry { const char* name; Constructor constructor; };
const Entry entries[]={
    {"Loader",nativeLoaderConstructor},
    {"FileLoader",nativeFileLoaderConstructor},
    {"ImageLoader",nativeImageLoaderConstructor},
    {"LoadingManager",nativeLoadingManagerConstructor},
    {"BufferGeometryLoader",nativeBufferGeometryLoaderConstructor}
};
struct RestoreConstructor {
    JSValue prototype; JSPropertyDescriptor descriptor;
    ~RestoreConstructor(){prototype.defineProperty("constructor",descriptor);}
};
}

TEST_CASE("Loader import constructors survive public constructor replacement getters and deletion", "[loaders][native][constructor]") {
    for(const auto& entry:entries){
        INFO(entry.name);
        auto constructor=entry.constructor();auto prototype=constructor.get("prototype");
        RestoreConstructor restore{prototype,*prototype.getOwnPropertyDescriptor("constructor")};
        auto replacement=JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>&)->JSValue{
            throw JSTypeError("The replacement is not the imported constructor");
        });
        prototype.set("constructor",replacement);
        REQUIRE(entry.constructor().sameValue(constructor));
        REQUIRE(prototype.get("constructor").sameValue(replacement));
        REQUIRE(entry.constructor().construct().getPrototype().sameValue(prototype));
        auto state=JSValue::object();state.set("calls",0);
        JSPropertyDescriptor getter;getter.configurable=true;
        getter.getter=JSValue::function([](const JSValue&,const std::vector<JSValue>&,const std::vector<JSValue>& captures)->JSValue{
            auto state=captures[0];state.set("calls",state.get("calls").number()+1);
            throw JSTypeError("Import lookup must not read prototype.constructor");
        },{state});
        prototype.defineProperty("constructor",getter);
        REQUIRE(entry.constructor().sameValue(constructor));
        REQUIRE(entry.constructor().construct().getPrototype().sameValue(prototype));
        REQUIRE(state.get("calls").strictEquals(0));
        REQUIRE(prototype.erase("constructor"));
        REQUIRE(entry.constructor().sameValue(constructor));
        REQUIRE(entry.constructor().construct().getPrototype().sameValue(prototype));
    }
}

TEST_CASE("Stable loader constructor caches retire with their heap and initialize again", "[loaders][native][constructor][gc]") {
    std::vector<bool> evidence;
    std::thread worker([&]{
        for(unsigned epoch=0;epoch<2;++epoch){
            std::vector<JSWeakValue> weak;
            {
                // Initialize every derived class before mutating the base's
                // public constructor property in this fresh heap.
                std::vector<JSValue> constructors;
                for(const auto& entry:entries)constructors.push_back(entry.constructor());
                for(const auto& entry:entries){
                    auto constructor=entry.constructor();weak.emplace_back(constructor);
                    auto prototype=constructor.get("prototype");prototype.erase("constructor");
                    evidence.push_back(entry.constructor().sameValue(constructor));
                }
            }
            JSValue::nativeSafepoint();
            for(const auto& value:weak)evidence.push_back(value.expired());
            evidence.push_back(!JSValue::hasActiveHeap());
        }
    });worker.join();
    REQUIRE(evidence.size()==22);
    for(const auto value:evidence)REQUIRE(value);
}
