#pragma once

#include "core/JsonValue.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

class JSHeap;
class JSWeakValue;
class JSWeakMap;
class JSMap;
class JSSet;
class JSIterator;
class JSWeakRef;
class JSFinalizationRegistry;
struct JSPropertyDescriptor;
struct JSHostObjectHooks;
struct JSHostPropertyResult;
class JSTypeError : public std::invalid_argument {
  public:
    using std::invalid_argument::invalid_argument;
};
class JSRangeError : public std::out_of_range {
  public:
    using std::out_of_range::out_of_range;
};

// Runtime values are distinct from JSON. Object assignment preserves identity;
// missing properties and sparse array slots return undefined without insertion.
class JSValue {
  public:
    struct Undefined { bool operator==(const Undefined&) const = default; };
    enum class Type { Undefined, Null, Boolean, Number, String, Object, Array, Function, Symbol };
    enum class PreferredType { Default, Number, String };
    enum class ConstructorKind { Class, Callable };
    using PropertyDescriptor = JSPropertyDescriptor;
    // Captures are explicit managed graph edges. A function pointer also accepts
    // noncapturing lambdas, without hiding owning JSValue handles in a closure.
    using NativeFunction = JSValue (*)(const JSValue& receiver, const std::vector<JSValue>& args,
                                      const std::vector<JSValue>& captures);
    // A callable intrinsic gets the original call receiver and undefined
    // newTarget for [[Call]], or undefined receiver and the real newTarget for
    // [[Construct]]. Captures remain explicit, heap-traced edges.
    using NativeBuiltinFunction = JSValue (*)(const JSValue& receiver, const JSValue& newTarget,
                                             const std::vector<JSValue>& args,
                                             const std::vector<JSValue>& captures);
    JSValue() = default;
    JSValue(std::nullptr_t) : value_(nullptr) {}
    JSValue(bool value) : value_(value) {}
    template <class T,
              std::enable_if_t<std::is_arithmetic_v<T> && !std::is_same_v<T, bool>, int> = 0>
    JSValue(T value) : value_(static_cast<double>(value)) {}
    JSValue(const char* value) : value_(normalizeString(std::string(value))) {}
    JSValue(std::string value) : value_(normalizeString(value)) {}
    JSValue(const JSValue& other);
    JSValue(JSValue&& other) noexcept;
    JSValue& operator=(const JSValue& other);
    JSValue& operator=(JSValue&& other) noexcept;
    ~JSValue();

