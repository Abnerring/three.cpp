#pragma once
#include "helpers/HelperUtils.h"
#include "objects/LineSegments.h"
#include "cameras/Camera.h"
class CameraHelper : public LineSegments {
  public:
    std::shared_ptr<Camera> camera;
    JSValue pointMap = JSValue::object();
    explicit CameraHelper(std::shared_ptr<Camera> camera);
    CameraHelper& setColors(const Color& frustum,const Color& cone,const Color& up,const Color& target,const Color& cross);
    virtual void update();
    std::shared_ptr<Object3D> clone(bool recursive=true) const override;
    void dispose() override;
  private:
    struct Data { std::shared_ptr<BufferGeometry> geometry;JSValue pointMap = JSValue::object(); };
    static Data createData();
    CameraHelper(std::shared_ptr<Camera> camera,Data data);
};
