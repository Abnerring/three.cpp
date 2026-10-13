#pragma once
#include "core/JSValue.h"
#include <vector>

// Shared native implementation of the warning portion of pinned src/utils.js.
// All callers share one warning cache per managed heap and the existing native
// console callback used by WebGLInfo. Other utils.js exports are separate work.
namespace ThreeUtils {
void setConsoleFunction(const JSValue& callback);
JSValue getConsoleFunction();
void warn(const std::vector<JSValue>& params);
void warnOnce(const std::vector<JSValue>& params);
}
