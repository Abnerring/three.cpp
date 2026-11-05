#pragma once

#include "core/JSValue.h"

// Port of src/nodes/lighting/PointShadowNode.js at the pinned revision (see upstream.json).
//
// Stage A lands the class shape: the type, the constructor, the basic filter lookup, the pass-through
// shadow coordinate and the cube render target with a CubeDepthTexture. The sampled point filter graphs and
// the per face render pass are still raising members and land in later stages.
JSValue nativePointShadowNodeModule();
JSValue nativePointShadowNodeBinding(const std::string& name);
JSValue nativePointShadowNodeConstructor();
// Port only test hook: answers one cube direction table so its construction can be inspected.
JSValue nativePointShadowNodeCubeTable(bool webgpu);