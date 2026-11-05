#include "materials/nodes/NodeMaterials.h"

#include "materials/nodes/Line2NodeMaterial.h"
#include "materials/nodes/LineBasicNodeMaterial.h"
#include "materials/nodes/LineDashedNodeMaterial.h"
#include "materials/nodes/MeshBasicNodeMaterial.h"
#include "materials/nodes/MeshStandardNodeMaterial.h"
#include "materials/nodes/MeshPhysicalNodeMaterial.h"
#include "materials/nodes/MeshSSSNodeMaterial.h"
#include "materials/nodes/MeshLambertNodeMaterial.h"
#include "materials/nodes/MeshMatcapNodeMaterial.h"
#include "materials/nodes/MeshNormalNodeMaterial.h"
#include "materials/nodes/MeshPhongNodeMaterial.h"
#include "materials/nodes/MeshToonNodeMaterial.h"
#include "materials/nodes/NodeMaterial.h"
#include "materials/nodes/PointsNodeMaterial.h"
#include "materials/nodes/ProxyNodeMaterial.h"
#include "materials/nodes/ShadowNodeMaterial.h"
#include "materials/nodes/SpriteNodeMaterial.h"
#include "materials/nodes/VolumeNodeMaterial.h"
#include "materials/nodes/manager/NodeMaterialObserver.h"

JSValue nativeNodeMaterialsModule() {
    // export { default as X } from './X.js' for every class that is ported.
    auto module = JSValue::object(nullptr);
    module.set("NodeMaterialObserver", nativeNodeMaterialObserverConstructor());
    module.set("NodeMaterial", nativeNodeMaterialConstructor());
    module.set("ProxyNodeMaterial", nativeProxyNodeMaterialConstructor());
    module.set("Line2NodeMaterial", nativeLine2NodeMaterialConstructor());
    module.set("LineBasicNodeMaterial", nativeLineBasicNodeMaterialConstructor());
    module.set("LineDashedNodeMaterial", nativeLineDashedNodeMaterialConstructor());
    module.set("MeshNormalNodeMaterial", nativeMeshNormalNodeMaterialConstructor());
    module.set("MeshBasicNodeMaterial", nativeMeshBasicNodeMaterialConstructor());
    module.set("MeshLambertNodeMaterial", nativeMeshLambertNodeMaterialConstructor());
    module.set("MeshPhongNodeMaterial", nativeMeshPhongNodeMaterialConstructor());
    module.set("MeshStandardNodeMaterial", nativeMeshStandardNodeMaterialConstructor());
    module.set("MeshPhysicalNodeMaterial", nativeMeshPhysicalNodeMaterialConstructor());
    module.set("MeshSSSNodeMaterial", nativeMeshSSSNodeMaterialConstructor());
    module.set("MeshToonNodeMaterial", nativeMeshToonNodeMaterialConstructor());
    module.set("MeshMatcapNodeMaterial", nativeMeshMatcapNodeMaterialConstructor());
    module.set("PointsNodeMaterial", nativePointsNodeMaterialConstructor());
    module.set("SpriteNodeMaterial", nativeSpriteNodeMaterialConstructor());
    module.set("ShadowNodeMaterial", nativeShadowNodeMaterialConstructor());
    module.set("VolumeNodeMaterial", nativeVolumeNodeMaterialConstructor());
    return module;
}

JSValue nativeNodeMaterialsBinding(const std::string& name) {
    return nativeNodeMaterialsModule().get(name);
}
