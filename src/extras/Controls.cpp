#include "extras/Controls.h"

Controls::Controls(std::shared_ptr<Object3D> object, JSValue domElement)
    : object(std::move(object)), domElement(std::move(domElement)) {
    // keys = {}; the input maps start as ordinary empty/placeholder objects upstream.
    keys = JSValue::object();
    mouseButtons = JSValue::object();
    mouseButtons.set("LEFT", JSValue(nullptr));
    mouseButtons.set("MIDDLE", JSValue(nullptr));
    mouseButtons.set("RIGHT", JSValue(nullptr));
    touches = JSValue::object();
    touches.set("ONE", JSValue(nullptr));
    touches.set("TWO", JSValue(nullptr));
}

void Controls::connect(JSValue element) {
    // `if ( this.domElement !== null ) this.disconnect();` keeps the strict null test:
    // undefined still disconnects, exactly like the upstream comparison.
    if (!domElement.isNull()) disconnect();
    domElement = std::move(element);
}
