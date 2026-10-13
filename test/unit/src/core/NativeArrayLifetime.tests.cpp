#include "core/NativeArray.h"
#include <array>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

namespace {
unsigned checks = 0;
void check(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
void lifetime() {
    std::array<JSWeakValue, 9> oldConstructors, oldPrototypes;
    JSWeakValue oldBase, oldBufferConstructor, oldBufferPrototype, oldMarker;
    std::weak_ptr<int> nativeMarker;
    {
        auto anchor = JSValue::object();
        auto owner = std::make_shared<int>(42);
        nativeMarker = owner;
        auto marker = JSValue::hostObject(owner, owner.get(), "ArrayModuleLifetimeMarker");
        oldMarker = JSWeakValue(marker);
        for (std::size_t kind = 0; kind < 9; ++kind) {
            auto constructor = NativeArray_detail::constructor(kind);
            auto prototype = constructor.get("prototype");
            auto base = prototype.getPrototype();
            oldConstructors[kind] = JSWeakValue(constructor);
            oldPrototypes[kind] = JSWeakValue(prototype);
            constructor.set("lifetimeMarker", marker);
            prototype.set("lifetimeMarker", marker);
            base.set("lifetimeMarker", marker);
            if (kind == 0) oldBase = JSWeakValue(base);
            else check(oldBase.lock()->sameValue(base), "TypedArray base prototype was not shared");
            auto array = constructor.construct({3});
            array.set("0", 9);
            check(array.get("0").strictEquals(9), "Constructor no longer creates its native array");
            check(array.get("length").strictEquals(3), "Native array length changed");
            auto buffer = array.get("buffer"), bufferPrototype = buffer.getPrototype();
            auto bufferConstructor = bufferPrototype.get("constructor");
            if (kind == 0) {
                oldBufferConstructor = JSWeakValue(bufferConstructor);
                oldBufferPrototype = JSWeakValue(bufferPrototype);
                bufferConstructor.set("lifetimeMarker", marker);
                bufferPrototype.set("lifetimeMarker", marker);
            } else check(oldBufferConstructor.lock()->sameValue(bufferConstructor), "ArrayBuffer constructor was not shared");
        }
        marker = {};
        owner.reset();
        anchor.collectGarbage();
        check(!oldMarker.expired() && !nativeMarker.expired(), "Heap intrinsics lost their native marker");
        for (std::size_t kind = 0; kind < 9; ++kind) {
            auto constructor = NativeArray_detail::constructor(kind);
            check(oldConstructors[kind].lock()->sameValue(constructor), "Constructor identity changed after collection");
            check(constructor.get("lifetimeMarker").sameValue(*oldMarker.lock()), "Constructor mutation lost after collection");
            check(oldPrototypes[kind].lock()->get("lifetimeMarker").sameValue(*oldMarker.lock()), "Prototype mutation lost after collection");
        }
        check(oldBase.lock()->get("lifetimeMarker").sameValue(*oldMarker.lock()), "TypedArray base mutation lost");
        check(oldBufferConstructor.lock()->get("lifetimeMarker").sameValue(*oldMarker.lock()), "ArrayBuffer constructor mutation lost");
        check(oldBufferPrototype.lock()->get("lifetimeMarker").sameValue(*oldMarker.lock()), "ArrayBuffer prototype mutation lost");
    }
    // This assertion runs before the worker exits: TLS destruction must not be
    // the mechanism that eventually frees an otherwise dead runtime heap.
    for (std::size_t kind = 0; kind < 9; ++kind) {
        check(oldConstructors[kind].expired(), "Constructor cache retains a heap with no application roots");
        check(oldPrototypes[kind].expired(), "Prototype cache retains a heap with no application roots");
    }
    check(oldBase.expired(), "TypedArray base retains a dead heap");
    check(oldBufferConstructor.expired() && oldBufferPrototype.expired(), "ArrayBuffer cache retains a dead heap");
    check(oldMarker.expired() && nativeMarker.expired(), "Intrinsic native payload survived heap teardown");
    {
        auto anchor = JSValue::object();
        for (std::size_t kind = 0; kind < 9; ++kind) {
            auto constructor = NativeArray_detail::constructor(kind);
            check(constructor.get("lifetimeMarker").isUndefined(), "Fresh heap inherited an old constructor mutation");
            check(constructor.get("prototype").get("lifetimeMarker").isUndefined(), "Fresh heap inherited an old prototype mutation");
            check(oldConstructors[kind].expired(), "Old weak handle locked a new heap object");
            auto array = constructor.construct({2});
            array.set("1", 7);
            check(array.get("1").strictEquals(7), "Recreated constructor is unusable");
            check(array.get("buffer").get("constructor").get("lifetimeMarker").isUndefined(), "Fresh heap inherited ArrayBuffer mutation");
        }
    }
}
}
int main() {
    std::exception_ptr error;
    std::thread worker([&] { try { lifetime(); } catch (...) { error = std::current_exception(); } });
    worker.join();
    if (error) {
        try { std::rethrow_exception(error); }
        catch (const std::exception& e) { std::cerr << "FAILED after " << checks << " checks: " << e.what() << '\n'; }
        return 2;
    }
    std::cout << "NativeArray intrinsic lifetime: " << checks << " checks passed\n";
}
