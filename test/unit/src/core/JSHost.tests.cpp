#include "catch.hpp"
#include "core/JSValue.h"
#include "core/TypedArray.h"

namespace {
JSPropertyDescriptor hostData(JSValue value, bool writable = true, bool enumerable = true) {
    JSPropertyDescriptor descriptor;
    descriptor.value = std::move(value); descriptor.writable = writable;
    descriptor.enumerable = enumerable; descriptor.configurable = true;
    return descriptor;
}
struct HostNumbers { double first = 4, last = 8; };
std::shared_ptr<JSHostObjectHooks> numberHooks() {
    auto hooks = std::make_shared<JSHostObjectHooks>();
    hooks->getOwnProperty = [](const JSValue& self, const std::string& key) -> JSHostPropertyResult {
        auto numbers = std::static_pointer_cast<HostNumbers>(self.hostPayload("numbers"));
        if (key == "0") return {true, hostData(numbers->first)};
        if (key == "2") return {true, hostData(numbers->last)};
        if (key == "length") return {true, hostData(3, false, false)};
        return {};
    };
    hooks->defineOwnProperty = [](const JSValue& self, const std::string& key,
                                  const JSPropertyDescriptor& descriptor) -> std::optional<bool> {
        if (key != "0" && key != "2" && key != "length") return std::nullopt;
        if (key == "length" || descriptor.getter || descriptor.setter) return false;
        const auto numbers = std::static_pointer_cast<HostNumbers>(self.hostPayload("numbers"));
        if (descriptor.value) {
            const auto value = descriptor.value->toNumber();
            (key == "0" ? numbers->first : numbers->last) = value;
        }
        return true;
    };
    hooks->erase = [](const JSValue&, const std::string& key) -> std::optional<bool> {
        if (key == "0" || key == "2" || key == "length") return false;
        return std::nullopt;
    };
    hooks->ownPropertyNames = [](const JSValue&) { return std::vector<std::string>{"0", "2", "length"}; };
    return hooks;
}
}

TEST_CASE("Native host identity and slots retain a traced graph without rooting the cache", "[core][runtime][host]") {
    auto anchor = JSValue::object();
    anchor.collectGarbage();
    const auto initial = anchor.heapObjectCount();
    auto native = std::make_shared<int>(7);
    std::weak_ptr<int> lifetime = native;
    {
        auto first = JSValue::hostObject(native, native.get(), "number");
        auto second = JSValue::hostObject(native, native.get(), "number");
        REQUIRE(first.strictEquals(second));
        REQUIRE(first.isHostObject("number"));
        REQUIRE_FALSE(first.isHostObject("other"));
        REQUIRE_THROWS_AS(first.hostPayload("other"), JSTypeError);
        auto edge = JSValue::object();
        edge.set("back", first);
        first.setHostSlot("edge", edge);
        native.reset();
        REQUIRE_FALSE(lifetime.expired());
        REQUIRE(first.keys().empty());
        REQUIRE(first.toJSON()->dump() == "{}");
        REQUIRE(anchor.collectGarbage() == 0);
        REQUIRE(second.getHostSlot("edge").strictEquals(edge));
        anchor.set("saved", second);
    }
    REQUIRE(anchor.collectGarbage() == 0);
    anchor.erase("saved");
    REQUIRE(anchor.collectGarbage() == 2);
    REQUIRE(lifetime.expired());
    REQUIRE(anchor.heapObjectCount() == initial);
}

TEST_CASE("Native indexed properties participate in ordinary access reflection and JSON", "[core][runtime][host]") {
    auto native = std::make_shared<HostNumbers>();
    auto array = JSValue::hostObject(native, native.get(), "numbers", JSValue::Type::Array);
    array.setHostHooks(numberHooks());
    array.set("tag", "native");
    array.set("1", 6);
    REQUIRE(array.length() == 3);
    REQUIRE(array.get("0").number() == 4);
    REQUIRE(array.toJSON()->dump() == "[4,6,8]");
    REQUIRE(array.ownPropertyNames() == std::vector<std::string>{"0", "1", "2", "length", "tag"});
    REQUIRE(array.keys() == std::vector<std::string>{"0", "1", "2", "tag"});
    array.set("2", 10);
    REQUIRE(native->last == 10);
    auto child = JSValue::object(array);
    child.set("0", 9);
    REQUIRE(child.get("0").number() == 9);
    REQUIRE(native->first == 4);
    REQUIRE_FALSE(array.erase("0"));
    REQUIRE_THROWS_AS(array.set("length", 1), JSTypeError);
    REQUIRE(array.erase("tag"));
    REQUIRE_FALSE(array.hasOwn("tag"));
}

