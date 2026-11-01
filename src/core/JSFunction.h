#pragma once
#include "core/JSValue.h"
#include <stdexcept>
class JSFunctionDynamicSourceUnsupported final : public std::logic_error {
  public:
    JSFunctionDynamicSourceUnsupported() : std::logic_error("Dynamic Function source compilation is not implemented") {}
};
// Stable private intrinsic; Function() with zero arguments is native and real.
// String parameter/body compilation requires a future native compiler.
JSValue nativeFunctionConstructor();
