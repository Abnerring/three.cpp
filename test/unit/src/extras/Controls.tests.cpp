#include "catch.hpp"
#include "extras/Controls.h"
#include "core/Object3D.h"
#include <memory>
#include <type_traits>

namespace {
class ProbeControls : public Controls {
  public:
    int disconnects = 0, disposes = 0, updates = 0;
    ProbeControls(std::shared_ptr<Object3D> object, JSValue domElement = JSValue(nullptr))
        : Controls(std::move(object), std::move(domElement)) {}
    void disconnect() override { ++disconnects; }
    void dispose() override { ++disposes; }
    void update(JSValue delta = {}) override {
        ++updates;
        lastDelta = std::move(delta);
    }
    JSValue lastDelta;
};
}

TEST_CASE("Controls constructor stores the managed object and defaults", "[controls]") {
    auto object = std::make_shared<Object3D>();
    Controls controls(object);
    REQUIRE(controls.object == object);
    REQUIRE(controls.domElement.isNull());
    REQUIRE(controls.enabled);
    REQUIRE(controls.state == -1);
    REQUIRE(controls.keys.isObject());
    REQUIRE(controls.keys.keys().empty());
    REQUIRE(controls.mouseButtons.isObject());
    REQUIRE(controls.mouseButtons.keys() == std::vector<std::string>({"LEFT", "MIDDLE", "RIGHT"}));
    REQUIRE(controls.mouseButtons.get("LEFT").isNull());
    REQUIRE(controls.mouseButtons.get("MIDDLE").isNull());
    REQUIRE(controls.mouseButtons.get("RIGHT").isNull());
    REQUIRE(controls.touches.isObject());
    REQUIRE(controls.touches.keys() == std::vector<std::string>({"ONE", "TWO"}));
    REQUIRE(controls.touches.get("ONE").isNull());
    REQUIRE(controls.touches.get("TWO").isNull());
    REQUIRE((std::is_base_of_v<EventDispatcher, Controls>));
}

TEST_CASE("Controls connect replaces the element and disconnects first", "[controls]") {
    auto object = std::make_shared<Object3D>();
    ProbeControls controls(object);
    REQUIRE(controls.domElement.isNull());
    controls.connect(JSValue("first"));
    REQUIRE(controls.disconnects == 0);
    REQUIRE(controls.domElement.string() == "first");
    controls.connect(JSValue("second"));
    REQUIRE(controls.disconnects == 1);
    REQUIRE(controls.domElement.string() == "second");
    // `undefined` is not `null`, so the strict upstream test still disconnects.
    controls.connect(JSValue());
    REQUIRE(controls.disconnects == 2);
    REQUIRE(controls.domElement.isUndefined());
    // Now domElement is undefined, which also differs from null.
    controls.connect(JSValue(nullptr));
    REQUIRE(controls.disconnects == 3);
    REQUIRE(controls.domElement.isNull());
    // A null element skips the disconnect call and stores the new element.
    controls.connect(JSValue("third"));
    REQUIRE(controls.disconnects == 3);
    REQUIRE(controls.domElement.string() == "third");
}

TEST_CASE("Controls constructed with an element disconnects on the next connect", "[controls]") {
    auto object = std::make_shared<Object3D>();
    ProbeControls controls(object, JSValue("initial"));
    REQUIRE(controls.domElement.string() == "initial");
    controls.connect(JSValue("next"));
    REQUIRE(controls.disconnects == 1);
    REQUIRE(controls.domElement.string() == "next");
}

TEST_CASE("Controls dispose and update are overridable hooks", "[controls]") {
    auto object = std::make_shared<Object3D>();
    ProbeControls controls(object);
    controls.dispose();
    REQUIRE(controls.disposes == 1);
    controls.update(JSValue(0.5));
    REQUIRE(controls.updates == 1);
    REQUIRE(controls.lastDelta.number() == 0.5);
    controls.update();
    REQUIRE(controls.updates == 2);
    REQUIRE(controls.lastDelta.isUndefined());
    // The base implementations are no-ops instead of unimplemented errors.
    Controls base(object);
    base.dispose();
    base.update(JSValue(1));
    base.disconnect();
}

TEST_CASE("Controls keeps the inherited event listener behavior", "[controls]") {
    auto object = std::make_shared<Object3D>();
    Controls controls(object);
    int calls = 0;
    std::string received;
    const auto listener = controls.addEventListener("change", [&](Event& event) {
        ++calls;
        received = event.type;
    });
    REQUIRE(controls.hasEventListener("change", listener));
    Event event;
    event.type = "change";
    controls.dispatchEvent(event);
    REQUIRE(calls == 1);
    REQUIRE(received == "change");
    controls.removeEventListener("change", listener);
    REQUIRE_FALSE(controls.hasEventListener("change", listener));
    controls.dispatchEvent(Event{"change"});
    REQUIRE(calls == 1);
}
