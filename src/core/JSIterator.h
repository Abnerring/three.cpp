#pragma once
#include "core/JSValue.h"
#include <exception>

// A synchronous Iterator Record captures next once, while return remains a
// dynamic lookup when IteratorClose is requested by the caller's algorithm.
struct JSIteratorRecord {
    JSValue iterator, nextMethod;
    bool done = false;
};
JSIteratorRecord getJSIterator(const JSValue& iterable);
std::optional<JSValue> stepJSIterator(JSIteratorRecord& record);
void closeJSIterator(JSIteratorRecord& record);
[[noreturn]] void closeJSIterator(JSIteratorRecord& record, std::exception_ptr abrupt);

class JSIterator {
  public:
    enum class Brand : unsigned { Map = 1, Set, Array, String };
    enum class Kind : unsigned { Key, Value, Entry };
    static JSValue collection(const JSValue& value, Brand brand, Kind kind);
    static JSValue array(const JSValue& receiver, Kind kind);
    static JSValue string(const JSValue& receiver);
    static JSValue next(const JSValue& receiver, Brand brand);
};
JSValue nativeIteratorPrototype();
JSValue nativeIteratorPrototype(JSIterator::Brand brand);
JSValue nativeStringPrototype();
void initializeNativeIteration(const JSValue& arrayPrototype);
