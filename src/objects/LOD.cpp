#include "objects/LOD.h"
#include "core/JSOperators.h"
#include "core/NativeObject.h"
#include "core/Raycaster.h"
#include "cameras/PerspectiveCamera.h"
#include "cameras/OrthographicCamera.h"
#include <cmath>
#include <limits>

namespace {
thread_local auto v1 = std::make_shared<Vector3>();
thread_local auto v2 = std::make_shared<Vector3>();
JSValue at(const JSValue& array, double index) { return array.get(JSValue(index).toString()); }
std::shared_ptr<Object3D> object(const JSValue& value) {
    return std::static_pointer_cast<Object3D>(value.hostPayload("three.Object3D"));
}
JSValue cloneObject(const JSValue& value) {
    if (value.isHostObject("three.Object3D") && !value.has("clone")) return nativeObject(object(value)->clone());
    const auto method = value.get("clone"); return method.call(value);
}
JSValue threshold(const JSValue& levels, double index) {
    auto distance = at(levels,index).get("distance");
    if (at(levels,index).get("object").get("visible").toBoolean()) {
        const auto hysteresis = at(levels,index).get("hysteresis");
        const auto left = distance.toNumber(); const auto right = hysteresis.toNumber();
        const auto product = left * right;
        distance = distance.toNumber() - product;
    }
    return distance;
}
void setVisibility(const JSValue& levels, double index, bool value) {
    auto selected = at(levels,index).get("object"); selected.set("visible",value);
}
void matrixPosition(Vector3& target, const JSValue& matrix) {
    const auto elements = matrix.get("elements");
    target.x = elements.get("12").toNumber(); target.y = elements.get("13").toNumber(); target.z = elements.get("14").toNumber();
}
}

