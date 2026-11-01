#pragma once
#include "core/JSValue.h"
#include "core/JSRegExp.h"

class JSJSONSyntaxError : public JSSyntaxError {
  public:
    using JSSyntaxError::JSSyntaxError;
};

// Per-realm JSON intrinsic. These APIs use native value/property operations;
// no JavaScript source is evaluated. Undefined stringify results stay undefined.
JSValue nativeJSONObject();
JSValue nativeJSONParse(JSValue text, JSValue reviver = {});
JSValue nativeJSONStringify(JSValue value, JSValue replacer = {},
                            JSValue space = {});
JSValue nativeJSONRawJSON(JSValue text);
bool nativeJSONIsRawJSON(const JSValue& value);
