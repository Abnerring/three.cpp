#pragma once
#include "core/JSValue.h"
#include "core/JSModuleObject.h"
#include <vector>

// Shared native logging and reversed-depth mapping from pinned src/utils.js.
// All callers share one warning cache per managed heap and the existing native
// console callback used by WebGLInfo. Other utils.js exports are separate work.
namespace ThreeUtils {
JSValue nativeReversedDepthFuncs();
inline constexpr JSModuleObject ReversedDepthFuncs{nativeReversedDepthFuncs};
void setConsoleFunction(const JSValue& callback);
JSValue getConsoleFunction();
// Browser scheduling is adapted to the native main loop (JSJobs host tasks).
JSValue yieldToMain();
void warn(const std::vector<JSValue>& params);
void log(const std::vector<JSValue>& params);
void warnOnce(const std::vector<JSValue>& params);
void error(const std::vector<JSValue>& params);
}
