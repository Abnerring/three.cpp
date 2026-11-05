#pragma once

#include "core/JSValue.h"

// Port of src/nodes/utils/ReflectorNode.js at the pinned revision (see upstream.json).
//
// Stage A lands the two classes and their construction and accessors: ReflectorBaseNode keeps the target,
// resolution scale, mipmap and bounce and depth options, the virtual camera and render target maps and the
// update typing, and ReflectorNode is a texture node sampling the default render target through the screen
// UV, with the reflector base node and the lazily created depth node. The frame update hook still raises.
JSValue nativeReflectorNodeModule();
JSValue nativeReflectorNodeBinding(const std::string& name);
JSValue nativeReflectorNodeConstructor();
JSValue nativeReflectorBaseNodeConstructor();