#pragma once

#include "core/JSValue.h"

#include <string>

// Aggregator port of src/materials/nodes/NodeMaterials.js from the pinned three.js revision
// (see upstream.json).
//
// The pinned file is a re-export map: nineteen node material classes under their export names. This
// port builds the same map from the constructors that exist in C++ today:
//
//   NodeMaterialObserver, NodeMaterial, ProxyNodeMaterial, LineBasicNodeMaterial,
//   LineDashedNodeMaterial, MeshNormalNodeMaterial, MeshBasicNodeMaterial,
//   MeshLambertNodeMaterial, MeshPhongNodeMaterial, MeshStandardNodeMaterial,
//   MeshPhysicalNodeMaterial, MeshSSSNodeMaterial, MeshToonNodeMaterial,
//   MeshMatcapNodeMaterial, PointsNodeMaterial, SpriteNodeMaterial, ShadowNodeMaterial,
//   VolumeNodeMaterial
//
// Still missing from the map (their modules are not ported yet):
//
//   Line2NodeMaterial
JSValue nativeNodeMaterialsModule();
JSValue nativeNodeMaterialsBinding(const std::string& name);
