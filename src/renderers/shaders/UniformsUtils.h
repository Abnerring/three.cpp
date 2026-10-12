#pragma once
#include "core/JSValue.h"

JSValue cloneUniforms(const JSValue& source);
JSValue mergeUniforms(const JSValue& uniforms);
JSValue cloneUniformsGroups(const JSValue& source);
JSValue getUnlitUniformColorSpace(const JSValue& renderer);

namespace UniformsUtils {
inline JSValue clone(const JSValue& source) { return cloneUniforms(source); }
inline JSValue merge(const JSValue& uniforms) { return mergeUniforms(uniforms); }
}
