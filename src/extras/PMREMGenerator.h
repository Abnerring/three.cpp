#pragma once
#include "core/JSValue.h"
#include "math/Vector3.h"
#include <cstdint>
#include <memory>
#include <vector>

class CubeCamera;
class Mesh;
class RenderTarget;
class Texture;

// Upstream PMREMGenerator (r187). The size derivation and the prefiltered mip
// chain layout are portable and implemented here; every entry point that needs
// the renderer's shader passes throws instead of pretending to produce a PMREM.
class PMREMGenerator {
  public:
    // Smaller inputs are upsampled so that every PMREM has enough mip levels.
    static constexpr double MIN_SIZE = 256;
    // Log2 of the face size of the roughest mip level.
    static constexpr double LOD_MIN = 3;

    explicit PMREMGenerator(JSValue renderer = {});
    ~PMREMGenerator();

    // Allocating a target lays out one mip per lod, each half the previous size.
    std::shared_ptr<RenderTarget> allocateTarget();
    double cubeSize() const { return _cubeSize; }
    const std::vector<std::uint32_t>& lodSizes() const { return _lodSizes; }

    std::shared_ptr<RenderTarget> fromScene(JSValue scene, double sigma = 0, double near = 0.1,
                                            double far = 100, JSValue options = JSValue::object());
    std::shared_ptr<RenderTarget> fromEquirectangular(JSValue equirectangular,
                                                      JSValue renderTarget = {});
    std::shared_ptr<RenderTarget> fromCubemap(JSValue cubemap, JSValue renderTarget = {});
    void compileCubemapShader();
    void compileEquirectangularShader();
    void dispose();

    JSValue _renderer;
    double _cubeSize = 0;
    JSValue _sourceTarget;
    std::shared_ptr<CubeCamera> _cubeCamera;
    std::shared_ptr<Mesh> _boxMesh;

  private:
  public:
    // Named like the upstream private helper; public here so the derivation is testable.
    void _setSize(double cubeSize);

  private:
    std::vector<std::uint32_t> _lodSizes;
    [[noreturn]] static void _needsRenderer(const char* what);
};
