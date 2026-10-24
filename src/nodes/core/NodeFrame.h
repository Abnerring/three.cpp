#pragma once
#include "core/JSMember.h"

// NodeFrame.js at the pinned revision. The managed object is the single source
// of state, including the weak update maps and fields changed during callbacks.
class NodeFrame {
    JSValue object_;
  public:
    NodeFrame();
    NodeFrame(const NodeFrame& other) : object_(other.object_) {}
    NodeFrame& operator=(const NodeFrame& other) {
        object_ = other.object_;
        return *this;
    }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue prototype();

    // Native performance host, shared within this runtime realm. now() returns
    // milliseconds; reads and calls happen at the same sites as upstream.
    static void setClock(JSValue performance);
    static void resetClock();

    JSMember time{&object_, "time"}, deltaTime{&object_, "deltaTime"};
    JSMember frameId{&object_, "frameId"}, renderId{&object_, "renderId"};
    JSMember updateMap{&object_, "updateMap"}, updateBeforeMap{&object_, "updateBeforeMap"};
    JSMember updateAfterMap{&object_, "updateAfterMap"};
    JSMember renderer{&object_, "renderer"}, material{&object_, "material"};
    JSMember camera{&object_, "camera"}, objectReference{&object_, "object"};
    JSMember scene{&object_, "scene"}, compute{&object_, "compute"}, lastTime{&object_, "lastTime"};

    void update();
    void updateBeforeNode(JSValue node);
    void updateAfterNode(JSValue node);
    void updateNode(JSValue node);
};
