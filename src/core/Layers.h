#pragma once
#include "core/JSValue.h"

// The native handle and its JS view share one object. Assignment aliases the
// layers object; Object3D.copy explicitly assigns mask on its existing object.
class Layers {
    JSValue object_;
    struct Existing {};
    Layers(const JSValue& object, Existing);
    void invoke(const char* name, const JSValue& value = {});

  public:
    class Mask {
        Layers* owner_;

      public:
        explicit Mask(Layers* owner) : owner_(owner) {}
        JSValue value() const;
        operator JSValue() const { return value(); }
        Mask& operator=(const JSValue& value);
        Mask& operator=(const Mask& value) {
            // Capture the assignment base before the RHS getter can replace
            // an embedded native layers field (Object3D.copy uses this path).
            auto receiver = owner_->object();
            const auto rhs = value.value();
            receiver.set("mask", rhs);
            return *this;
        }
        double toNumber() const { return value().toNumber(); }
        bool operator==(double value) const { return toNumber() == value; }
    };
    Layers();
    Layers(const Layers& other);
    Layers(Layers&& other);
    Layers& operator=(const Layers& other);
    Layers& operator=(Layers&& other);
    static Layers fromObject(const JSValue& object) { return Layers(object, Existing{}); }
    static JSValue constructor();
    static JSValue prototype();
    JSValue object() const;
    operator JSValue() const { return object(); }
    // Native ownership tracing uses the actual field address without copying
    // or reading JS properties. It never invokes a user getter.
    const JSValue* runtimeRoot() const { return &object_; }
    Mask mask{this};
    void set(const JSValue& layer = {});
    void enable(const JSValue& layer = {});
    void enableAll();
    void toggle(const JSValue& layer = {});
    void disable(const JSValue& layer = {});
    void disableAll();
    bool test(const Layers& layers) const;
    bool isEnabled(const JSValue& layer = {}) const;
};

JSValue nativeLayersConstructor();
JSValue nativeLayers(const Layers& layers);