LOD::LOD() { isLOD = true; type = "LOD"; }
LOD& LOD::addLevel(const std::shared_ptr<Object3D>& child, JSValue distance, JSValue hysteresis) {
    return addLevel(nativeObject(child),distance,hysteresis);
}
LOD& LOD::addLevel(JSValue child, JSValue distance, JSValue hysteresis) {
    if (distance.isUndefined()) distance = 0;
    if (hysteresis.isUndefined()) hysteresis = 0;
    distance = std::abs(distance.toNumber());
    const auto array = levels_;
    double index = 0;
    for (; JSOperators::less(index,array.get("length")); ++index)
        if (JSOperators::less(distance,at(array,index).get("distance"))) break;
    const auto splice = array.get("splice");
    auto level = JSValue::object(); level.set("distance",distance); level.set("hysteresis",hysteresis); level.set("object",child);
    splice.call(array,{index,0,level});
    // Object3D's existing native ownership/error policy applies after insertion.
    add(object(child));
    return *this;
}
bool LOD::removeLevel(JSValue distance) {
    const auto array = levels_;
    for (double i = 0; JSOperators::less(i,array.get("length")); ++i) {
        if (at(array,i).get("distance").strictEquals(distance)) {
            const auto splice = array.get("splice"), removed = splice.call(array,{i,1});
            const auto child = removed.get("0").get("object");
            if (child.isHostObject("three.Object3D")) remove(object(child));
            return true;
        }
    }
    return false;
}
JSValue LOD::getObjectForDistance(JSValue distance) const {
    const auto array = levels_;
    if (JSOperators::greater(array.get("length"),0)) {
        const auto length = array.get("length"); double i = 1;
        for (; JSOperators::less(i,length); ++i)
            if (JSOperators::less(distance,threshold(array,i))) break;
        return at(array,i-1).get("object");
    }
    return nullptr;
}
void LOD::updateDistance(double distance) {
    const auto array = levels_;
    setVisibility(array,0,true);
    const auto length = array.get("length"); double i = 1;
    for (; JSOperators::less(i,length); ++i) {
        if (JSOperators::greaterEqual(distance,threshold(array,i))) {
            setVisibility(array,i-1,false); setVisibility(array,i,true);
        } else break;
    }
    _currentLevel = i-1;
    for (; JSOperators::less(i,length); ++i) setVisibility(array,i,false);
}
void LOD::update(Camera& camera) {
    if (!JSOperators::greater(levels_.get("length"),1)) return;
    v1->setFromMatrixPosition(camera.matrixWorld); v2->setFromMatrixPosition(matrixWorld);
    double zoom = std::numeric_limits<double>::quiet_NaN();
    if (const auto* perspective = dynamic_cast<PerspectiveCamera*>(&camera)) zoom = perspective->zoom;
    else if (const auto* orthographic = dynamic_cast<OrthographicCamera*>(&camera)) zoom = orthographic->zoom;
    const auto distance = v1->distanceTo(*v2) / zoom; updateDistance(distance);
}
void LOD::update(JSValue camera) {
    if (!JSOperators::greater(levels_.get("length"),1)) return;
    matrixPosition(*v1,camera.get("matrixWorld")); v2->setFromMatrixPosition(matrixWorld);
    const auto distance = v1->distanceTo(*v2); const auto zoom = camera.get("zoom");
    updateDistance(distance / zoom.toNumber());
}
bool LOD::raycast(const Raycaster& raycaster, std::vector<Intersection>& intersects) {
    if (JSOperators::greater(levels_.get("length"),0)) {
        v1->setFromMatrixPosition(matrixWorld);
        const auto distance = raycaster.ray.origin.distanceTo(*v1); const auto selected = getObjectForDistance(distance);
        if (selected.isObject() && selected.has("raycast"))
            throw JSTypeError("LOD native raycast requires native intersection storage; use its managed overload for dynamic raycast callbacks");
        const auto child = object(selected);
        if (!child) throw JSTypeError("LOD selected object has no native raycast method");
        child->raycast(raycaster,intersects);
    }
    return true;
}
void LOD::raycast(JSValue raycaster, JSValue intersects) {
    if (JSOperators::greater(levels_.get("length"),0)) {
        v1->setFromMatrixPosition(matrixWorld);
        const auto origin = raycaster.get("ray").get("origin"), method = origin.get("distanceTo");
        const auto distance = method.call(origin,{nativeVector(v1)});
        const auto selected = getObjectForDistance(distance), raycast = selected.get("raycast");
        raycast.call(selected,{raycaster,intersects});
    }
}
LOD& LOD::copy(const Object3D& source, bool) {
    Object3D::copy(source,false);
    const auto* lod = dynamic_cast<const LOD*>(&source);
    if (!lod) throw JSTypeError("LOD.copy source has no levels array");
    const auto array = lod->levels_, length = array.get("length");
    for (double i = 0; JSOperators::less(i,length); ++i) {
        const auto level = at(array,i), child = cloneObject(level.get("object"));
        const auto distance = level.get("distance"), hysteresis = level.get("hysteresis");
        addLevel(child,distance,hysteresis);
    }
    autoUpdate = lod->autoUpdate; return *this;
}
std::shared_ptr<Object3D> LOD::clone(bool) const {
    auto result = std::make_shared<LOD>(); result->copy(*this); return result;
}
JsonValue LOD::toJSON(Object3DJSONContext* meta) const {
    auto data = Object3D::toJSON(meta); auto& output = data["object"];
    if (const auto value = autoUpdate.toJSON()) output["autoUpdate"] = *value;
    output["levels"] = JsonValue::Array{};
    const auto array = levels_, length = array.get("length");
    for (double i = 0; JSOperators::less(i,length); ++i) {
        const auto level = at(array,i);
        auto record = JSValue::object(); record.set("object",level.get("object").get("uuid"));
        record.set("distance",level.get("distance")); record.set("hysteresis",level.get("hysteresis"));
        output["levels"].get<JsonValue::Array>().push_back(record.toJSON().value());
    }
    return data;
}
std::optional<JsonValue> LOD::propertyValue(const std::string& property) const {
    if (property == "isLOD") return isLOD;
    if (property == "autoUpdate") return autoUpdate.toJSON();
    if (property == "_currentLevel") return _currentLevel.toJSON();
    return Object3D::propertyValue(property);
}
