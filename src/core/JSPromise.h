#pragma once
#include "core/JSJobs.h"

// Preserves an arbitrary rejection/throw value across native C++ callbacks.
class JSThrownValue : public std::exception {
  public:
    explicit JSThrownValue(JSValue value) : value(std::move(value)) {}
    const char* what() const noexcept override { return "Native JavaScript abrupt completion"; }
    JSValue value;
};
struct JSPromiseCapability { JSValue promise, resolve, reject; };
JSValue nativePromiseConstructor();
JSValue createNativePromise(JSValue executor);
JSPromiseCapability createNativePromiseCapability();
JSValue resolveNativePromise(JSValue value = {});
JSValue rejectNativePromise(JSValue reason = {});
bool isNativePromise(const JSValue& value);
