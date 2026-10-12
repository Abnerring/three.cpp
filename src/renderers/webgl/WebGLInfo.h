#pragma once
#include "core/JSMember.h"
#include <utility>

// Managed handle for the plain object returned by three.js WebGLInfo.
// The supplied GL object provides constants only; this module issues no GPU calls.
class WebGLInfo {
    JSValue object_;
  public:
    explicit WebGLInfo(JSValue gl = {});
    WebGLInfo(const WebGLInfo& other) : object_(other.object_) {}
    WebGLInfo(WebGLInfo&& other) noexcept : object_(std::move(other.object_)) {}
    WebGLInfo& operator=(const WebGLInfo& other) { object_ = other.object_; return *this; }
    WebGLInfo& operator=(WebGLInfo&& other) noexcept { object_ = std::move(other.object_); return *this; }
    JSValue object() const { return object_; }
    operator JSValue() const { return object_; }
    JSMember memory{&object_, "memory"}, render{&object_, "render"};
    JSMember programs{&object_, "programs"}, autoReset{&object_, "autoReset"};
    JSValue update(JSValue count = {}, JSValue mode = {}, JSValue instanceCount = {});
    JSValue reset();

    // Native console adapter, with the same arguments and truthiness rules as
    // utils.setConsoleFunction. A managed callback can retain or throw values.
    // With no handler, errors are written to the native standard error stream.
    static void setConsoleFunction(JSValue callback);
    static JSValue getConsoleFunction();
};
