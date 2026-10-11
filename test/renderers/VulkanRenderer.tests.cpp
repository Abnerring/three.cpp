#include "renderers/VulkanRenderer.h"
#include "cameras/OrthographicCamera.h"
#include "cameras/PerspectiveCamera.h"
#include "geometries/PlaneGeometry.h"
#include "materials/MeshBasicMaterial.h"
#include "objects/Group.h"
#include "objects/Mesh.h"
#include "scenes/FogExp2.h"
#include "scenes/Scene.h"
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
int assertions = 0;
void require(bool condition, const char* description) {
    ++assertions;
    if (!condition)
        throw std::runtime_error(description);
}
std::array<int, 4> pixel(const VulkanRenderer& renderer, unsigned x = 32, unsigned y = 32) {
    const auto pixels = renderer.readPixels();
    const auto index = (y * renderer.getWidth() + x) * 4;
    return {pixels.at(index), pixels.at(index + 1), pixels.at(index + 2), pixels.at(index + 3)};
}
void expectPixel(const VulkanRenderer& renderer, std::array<int, 4> expected,
                 const char* description, unsigned x = 32, unsigned y = 32, int tolerance = 1) {
    const auto actual = pixel(renderer, x, y);
    for (int i = 0; i < 4; ++i)
        if (std::abs(actual[i] - expected[i]) > tolerance) {
            std::cerr << description << ": expected " << expected[0] << ',' << expected[1] << ','
                      << expected[2] << ',' << expected[3] << ", received " << actual[0] << ','
                      << actual[1] << ',' << actual[2] << ',' << actual[3] << '\n';
            require(false, description);
        }
    require(true, description);
}
std::shared_ptr<Mesh> plane(const Color& color) {
    return std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(1.6, 1.6),
                                  std::make_shared<MeshBasicMaterial>(color));
}
std::shared_ptr<MeshBasicMaterial> basic(const std::shared_ptr<Mesh>& mesh) {
    return std::dynamic_pointer_cast<MeshBasicMaterial>(mesh->material);
}
} // namespace