    // Stable intrinsic binding, independent of mutable Object.prototype.constructor.
    static JSValue intrinsicObjectConstructor();
    static JSValue intrinsicArrayConstructor();
    // Non-allocating test for an existing native realm keeper.
    static bool hasActiveHeap();
    static JSValue object();
    static JSValue object(const JSValue& prototype);
    static JSValue array(std::uint32_t length = 0);
    static JSValue symbol(const std::optional<std::string>& description = std::nullopt);
    static JSValue wellKnownSymbol(const std::string& name);
    std::optional<std::string> symbolDescription() const;
    JSValue toPropertyKey() const;
    // Native hosts share the normal property/prototype graph. The cache is weak:
    // only reachable JS handles/graph edges retain a host record and its payload.
    // Store managed edges in host slots, or explicitly describe native JSValue
    // fields and shared_ptr links through nativeOwnership; unreported fields
    // are ordinary external roots and cannot participate in cycle collection.
    static JSValue hostObject(std::shared_ptr<void> payload, const void* identity,
                              const std::string& tag, Type type = Type::Object);
    bool isHostObject(const std::string& tag) const;
    std::shared_ptr<void> hostPayload(const std::string& expectedTag) const;
    void setHostHooks(std::shared_ptr<const JSHostObjectHooks> hooks);
    void setHostSlot(const std::string& key, const JSValue& value);
    JSValue getHostSlot(const std::string& key) const;
    JSValue findHostObject(const void* identity, const std::string& tag,
                           Type type = Type::Object) const;
    static JSValue function(NativeFunction callback, const std::vector<JSValue>& captures = {},
                            const std::string& name = "", std::uint32_t arity = 0);
    // Construction is explicit. Class constructors reject ordinary calls;
    // Callable constructors also support strict calls through NativeFunction.
    // The callback receives a fresh object inheriting from newTarget.prototype.
    static JSValue constructor(NativeFunction callback, const std::vector<JSValue>& captures = {},
                               const std::string& name = "", std::size_t arity = 0,
                               ConstructorKind kind = ConstructorKind::Class);
    // Intrinsic constructors perform argument checks and allocation themselves.
    // The callback receives newTarget as its receiver and must return an object.
    static JSValue builtinConstructor(NativeFunction callback, const std::vector<JSValue>& captures = {},
                                      const std::string& name = "", std::size_t arity = 0);
    static JSValue callableBuiltinConstructor(NativeBuiltinFunction callback,
                                             const std::vector<JSValue>& captures = {},
                                             const std::string& name = "", std::size_t arity = 0);
    static JSValue fromJSON(const JsonValue& value);
    Type type() const;
    bool isUndefined() const { return type() == Type::Undefined; }
    bool isNull() const { return type() == Type::Null; }
    bool isSymbol() const { return type() == Type::Symbol; }
    bool isObject() const { return type() == Type::Object || type() == Type::Array || isCallable(); }
    bool isCallable() const { return type() == Type::Function; }
    bool isConstructible() const;
    bool strictEquals(const JSValue& other) const;
    bool sameValue(const JSValue& other) const;
    bool toBoolean() const;
    JSValue toPrimitive(PreferredType hint = PreferredType::Default) const;
    double toNumber() const;
    std::string toString() const;
    JSValue call() const;
    JSValue call(const JSValue& receiver, const std::vector<JSValue>& args = {}) const;
    JSValue bind(const JSValue& receiver, const std::vector<JSValue>& args = {}) const;
    JSValue construct(const std::vector<JSValue>& args = {}) const;
    JSValue construct(const std::vector<JSValue>& args, const JSValue& newTarget) const;
    std::string nativeFunctionSource() const;
    // Exact known source text for transplanted source functions, stored in the
    // function record; Function.prototype.toString stays a live prototype method.
    void setFunctionSourceText(const std::string& source);
    double number() const { return std::get<double>(value_); }
    const std::string& string() const { return std::get<std::string>(value_); }

    JSValue get(const std::string& key) const;
    JSValue get(const std::string& key, const JSValue& receiver) const;
    void set(const std::string& key, const JSValue& value);
    bool reflectSet(const std::string& key, const JSValue& value, const JSValue& receiver) const;
    bool hasOwn(const std::string& key) const;
    bool has(const std::string& key) const;
    bool erase(const std::string& key);
    std::vector<std::string> keys() const;
    std::vector<std::string> ownPropertyNames() const;
    std::optional<PropertyDescriptor> getOwnPropertyDescriptor(const std::string& key) const;
    bool defineOwnProperty(const std::string& key, const PropertyDescriptor& descriptor);
    void defineProperty(const std::string& key, const PropertyDescriptor& descriptor);
    // Real Symbol keys share ordinary descriptor/prototype semantics. String
    // overloads remain the native host hook boundary and retain source compatibility.
    JSValue get(const JSValue& key) const;
    JSValue get(const JSValue& key, const JSValue& receiver) const;
    void set(const JSValue& key, const JSValue& value);
    bool reflectSet(const JSValue& key, const JSValue& value, const JSValue& receiver) const;
    bool hasOwn(const JSValue& key) const;
    bool has(const JSValue& key) const;
    bool erase(const JSValue& key);
    std::optional<PropertyDescriptor> getOwnPropertyDescriptor(const JSValue& key) const;
    bool defineOwnProperty(const JSValue& key, const PropertyDescriptor& descriptor);
    void defineProperty(const JSValue& key, const PropertyDescriptor& descriptor);
    std::vector<JSValue> ownPropertySymbols() const;
    std::vector<JSValue> ownPropertyKeys() const;
    JSValue get(const char* key) const { return get(std::string(key)); }
    JSValue get(const char* key, const JSValue& receiver) const { return get(std::string(key),receiver); }
    void set(const char* key, const JSValue& value) { set(std::string(key),value); }
    bool reflectSet(const char* key, const JSValue& value, const JSValue& receiver) const { return reflectSet(std::string(key),value,receiver); }
    bool hasOwn(const char* key) const { return hasOwn(std::string(key)); }
    bool has(const char* key) const { return has(std::string(key)); }
    bool erase(const char* key) { return erase(std::string(key)); }
    std::optional<PropertyDescriptor> getOwnPropertyDescriptor(const char* key) const;
    bool defineOwnProperty(const char* key, const PropertyDescriptor& descriptor) { return defineOwnProperty(std::string(key),descriptor); }
    void defineProperty(const char* key, const PropertyDescriptor& descriptor) { defineProperty(std::string(key),descriptor); }
    JSValue getPrototype() const;
    bool setPrototype(const JSValue& prototype);
    void preventExtensions();
    bool isExtensible() const;
    // Convenience adapter for an own real Symbol.toPrimitive data property.
    void setPrimitiveConverter(const JSValue& converter);
    std::uint32_t length() const;

