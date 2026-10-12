#include "core/JSValue.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
int assertions = 0, cases = 0;
void check(bool condition, const char* reason) {
    ++assertions;
    if (!condition) throw std::runtime_error(reason);
}
JSValue captured(const JSValue&, const std::vector<JSValue>&, const std::vector<JSValue>& captures) {
    return captures.empty() ? JSValue{} : captures.front();
}
struct Owner {
    JSValue field;
    inline static int destroyed = 0;
    ~Owner() { ++destroyed; }
};
std::vector<JSNativeOwnerNode> ownership(const JSValue& self) {
    const auto owner = std::static_pointer_cast<Owner>(self.hostPayload("WeakTestOwner"));
    return {{owner, {&owner->field}, {}}};
}
JSValue wrap(const std::shared_ptr<Owner>& owner, const void* identity = nullptr) {
    auto value = JSValue::hostObject(owner, identity ? identity : owner.get(), "WeakTestOwner");
    auto hooks = std::make_shared<JSHostObjectHooks>();
    hooks->nativeOwnership = ownership;
    value.setHostHooks(std::move(hooks));
    return value;
}
void primitives() {
    ++cases;
    const std::vector<JSValue> values{JSValue{}, nullptr, false, true, 0.0, -0.0,
        std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(), 3.5, "", "abc"};
    for (const auto& value : values) {
        JSWeakValue weak(value);
        check(weak.type() == value.type(), "primitive type lost");
        check(!weak.isObject(), "primitive classified as reference");
        check(!weak.expired(), "primitive must never expire");
        const auto locked = weak.lock();
        check(locked.has_value(), "primitive lock must have value, including undefined");
        check(locked->sameValue(value), "primitive SameValue differs");
    }
    JSWeakValue empty;
    check(!empty.expired() && empty.lock()->isUndefined(), "default weak undefined ambiguous");
}
void strongAndTypes() {
    ++cases;
    const std::vector<JSValue> values{JSValue::object(), JSValue::array(3), JSValue::function(captured)};
    for (const auto& value : values) {
        JSWeakValue weak(value);
        check(weak.isObject() && weak.type() == value.type(), "reference type lost");
        value.collectGarbage();
        check(!weak.expired(), "strong root failed to keep weak alive");
        auto locked = weak.lock();
        check(locked && locked->sameValue(value), "locked identity changed");
    }
}
void lockedRoot() {
    ++cases;
    auto witness = JSValue::object();
    auto source = JSValue::object();
    JSWeakValue weak(source);
    auto locked = weak.lock();
    source = JSValue{};
    witness.collectGarbage();
    check(!weak.expired(), "lock did not establish a root");
    locked.reset();
    witness.collectGarbage();
    check(weak.expired() && !weak.lock(), "released lock still kept record rooted");
}
void noWeakRootsAndNoReuse() {
    ++cases;
    auto witness = JSValue::object();
    witness.collectGarbage();
    const auto count = witness.heapObjectCount();
    auto source = JSValue::object();
    source.set("self", source);
    JSWeakValue weak(source), copy(weak);
    JSWeakValue moved(std::move(copy));
    std::vector<JSWeakValue> cache(128, weak);
    cache.push_back(std::move(moved));
    source = JSValue{};
    check(!weak.expired(), "record must exist until GC");
    check(witness.collectGarbage() == 1, "weak cache prevented cycle collection");
    check(witness.heapObjectCount() == count, "weak cache grew the live heap");
    for (const auto& entry : cache)
        check(entry.expired() && !entry.lock(), "weak copy/move retained a record");
    for (int i = 0; i < 256; ++i) {
        auto next = JSValue::object();
        check(!weak.lock(), "dead record identity was recycled");
    }
    witness.collectGarbage();
    check(weak.expired() && !weak.lock(), "old record resurrected after allocation");
}
void ordinaryGraph() {
    ++cases;
    auto root = JSValue::object();
    auto child = JSValue::array();
    JSWeakValue weak(child);
    root.set("child", child);
    child = JSValue{};
    root.collectGarbage();
    check(!weak.expired(), "reachable property edge failed");
    check(weak.lock()->sameValue(root.get("child")), "property edge identity changed");
    root.erase("child");
    root.collectGarbage();
    check(weak.expired() && !weak.lock(), "deleted graph edge retained record");
}
void functionGraph() {
    ++cases;
    auto witness = JSValue::object();
    auto child = JSValue::object();
    JSWeakValue weak(child);
    auto function = JSValue::function(captured, {child});
    child = JSValue{};
    witness.collectGarbage();
    check(!weak.expired(), "function capture edge failed");
    check(weak.lock()->sameValue(function.call()), "capture identity changed");
    function = JSValue{};
    witness.collectGarbage();
    check(weak.expired() && !weak.lock(), "collected function retained capture");
}
void externalNativeOwner() {
    ++cases;
    auto witness = JSValue::object();
    auto owner = std::make_shared<Owner>();
    owner->field = JSValue::object();
    JSWeakValue field(owner->field);
    auto value = wrap(owner);
    JSWeakValue weak(value);
    value = JSValue{};
    witness.collectGarbage();
    check(!weak.expired(), "external native owner failed to preserve cached wrapper");
    check(!field.expired(), "external native owner's JS field was collected");
    const auto deaths = Owner::destroyed;
    owner.reset();
    witness.collectGarbage();
    check(Owner::destroyed == deaths + 1, "native payload stayed alive in weak cache");
    check(weak.expired() && !weak.lock(), "native wrapper not collected");
    check(field.expired() && !field.lock(), "native field not collected");
}
void nativeCycle() {
    ++cases;
    auto witness = JSValue::object();
    auto owner = std::make_shared<Owner>();
    auto value = wrap(owner);
    owner->field = value;
    JSWeakValue weak(value);
    const auto deaths = Owner::destroyed;
    value = JSValue{};
    owner.reset();
    witness.collectGarbage();
    check(Owner::destroyed == deaths + 1, "weak handle prevented native/JS cycle GC");
    check(weak.expired() && !weak.lock(), "native/JS cycle weak still lockable");
}
void hostIdentityReuse() {
    ++cases;
    auto witness = JSValue::object();
    int nativeAddress;
    auto firstOwner = std::make_shared<Owner>();
    auto first = wrap(firstOwner, &nativeAddress);
    JSWeakValue old(first);
    firstOwner.reset();
    first = JSValue{};
    witness.collectGarbage();
    check(old.expired(), "retired host still live");
    auto secondOwner = std::make_shared<Owner>();
    auto second = wrap(secondOwner, &nativeAddress);
    check(!old.lock(), "native address reuse revived old weak reference");
    JSWeakValue current(second);
    check(current.lock()->sameValue(second), "new host weak identity incorrect");
}
void heapLifetime() {
    ++cases;
    JSWeakValue weak;
    std::thread creator([&] {
        auto source = JSValue::object();
        weak = JSWeakValue(source);
        source = JSValue{};
        check(weak.expired() && !weak.lock(), "weak handle retained its entire heap");
        auto replacementHeap = JSValue::object();
        check(weak.expired() && !weak.lock(), "new heap recycled an old weak identity");
    });
    creator.join();
}
void wrongThread() {
    ++cases;
    auto source = JSValue::object();
    JSWeakValue weak(source);
    JSWeakValue primitive(-0.0);
    int rejections = 0;
    bool metadata = false, primitiveWorks = false;
    std::thread reader([&] {
        try { JSWeakValue invalidWithoutLocalHeap(source); } catch (const JSTypeError&) { ++rejections; }
        auto otherHeap = JSValue::object();
        (void)otherHeap;
        metadata = weak.isObject() && weak.type() == JSValue::Type::Object;
        try { (void)weak.expired(); } catch (const JSTypeError&) { ++rejections; }
        try { (void)weak.lock(); } catch (const JSTypeError&) { ++rejections; }
        try { JSWeakValue invalid(source); } catch (const JSTypeError&) { ++rejections; }
        primitiveWorks = !primitive.expired() && std::signbit(primitive.lock()->number());
    });
    reader.join();
    check(rejections == 4, "foreign thread allowed heap access");
    check(metadata, "weak metadata unnecessarily depends on heap thread");
    check(primitiveWorks, "primitive transport unnecessarily thread-bound");
    check(!weak.expired() && weak.lock()->sameValue(source), "wrong-thread rejection damaged handle");
}
}
int main() {
    try {
        primitives(); strongAndTypes(); lockedRoot(); noWeakRootsAndNoReuse(); ordinaryGraph();
        functionGraph(); externalNativeOwner(); nativeCycle(); hostIdentityReuse(); heapLifetime(); wrongThread();
        std::cout << cases << " cases / " << assertions << " assertions passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAILED: " << error.what() << '\n';
        return 1;
    }
}
