#pragma once
#include "core/JSValue.h"
#include <string>
#include <vector>

// Source translators supply a diagnostic expression derived from the fixed AST.
// Property lookup and argument evaluation stay separate so their order is not
// changed by formatting the TypeError for a subsequent non-callable value.
namespace JSSourceOperations {
inline JSValue get(const JSValue& receiver, const char* property) {
    if (receiver.isNull() || receiver.isUndefined())
        throw JSTypeError(std::string("Cannot read properties of ") +
                          (receiver.isNull() ? "null" : "undefined") +
                          " (reading '" + property + "')");
    return receiver.get(property);
}

inline JSValue call(const JSValue& method, const JSValue& receiver,
                    const std::vector<JSValue>& args, const char* expression) {
    // isCallable() is a pure Type tag check. Preserve call()'s validation order
    // on the failing branch; callable values use the original call path intact.
    if (!method.isCallable()) {
        static_cast<void>(JSWeakValue(method));
        static_cast<void>(JSWeakValue(receiver));
        for (const auto& argument : args) static_cast<void>(JSWeakValue(argument));
        throw JSTypeError(std::string(expression) + " is not a function");
    }
    return method.call(receiver, args);
}
}
