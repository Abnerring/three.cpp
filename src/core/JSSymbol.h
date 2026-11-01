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


// Realm-private Symbol intrinsics. Mutable globals/prototype.constructor are aliases.
JSValue nativeSymbolConstructor();
JSValue nativeSymbolPrototype();
JSValue createNativeSymbolObject(const JSValue& primitiveSymbol);
bool isNativeSymbolObject(const JSValue& value);
JSValue nativeSymbolValue(const JSValue& receiver);
JSValue nativeSymbolFor(const JSValue& key);
JSValue nativeSymbolKeyFor(const JSValue& symbol);
// Shared ECMAScript CanBeHeldWeakly, including unregistered and well-known symbols.
bool nativeCanBeHeldWeakly(const JSValue& value);
