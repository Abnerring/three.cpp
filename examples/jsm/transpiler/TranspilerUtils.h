#pragma once

#include "core/JSValue.h"

#include <string>

// Native port of examples/jsm/transpiler/TranspilerUtils.js from the pinned three.js revision
// (see upstream.json).
//
// Four small helpers shared by the transpiler front ends. The pinned module inspects plain AST
// objects through their flags and classifies type name strings with regular expressions; being
// language neutral C++ takes the statement as a JSValue (so the same flags are read) and the type
// names as std::string. The regular expressions keep the pinned semantics, including the anchors.
namespace TranspilerUtils {
bool isExpression(const JSValue& statement);
bool isPrimitive(const std::string& value);
bool isBuiltinType(const std::string& type);
std::string toFloatType(const std::string& type);
}
