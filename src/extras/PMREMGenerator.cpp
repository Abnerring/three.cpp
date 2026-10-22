#include "extras/PMREMGenerator.h"
#include "cameras/CubeCamera.h"
#include "core/RenderTarget.h"
#include "core/WebGLCubeRenderTarget.h"
#include "constants.h"
#include "geometries/BoxGeometry.h"
#include "math/MathUtils.h"
#include "objects/Mesh.h"
#include "textures/Texture.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

PMREMGenerator::PMREMGenerator(JSValue renderer) : _renderer(std::move(renderer)) {
    _cubeSize = 0;
    _sourceTarget = nullptr;
    _cubeCamera = std::make_shared<CubeCamera>(1, 10, JSValue(nullptr));
    _boxMesh = std::make_shared<Mesh>(std::make_shared<BoxGeometry>(5, 5, 5), nullptr);
}

PMREMGenerator::~PMREMGenerator() = default;

void PMREMGenerator::_setSize(double cubeSize) {
    _cubeSize = std::max(MIN_SIZE, MathUtils::floorPowerOfTwo(cubeSize));
}

void PMREMGenerator::_needsRenderer(const char* what) {
    throw std::logic_error(std::string("PMREMGenerator::") + what +
                           " needs the renderer's PMREM shader passes, which are not implemented yet");
}

std::shared_ptr<RenderTarget> PMREMGenerator::_createRenderTarget(double size, bool generateMipmaps,
                                                                  bool depthBuffer) {
    // Upstream options, in the same order: minFilter, generateMipmaps, type, colorSpace,
    // depthBuffer. The cube render target applies them to its texture.
    auto options = JSValue::object();
    options.set("minFilter", LinearMipmapLinearFilter);
    options.set("generateMipmaps", generateMipmaps);
    options.set("type", HalfFloatType);
    options.set("colorSpace", LinearSRGBColorSpace);
    options.set("depthBuffer", depthBuffer);
    return std::make_shared<WebGLCubeRenderTarget>(size, options);
}

std::shared_ptr<RenderTarget> PMREMGenerator::_getSourceTarget(bool depthBuffer) {
    const auto size = _cubeSize;
    const auto sourceTarget = _sourceTarget;
    if (sourceTarget == nullptr || sourceTarget->width != size ||
        (depthBuffer && !sourceTarget->depthBuffer)) {
        _sourceTarget = _createRenderTarget(size, true, depthBuffer);
    }
    return _sourceTarget;
}

std::shared_ptr<RenderTarget> PMREMGenerator::_fromTexture(
    const Texture& texture, const std::shared_ptr<RenderTarget>& renderTarget) {
    const auto mapping = texture.mapping;
    if (mapping == CubeReflectionMapping || mapping == CubeRefractionMapping) {
        const auto& faces = texture.image.get()->faces;
        // Upstream falls back to image[0].image.width for nested images; this model keeps
        // the width on the descriptor itself, so that fallback has no reachable case.
        _setSize(faces.empty() ? MIN_SIZE : faces.front()->width);
    } else {  // Equirectangular
        _setSize(texture.image.get()->width / 4.0);
    }
    auto pmremTarget = renderTarget ? renderTarget : allocateTarget();
    (void) pmremTarget;
    _needsRenderer("_fromTexture");
}

std::shared_ptr<RenderTarget> PMREMGenerator::allocateTarget() {
    const auto size = static_cast<std::uint32_t>(_cubeSize);
    auto pmremTarget = std::make_shared<RenderTarget>(size, size);
    // Only the prefiltered mip levels are allocated, without automatic generation.
    const auto maxLod = static_cast<int>(std::log2(size) - LOD_MIN);
    _lodSizes.clear();
    pmremTarget->texture.get()->mipmaps->clear();
    for (int lod = 0; lod <= maxLod; ++lod) {
        const auto face = static_cast<std::uint32_t>(size >> lod);
        _lodSizes.push_back(face);
        auto level = std::make_shared<TextureImage>();
        level->width = face;
        level->height = face;
        pmremTarget->texture.get()->mipmaps->push_back(std::move(level));
    }
    pmremTarget->texture.get()->name = "PMREM";
    pmremTarget->texture.get()->isPMREMTexture = true;
    return pmremTarget;
}

std::shared_ptr<RenderTarget> PMREMGenerator::fromScene(JSValue scene, double sigma, double near,
                                                        double far, JSValue options) {
    // Upstream destructures size (default 256) and position (default origin) first, then
    // derives the cube size and allocates the target before any rendering happens.
    const auto requested = options.get("size");
    _setSize(requested.isUndefined() ? 256 : requested.toNumber());
    auto pmremTarget = allocateTarget();
    (void) pmremTarget;
    (void) scene;
    (void) sigma;
    (void) near;
    (void) far;
    _needsRenderer("fromScene");
}
std::shared_ptr<RenderTarget> PMREMGenerator::fromEquirectangular(JSValue, JSValue) {
    _needsRenderer("fromEquirectangular");
}
std::shared_ptr<RenderTarget> PMREMGenerator::fromCubemap(JSValue, JSValue) {
    _needsRenderer("fromCubemap");
}
void PMREMGenerator::compileCubemapShader() { _needsRenderer("compileCubemapShader"); }
void PMREMGenerator::compileEquirectangularShader() {
    _needsRenderer("compileEquirectangularShader");
}
void PMREMGenerator::dispose() {
    _cubeSize = 0;
    _sourceTarget = nullptr;
    _lodSizes.clear();
}
