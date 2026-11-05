#include "helpers/RectAreaLightHelper.h"

#include "constants.h"
#include "core/BufferAttribute.h"
#include "core/BufferGeometry.h"
#include "lights/RectAreaLight.h"
#include "materials/LineBasicMaterial.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"

#include <algorithm>
#include <memory>
#include <vector>

namespace {
std::shared_ptr<BufferGeometry> buildOutlineGeometry() {
    // const positions = [ 1, 1, 0, -1, 1, 0, -1, -1, 0, 1, -1, 0, 1, 1, 0 ]
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute(
        "position",
        std::make_shared<BufferAttribute>(
            std::vector<float>{1, 1, 0, -1, 1, 0, -1, -1, 0, 1, -1, 0, 1, 1, 0}, 3));
    geometry->computeBoundingSphere();
    return geometry;
}

std::shared_ptr<Material> buildOutlineMaterial() {
    // new LineBasicMaterial( { fog: false } )
    auto material = std::make_shared<LineBasicMaterial>();
    material->fog = false;
    return material;
}

std::shared_ptr<BufferGeometry> buildFillGeometry() {
    // const positions2 = [ 1, 1, 0, -1, 1, 0, -1, -1, 0, 1, 1, 0, -1, -1, 0, 1, -1, 0 ]
    auto geometry = std::make_shared<BufferGeometry>();
    geometry->setAttribute(
        "position",
        std::make_shared<BufferAttribute>(
            std::vector<float>{1, 1, 0, -1, 1, 0, -1, -1, 0, 1, 1, 0, -1, -1, 0, 1, -1, 0}, 3));
    geometry->computeBoundingSphere();
    return geometry;
}

std::shared_ptr<Material> buildFillMaterial() {
    // new MeshBasicMaterial( { side: BackSide, fog: false } )
    auto material = std::make_shared<MeshBasicMaterial>();
    material->side = BackSide;
    material->fog = false;
    return material;
}
}  // namespace

RectAreaLightHelper::RectAreaLightHelper(const std::shared_ptr<RectAreaLight>& light,
                                         std::optional<Color> color)
    : Line(buildOutlineGeometry(), buildOutlineMaterial()), light(light), color(std::move(color)) {
    type = "RectAreaLightHelper";
    add(std::make_shared<Mesh>(buildFillGeometry(), buildFillMaterial()));
}

void RectAreaLightHelper::updateMatrixWorld(bool force) {
    // this.scale.set( 0.5 * this.light.width, 0.5 * this.light.height, 1 )
    auto outlineMaterial = std::dynamic_pointer_cast<LineBasicMaterial>(material);
    const auto fill = children.empty() ? nullptr : std::dynamic_pointer_cast<Mesh>(children[0]);
    auto fillMaterial = fill ? std::dynamic_pointer_cast<MeshBasicMaterial>(fill->material) : nullptr;
    if (light) scale.set(0.5 * light->width, 0.5 * light->height, 1);

    if (color.has_value()) {
        if (outlineMaterial) outlineMaterial->color.set(*color);
        if (fillMaterial) fillMaterial->color.set(*color);
    } else if (light) {
        if (outlineMaterial) {
            outlineMaterial->color.copy(light->color).multiplyScalar(light->intensity);
            auto& current = outlineMaterial->color;
            const double max = std::max({current.r, current.g, current.b});
            if (max > 1) current.multiplyScalar(1 / max);
        }
        if (fillMaterial && outlineMaterial) fillMaterial->color.copy(outlineMaterial->color);
    }

    // this.matrixWorld.extractRotation( light.matrixWorld ).scale( this.scale ).copyPosition( light.matrixWorld )
    if (light) {
        matrixWorld.extractRotation(light->matrixWorld).scale(scale).copyPosition(light->matrixWorld);
        if (fill) fill->matrixWorld.copy(matrixWorld);
    }
    // The pinned override does not call the base implementation; the fill matrix is copied above.
    (void) force;
}

void RectAreaLightHelper::dispose() {
    if (geometry) geometry->dispose();
    if (material) material->dispose();
    if (!children.empty()) {
        const auto fill = std::dynamic_pointer_cast<Mesh>(children[0]);
        if (fill) {
            if (fill->geometry) fill->geometry->dispose();
            if (fill->material) fill->material->dispose();
        }
    }
}
