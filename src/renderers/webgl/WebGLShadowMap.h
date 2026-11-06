#pragma once
#include "core/JSValue.h"

// Complete pinned WebGLShadowMap source factory. Renderer entry points and
// object updates are injected native services; module closure state is shared.
JSValue nativeWebGLShadowMapConstructor();
JSValue nativeWebGLShadowMap(JSValue renderer, JSValue objects, JSValue capabilities);
