#pragma once
#include "core/JSValue.h"

// NumberData and the Number methods needed by native renderer source protocols.
// No global Number constructor/constants are exported. Decimal toString and the
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
