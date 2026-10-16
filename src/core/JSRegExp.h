#pragma once
#include "core/JSValue.h"
#include <string_view>
class JSSyntaxError : public std::invalid_argument {
  public:
    using std::invalid_argument::invalid_argument;
};
class JSRegExpUnsupported : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};
JSValue nativeRegExpConstructor();
JSValue nativeRegExpPrototype();
JSValue createNativeRegExp(const JSValue& pattern, const JSValue& flags = {});
bool isNativeRegExpObject(const JSValue& value);
bool jsIsRegExp(const JSValue& value);
JSValue jsRegExpExec(const JSValue& receiver, const JSValue& string);
// Shared abstract GetSubstitution operation; captures contain strings/undefined.
std::u16string jsGetSubstitution(std::u16string_view matched, std::u16string_view string,
                                 std::size_t position, const std::vector<JSValue>& captures,
                                 const JSValue& namedCaptures, std::u16string_view replacement);
