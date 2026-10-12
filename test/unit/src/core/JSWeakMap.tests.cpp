#include "catch.hpp"
#include "core/JSWeakMap.h"
#include <atomic>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <thread>

namespace {
using Args = std::vector<JSValue>;
JSValue noop(const JSValue&, const Args&, const Args&) { return {}; }
struct Owner {
    JSValue field;
    JSWeakValue resurrect;
    JSValue* saved = nullptr;
    inline static unsigned deaths = 0;
    ~Owner() { ++deaths; if (saved) { const auto key = resurrect.lock(); if (key) *saved = *key; } }
};
std::vector<JSNativeOwnerNode> ownership(const JSValue& self) {
    auto owner = std::static_pointer_cast<Owner>(self.hostPayload("weakmap.test.Owner"));
    return {{owner, {&owner->field}, {}}};
}
JSValue wrap(const std::shared_ptr<Owner>& owner) {
    auto result = JSValue::hostObject(owner, owner.get(), "weakmap.test.Owner");
    auto hooks = std::make_shared<JSHostObjectHooks>(); hooks->nativeOwnership = ownership;
    result.setHostHooks(hooks); return result;
}
}
TEST_CASE("Heap module roots retain their graph without owning the runtime", "[runtime][module][gc]") {
    bool retained=false, expired=false, fresh=false;
    std::exception_ptr error;
    std::thread worker([&] {
        try {
            JSWeakValue weakRoot, weakChild;
            {
                auto witness=JSValue::object();
                auto root=JSValue::object(), child=JSValue::object();
                root.set("child",child); child.set("parent",root);
                root.retainForHeapLifetime(); root.retainForHeapLifetime();
                weakRoot=JSWeakValue(root); weakChild=JSWeakValue(child);
                root={}; child={}; witness.collectGarbage();
                retained=!weakRoot.expired() && !weakChild.expired() &&
                    weakRoot.lock()->get("child").sameValue(*weakChild.lock());
            }
            expired=weakRoot.expired() && weakChild.expired();
            auto next=JSValue::object(); next.collectGarbage();
            fresh=weakRoot.expired() && weakChild.expired();
        } catch (...) { error=std::current_exception(); }
    });
    worker.join(); if(error)std::rethrow_exception(error);
    REQUIRE(retained); REQUIRE(expired); REQUIRE(fresh);
    for(const auto& primitive:Args{{},nullptr,false,1,"module"})
        REQUIRE_THROWS_AS(primitive.retainForHeapLifetime(),JSTypeError);
    const auto foreign=JSValue::object(); bool rejected=false;
    std::thread other([&] { try { foreign.retainForHeapLifetime(); } catch(const JSTypeError&) { rejected=true; } });
    other.join(); REQUIRE(rejected);
}
TEST_CASE("WeakMap global constructor survives collection within its original heap", "[runtime][weakmap][module][gc]") {
    bool retained=false, expired=false, reset=false;
    std::exception_ptr error;
    std::thread worker([&] {
        try {
            JSWeakValue weakConstructor,weakPrototype;
            {
                auto witness=JSValue::object();
                {
                    auto constructor=nativeWeakMapConstructor();
                    auto prototype=constructor.get("prototype"); prototype.set("moduleMarker",42);
                    weakConstructor=JSWeakValue(constructor); weakPrototype=JSWeakValue(prototype);
                }
                witness.collectGarbage(); auto constructor=nativeWeakMapConstructor();
                retained=!weakConstructor.expired() && constructor.sameValue(*weakConstructor.lock()) &&
                    constructor.get("prototype").sameValue(*weakPrototype.lock()) &&
                    constructor.construct().get("moduleMarker").strictEquals(42);
            }
            expired=weakConstructor.expired() && weakPrototype.expired();
            reset=nativeWeakMapConstructor().get("prototype").get("moduleMarker").isUndefined();
        } catch (...) { error=std::current_exception(); }
    });
    worker.join(); if(error)std::rethrow_exception(error);
    REQUIRE(retained); REQUIRE(expired); REQUIRE(reset);
}
TEST_CASE("WeakMap keys preserve identity and never coerce primitives or object keys", "[runtime][weakmap]") {
    JSWeakMap map; const Args keys{JSValue::object(), JSValue::array(), JSValue::function(noop)};
    for (std::size_t i = 0; i < keys.size(); ++i) {
        REQUIRE_FALSE(map.has(keys[i])); REQUIRE(map.set(keys[i], i).sameValue(map.object()));
        REQUIRE(map.has(keys[i])); REQUIRE(map.get(keys[i]).number() == i);
    }
    const Args primitives{{}, nullptr, false, true, -0.0, 1.0, std::numeric_limits<double>::quiet_NaN(), "key"};
    for (const auto& key : primitives) {
        REQUIRE_FALSE(map.has(key)); REQUIRE(map.get(key).isUndefined()); REQUIRE_FALSE(map.remove(key));
        REQUIRE_THROWS_AS(map.set(key, 1), JSTypeError);
    }
    auto key = JSValue::object(); key.setPrimitiveConverter(JSValue::function(+[](const JSValue&, const Args&, const Args&)->JSValue {
        throw std::runtime_error("Key was coerced");
    }));
    map.set(key, {}); REQUIRE(map.has(key)); REQUIRE(map.get(key).isUndefined()); REQUIRE(map.remove(key)); REQUIRE_FALSE(map.remove(key));
    REQUIRE_FALSE(map.has(JSValue::object())); REQUIRE_FALSE(map.has(JSValue::array()));
}
TEST_CASE("WeakMap methods validate their brand before considering an invalid key", "[runtime][weakmap]") {
    const auto prototype = nativeWeakMapConstructor().get("prototype");
    REQUIRE_FALSE(isNativeWeakMap(prototype)); REQUIRE_THROWS_AS(JSWeakMap(prototype), JSTypeError);
    for (const auto* name : {"get", "has", "delete", "set"}) {
        const auto method = prototype.get(name);
        for (const auto& receiver : Args{{}, nullptr, false, 1, "text", JSValue::object(), JSValue::array(), JSValue::function(noop)})
            REQUIRE_THROWS_AS(method.call(receiver, {nullptr}), JSTypeError);
        const auto descriptor = prototype.getOwnPropertyDescriptor(name);
        REQUIRE(descriptor->writable == true); REQUIRE(descriptor->configurable == true); REQUIRE(descriptor->enumerable == false);
    }
    REQUIRE_THROWS_AS(nativeWeakMapConstructor().call(), JSTypeError);
    REQUIRE(isNativeWeakMap(nativeWeakMapConstructor().construct({nullptr})));
    REQUIRE(isNativeWeakMap(nativeWeakMapConstructor().construct({JSValue{}})));
}
TEST_CASE("WeakMap values are retained only while map and key are reachable", "[runtime][weakmap][gc]") {
    auto witness = JSValue::object(); JSWeakMap map; auto key = JSValue::object(), value = JSValue::object();
    value.set("key", key); JSWeakValue weakKey(key), weakValue(value); map.set(key, value); value = {};
    witness.collectGarbage(); REQUIRE_FALSE(weakValue.expired()); REQUIRE(map.get(key).get("key").sameValue(key));
    key = {}; witness.collectGarbage(); REQUIRE(weakKey.expired()); REQUIRE(weakValue.expired());
    REQUIRE_FALSE(map.has(JSValue::object()));
}
TEST_CASE("Independently rooted keys cannot activate unreachable maps", "[runtime][weakmap][gc]") {
    auto witness = JSValue::object(), key = JSValue::object(); JSWeakValue weakMap, weakValue;
    {
        JSWeakMap map; auto value = JSValue::object(); value.set("map", map.object());
        weakMap = JSWeakValue(map.object()); weakValue = JSWeakValue(value); map.set(key, value);
    }
    witness.collectGarbage(); REQUIRE(weakMap.expired()); REQUIRE(weakValue.expired()); REQUIRE(key.isObject());
}
TEST_CASE("Unrooted map-key-value cycles and maps used as their own keys collect", "[runtime][weakmap][gc]") {
    auto witness = JSValue::object(); JSWeakValue weakMap, weakKey, weakValue;
    {
        JSWeakMap map; auto key = JSValue::object(), value = JSValue::object();
        key.set("map", map.object()); value.set("key", key); map.set(key, value);
        weakMap = JSWeakValue(map.object()); weakKey = JSWeakValue(key); weakValue = JSWeakValue(value);
    }
    witness.collectGarbage(); REQUIRE(weakMap.expired()); REQUIRE(weakKey.expired()); REQUIRE(weakValue.expired());
    {
        JSWeakMap map; auto value = JSValue::object(); map.set(map.object(), value);
        weakMap = JSWeakValue(map.object()); weakValue = JSWeakValue(value); value = {};
        witness.collectGarbage(); REQUIRE_FALSE(weakValue.expired());
    }
    witness.collectGarbage(); REQUIRE(weakMap.expired()); REQUIRE(weakValue.expired());
}
TEST_CASE("Ephemeron chains reach a fixed point across maps, keys, and newly discovered maps", "[runtime][weakmap][gc]") {
    auto witness = JSValue::object(); Args keys, maps;
    for (unsigned i = 0; i < 64; ++i) { keys.push_back(JSValue::object()); maps.push_back(createNativeWeakMap()); }
    auto last = JSValue::object(); JSWeakValue weakLast(last), weakMiddle(keys[31]);
    for (std::size_t i = 64; i-- > 0;) JSWeakMap(maps[i]).set(keys[i], i == 63 ? last : keys[i + 1]);
    auto root = keys[0]; keys.clear(); last = {};
    witness.collectGarbage(); REQUIRE_FALSE(weakMiddle.expired()); REQUIRE_FALSE(weakLast.expired());
    JSWeakMap(maps[0]).remove(root); witness.collectGarbage(); REQUIRE(weakMiddle.expired()); REQUIRE(weakLast.expired());
    JSWeakMap map; auto child = createNativeWeakMap(), value = JSValue::object(); JSWeakValue weakChild(child), weakValue(value);
    JSWeakMap(child).set(root, value); map.set(root, child); child = {}; value = {};
    witness.collectGarbage(); REQUIRE_FALSE(weakChild.expired()); REQUIRE_FALSE(weakValue.expired());
    map.remove(root); witness.collectGarbage(); REQUIRE(weakChild.expired()); REQUIRE(weakValue.expired());
}
TEST_CASE("WeakMap erase and replacement release old values without introducing roots", "[runtime][weakmap][gc]") {
    auto witness = JSValue::object(), key = JSValue::object(); JSWeakMap map;
    auto old = JSValue::object(), next = JSValue::object(); JSWeakValue weakOld(old), weakNext(next);
    map.set(key, old); old = {}; witness.collectGarbage(); REQUIRE_FALSE(weakOld.expired());
    map.set(key, next); next = {}; witness.collectGarbage(); REQUIRE(weakOld.expired()); REQUIRE_FALSE(weakNext.expired());
    REQUIRE(map.remove(key)); witness.collectGarbage(); REQUIRE(weakNext.expired());
    map.set(key, {}); REQUIRE(map.has(key)); REQUIRE(map.get(key).isUndefined());
}
TEST_CASE("Long ephemeron chains keep their tail and release it when the first conditional edge is removed", "[runtime][weakmap][gc]") {
    auto witness = JSValue::object(nullptr); JSWeakMap map;
    Args keys; keys.reserve(8193);
    for (unsigned i = 0; i <= 8192; ++i) keys.push_back(JSValue::object(nullptr));
    JSWeakValue middle(keys[4096]), last(keys.back());
    for (std::size_t i = 0; i < 8192; ++i) map.set(keys[i], keys[i + 1]);
    auto root = keys.front(); keys.clear();
    witness.collectGarbage(); REQUIRE_FALSE(middle.expired()); REQUIRE_FALSE(last.expired());
    REQUIRE(map.remove(root)); witness.collectGarbage(); REQUIRE(middle.expired()); REQUIRE(last.expired());
}
TEST_CASE("A key discovered through native ownership activates conditional values across several maps", "[runtime][weakmap][gc]") {
    auto witness = JSValue::object(); JSWeakMap first;
    auto owner = std::make_shared<Owner>(); owner->field = JSValue::object(nullptr);
    auto key = JSValue::object(nullptr); auto host = wrap(owner);
    Args maps; std::vector<JSWeakValue> values;
    for (unsigned i = 0; i < 32; ++i) {
        maps.push_back(JSWeakMap::create(nullptr)); auto value = JSValue::object(nullptr);
        value.set("back", host); values.emplace_back(value);
        JSWeakMap(maps.back()).set(owner->field, value);
    }
    first.set(key, host); host = {}; owner.reset();
    witness.collectGarbage(); for (const auto& value : values) REQUIRE_FALSE(value.expired());
    REQUIRE(first.remove(key)); witness.collectGarbage();
    for (const auto& value : values) REQUIRE(value.expired());
}
TEST_CASE("Native ownership and ephemeron marking participate in the same fixed point", "[runtime][weakmap][gc]") {
    auto witness = JSValue::object(); JSWeakMap first, second;
    auto owner = std::make_shared<Owner>(); owner->field = JSValue::object(); auto host = wrap(owner);
    auto key = JSValue::object(), value = JSValue::object(); JSWeakValue weakHost(host), weakValue(value), weakKey(owner->field);
    second.set(owner->field, value); first.set(key, host); host = {}; value = {}; owner.reset();
    witness.collectGarbage(); REQUIRE_FALSE(weakHost.expired()); REQUIRE_FALSE(weakKey.expired()); REQUIRE_FALSE(weakValue.expired());
    first.remove(key); witness.collectGarbage(); REQUIRE(weakHost.expired()); REQUIRE(weakKey.expired()); REQUIRE(weakValue.expired());
}
TEST_CASE("External native key owners retain renderer cache values until their ownership ends", "[runtime][weakmap][gc]") {
    auto witness = JSValue::object(); JSWeakMap map; auto owner = std::make_shared<Owner>();
    auto key = wrap(owner), value = JSValue::object(); value.set("back", key);
    JSWeakValue weakKey(key), weakValue(value); map.set(key, value); key = {}; value = {};
    witness.collectGarbage(); REQUIRE_FALSE(weakKey.expired()); REQUIRE_FALSE(weakValue.expired());
    REQUIRE(map.get(*weakKey.lock()).get("back").sameValue(*weakKey.lock()));
    owner.reset(); witness.collectGarbage(); REQUIRE(weakKey.expired()); REQUIRE(weakValue.expired());
    for (unsigned index = 0; index < 64; ++index) REQUIRE_FALSE(map.has(JSValue::object()));
}
TEST_CASE("Weak entries are pruned only after native finalizers and their re-mark", "[runtime][weakmap][gc]") {
    auto witness = JSValue::object(); JSWeakMap map; auto key = JSValue::object(); JSValue saved;
    auto owner = std::make_shared<Owner>(); owner->field = key; owner->resurrect = JSWeakValue(key); owner->saved = &saved;
    auto value = wrap(owner); JSWeakValue weakKey(key), weakValue(value); map.set(key, value);
    key = {}; value = {}; const auto deaths = Owner::deaths; owner.reset();
    witness.collectGarbage(); REQUIRE(Owner::deaths == deaths + 1); REQUIRE_FALSE(saved.isUndefined());
    REQUIRE_FALSE(weakKey.expired()); REQUIRE_FALSE(weakValue.expired()); REQUIRE(map.get(saved).sameValue(*weakValue.lock()));
    REQUIRE_FALSE(map.get(saved).isHostObject("weakmap.test.Owner"));
    saved = {}; witness.collectGarbage(); REQUIRE(weakKey.expired()); REQUIRE(weakValue.expired());
}
TEST_CASE("WeakMap does not retain a heap and old identities cannot alias replacement heaps", "[runtime][weakmap][gc]") {
    // A fresh native thread isolates this check from unrelated runtime globals
    // which intentionally keep their own intrinsic prototypes strongly rooted.
    bool deadBefore[3]{}, deadAfter[3]{};
    std::thread worker([&] {
        JSWeakValue weakMap, weakKey, weakValue;
        { JSWeakMap map; auto key = JSValue::object(), value = JSValue::object(); map.set(key, value);
          weakMap = JSWeakValue(map.object()); weakKey = JSWeakValue(key); weakValue = JSWeakValue(value); }
        deadBefore[0] = weakMap.expired(); deadBefore[1] = weakKey.expired(); deadBefore[2] = weakValue.expired();
        JSWeakMap replacement; auto key = JSValue::object(); replacement.set(key, 1);
        deadAfter[0] = weakMap.expired(); deadAfter[1] = weakKey.expired(); deadAfter[2] = weakValue.expired();
    }); worker.join();
    for (const auto dead : deadBefore) REQUIRE(dead);
    for (const auto dead : deadAfter) REQUIRE(dead);
}
TEST_CASE("WeakMap rejects foreign managed handles before reading or retaining their heap", "[runtime][weakmap][threads]") {
    JSWeakMap map; auto localKey = JSValue::object();
    struct Foreign { JSWeakMap map; JSValue handle = map.object(), key = JSValue::object(), value = JSValue::object(), primitive = "native"; };
    std::mutex mutex; std::condition_variable changed; Foreign* foreign = nullptr; bool done = false;
    std::thread worker([&] { Foreign values; std::unique_lock lock(mutex); foreign = &values; changed.notify_all(); changed.wait(lock, [&] { return done; }); });
    struct Join { std::thread& worker; std::mutex& mutex; std::condition_variable& changed; bool& done;
      ~Join() { { std::lock_guard lock(mutex); done = true; } changed.notify_all(); worker.join(); } } join{worker, mutex, changed, done};
    { std::unique_lock lock(mutex); changed.wait(lock, [&] { return foreign != nullptr; }); }
    REQUIRE_THROWS_AS(foreign->map.object(), JSTypeError);
    REQUIRE_THROWS_AS(JSWeakMap(foreign->handle), JSTypeError);
    REQUIRE_THROWS_AS(map.has(foreign->key), JSTypeError); REQUIRE_THROWS_AS(map.get(foreign->key), JSTypeError);
    REQUIRE_THROWS_AS(map.remove(foreign->key), JSTypeError); REQUIRE_THROWS_AS(map.set(foreign->key, 1), JSTypeError);
    REQUIRE_THROWS_AS(map.set(localKey, foreign->value), JSTypeError); REQUIRE_FALSE(map.has(localKey));
    map.set(localKey, foreign->primitive); REQUIRE(map.get(localKey).string() == "native");
}