    // JSON.stringify rules for supported data values: undefined object properties
    // are omitted, array holes/undefined and nonfinite numbers become null.
    // A top-level undefined has no JSON result. Cycles throw.
    std::optional<JsonValue> toJSON() const;
    JsonValue trace() const;

    // Collect unreachable object/array cycles in this value's heap. Live C++
    // JSValue handles are roots; properties are managed, non-owning graph edges.
    std::size_t collectGarbage() const;
    std::size_t heapObjectCount() const;
    // Keep a native module/global object and its graph alive while this heap
    // exists. This root adds no shared ownership of the heap. Native payloads
    // can still contain owning handles and require their own lifetime policy.
    void retainForHeapLifetime() const;
    // Owning-thread native host checkpoint. Retires per-heap intrinsic graphs
    // only when no application JS handles, kept job targets, external native
    // owners or unreclaimable C++ ownership cycles remain. shared_ptr reset
    // itself is unobservable; weak-cache access/new heap use also check here.
    static std::size_t nativeSafepoint();

  private:
    struct Reference { std::uint64_t id; Type type; };
    using Storage = std::variant<Undefined, std::nullptr_t, bool, double, std::string, Reference>;
    Storage value_ = Undefined{};
    std::shared_ptr<JSHeap> owner_;
    JSHeap* heap_ = nullptr;
    JSValue(std::shared_ptr<JSHeap> heap, std::uint64_t id, Type type);
    JSValue graphEdge() const;
    static std::string normalizeString(const std::string& value);
    void retain();
    void release();
    // Heap-owned ephemeron storage. Only the native WeakMap adapter may create
    // or access these internal slots; entries are never ordinary strong roots.
    static JSValue createCollectionData(const JSValue& prototype,unsigned brand);
    bool hasCollectionData(unsigned brand) const;
    std::size_t collectionSize(unsigned brand) const;
    bool collectionHas(const JSValue& key,unsigned brand) const;
    JSValue collectionGet(const JSValue& key,unsigned brand) const;
    void collectionSet(const JSValue& key,const JSValue& value,unsigned brand);
    bool collectionErase(const JSValue& key,unsigned brand);
    void collectionClear(unsigned brand);
    void collectionForEach(const JSValue& callback,const JSValue& thisArg,unsigned brand) const;
    static JSValue createIteratorData(const JSValue& prototype,const JSValue& receiver,unsigned brand,unsigned kind);
    JSValue iteratorNext(unsigned brand) const;
    static JSValue createWeakMapData(const JSValue& prototype);
    bool hasWeakMapData() const;
    JSValue weakMapGet(const JSValue& key) const;
    bool weakMapHas(const JSValue& key) const;
    bool weakMapErase(const JSValue& key);
    void weakMapSet(const JSValue& key, const JSValue& value);
    static JSValue createWeakRefData(const JSValue& prototype, const JSValue& target);
    bool hasWeakRefData() const;
    JSValue weakRefDeref() const;
    static JSValue createFinalizationRegistryData(const JSValue& prototype, const JSValue& callback);
    bool hasFinalizationRegistryData() const;
    void finalizationRegistryRegister(const JSValue& target, const JSValue& heldValue,
                                      const JSValue& token);
    bool finalizationRegistryUnregister(const JSValue& token);
    void finalizationRegistryCleanup();
    static void clearKeptObjects();
    friend class JSHeap;
    friend class JSWeakValue;
    friend class JSWeakMap;
    friend class JSMap;
    friend class JSSet;
    friend class JSIterator;
    friend class JSWeakRef;
    friend class JSFinalizationRegistry;
    friend void endJSJob();
};

