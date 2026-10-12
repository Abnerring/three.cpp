#pragma once
#include "core/Object3D.h"

class Group : public Object3D {
  public:
    Group() {
        isGroup = true;
        type = "Group";
    }
    std::shared_ptr<Object3D> clone(bool recursive = true) const override {
        auto group = std::make_shared<Group>();
        group->copy(*this, recursive);
        return group;
    }

  protected:
    std::optional<JsonValue> propertyValue(const std::string& property) const override {
        if (property == "isGroup")
            return isGroup;
        return Object3D::propertyValue(property);
    }
};
