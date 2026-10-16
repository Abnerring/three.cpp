// Differential probe for the pinned src/extras/Controls.js port.
//
// The probe builds the same observable state as compare-controls.mjs: constructor
// defaults, the connect() sequence with a disconnect override that counts calls, and the
// strict `domElement !== null` rule. One JSON object is printed, with no input needed.
#include "core/JsonValue.h"
#include "core/Object3D.h"
#include "extras/Controls.h"
#include <iostream>
#include <memory>
#include <type_traits>

namespace {
class ProbeControls : public Controls {
  public:
    double disconnects = 0;
    ProbeControls(std::shared_ptr<Object3D> object, JSValue domElement)
        : Controls(std::move(object), std::move(domElement)) {}
    void disconnect() override { disconnects += 1; }
};
JsonValue domElementTag(const Controls& controls) {
    if (controls.domElement.isNull()) return "null";
    if (controls.domElement.isUndefined()) return "undefined";
    return controls.domElement.toString();
}
JsonValue mapValues(const JSValue& map) {
    JsonValue result = JsonValue::Object{};
    for (const auto& key : map.keys()) result[key] = map.get(key).isNull() ? JsonValue{} : JsonValue(map.get(key).toString());
    return result;
}
JsonValue step(const ProbeControls& controls, const std::string& name) {
    return JsonValue::Object{{"step", name}, {"disconnectCalls", controls.disconnects},
        {"domElement", domElementTag(controls)}};
}
}

int main() {
    auto object = std::make_shared<Object3D>();
    Controls defaults(object);
    JsonValue output = JsonValue::Object{};
    output["defaults"] = JsonValue::Object{{"objectIsObject3D", defaults.object != nullptr},
        {"domElement", domElementTag(defaults)},
        {"enabled", defaults.enabled},
        {"state", defaults.state},
        {"keys", mapValues(defaults.keys)},
        {"mouseButtons", mapValues(defaults.mouseButtons)},
        {"touches", mapValues(defaults.touches)},
        {"isEventDispatcher", std::is_base_of_v<EventDispatcher, Controls>}};

    ProbeControls sequence(object, JSValue(nullptr));
    JsonValue::Array steps;
    steps.push_back(step(sequence, "construct"));
    sequence.connect(JSValue("first"));
    steps.push_back(step(sequence, "connect first"));
    sequence.connect(JSValue("second"));
    steps.push_back(step(sequence, "connect second"));
    sequence.connect(JSValue());
    steps.push_back(step(sequence, "connect undefined"));
    sequence.connect(JSValue(nullptr));
    steps.push_back(step(sequence, "connect null"));
    sequence.connect(JSValue("third"));
    steps.push_back(step(sequence, "connect third"));
    output["sequence"] = std::move(steps);

    ProbeControls preset(object, JSValue("initial"));
    JsonValue::Array presetSteps;
    presetSteps.push_back(step(preset, "construct with element"));
    preset.connect(JSValue("next"));
    presetSteps.push_back(step(preset, "connect next"));
    output["preset"] = std::move(presetSteps);
    output["updateReturnsUndefined"] = [&] {
        Controls plain(object);
        plain.update(JSValue(0.25));
        plain.dispose();
        return true;
    }();

    std::cout << output.dump() << '\n';
    return 0;
}
