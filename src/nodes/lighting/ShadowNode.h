#pragma once

#include "core/JSValue.h"

// Port of src/nodes/lighting/ShadowNode.js at the pinned revision (see upstream.json).
//
// Stage A lands the class shape: the constructor fields, the static type getter, the shadow filter
// lookup and the dispose / reset cache handling. The node graph builder (setupShadow and the setup
// family), the VSM passes and the frame hooks still need their own rounds; calling them raises
// instead of returning a fabricated node graph.
JSValue nativeShadowNodeModule();
JSValue nativeShadowNodeBinding(const std::string& name);
JSValue nativeShadowNodeConstructor();
// Port only test hook: builds the VSMPassVertical / VSMPassHorizontal graph node so its construction can be
// covered by a unit test. Not part of the pinned module surface.
JSValue nativeShadowNodeVsmPassGraph(bool vertical);
// Port only test hook: answers the module level VSM quad mesh so its identity reuse can be asserted.
JSValue nativeShadowNodeQuadMesh();