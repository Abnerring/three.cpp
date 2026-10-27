#pragma once
#include "core/JSValue.h"

// Native ECMAScript proxy internal methods. No source execution is involved.
JSValue createNativeProxy(const JSValue& target, const JSValue& handler);
void revokeNativeProxy(const JSValue& proxy);
bool isNativeArray(const JSValue& value);
namespace JSProxyDetail {
JSValue get(const JSValue&, const JSValue&, const JSValue&);
bool set(const JSValue&, const JSValue&, const JSValue&, const JSValue&);
bool has(const JSValue&, const JSValue&);
bool erase(const JSValue&, const JSValue&);
std::optional<JSPropertyDescriptor> descriptor(const JSValue&, const JSValue&);
bool define(const JSValue&, const JSValue&, const JSPropertyDescriptor&);
std::vector<JSValue> keys(const JSValue&);
JSValue prototype(const JSValue&);
bool setPrototype(const JSValue&, const JSValue&);
bool extensible(const JSValue&);
bool preventExtensions(const JSValue&);
JSValue call(const JSValue&, const JSValue&, const std::vector<JSValue>&);
JSValue construct(const JSValue&, const std::vector<JSValue>&, const JSValue&);
}
