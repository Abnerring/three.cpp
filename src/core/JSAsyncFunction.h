#pragma once
#include "core/JSPromise.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

class JSAsyncFunctionDynamicSourceUnsupported final : public std::logic_error {
  public:
    JSAsyncFunctionDynamicSourceUnsupported()
        : std::logic_error("Dynamic AsyncFunction source compilation is not implemented") {}
};

// Async methods are non-constructible callable functions whose actual parent is
// the private %AsyncFunction.prototype% intrinsic. The body starts synchronously.
// It receives the one intrinsic promise capability returned by this invocation;
// every normal return/throw/await path must settle or suspend that capability.
// Keep every value crossing await in JSValue captures/internal slots. A native
// function pointer must never conceal owning JSValue handles in a C++ closure.
using NativeAsyncBody = void (*)(
    const JSValue& receiver, const std::vector<JSValue>& arguments,
    const std::vector<JSValue>& captures, const JSPromiseCapability& capability);
JSValue createNativeAsyncFunction(NativeAsyncBody body,
    const std::vector<JSValue>& captures = {}, const std::string& name = "",
    std::uint32_t arity = 0, const std::string& source = "");
JSValue nativeAsyncFunctionPrototype();
JSValue nativeAsyncFunctionConstructor();

enum class JSAsyncCompletionKind { Normal, Throw };
struct JSAsyncCompletion {
    JSAsyncCompletionKind kind = JSAsyncCompletionKind::Normal;
    JSValue value;
};
struct JSAsyncStep {
    enum class Kind { Await, Return, Throw };
    Kind kind;
    JSValue value;
    static JSAsyncStep awaitValue(JSValue value = {});
    static JSAsyncStep complete(JSValue value = {});
    static JSAsyncStep abrupt(JSValue value);
};
using JSAsyncInitializer = void (*)(
    const JSValue& state, const JSValue& receiver,
    const std::vector<JSValue>& arguments, const std::vector<JSValue>& captures);
using JSAsyncStateBody = JSAsyncStep (*)(
    const JSValue& state, const JSAsyncCompletion& completion,
    const JSValue& receiver, const std::vector<JSValue>& arguments,
    const std::vector<JSValue>& captures);
// Each call owns a separate traced frame. Await always suspends to the existing
// promise job queue, including primitive/already-fulfilled inputs. A resumed
// rejection is a Throw completion, so source bodies can implement catch/finally.
// Complete/abrupt clear suspension edges before invoking the resolving function.
JSValue createNativeAsyncStateFunction(JSAsyncStateBody body,
    const std::vector<JSValue>& captures = {}, const std::string& name = "",
    std::uint32_t arity = 0, const std::string& source = "",
    JSAsyncInitializer initialize = nullptr);
