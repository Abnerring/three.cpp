#pragma once
#include "cameras/Camera.h"
#include "core/JSValue.h"
#include "math/Frustum.h"
#include "math/Vector2.h"
#include "math/Vector4.h"
#include <memory>
#include <vector>

class Light;
class RenderTarget;
class LightShadow {
  public:
    std::shared_ptr<Camera> camera;
    double intensity = 1, bias = 0;
    JSValue biasNode{nullptr};
    double normalBias = 0, radius = 1, blurSamples = 8;
    Vector2 mapSize{512, 512};
    double mapType = UnsignedByteType;
    std::shared_ptr<RenderTarget> map, mapPass;
    Matrix4 matrix;
    bool autoUpdate = true, needsUpdate = false;
    Frustum _frustum;
    Vector2 _frameExtents{1, 1};
    double _viewportCount = 1;
    std::vector<Vector4> _viewports{Vector4(0, 0, 1, 1)};
    explicit LightShadow(std::shared_ptr<Camera> camera = nullptr);
    virtual ~LightShadow() = default;
    double getViewportCount() const { return _viewportCount; }
    virtual std::shared_ptr<Camera> getCamera(std::size_t viewportIndex = 0) const;
    Frustum& getFrustum() { return _frustum; }
    virtual void updateMatrices(Light& light);
    void _updateMatrix(Camera& shadowCamera, Matrix4& shadowMatrix, Frustum& frustum, const Vector4* viewport = nullptr);
    Vector4& getViewport(std::size_t viewportIndex) { return _viewports.at(viewportIndex); }
    Vector2& getFrameExtents() { return _frameExtents; }
    virtual void dispose();
    virtual LightShadow& copy(const LightShadow& source);
    virtual std::shared_ptr<LightShadow> clone() const;
    virtual JsonValue toJSON() const;
};
