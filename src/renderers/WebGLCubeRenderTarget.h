#pragma once
#include "core/JSValue.h"

// Canonical source module. The legacy typed WebGLCubeRenderTarget remains in
// core/WebGLCubeRenderTarget.h and is independent of these managed objects.
JSValue nativeWebGLCubeRenderTargetModule();
JSValue nativeWebGLCubeRenderTargetBinding(const std::string& name);
JSValue nativeWebGLCubeRenderTargetConstructor();
