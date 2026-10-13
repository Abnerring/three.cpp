#pragma once
#include "core/JSValue.h"
#include <memory>
class BufferGeometry;
class Object3D;
class Skeleton;
// Called by the single NativeObject wrapper initializer. The attribute-map
// wrapper shares the existing native geometry graph hooks and storage owner.
void installNativeGeometryRenderingBridge(JSValue object, const std::shared_ptr<BufferGeometry>& geometry,
                                          const std::shared_ptr<const JSHostObjectHooks>& geometryHooks);
void installNativeObjectRenderingBridge(JSValue object, const std::shared_ptr<Object3D>& owner);
void installNativeSkeletonRenderingBridge(JSValue object, const std::shared_ptr<Skeleton>& owner);
