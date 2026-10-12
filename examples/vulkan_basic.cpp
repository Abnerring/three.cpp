#include "Three.h"
#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    try {
        const std::string output = argc > 1 ? argv[1] : "vulkan-basic.ppm";
        const bool validation = argc > 2 && std::string(argv[2]) == "--validation";
        VulkanRenderer renderer({512, 512, validation});
        Scene scene;
        scene.background = Color(0x202734);
        PerspectiveCamera camera(45, 1, 0.1, 100);
        camera.position.set(3, 2.5, 4);
        camera.lookAt(0, 0, 0);
        std::vector<std::shared_ptr<Material>> faces;
        for (int color : {0xff725e, 0xb45748, 0xffd166, 0x977c40, 0x4cc9f0, 0x3485a0})
            faces.push_back(std::make_shared<MeshBasicMaterial>(Color(color)));
        auto box = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(1.5, 1.5, 1.5), faces);
        box->rotation.set(0.15, 0.35, 0);
        scene.add(box);
        auto groundMaterial = std::make_shared<MeshBasicMaterial>(Color(0x354256));
        auto ground = std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(7, 7), groundMaterial);
        ground->rotation.set(-1.5707963267948966, 0, 0);
        ground->position.set(0, -0.78, 0);
        scene.add(ground);
        renderer.render(scene, camera);
        const auto pixels = renderer.readPixels();
        std::ofstream file(output, std::ios::binary);
        if (!file)
            throw std::runtime_error("Cannot create output image: " + output);
        file << "P6\n" << renderer.getWidth() << ' ' << renderer.getHeight() << "\n255\n";
        for (std::size_t i = 0; i < pixels.size(); i += 4)
            file.write(reinterpret_cast<const char*>(pixels.data() + i), 3);
        if (!file)
            throw std::runtime_error("Cannot write output image: " + output);
        const auto& info = renderer.getInfo();
        std::cout << "Vulkan device: " << info.deviceName << "\nDraw calls: " << info.calls
                  << "; triangles: " << info.triangles << "\nImage: " << output << '\n';
        for (const auto& message : renderer.getValidationMessages())
            std::cerr << message << '\n';
        return renderer.getValidationMessages().empty() ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
