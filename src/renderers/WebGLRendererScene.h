#pragma once
#include "core/JSValue.h"

// Native CPU scene pipeline from the pinned WebGLRenderer. The returned render
// function receives the original renderer as `this` and original scene/camera
// arguments. Backend services are captured once as lexical bindings; no typed
// Scene/Material/Geometry snapshot is made.
//
// Required bindings: _this, objects, renderLists, renderStates, clipping,
// background, shadowMap, textures, state, xr, bindingStates, extensions,
// capabilities. These must be the backend's actual protocol objects.
// Optional initial bindings use the original defaults: output, stencil,
// _isContextLost, _nodesHandler, _currentRenderTarget, _currentActiveCubeFace,
// _currentActiveMipmapLevel, _currentViewport, _opaqueSort, _transparentSort.
// The other source private state can also be supplied for construction tests.
//
// Internal slots WebGLRendererScene.get / WebGLRendererScene.set expose
// functions (name) and (name,value) accessing the shared lexical cells. They
// allow actual backend target/state methods to synchronize their source
// private variables. Source const bindings remain const; mutate the returned
// _currentViewport object rather than assigning another viewport.
// Internal slots WebGLRendererScene.projectObject / renderScene /
// renderObjects / renderObject / renderTransmissionPass expose the original
// private functions for backend integration and CPU protocol verification.
//
// Nested render calls share the original render-list/state stacks. Abrupt
// completion preserves the source's unfinished stacks and temporary state;
// this helper does not add cleanup/finally logic. GPU submission/ownership
// cleanup belongs to the injected backend and must not rewrite source state.
JSValue nativeWebGLRendererScenePipeline(JSValue bindings);
