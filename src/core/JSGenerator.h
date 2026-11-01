#pragma once
#include "core/JSValue.h"
#include "core/JSPromise.h"
#include <exception>
#include <string>
#include <utility>
#include <vector>

// Native completion records retain arbitrary thrown values without conversion.
enum class JSGeneratorCompletionKind { Normal, Return, Throw };
struct JSGeneratorCompletion {
    JSGeneratorCompletionKind kind = JSGeneratorCompletionKind::Normal;
    JSValue value;
};
struct JSGeneratorStep {
    enum class Kind { Yield, Return, Throw };
    Kind kind;
    JSValue value;
    static JSGeneratorStep yield(JSValue value = {});
    static JSGeneratorStep complete(JSValue value = {});
    static JSGeneratorStep abrupt(JSValue value);
};
class JSGeneratorThrown final : public JSThrownValue {
  public:
    explicit JSGeneratorThrown(JSValue value) : JSThrownValue(std::move(value)) {}
    const JSValue& value() const { return JSThrownValue::value; }
    const char* what() const noexcept override { return "JavaScript generator throw completion"; }
};
class JSGeneratorDynamicSourceUnsupported final : public std::logic_error {
  public:
    JSGeneratorDynamicSourceUnsupported() : std::logic_error("Dynamic GeneratorFunction source compilation is not implemented") {}
};
// Body state is a private ordinary managed object. Store every suspension-local
// JSValue on that object, not in a C++ closure. The body is never called during
// generator creation or a suspended-start return/throw. First next input is
// discarded; subsequent completions are delivered unchanged, including Return
// and Throw, so a native body can implement catch/finally with yielding cleanup.
using JSGeneratorInitializer = void (*)(
    const JSValue& state, const JSValue& receiver,
    const std::vector<JSValue>& arguments, const std::vector<JSValue>& captures);
using JSGeneratorBody = JSGeneratorStep (*)(
    const JSValue& state, const JSGeneratorCompletion& completion,
    const JSValue& receiver, const std::vector<JSValue>& arguments,
    const std::vector<JSValue>& captures);
JSValue createNativeGeneratorFunction(JSGeneratorBody body,
    const std::vector<JSValue>& captures = {}, const std::string& name = "",
    std::uint32_t arity = 0, const std::string& source = "",
    JSGeneratorInitializer initialize = nullptr);
JSValue nativeGeneratorPrototype();
JSValue nativeGeneratorFunctionPrototype();
JSValue nativeGeneratorFunctionConstructor();
bool isNativeGeneratorObject(const JSValue& value);
JSValue resumeNativeGenerator(const JSValue& generator,
    JSGeneratorCompletion completion = {});
