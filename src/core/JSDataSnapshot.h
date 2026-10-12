#pragma once
#include "core/JSValue.h"

// Static native data adapter, distinct from JSON.stringify. Preserve binary64
// numbers and read enumerable fields without invoking toJSON methods. JsonValue
// cannot retain object identity or undefined; omitted fields/array nulls retain
// that existing boundary. Runtime-facing APIs should keep returning JSValue.
std::optional<JsonValue> snapshotData(const JSValue& value);
