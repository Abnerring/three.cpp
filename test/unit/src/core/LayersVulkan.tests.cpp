#include "cameras/OrthographicCamera.h"
#include "core/JSJobs.h"
#include "core/NativeObject.h"
#include "geometries/PlaneGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Mesh.h"
#include "renderers/VulkanRenderer.h"
#include "scenes/Scene.h"
#include <array>
#include <iostream>
namespace {
using Args = std::vector<JSValue>;
unsigned checks = 0;
void check(bool value, const char* message) {
    ++checks;
    if (!value)
        throw std::runtime_error(message);
}
std::array<unsigned, 4> pixel(const VulkanRenderer& renderer) {
    auto p = renderer.readPixels();
    const auto i = (16 * renderer.getWidth() + 16) * 4;
    return {p.at(i), p.at(i + 1), p.at(i + 2), p.at(i + 3)};
}
void turn(VulkanRenderer& renderer, Scene& scene, Camera& camera, const JSValue& realm) {
    endJSJob();
    realm.collectGarbage();
    renderer.render(scene, camera);
}
} // namespace
int main() {
    try {
        auto realm = JSValue::object();
        Scene scene;
        auto camera = std::make_shared<OrthographicCamera>(-1, 1, 1, -1, 0.1, 10);
        camera->position.z = 2;
        auto mesh = std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(1.6, 1.6),
                                           std::make_shared<MeshBasicMaterial>(Color(1, 0, 0)));
        scene.add(mesh);
        mesh->layers.mask = 2;
        camera->layers.mask = 1;
        VulkanRenderer renderer({32, 32, false});
        renderer.srgbOutput = false;
        renderer.setClearColor(Color(0, 0, 0), 1);
        turn(renderer, scene, *camera, realm);
        check(pixel(renderer) == std::array<unsigned, 4>{0, 0, 0, 255}, "nonmatching layers draw");
        check(renderer.getInfo().calls == 0, "nonmatching layers increment calls");
        auto layers = mesh->layers.object(), cameraWrapper = nativeObject(camera),
             replacement = Layers().object();
        replacement.set("mask", 2);
        auto state = JSValue::object();
        state.set("n", 0);
        JSPropertyDescriptor d;
        d.configurable = true;
        d.getter = JSValue::function(
            +[](const JSValue&, const Args&, const Args& c) -> JSValue {
                auto state = c[0], camera = c[1];
                state.set("n", state.get("n").toNumber() + 1);
                camera.set("layers", c[2]);
                return c[3];
            },
            {state, cameraWrapper, replacement, Layers::prototype().get("test")});
        layers.defineProperty("test", d);
        turn(renderer, scene, *camera, realm);
        check(pixel(renderer) == std::array<unsigned, 4>{255, 0, 0, 255},
              "getter must replace camera layers before argument is captured");
        check(camera->layers.object().sameValue(replacement), "camera layer replacement identity");
        check(state.get("n").toNumber() >= 1, "GPU path bypassed managed test getter");
        check(renderer.getInfo().calls == 1, "matching object submission count");
        d.getter = JSValue::function(
            +[](const JSValue&, const Args&, const Args& c) -> JSValue {
                auto camera = c[0];
                camera.set("layers", c[1]);
                throw JSRangeError("layers getter");
                return {};
            },
            {cameraWrapper, replacement});
        layers.defineProperty("test", d);
        bool threw = false;
        try {
            turn(renderer, scene, *camera, realm);
        } catch (const JSRangeError&) {
            threw = true;
        }
        check(threw, "layer method getter exception was swallowed");
        check(camera->layers.object().sameValue(replacement),
              "partial getter writes were rolled back");
        layers.erase("test");
        turn(renderer, scene, *camera, realm);
        check(pixel(renderer) == std::array<unsigned, 4>{255, 0, 0, 255},
              "renderer did not recover after getter exception");
        mesh->layers.mask = JSValue::symbol("mask");
        threw = false;
        try {
            turn(renderer, scene, *camera, realm);
        } catch (const JSTypeError&) {
            threw = true;
        }
        check(threw, "Symbol mask was silently converted by GPU path");
        mesh->layers.mask = 2;
        turn(renderer, scene, *camera, realm);
        check(pixel(renderer) == std::array<unsigned, 4>{255, 0, 0, 255},
              "renderer did not recover after numeric conversion exception");
        check(renderer.getValidationMessages().empty(), "Vulkan validation messages");
        std::cout << "Layers GPU: " << renderer.getInfo().deviceName << "; " << checks
                  << " checks passed; validation=" << renderer.getInfo().validationEnabled << '\n';
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