int main(int argc, char** argv) {
    try {
        const bool validation = argc > 1 && std::string(argv[1]) == "--validation";
        VulkanRenderer renderer({64, 64, validation});
        OrthographicCamera camera(-1, 1, 1, -1, 0.1, 20);
        camera.position.set(0, 0, 5);
        Scene scene;
        renderer.setClearColor(Color(0.5, 0.25, 0), 0.75);
        renderer.clear();
        expectPixel(renderer, {188, 137, 0, 191}, "Linear clear color is encoded as sRGB");
        renderer.setClearColor(Color(0, 0, 0));
        auto front = plane(Color(1, 0, 0));
        scene.add(front);
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 255}, "Front-facing indexed plane reaches the GPU");
        expectPixel(renderer, {0, 0, 0, 255}, "Pixels outside geometry retain clear color", 0, 0);
        require(renderer.getInfo().calls == 1 && renderer.getInfo().triangles == 2,
                "Draw statistics match indexed plane");
        basic(front)->side = BackSide;
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 0, 0, 255}, "BackSide culls a front-facing plane");
        basic(front)->side = DoubleSide;
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 255}, "DoubleSide renders both orientations");
        basic(front)->side = FrontSide;
        front->scale.set(-1, 1, 1);
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Mirrored world transforms preserve three.js front-side semantics");
        front->scale.set(1, 1, 1);
        front->position.set(0, 0, 1);
        auto rear = plane(Color(0, 0, 1));
        scene.add(rear);
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Depth attachment prevents a later farther draw from replacing the front mesh");
        basic(rear)->depthTest = false;
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 0, 255, 255}, "depthTest false permits the farther draw");
        basic(rear)->depthTest = true;
        basic(front)->depthWrite = false;
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 0, 255, 255},
                    "depthWrite false leaves depth available for subsequent geometry");
        basic(front)->depthWrite = true;
        basic(front)->transparent = true;
        basic(front)->opacity = 0.5;
        renderer.render(scene, camera);
        expectPixel(renderer, {128, 0, 127, 255},
                    "Transparent normal blending follows opaque draws");
        basic(front)->alphaTest = 0.6;
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 0, 255, 255}, "alphaTest discards fragments before blending");
        basic(front)->alphaTest = 0;
        basic(front)->transparent = false;
        basic(front)->opacity = 1;
        basic(front)->colorWrite = false;
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 0, 0, 255},
                    "colorWrite false still writes depth and occludes farther color");
        basic(front)->colorWrite = true;
        scene.remove(rear);
        basic(front)->color.set(0.5, 0.5, 0.5);
        renderer.render(scene, camera);
        expectPixel(renderer, {188, 188, 188, 255},
                    "MeshBasicMaterial linear color is encoded as sRGB");
        renderer.srgbOutput = false;
        renderer.render(scene, camera);
        expectPixel(renderer, {128, 128, 128, 255}, "Linear output bypasses the sRGB transfer");
        renderer.srgbOutput = true;
        basic(front)->color.set(1, 1, 1);
        basic(front)->vertexColors = true;
        front->geometry->setAttribute(
            "color",
            std::make_shared<Uint8BufferAttribute>(
                std::vector<std::uint8_t>{0, 255, 0, 0, 255, 0, 0, 255, 0, 0, 255, 0}, 3, true));
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 255, 0, 255},
                    "Normalized integer vertex colors are consumed by the graphics pipeline");
        basic(front)->vertexColors = false;
        basic(front)->color.set(1, 0, 0);
        front->geometry->setDrawRange(0, 0);
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 0, 0, 255}, "An empty drawRange emits no fragments");
        front->geometry->setDrawRange(0, 6);
        front->layers.set(1);
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 0, 0, 255}, "Camera layers filter mesh draws");
        front->layers.set(0);
        auto parent = std::make_shared<Group>();
        scene.remove(front);
        parent->add(front);
        scene.add(parent);
        parent->visible = false;
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 0, 0, 255}, "An invisible parent excludes its subtree");
        parent->visible = true;
        parent->layers.set(1);
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "A parent's unmatched layer still allows matching descendants");
        front->position.set(20, 0, 0);
        renderer.render(scene, camera);
        require(renderer.getInfo().calls == 0, "Frustum culling removes fully out-of-view meshes");
        front->frustumCulled = false;
        renderer.render(scene, camera);
        require(renderer.getInfo().calls == 1, "frustumCulled false preserves the draw submission");
        scene.clear();
        {
            auto projective = plane(Color(1, 0, 0));
            projective->matrixAutoUpdate = false;
            projective->matrix.makeTranslation(2.5, 0, 0);
            projective->matrix.elements()[15] = 2;
            scene.add(projective);
            renderer.render(scene, camera);
            require(renderer.getInfo().calls == 1,
                    "Projective world matrices transform the culling sphere with homogeneous division");
            expectPixel(renderer, {255, 0, 0, 255},
                        "Projective geometry remains visible at the right viewport edge", 61, 32);
            const auto culledPixels = renderer.readPixels();
            projective->frustumCulled = false;
            renderer.render(scene, camera);
            expectPixel(renderer, {255, 0, 0, 255},
                        "The projective geometry also renders with culling disabled", 61, 32);
            require(renderer.readPixels() == culledPixels,
                    "Culling preserves the complete projective geometry output");
            scene.clear();
        }
        auto fogged = plane(Color(1, 0, 0));
        scene.add(fogged);
        scene.fog = std::make_shared<Fog>(Color(0, 0, 1), 4, 6);
        renderer.render(scene, camera);
        expectPixel(renderer, {128, 0, 128, 255},
                    "Linear fog interpolates after output color conversion");
        scene.fog = std::make_shared<FogExp2>(Color(0, 0, 1), 0.2);
        renderer.render(scene, camera);
        expectPixel(renderer, {94, 0, 161, 255},
                    "Exponential squared fog uses interpolated view-space depth");
        basic(fogged)->fog = false;
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 255}, "material.fog false disables scene fog");
        scene.fog.reset();
        basic(fogged)->blending = NoBlending;
        basic(fogged)->opacity = 0.5;
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 128},
                    "NoBlending preserves opacity without the OPAQUE define");
        basic(fogged)->blending = NormalBlending;
        basic(fogged)->transparent = true;
        basic(fogged)->side = DoubleSide;
        const auto doubleSideVersion = basic(fogged)->version;
        renderer.render(scene, camera);
        require(renderer.getInfo().calls == 2,
                "Transparent DoubleSide defaults to back and front draws");
        require(basic(fogged)->version == doubleSideVersion + 2,
                "Transparent DoubleSide invalidates material state for both side passes");
        basic(fogged)->forceSinglePass = true;
        renderer.render(scene, camera);
        require(renderer.getInfo().calls == 1,
                "forceSinglePass disables the transparent DoubleSide second draw");
        require(basic(fogged)->version == doubleSideVersion + 2,
                "forceSinglePass does not invalidate material state for side passes");
        basic(fogged)->transparent = false;
        basic(fogged)->opacity = 1;
        basic(fogged)->wireframe = true;
        renderer.render(scene, camera);
        require(renderer.getInfo().calls == 1 && renderer.getInfo().lines == 6,
                "Wireframe expands indexed triangle edges into GPU lines");
        scene.clear();
        auto nearTransparent = plane(Color(1, 0, 0)), farTransparent = plane(Color(0, 0, 1));
        nearTransparent->position.set(0, 0, 1);
        for (const auto& mesh : {nearTransparent, farTransparent}) {
            basic(mesh)->transparent = true;
            basic(mesh)->opacity = 0.5;
            basic(mesh)->depthWrite = false;
            scene.add(mesh);
        }
        renderer.render(scene, camera);
        expectPixel(renderer, {128, 0, 64, 255},
                    "Transparent objects sort far to near independently of creation order");
        nearTransparent->renderOrder = -1;
        renderer.render(scene, camera);
        expectPixel(renderer, {64, 0, 128, 255},
                    "Explicit renderOrder precedes transparent depth sorting");
        scene.clear();
        auto morphGeometry = std::make_shared<PlaneGeometry>(0.4, 0.4);
        morphGeometry->morphTargetsRelative = true;
        morphGeometry->morphAttributes["position"] = {std::make_shared<Float32BufferAttribute>(
            std::vector<float>{0.8f, 0, 0, 0.8f, 0, 0, 0.8f, 0, 0, 0.8f, 0, 0}, 3)};
        auto morphed = std::make_shared<Mesh>(morphGeometry,
                                              std::make_shared<MeshBasicMaterial>(Color(1, 0, 0)));
        morphed->morphTargetInfluences->at(0) = 1;
        scene.add(morphed);
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 0, 0, 255},
                    "Morph target positions move fragments away from the undeformed position");
        expectPixel(renderer, {255, 0, 0, 255},
                    "Morph target influences update the GPU vertex positions", 57, 32);
        require(morphGeometry->boundingSphere.has_value(),
                "Renderer computes and retains geometry boundingSphere like three.js");
        morphGeometry->boundingSphere = Sphere(Vector3(30, 0, 0), 0.1);
        renderer.render(scene, camera);
        require(renderer.getInfo().calls == 0,
                "An explicit boundingSphere controls frustum culling");
        scene.clear();
        auto hooked = plane(Color(1, 0, 0));
        scene.add(hooked);
        std::vector<std::string> callbackOrder;
        scene.onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
            callbackOrder.push_back("scene-before");
        };
        hooked->onBeforeRender = [&](auto&, auto&, auto&, auto* geometry, auto* material,
                                     auto* group) {
            require(geometry == hooked->geometry.get() && material == hooked->material.get() &&
                        group == nullptr,
                    "Object hook receives render item geometry/material/group");
            callbackOrder.push_back("object-before");
        };
        basic(hooked)->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto* object, auto*) {
            require(std::abs(object->modelViewMatrix.elements()[14] + 5) < 1e-10,
                    "Material hook observes the updated model-view matrix");
            basic(hooked)->color.set(0, 1, 0);
            callbackOrder.push_back("material-before");
        };
        hooked->onAfterRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
            callbackOrder.push_back("object-after");
        };
        scene.onAfterRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
            callbackOrder.push_back("scene-after");
        };
        renderer.render(scene, camera);
        require(callbackOrder == std::vector<std::string>{"scene-before", "object-before",
                                                          "material-before", "object-after",
                                                          "scene-after"},
                "Render hooks follow three.js scene/object/material order");
        expectPixel(renderer, {0, 255, 0, 255},
                    "Material changes in a render hook reach that draw");
        hooked->onBeforeRender = [&](auto&, auto&, auto&, auto* geometry, auto*, auto*) {
            geometry->setDrawRange(0, 0);
        };
        renderer.render(scene, camera);
        require(renderer.getInfo().calls == 0,
                "Draw-range changes in onBeforeRender affect the current draw");
        scene.onBeforeRender = {};
        scene.onAfterRender = {};
        hooked->onBeforeRender = {};
        hooked->onAfterRender = {};
        basic(hooked)->onBeforeRender = {};
        hooked->geometry->setDrawRange(0, 6);
        basic(hooked)->alphaHash = true;
        bool rejected = false;
        try {
            renderer.render(scene, camera);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(
            rejected,
            "Unsupported alphaHash is rejected instead of silently rendering a different material");
        basic(hooked)->alphaHash = false;
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 255, 0, 255},
                    "Renderer remains usable after unsupported input rejection");
        scene.clear();
        {
            auto first =
                std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(0.8, 1.6),
                                       std::make_shared<MeshBasicMaterial>(Color(1, 0, 0)));
            auto second =
                std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(0.8, 1.6),
                                       std::make_shared<MeshBasicMaterial>(Color(0, 1, 0)));
            first->position.set(-0.5, 0, 0);
            second->position.set(0.5, 0, 0);
            bool secondHookCalled = false;
            first->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                second->layers.set(1);
            };
            second->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                secondHookCalled = true;
            };
            scene.add(first);
            scene.add(second);
            renderer.render(scene, camera);
            expectPixel(renderer, {255, 0, 0, 255}, "The first layer mutation draw remains visible",
                        16, 32);
            expectPixel(renderer, {0, 0, 0, 255},
                        "Layers are rechecked after earlier object callbacks", 48, 32);
            require(!secondHookCalled && renderer.getInfo().calls == 1,
                    "A queued object with newly unmatched layers skips hooks and draw");
            first->onBeforeRender = {};
            second->onBeforeRender = {};
            second->layers.set(0);
            auto originalOverride = std::make_shared<MeshBasicMaterial>(Color(0, 1, 0));
            auto laterOverride = std::make_shared<MeshBasicMaterial>(Color(0, 0, 1));
            scene.overrideMaterial = originalOverride;
            first->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                scene.overrideMaterial = laterOverride;
            };
            renderer.render(scene, camera);
            expectPixel(renderer, {0, 255, 0, 255},
                        "Override material is cached for the current opaque list", 48, 32);
            scene.overrideMaterial = originalOverride;
            basic(second)->transparent = true;
            renderer.render(scene, camera);
            expectPixel(renderer, {0, 255, 0, 255}, "The opaque list keeps its original override",
                        16, 32);
            expectPixel(renderer, {0, 0, 255, 255},
                        "The transparent list captures the later override", 48, 32);
            scene.overrideMaterial.reset();
            scene.clear();
        }
        {
            auto changingClear = plane(Color(1, 0, 0));
            scene.add(changingClear);
            changingClear->onBeforeRender = [](auto& activeRenderer, auto&, auto&, auto*, auto*,
                                               auto*) {
                activeRenderer.setClearColor(Color(0, 0, 1));
            };
            renderer.render(scene, camera);
            expectPixel(renderer, {0, 0, 0, 255},
                        "Object hooks change clear color after this frame's background clear", 0,
                        0);
            renderer.render(scene, camera);
            expectPixel(renderer, {0, 0, 255, 255},
                        "The changed clear color applies to the next frame", 0, 0);
            renderer.setClearColor(Color(0, 0, 0));
            scene.clear();
        }
        {
            auto clearing = plane(Color(1, 0, 0));
            auto farther = plane(Color(0, 1, 0));
            clearing->position.set(0, 0, 1);
            bool clearDepth = false;
            clearing->onBeforeRender = [&](auto& activeRenderer, auto&, auto&, auto*, auto*,
                                           auto*) {
                expectPixel(activeRenderer, {0, 0, 0, 255},
                            "Readback inside onBeforeRender sees the current background clear");
            };
            clearing->onAfterRender = [&](auto& activeRenderer, auto&, auto&, auto*, auto*, auto*) {
                expectPixel(activeRenderer, {255, 0, 0, 255},
                            "Readback inside onAfterRender sees the current draw");
                activeRenderer.setClearColor(Color(0, 0, 1));
                activeRenderer.clear(true, clearDepth);
                expectPixel(activeRenderer, {0, 0, 255, 255},
                            "Clear inside onAfterRender follows the current draw");
            };
            scene.add(clearing);
            scene.add(farther);
            renderer.render(scene, camera);
            expectPixel(renderer, {0, 0, 255, 255},
                        "Callback readback and color clear preserve depth for the next draw");
            renderer.setClearColor(Color(0, 0, 0));
            clearDepth = true;
            renderer.render(scene, camera);
            expectPixel(renderer, {0, 255, 0, 255},
                        "Callback depth clear allows the following farther draw");
            renderer.setClearColor(Color(0, 0, 0));
            scene.clear();
        }
        {
            auto masked = plane(Color(1, 0, 0));
            basic(masked)->colorWrite = false;
            masked->onAfterRender = [](auto& activeRenderer, auto&, auto&, auto*, auto*, auto*) {
                activeRenderer.setClearColor(Color(0, 0, 1));
                activeRenderer.clear(true, false);
            };
            scene.add(masked);
            renderer.render(scene, camera);
            expectPixel(renderer, {0, 0, 0, 255},
                        "Manual callback clear respects the last material's colorWrite mask");
            renderer.clear();
            expectPixel(renderer, {0, 0, 0, 255},
                        "Manual clear outside rendering also respects colorWrite");
            scene.clear();
            renderer.render(scene, camera);
            expectPixel(renderer, {0, 0, 255, 255},
                        "Automatic background clear restores color writes");
            renderer.setClearColor(Color(0, 0, 0));
        }
        {
            scene.background = Color(0, 0, 1);
            auto backgroundClear = plane(Color(1, 0, 0));
            backgroundClear->onAfterRender = [](auto& activeRenderer, auto&, auto&, auto*, auto*,
                                                auto*) { activeRenderer.clear(true, false); };
            scene.add(backgroundClear);
            renderer.render(scene, camera);
            expectPixel(renderer, {0, 0, 255, 255},
                        "Callback clear uses the active scene background clear color");
            require(renderer.getClearColor().equals(Color(0, 0, 0)),
                    "Scene background leaves the user's clear color unchanged");
            renderer.clear();
            expectPixel(renderer, {0, 0, 255, 255},
                        "Manual clear after rendering retains the active background clear state");
            scene.background.reset();
            scene.clear();
            renderer.render(scene, camera);
            expectPixel(
                renderer, {0, 0, 0, 255},
                "Removing scene background restores the user's clear color on the next frame");
        }
        {
            auto guarded = plane(Color(1, 0, 0));
            bool renderRejected = false, resizeRejected = false;
            guarded->onBeforeRender = [&](auto& activeRenderer, auto& activeScene,
                                          auto& activeCamera, auto*, auto*, auto*) {
                try {
                    activeRenderer.render(activeScene, activeCamera);
                } catch (const std::logic_error&) {
                    renderRejected = true;
                }
                try {
                    activeRenderer.setSize(8, 8);
                } catch (const std::logic_error&) {
                    resizeRejected = true;
                }
            };
            scene.add(guarded);
            renderer.render(scene, camera);
            require(
                renderRejected && resizeRejected,
                "Unsafe nested render and resize are explicitly rejected during active callbacks");
            expectPixel(renderer, {255, 0, 0, 255},
                        "The outer frame continues after caught callback-operation rejections");
            scene.clear();
        }
        std::weak_ptr<Mesh> detachedMesh;
        bool detachedBefore = false, detachedAfter = false;
        {
            auto transient = plane(Color(1, 0, 0));
            auto* pointer = transient.get();
            detachedMesh = transient;
            transient->onBeforeRender = [&, pointer](auto&, auto&, auto&, auto*, auto*, auto*) {
                detachedBefore = true;
                pointer->removeFromParent();
            };
            transient->onAfterRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                detachedAfter = true;
                require(!detachedMesh.expired(),
                        "A detached render item remains alive through onAfterRender");
            };
            scene.add(transient);
        }
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "A scene-owned mesh detached by onBeforeRender still draws this frame");
        require(detachedBefore && detachedAfter && detachedMesh.expired(),
                "Render item lifetime ends safely after both hooks and the frame");
        renderer.render(scene, camera);
        require(renderer.getInfo().calls == 0,
                "A detached scene-owned mesh is absent from the next frame");
        expectPixel(renderer, {0, 0, 0, 255}, "The next frame clears the detached mesh's pixels");

        auto managedScene = std::make_shared<Scene>();
        std::weak_ptr<Scene> releasedScene = managedScene;
        std::weak_ptr<Camera> releasedCamera;
        std::weak_ptr<Mesh> releasedFirst, releasedSecond;
        Camera* managedCameraPointer = nullptr;
        bool afterReleasedScene = false;
        {
            auto managedCamera = std::make_shared<OrthographicCamera>(-1, 1, 1, -1, 0.1, 20);
            managedCamera->position.set(0, 0, 5);
            managedCameraPointer = managedCamera.get();
            releasedCamera = managedCamera;
            managedScene->add(managedCamera);
            auto first =
                std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(0.8, 1.6),
                                       std::make_shared<MeshBasicMaterial>(Color(1, 0, 0)));
            auto second =
                std::make_shared<Mesh>(std::make_shared<PlaneGeometry>(0.8, 1.6),
                                       std::make_shared<MeshBasicMaterial>(Color(0, 1, 0)));
            first->position.set(-0.5, 0, 0);
            second->position.set(0.5, 0, 0);
            releasedFirst = first;
            releasedSecond = second;
            first->onBeforeRender = [&](auto&, auto& activeScene, auto&, auto*, auto*, auto*) {
                activeScene.clear();
                managedScene.reset();
            };
            second->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                require(
                    !releasedFirst.expired() && !releasedSecond.expired() &&
                        !releasedCamera.expired() && !releasedScene.expired(),
                    "Clearing the scene retains queued meshes, camera and scene during rendering");
            };
            managedScene->onAfterRender = [&](auto&, auto&, auto&, auto*, auto*, auto*) {
                afterReleasedScene = true;
                require(!releasedScene.expired() && !releasedCamera.expired(),
                        "Managed scene and camera survive through the scene's final hook");
            };
            managedScene->add(first);
            managedScene->add(second);
        }
        renderer.render(*managedScene, *managedCameraPointer);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Clearing scene ownership during a hook preserves the first queued draw", 16,
                    32);
        expectPixel(renderer, {0, 255, 0, 255},
                    "Clearing scene ownership during a hook preserves subsequent queued draws", 48,
                    32);
        require(afterReleasedScene && releasedScene.expired() && releasedCamera.expired() &&
                    releasedFirst.expired() && releasedSecond.expired(),
                "Managed render objects are released after the frame without ownership cycles");
        auto square = std::make_shared<BufferGeometry>();
        square->setAttribute(
            "position",
            std::make_shared<Float32BufferAttribute>(
                std::vector<float>{-.8f, -.8f, 0, .8f, -.8f, 0, .8f, .8f, 0, -.8f, .8f, 0}, 3));
        square->setIndex(std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3});
        square->addGroup(0, 3, 0);
        square->addGroup(3, 3, 1);
        auto red = std::make_shared<MeshBasicMaterial>(Color(1, 0, 0)),
             green = std::make_shared<MeshBasicMaterial>(Color(0, 1, 0));
        auto groupedMesh =
            std::make_shared<Mesh>(square, std::vector<std::shared_ptr<Material>>{red, green});
        scene.add(groupedMesh);
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 255}, "Geometry group zero selects material zero", 48,
                    48);
        expectPixel(renderer, {0, 255, 0, 255}, "Geometry group one selects material one", 16, 16);
        groupedMesh->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto* group) {
            if (group->materialIndex == 0) {
                require(group == &square->groups[0],
                        "Render callbacks receive the original geometry group object");
                group->count = 0;
            }
        };
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 0, 0, 255},
                    "Changing the original group count in a hook changes this frame's draw", 48,
                    48);
        expectPixel(renderer, {0, 255, 0, 255},
                    "Mutating one queued group leaves the other group visible", 16, 16);
        require(renderer.getInfo().calls == 1, "The mutated zero-count group skips only its draw");
        square->groups[0].count = 3;
        std::weak_ptr<BufferGeometry::Group> queuedFirst = square->groups.owner(square->groups[0]);
        std::weak_ptr<BufferGeometry::Group> queuedSecond = square->groups.owner(square->groups[1]);
        const auto* queuedSecondPointer = &square->groups[1];
        groupedMesh->onBeforeRender = [&](auto&, auto&, auto&, auto*, auto*, auto* group) {
            if (group->materialIndex == 0) {
                square->clearGroups();
                require(!queuedFirst.expired() && !queuedSecond.expired(),
                        "Clearing geometry groups retains both queued group objects");
            } else {
                require(group == queuedSecondPointer && group->count == 3,
                        "A later callback receives the original group after clearGroups");
            }
        };
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Clearing groups in a hook preserves the current queued group draw", 48, 48);
        expectPixel(renderer, {0, 255, 0, 255},
                    "Clearing groups in a hook preserves a later queued group draw", 16, 16);
        require(queuedFirst.expired() && queuedSecond.expired(),
                "Cleared groups are released when this frame's render items expire");
        renderer.render(scene, camera);
        require(renderer.getInfo().calls == 0,
                "Cleared geometry groups are absent from the next frame");
        expectPixel(renderer, {0, 0, 0, 255},
                    "The next frame clears pixels from removed geometry groups");
        groupedMesh->onBeforeRender = {};
        square->addGroup(0, 3, 0);
        square->addGroup(3, 3, 1);
        square->setIndex(std::make_shared<Uint16BufferAttribute>(
            std::vector<std::uint16_t>{0, 1, 2, 0, 2, 3}, 1, true));
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Index fetching ignores the vertex attribute normalized flag", 48, 48);
        square->setIndex(
            std::make_shared<Int32BufferAttribute>(std::vector<std::int32_t>{0, 1, 2, 0, 2, 3}, 1));
        rejected = false;
        try {
            renderer.render(scene, camera);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "Signed integer index types are rejected instead of silently converted");
        square->setIndex(std::vector<std::uint32_t>{0, 1, 2, 0, 2, 3});
        square->setDrawRange(3, 3);
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 0, 0, 255}, "drawRange intersects geometry groups", 48, 48);
        expectPixel(renderer, {0, 255, 0, 255}, "The surviving group retains its material", 16, 16);
        scene.overrideMaterial = std::make_shared<MeshBasicMaterial>(Color(0, 0, 1));
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 0, 255, 255}, "Scene overrideMaterial replaces a group material",
                    16, 16);
        scene.overrideMaterial.reset();
        scene.background = Color(1, 0, 1);
        renderer.autoClear = false;
        scene.clear();
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 255, 255},
                    "A solid scene background forces color clear with autoClear false");
        scene.background.reset();
        renderer.autoClear = true;
        scene.add(plane(Color(1, 0, 0)));
        camera.coordinateSystem = WebGPUCoordinateSystem;
        camera.updateProjectionMatrix();
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 255}, "WebGPU zero-to-one projection depth is preserved");
        camera.reversedDepth = true;
        camera.updateProjectionMatrix();
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Reversed projection uses reversed depth clear and comparison");
        camera.coordinateSystem = WebGLCoordinateSystem;
        camera.updateProjectionMatrix();
        auto depthProbe = std::dynamic_pointer_cast<Mesh>(scene.children[0]);
        depthProbe->frustumCulled = false;
        depthProbe->position.set(0, 0, -17);
        renderer.render(scene, camera);
        expectPixel(
            renderer, {0, 0, 0, 255},
            "Default WebGL reversed projection clips beyond far without an extra depth remap");
        depthProbe->position.set(0, 0, 4.95);
        renderer.render(scene, camera);
        expectPixel(renderer, {0, 0, 0, 255},
                    "Default WebGL reversed projection clips before near");
        depthProbe->position.set(0, 0, 4.8);
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Default WebGL reversed projection preserves geometry just beyond near");
        depthProbe->position.set(0, 0, -14.8);
        renderer.render(scene, camera);
        expectPixel(renderer, {255, 0, 0, 255},
                    "Default WebGL reversed projection preserves geometry just before far");
        depthProbe->frustumCulled = true;
        depthProbe->position.set(0, 0, -17);
        renderer.render(scene, camera);
        require(renderer.getInfo().calls == 0,
                "Default WebGL reversed projection extracts the zero-to-one far frustum plane");
        depthProbe->position.set(0, 0, 0);
        renderer.setSize(32, 48);
        renderer.render(scene, camera);
        require(renderer.readPixels().size() == 32 * 48 * 4,
                "Resize recreates color, depth, framebuffer and readback resources");
        expectPixel(renderer, {255, 0, 0, 255}, "Resized framebuffer receives geometry", 16, 24);
        for (const auto& message : renderer.getValidationMessages())
            std::cerr << message << '\n';
        require(renderer.getValidationMessages().empty(),
                "Vulkan validation emitted a warning or error");
        std::cout << "GPU: " << renderer.getInfo().deviceName << "; " << assertions
                  << " assertions passed; validation=" << renderer.getInfo().validationEnabled
                  << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Vulkan renderer test failed: " << error.what() << '\n';
        return 1;
    }
}
