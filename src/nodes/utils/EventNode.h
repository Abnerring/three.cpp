#pragma once
#include <string>
#include "core/JSValue.h"
JSValue nativeEventNodeConstructor();
JSValue nativeEventNodeModule();
JSValue nativeEventNodeBinding(const std::string& name);

// Port of the event table in src/nodes/utils/EventNode.js at the pinned revision. The node runs a
// callback at some point in the render loop, and which point is decided by a name. Two things
// about that decision are worth writing down: the name picks not only when but which of three
// slots is filled, and two of the names bypass the slot system altogether.
class EventNode {
    JSValue object_;
  public:
    explicit EventNode(const JSValue& eventType = {}, const JSValue& callback = {});
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    static JSValue constructor() { return nativeEventNodeBinding("default"); }
    static constexpr const char* kTypeName = "EventNode";

    // The type the node reports. It produces nothing, so the type says so rather than being a
    // value type like the rest.
    static constexpr const char* kNodeType = "void";

    // Which of the three update slots an event name fills. The middle and before names use the
    // after and before slots rather than the plain one, so the name decides the slot as well as
    // the timing.
    enum class Slot { None, Plain, After, Before, Pipeline };

    // The kind of update, which is one of the three the frame understands.
    enum class Kind { None, Object, Render, Frame };

    // The names, as the class exposes them, so that a test can check each without writing the
    // strings out.
    static constexpr const char* kObject = "object";
    static constexpr const char* kMaterial = "material";
    static constexpr const char* kFrame = "frame";
    static constexpr const char* kAfterObject = "afterObject";
    static constexpr const char* kBeforeObject = "beforeObject";
    static constexpr const char* kBeforeMaterial = "beforeMaterial";
    static constexpr const char* kBeforeFrame = "beforeFrame";
    static constexpr const char* kBeforePipeline = "beforeRenderPipeline";
    static constexpr const char* kAfterPipeline = "afterRenderPipeline";

    static Slot slotFor(const std::string& eventType);
    static Kind kindFor(const std::string& eventType);

    // Whether an event name is one the node knows. A name it does not know fills no slot at all,
    // so its callback never runs and nothing is reported: not an error, just silence.
    static bool isKnown(const std::string& eventType);

    // Whether the node may be cached. It may not.
    static bool isCacheable();
};
