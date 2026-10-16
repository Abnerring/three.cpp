#pragma once
#include "core/EventDispatcher.h"
#include "core/JSValue.h"
#include <memory>

class Object3D;

// Native port of the pinned src/extras/Controls.js.
//
// Abstract base class for controls: it stores the managed object, the DOM element that
// receives the events, the input maps and the internal state, and defines the connect /
// disconnect / dispose / update hooks that concrete controls override. The native host
// element is a managed value, so a C++ host can attach any native handle or leave the
// default `null`, and derived classes keep the upstream strict `!== null` test in
// connect() rather than collapsing null and undefined.
class Controls : public EventDispatcher {
  public:
    // The object that is managed by the controls.
    std::shared_ptr<Object3D> object;
    // The HTML element used for event listeners; `null` by default.
    JSValue domElement;
    // Whether the controls responds to user input or not.
    bool enabled = true;
    // The internal state of the controls.
    double state = -1;
    // This object defines the keyboard input of the controls.
    JSValue keys;
    // Mouse button to action mapping: { LEFT: null, MIDDLE: null, RIGHT: null }.
    JSValue mouseButtons;
    // Touch interaction to action mapping: { ONE: null, TWO: null }.
    JSValue touches;

    explicit Controls(std::shared_ptr<Object3D> object = nullptr, JSValue domElement = JSValue(nullptr));
    ~Controls() override = default;

    // Connects the controls to the DOM element, disconnecting the previous one first.
    virtual void connect(JSValue element);
    // Disconnects the controls from the DOM. Concrete controls remove their listeners.
    virtual void disconnect() {}
    // Frees internal resources and removes all event listeners.
    virtual void dispose() {}
    // Per simulation step hook.
    virtual void update(JSValue delta = {}) { (void)delta; }
};
