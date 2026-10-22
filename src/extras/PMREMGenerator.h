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
    std::shared_ptr<RenderTarget> _sourceTarget;
    std::shared_ptr<CubeCamera> _cubeCamera;
    std::shared_ptr<Mesh> _boxMesh;

  private:
  public:
    // Named like the upstream private helpers; public here so the rules are testable.
    void _setSize(double cubeSize);
    // Reuses the cached source target unless the size changed, or a depth buffer is
    // requested and the cached one has none. Note the asymmetry upstream has: a cached
    // target with a depth buffer is kept even when the caller no longer needs one.
    std::shared_ptr<RenderTarget> _getSourceTarget(bool depthBuffer = false);
    // Picks the cube size from the source texture (cube face width, or equirectangular
    // width over four), then allocates the target before the GPU work upstream does.
    std::shared_ptr<RenderTarget> _fromTexture(const Texture& texture,
                                               const std::shared_ptr<RenderTarget>& renderTarget = {});
    static std::shared_ptr<RenderTarget> _createRenderTarget(double size, bool generateMipmaps,
                                                             bool depthBuffer);

  private:
    std::vector<std::uint32_t> _lodSizes;
    [[noreturn]] static void _needsRenderer(const char* what);
};
