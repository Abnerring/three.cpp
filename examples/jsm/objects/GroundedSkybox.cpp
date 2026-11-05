#include "objects/GroundedSkybox.h"

#include "core/VertexAttribute.h"
#include "geometries/SphereGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "math/Vector3.h"
#include "textures/Texture.h"

#include <memory>
#include <stdexcept>
#include <utility>

namespace {
std::shared_ptr<Material> buildMaterial(std::shared_ptr<Texture> map) {
    // new MeshBasicMaterial( { map, depthWrite: false } )
    auto material = std::make_shared<MeshBasicMaterial>();
    material->map = std::move(map);
    material->depthWrite = false;
    return material;
}

std::shared_ptr<BufferGeometry> buildGeometry(double height, double radius, double resolution) {
    if (height <= 0 || radius <= 0 || resolution <= 0) {
        throw std::runtime_error(
            "THREE.GroundedSkybox: height, radius, and resolution must be positive.");
    }

    // const geometry = new SphereGeometry( radius, 2 * resolution, resolution ); geometry.scale( 1, 1, - 1 )
    auto geometry = std::make_shared<SphereGeometry>(radius, 2 * resolution, resolution);
    geometry->scale(1, 1, -1);

    const auto position = geometry->getAttribute("position");
    Vector3 temporary;
    const double count = position->getCount();
    for (double i = 0; i < count; ++i) {
        temporary.fromBufferAttribute(*position, i);
        if (temporary.y < 0) {
            const double y1 = -height * 3 / 2;
            const double factor = temporary.y < y1
                ? -height / temporary.y
                : (1 - temporary.y * temporary.y / (3 * y1 * y1));
            temporary.multiplyScalar(factor);
            // tmp.toArray( pos.array, 3 * i ) writes the same three components back into the attribute.
            position->setXYZ(i, temporary.x, temporary.y, temporary.z);
        }
    }
    position->setNeedsUpdate(true);
    return geometry;
}
}  // namespace

GroundedSkybox::GroundedSkybox(std::shared_ptr<Texture> map, double height, double radius,
                               double resolution)
    : Mesh(buildGeometry(height, radius, resolution), buildMaterial(std::move(map))) {}
