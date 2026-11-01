#pragma once
#include "core/JSValue.h"

// NumberData and the Number methods needed by native renderer source protocols.
// The native Number constructor and isInteger share the existing NumberData. Decimal toString and the
// complete toFixed/valueOf methods are supplied; finite non-decimal formatting
// remains an explicit unsupported operation, not a JavaScript RangeError.
class JSNumberRadixUnsupported : public std::logic_error {
  public:
    JSNumberRadixUnsupported()
        : std::logic_error("Number.prototype.toString non-decimal radix is not implemented") {}
};
JSValue nativeNumberPrototype();
JSValue createNativeNumberObject(const JSValue& primitiveNumber);
bool isNativeNumberObject(const JSValue& value);
JSValue nativeNumberValue(const JSValue& receiver);

JSValue nativeNumberConstructor();
