#pragma once
#include "core/JSValue.h"

// Modern CubeRenderTarget, with the same managed RenderTarget/texture objects.
// Equirectangular conversion requires the remaining NodeMaterial render path.
JSValue nativeCubeRenderTargetConstructor();
