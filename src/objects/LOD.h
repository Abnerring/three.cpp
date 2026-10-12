#pragma once
#include "core/Object3D.h"

class Camera;

class LOD : public Object3D {
    JSValue levels_ = JSValue::array();
  public:
    // The binding is read-only, while its array and level records remain mutable.
    // Copy the handle to mutate indices: auto array = lod.levels; array.set(...).
    const JSValue& levels = levels_;
    JSValue _currentLevel = 0, autoUpdate = true;
    LOD();
    virtual LOD& addLevel(JSValue object, JSValue distance = 0, JSValue hysteresis = 0);
    LOD& addLevel(const std::shared_ptr<Object3D>& object, JSValue distance = 0, JSValue hysteresis = 0);
    virtual bool removeLevel(JSValue distance = {});
    JSValue getCurrentLevel() const { return _currentLevel; }
    virtual JSValue getObjectForDistance(JSValue distance = {}) const;
    virtual void update(Camera& camera);
    void update(JSValue camera);
    bool raycast(const Raycaster& raycaster, std::vector<Intersection>& intersects) override;
    void raycast(JSValue raycaster, JSValue intersects);
    LOD& copy(const Object3D& source, bool recursive = true) override;
    std::shared_ptr<Object3D> clone(bool recursive = true) const override;
    using Object3D::toJSON;
    JsonValue toJSON(Object3DJSONContext* meta = nullptr) const override;
    std::vector<const JSValue*> runtimeRoots() const { return {&levels_, &_currentLevel, &autoUpdate}; }

  protected:
    std::optional<JsonValue> propertyValue(const std::string& property) const override;

  private:
    void updateDistance(double distance);
};