TEST_CASE("Native hooks retain the original receiver through detachment and collection", "[core][runtime][host]") {
    struct Detacher { JSValue* exposed; };
    JSValue exposed;
    auto native = std::make_shared<Detacher>(Detacher{&exposed});
    auto hooks = std::make_shared<JSHostObjectHooks>();
    hooks->getOwnProperty = [](const JSValue& self, const std::string& key) -> JSHostPropertyResult {
        if (key != "x") return {};
        const auto native = std::static_pointer_cast<Detacher>(self.hostPayload("detach"));
        *native->exposed = JSValue{};
        self.collectGarbage();
        auto descriptor = JSPropertyDescriptor{};
        descriptor.getter = self.getHostSlot("getter"); descriptor.setter = JSValue{};
        descriptor.enumerable = true; descriptor.configurable = true;
        return {true, descriptor};
    };
    exposed = JSValue::hostObject(native, native.get(), "detach");
    exposed.setHostHooks(hooks);
    exposed.setHostSlot("getter", JSValue::function([](const JSValue& receiver,
        const std::vector<JSValue>&, const std::vector<JSValue>&) -> JSValue { return receiver.isHostObject("detach"); }));
    REQUIRE(exposed.get("x").strictEquals(true));
    REQUIRE(exposed.isUndefined());
}

TEST_CASE("Native payload finalizers may allocate and request collection", "[core][runtime][host]") {
    auto anchor = JSValue::object();
    struct Finalizer {
        JSValue* anchor;
        ~Finalizer() {
            auto replacement = JSValue::object();
            replacement.set("alive", true);
            anchor->set("replacement", replacement);
            anchor->collectGarbage();
        }
    };
    auto native = std::make_shared<Finalizer>(); native->anchor = &anchor;
    { auto host = JSValue::hostObject(native, native.get(), "finalizer"); }
    native.reset();
    REQUIRE(anchor.collectGarbage() >= 1);
    REQUIRE(anchor.get("replacement").get("alive").strictEquals(true));
    REQUIRE(anchor.collectGarbage() == 0);
}

TEST_CASE("Native hooks delegate using the retained object after the caller is reset", "[core][runtime][host]") {
    struct Detacher { JSValue* exposed; };
    JSValue exposed;
    auto native = std::make_shared<Detacher>(Detacher{&exposed});
    auto hooks = std::make_shared<JSHostObjectHooks>();
    hooks->getOwnProperty = [](const JSValue& self, const std::string&) -> JSHostPropertyResult {
        *std::static_pointer_cast<Detacher>(self.hostPayload("fallback"))->exposed = {};
        self.collectGarbage(); return {};
    };
    hooks->erase = [](const JSValue& self, const std::string&) -> std::optional<bool> {
        *std::static_pointer_cast<Detacher>(self.hostPayload("fallback"))->exposed = {};
        self.collectGarbage(); return {};
    };
    hooks->ownPropertyNames = [](const JSValue& self) {
        *std::static_pointer_cast<Detacher>(self.hostPayload("fallback"))->exposed = {};
        self.collectGarbage(); return std::vector<std::string>{};
    };
    auto retained = JSValue::hostObject(native, native.get(), "fallback");
    retained.set("x", 7);
    retained.setHostHooks(hooks);
    exposed = retained;
    REQUIRE(exposed.getOwnPropertyDescriptor("x")->value->number() == 7);
    REQUIRE(exposed.isUndefined());
    exposed = retained;
    REQUIRE(exposed.ownPropertyNames() == std::vector<std::string>{"x"});
    REQUIRE(exposed.isUndefined());
    exposed = retained;
    REQUIRE(exposed.erase("x"));
    REQUIRE(exposed.isUndefined());
    REQUIRE_FALSE(retained.hasOwn("x"));
}

TEST_CASE("Typed array native identity distinguishes independent views", "[core][runtime][host]") {
    Float32Array first{1.f, 2.f};
    const auto alias = first;
    const auto view = first.subarray();
    REQUIRE(first.identity() == alias.identity());
    REQUIRE(first.identity() != view.identity());
    REQUIRE(first.buffer.sameIdentity(view.buffer));
}

TEST_CASE("Native graph tracing retains referenced wrappers and releases replaced fields", "[core][runtime][host]") {
    struct Tree { std::shared_ptr<Tree> child; };
    auto native = std::make_shared<Tree>(); native->child = std::make_shared<Tree>();
    auto root = JSValue::hostObject(native, native.get(), "tree");
    root.collectGarbage();
    auto hooks = std::make_shared<JSHostObjectHooks>();
    hooks->trace = [](const JSValue& self) {
        auto tree = std::static_pointer_cast<Tree>(self.hostPayload("tree"));
        return std::vector<JSValue>{self.findHostObject(tree->child.get(), "tree")};
    };
    root.setHostHooks(hooks);
    const void* childIdentity = native->child.get();
    const std::weak_ptr<Tree> childLifetime = native->child;
    {
        auto child = JSValue::hostObject(native->child, childIdentity, "tree");
        child.set("custom", 42);
    }
    REQUIRE(root.collectGarbage() == 0);
    REQUIRE(root.findHostObject(childIdentity, "tree").get("custom").number() == 42);
    REQUIRE(root.keys().empty());
    native->child.reset();
    REQUIRE(root.collectGarbage() == 1);
    REQUIRE(childLifetime.expired());
    REQUIRE(root.findHostObject(childIdentity, "tree").isUndefined());
}
