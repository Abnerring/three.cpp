#include "environments/ColorEnvironment.h"

#include "constants.h"
#include "geometries/SphereGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "math/Color.h"
#include "objects/Mesh.h"

#include <memory>
#include <stdexcept>

ColorEnvironment::ColorEnvironment(int color) : Scene() {
    name = "ColorEnvironment";
    // new SphereGeometry( 1, 16, 16 )
    auto geometry = std::make_shared<SphereGeometry>(1, 16, 16);
    // new MeshBasicMaterial( { color: color, side: BackSide } )
    auto material = std::make_shared<MeshBasicMaterial>(Color(color));
    material->side = BackSide;
    // this.add( new Mesh( geometry, material ) )
    add(std::make_shared<Mesh>(geometry, material));
}

void ColorEnvironment::dispose() {
    if (children.empty()) {
        // The pinned code reads children[ 0 ] unconditionally, so disposing an edited scene raises there
        // instead of doing nothing.
        throw std::runtime_error("ColorEnvironment: no child mesh to dispose");
    }
    const auto mesh = std::dynamic_pointer_cast<Mesh>(children[0]);
    if (!mesh) return;
    if (mesh->geometry) mesh->geometry->dispose();
    if (mesh->material) mesh->material->dispose();
}
