#pragma once
#include "helpers/HelperUtils.h"
#include "objects/LineSegments.h"
class BoxHelper : public LineSegments {
  public:
    std::shared_ptr<Object3D> object;
    explicit BoxHelper(std::shared_ptr<Object3D> object=nullptr,HelperColor color=0xffff00);
    virtual void update();
    BoxHelper& setFromObject(std::shared_ptr<Object3D> object);
    BoxHelper& copy(const Object3D& source,bool recursive=true) override;
    std::shared_ptr<Object3D> clone(bool recursive=true) const override;
    void dispose() override;
};
