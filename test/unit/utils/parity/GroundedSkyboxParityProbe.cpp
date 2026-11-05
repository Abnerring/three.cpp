#include "objects/GroundedSkybox.h"

#include "core/JSValue.h"
#include "core/VertexAttribute.h"
#include "geometries/SphereGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "textures/Texture.h"

#include <iostream>
#include <memory>

int main() {
    try {
        auto texture = std::make_shared<Texture>();
        GroundedSkybox skybox(texture, 15, 100, 4);

        const auto geometry = std::dynamic_pointer_cast<SphereGeometry>(skybox.geometry);
        const auto material = std::dynamic_pointer_cast<MeshBasicMaterial>(skybox.material);
        const auto position = skybox.geometry ? skybox.geometry->getAttribute("position") : nullptr;

        auto positions = JSValue::array();
        double minimumY = 1e30;
        if (position) {
            std::size_t index = 0;
            for (double i = 0; i < position->getCount(); ++i) {
                positions.set(index++, JSValue(position->getX(i)));
                positions.set(index++, JSValue(position->getY(i)));
                positions.set(index++, JSValue(position->getZ(i)));
                if (position->getY(i) < minimumY) minimumY = position->getY(i);
            }
        }

        auto output = JSValue::object(nullptr);
        output.set("isMesh", JSValue(skybox.isMesh));
        output.set("isSphereGeometry", JSValue(geometry != nullptr));
        output.set("radius", JSValue(geometry ? geometry->parameters.radius : -1.0));
        output.set("widthSegments", JSValue(geometry ? geometry->parameters.widthSegments : -1.0));
        output.set("heightSegments", JSValue(geometry ? geometry->parameters.heightSegments : -1.0));
        output.set("vertexCount", JSValue(position ? position->getCount() : -1.0));
        output.set("minimumY", JSValue(minimumY));
        output.set("materialIsBasic", JSValue(material != nullptr));
        output.set("mapIsSameTexture", JSValue(material && material->map == texture));
        output.set("depthWrite", JSValue(material ? material->depthWrite : true));
        output.set("positions", positions);
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
