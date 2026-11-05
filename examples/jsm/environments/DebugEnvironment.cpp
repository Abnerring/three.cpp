#include "environments/DebugEnvironment.h"

#include "constants.h"
#include "geometries/BoxGeometry.h"
#include "lights/PointLight.h"
#include "materials/MeshLambertMaterial.h"
#include "materials/MeshStandardMaterial.h"
#include "math/Color.h"
#include "objects/Mesh.h"

#include <memory>
#include <unordered_set>
#include <vector>

DebugEnvironment::DebugEnvironment() : Scene() {
    // const geometry = new BoxGeometry(); geometry.deleteAttribute( 'uv' )
    auto geometry = std::make_shared<BoxGeometry>();
    geometry->deleteAttribute("uv");

    // const roomMaterial = new MeshStandardMaterial( { metalness: 0, side: BackSide } )
    auto roomMaterial = std::make_shared<MeshStandardMaterial>();
    roomMaterial->metalness = 0;
    roomMaterial->side = BackSide;
    auto room = std::make_shared<Mesh>(geometry, roomMaterial);
    room->scale.setScalar(10);
    add(room);

    // const mainLight = new PointLight( 0xffffff, 50, 0, 2 )
    add(std::make_shared<PointLight>(Color(0xffffff), 50, 0, 2));

    struct LightBox {
        int color;
        double x, y, z;
        double sx, sy, sz;
    };
    const LightBox boxes[] = {
        {0xff0000, -5, 2, 0, 0.1, 1, 1},
        {0x00ff00, 0, 5, 0, 1, 0.1, 1},
        {0x0000ff, 2, 1, 5, 1.5, 2, 0.1},
    };
    for (const auto& box : boxes) {
        auto material = std::make_shared<MeshLambertMaterial>();
        material->color = Color(box.color);
        material->emissive = Color(0xffffff);
        material->emissiveIntensity = 10;
        auto lightBox = std::make_shared<Mesh>(geometry, material);
        lightBox->position.set(box.x, box.y, box.z);
        lightBox->scale.set(box.sx, box.sy, box.sz);
        add(lightBox);
    }
}

void DebugEnvironment::dispose() {
    // const resources = new Set(); this.traverse( object => { if ( object.isMesh ) { ... } } )
    std::unordered_set<BufferGeometry*> geometries;
    std::unordered_set<Material*> materials;
    std::vector<BufferGeometry*> geometryOrder;
    std::vector<Material*> materialOrder;
    traverse([&](Object3D& object) {
        if (!object.isMesh) return;
        const auto mesh = dynamic_cast<Mesh*>(&object);
        if (mesh == nullptr) return;
        if (mesh->geometry && geometries.insert(mesh->geometry.get()).second) {
            geometryOrder.push_back(mesh->geometry.get());
        }
        if (mesh->material && materials.insert(mesh->material.get()).second) {
            materialOrder.push_back(mesh->material.get());
        }
    });
    for (auto* geometry : geometryOrder) geometry->dispose();
    for (auto* material : materialOrder) material->dispose();
}
