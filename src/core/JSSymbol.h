#pragma once
#include "core/JSValue.h"
// Real primitive identities and well-known property keys used by native protocols.
// Symbol registry, primitive boxing, and the full global Symbol API are separate.
namespace JSSymbol {
inline JSValue create(const std::optional<std::string>& description=std::nullopt){return JSValue::symbol(description);}
inline JSValue iterator(){return JSValue::wellKnownSymbol("iterator");}
inline JSValue toPrimitive(){return JSValue::wellKnownSymbol("toPrimitive");}
inline JSValue toStringTag(){return JSValue::wellKnownSymbol("toStringTag");}
inline JSValue species(){return JSValue::wellKnownSymbol("species");}
}
