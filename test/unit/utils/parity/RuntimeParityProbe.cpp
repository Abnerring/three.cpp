#include "Three.Core.h"
#include <iostream>
#include <limits>

namespace {
JSValue vector(const std::vector<double>& values) {
    auto result = JSValue::array();
    for (std::size_t i = 0; i < values.size(); ++i) result.set(std::to_string(i), values[i]);
    return result;
}
}

int main() {
    JsonValue output = JsonValue::Object{};
    const auto emit = [&](const std::string& name, const JSValue& value) { output[name] = value.trace(); };
    emit("undefined", JSValue{});
    emit("null", nullptr);
    emit("negativeZero", -0.0);
    emit("nan", std::numeric_limits<double>::quiet_NaN());
    emit("positiveInfinity", std::numeric_limits<double>::infinity());
    emit("negativeInfinity", -std::numeric_limits<double>::infinity());
    auto object = JSValue::object();
    object.set("z", 1); object.set("10", 10); object.set("2", 2); object.set("01", 1);
    object.set("a", JSValue{});
    auto alias = object;
    alias.set("z", 8);
    emit("object", object);
    auto relations = JSValue::array();
    relations.set("0", alias.strictEquals(object));
    relations.set("1", object.strictEquals(JSValue::object()));
    relations.set("2", object.hasOwn("a"));
    relations.set("3", object.hasOwn("absent"));
    relations.set("4", JSValue(-0.0).strictEquals(0.0));
    relations.set("5", JSValue(std::numeric_limits<double>::quiet_NaN()).strictEquals(std::numeric_limits<double>::quiet_NaN()));
    relations.set("6", JSValue(1).strictEquals("1"));
    emit("relations", relations);
    emit("missingProperty", object.get("absent"));
    object.erase("z"); object.set("z", 3);
    emit("reinserted", object);
    auto sparse = JSValue::array(4);
    sparse.set("1", JSValue{}); sparse.set("3", nullptr); sparse.set("tag", "present");
    emit("sparse", sparse);
    sparse.set("length", 2);
    emit("truncated", sparse);
    sparse.set("5", 5); sparse.set("4294967295", "string key");
    emit("expanded", sparse);
    auto graph = JSValue::object(), child = JSValue::object();
    child.set("value", 7); graph.set("first", child); graph.set("second", child);
    child.set("parent", graph);
    emit("cyclicAliases", graph);
    const auto errorName = [](const auto& action) -> JSValue {
        try { action(); } catch (const JSTypeError&) { return "TypeError"; }
        catch (const JSRangeError&) { return "RangeError"; }
        return "no error";
    };
    auto errors = JSValue::array();
    errors.set("0", errorName([] { JSValue{}.get("x"); }));
    errors.set("1", errorName([] { JSValue(nullptr).get("x"); }));
    errors.set("2", errorName([&] { sparse.set("length", -1); }));
    errors.set("3", errorName([&] { sparse.set("length", 1.5); }));
    errors.set("4", errorName([&] { sparse.set("length", std::numeric_limits<double>::infinity()); }));
    errors.set("5", errorName([&] { graph.toJSON(); }));
    emit("errorClasses", errors);
    auto jsonData = JSValue::object();
    jsonData.set("z", 1); jsonData.set("2", 2); jsonData.set("undefined", JSValue{});
    jsonData.set("nan", std::numeric_limits<double>::quiet_NaN());
    auto jsonArray = JSValue::array(3); jsonArray.set("1", JSValue{}); jsonArray.set("2", nullptr);
    jsonData.set("array", jsonArray);
    emit("serialized", jsonData.toJSON().value().dump());

    Vector3 v(1, 2, 3); v.x = -0.0; v.y = 8;
    auto directVector = JSValue::object();
    directVector.set("values", vector(v.toArray()));
    directVector.set("returnSelf", &v.setZ(9) == &v);
    emit("vectorProperties", directVector);
    Color color; color.r = .1; color.g = .2; color.b = .3;
    emit("colorProperties", vector(color.toArray()));
    Quaternion quaternion;
    auto quaternionEvents = JSValue::array(); std::size_t quaternionCount = 0;
    quaternion._onChange([&] { quaternionEvents.set(std::to_string(quaternionCount++), vector(quaternion.toArray())); });
    quaternion.x = .25; quaternion.x = .25; quaternion.y = -.5;
    quaternion.set(1, 2, 3, 4); quaternion.normalize();
    quaternion.setFromEuler(Euler(.2, .3, .4), false);
    quaternion.z = .8;
    emit("quaternionCallbacks", quaternionEvents);
    Euler euler;
    auto eulerEvents = JSValue::array(); std::size_t eulerCount = 0;
    euler._onChange([&] {
        auto event = vector({static_cast<double>(euler.x), static_cast<double>(euler.y), static_cast<double>(euler.z)});
        event.set("3", euler.getOrderString()); eulerEvents.set(std::to_string(eulerCount++), event);
    });
    euler.x = .1; euler.x = .1; euler.order = EulerOrder::YXZ;
    euler.set(.2, .3, .4, EulerOrder::ZYX);
    euler.setFromQuaternion(Quaternion(0, 0, 0, 1), std::nullopt, false);
    euler.y = .5;
    emit("eulerCallbacks", eulerEvents);
    Object3D transform;
    transform.rotation.x = .3; transform.rotation.y = -.2;
    auto synchronization = JSValue::object();
    synchronization.set("quaternion", vector(transform.quaternion.toArray()));
    transform.quaternion.set(0, 0, 0, 1);
    synchronization.set("rotation", vector({static_cast<double>(transform.rotation.x), static_cast<double>(transform.rotation.y), static_cast<double>(transform.rotation.z)}));
    emit("transformSynchronization", synchronization);

    JsonValue parameters = JsonValue::Object{{"userData", JsonValue::Object{{"value", 1}}}};
    MeshBasicMaterial material;
    material.setValues(parameters);
    auto shared = JSValue::object();
    shared.set("parameterAlias", &material.userData.storage() == &parameters.at("userData").storage());
    parameters["userData"]["value"] = 2;
    auto serialized = material.toJSON();
    shared.set("serializationAlias", &material.userData.storage() == &serialized.at("userData").storage());
    auto clone = material.clone(); material.userData["value"] = 3;
    shared.set("source", material.userData.at("value").get<double>());
    shared.set("serialized", serialized.at("userData").at("value").get<double>());
    shared.set("cloned", clone->userData.at("value").get<double>());
    emit("materialSharing", shared);
    std::cout << output.dump();
}
