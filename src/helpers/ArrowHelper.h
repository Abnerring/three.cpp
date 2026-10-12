#pragma once
#include "helpers/HelperUtils.h"
class ArrowHelper : public Object3D {
  public:
    std::shared_ptr<Line> line;
    std::shared_ptr<Mesh> cone;
    ArrowHelper(const Vector3& dir=Vector3(0,0,1),const Vector3& origin=Vector3(),double length=1,HelperColor color=0xffff00,std::optional<double> headLength={},std::optional<double> headWidth={});
    void setDirection(const Vector3& dir);
    void setLength(double length,std::optional<double> headLength={},std::optional<double> headWidth={});
    void setColor(HelperColor color);
    ArrowHelper& copy(const Object3D& source,bool recursive=true) override;
    std::shared_ptr<Object3D> clone(bool recursive=true) const override;
    void dispose() override;
};
