#pragma once
#include "core/JSModuleObject.h"

JSValue cloneUniforms(const JSValue& source);
JSValue mergeUniforms(const JSValue& uniforms);
JSValue cloneUniformsGroups(const JSValue& source);
JSValue getUnlitUniformColorSpace(const JSValue& renderer);

// The pinned module's mutable legacy literal and standalone function exports.
// Resolver calls return realm-local identities, without an owning TLS heap root.
JSValue nativeUniformsUtils();
JSValue nativeCloneUniforms();
JSValue nativeMergeUniforms();
JSValue nativeCloneUniformsGroups();
JSValue nativeGetUnlitUniformColorSpace();

inline constexpr JSModuleObject UniformsUtils(nativeUniformsUtils);
