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
// Program hooks use the same cells and scene stacks: getProgram,
// getProgramVariant, getUniformList, updateCommonMaterialProperties,
// findLightProbeGrid, setProgram, markUniformsLightsNeedsUpdate,
// materialNeedsLights, onMaterialDispose, deallocateMaterial, and
// releaseMaterialProgramReferences. Each is exposed through the corresponding
// WebGLRendererScene.<name> internal slot.
//
// To call program hooks, supply actual backend services _gl, properties,
// programCache, environments, materials, morphtargets, and uniformsGroups.
// These new bindings are read only when supplied as own properties and stay
// undefined otherwise. getDFGLUT is an immutable imported native service;
// supply it for materials whose uniforms contain dfgLUT. WebGLUniforms uses
// the existing real source port, not an injected replacement.
// Source scratch vectors and _emptyScene use their original defaults;
// _pixelRatio defaults to 1. Supply _height with the backend canvas height.
// The set/get slots can synchronize mutable backend cells, including _height,
// without replacing original material/program/uniform objects.
//
// renderBufferDirect is the complete original public function, available as
// result.renderBufferDirect and WebGLRendererScene.renderBufferDirect. It uses
// these additional optional actual backend services: geometries, attributes,
// bufferRenderer, indexedBufferRenderer. Their source methods own index upload,
// binding setup and draw dispatch. getTargetPixelRatio is compiled into the
// same lexical scope and exposed through its corresponding internal slot.
//
// The returned object also exposes the complete source public target methods:
// getActiveCubeFace/getActiveMipmapLevel/getRenderTarget/setRenderTarget,
// setRenderTargetTextures/setRenderTargetFramebuffer. They share the cells used
// by render/setProgram/renderTransmissionPass. WebGLRenderTarget is imported
// from its canonical shared module, including its original inherited methods.
// No backend target snapshot or application callback wrapper is introduced.
//
// Source viewport/scissor/size methods are exposed on the same object and as
// WebGLRendererScene.<name> slots: get/setPixelRatio, get/setSize,
// get/setDrawingBufferSize, getCurrentViewport, get/setViewport, get/setScissor,
// get/setScissorTest, setOpaqueSort and setTransparentSort. Install these actual
// functions on the renderer so calls through `this` use the same source cells.
// Supply the original canvas as the optional canvas binding. Initial _width and
// _height default to its width/height; explicit initial cells are also accepted.
// _viewport/_scissor are const Vector4 objects, _scissorTest defaults to false;
// _currentScissor is a const Vector4, _currentScissorTest starts null. Keep actual
// GPU state in the supplied state.viewport/scissor/setScissorTest services.
//
// Target methods additionally require real state.bindFramebuffer/drawBuffers,
// textures.setupRenderTarget/rebindTextures/setupDepthRenderbuffer/
// useMultisampledRTT, properties.get/has and _gl.framebufferTexture2D/Layer.
// _scratchFramebuffer is the original mutable cell, initially null; install the
// actual backend scratch framebuffer in that cell for nonzero mip levels.
// Unavailable backend operations must fail at their actual device boundary.
// Target cells change before setup/binding and are not rolled back on throws
// or callback reentry. Backend services must not reset these source cells.
//
// Nested render calls share the original render-list/state stacks. Abrupt
// completion preserves the source's unfinished stacks and temporary state;
// this helper does not add cleanup/finally logic. GPU submission/ownership
// cleanup belongs to the injected backend and must not rewrite source state.
JSValue nativeWebGLRendererScenePipeline(JSValue bindings);