// Cache-only runtime handle. Reference values retain neither their heap nor
// their record and never contribute a GC root. Primitive values remain values,
// so lock(undefined) succeeds while locking a collected reference is empty.
// Reference access is confined to the thread that created the owning heap.
class JSWeakValue {
  public:
    JSWeakValue() = default;
    explicit JSWeakValue(const JSValue& value);
    JSValue::Type type() const;
    bool isObject() const;
    bool expired() const;
    std::optional<JSValue> lock() const;

  private:
    JSValue::Storage value_ = JSValue::Undefined{};
    std::weak_ptr<JSHeap> heap_;
    std::uint64_t threadToken_ = 0;
    void checkThread() const;
};

// Missing fields mean "leave unchanged"; a present undefined getter/setter
// means an accessor with no callback. Newly defined attributes default false.
struct JSPropertyDescriptor {
    std::optional<JSValue> value;
    std::optional<bool> writable;
    std::optional<JSValue> getter;
    std::optional<JSValue> setter;
    std::optional<bool> enumerable;
    std::optional<bool> configurable;
};

// Returning unhandled/nullopt delegates to ordinary JS object semantics. A
// handled absent descriptor represents a native index that does not exist.
struct JSHostPropertyResult {
    bool handled = false;
    std::optional<JSPropertyDescriptor> descriptor;
};
// Native references are identified by their shared_ptr control block. Slots are
// actual shared_ptr field addresses, so overlapping host graphs can be deduplicated.
struct JSNativeOwnerEdge {
    const void* slot;
    std::weak_ptr<void> owner;
};
struct JSNativeOwnerNode {
    // Roots and slots must name valid storage when the description is returned.
    // Roots are copied as non-owning edges before calling the next hook; later
    // changes to field storage are re-audited before sweeping.
    std::weak_ptr<void> owner;
    std::vector<const JSValue*> roots;
    std::vector<JSNativeOwnerEdge> edges;
};
struct JSHostObjectHooks {
    JSHostPropertyResult (*getOwnProperty)(const JSValue&, const std::string&) = nullptr;
    std::optional<bool> (*defineOwnProperty)(const JSValue&, const std::string&,
                                           const JSPropertyDescriptor&) = nullptr;
    std::optional<bool> (*erase)(const JSValue&, const std::string&) = nullptr;
    std::vector<std::string> (*ownPropertyNames)(const JSValue&) = nullptr;
    std::optional<bool> (*reflectSet)(const JSValue&, const std::string&,
                                    const JSValue&, const JSValue&) = nullptr;
    // Enumerate cached wrappers referenced by current native fields. This must
    // not invoke user getters or allocate new host wrappers.
    // Graph changes are re-audited before sweeping and must eventually settle.
    std::vector<JSValue> (*trace)(const JSValue&) = nullptr;
    // Describe existing native storage without invoking getters, copying JSValue
    // fields, retaining extra shared_ptr owners, or mutating the graph.
    // Pure shared_ptr cycles and their JS fields remain alive until those native
    // cycles are explicitly broken. External native owners retain cached host
    // wrappers, including aliasing payloads that share their control block.
    // During collection, native destructors may read or externally retain their
    // JS fields. Their finalized payload cannot be resurrected: retired wrappers
    // retain ordinary JS properties but lose host hooks and native identity.
    std::vector<JSNativeOwnerNode> (*nativeOwnership)(const JSValue&) = nullptr;
};
