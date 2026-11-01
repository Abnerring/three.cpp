#pragma once
#include "core/JSValue.h"
#include <cstdint>
#include <memory>

// Native input/window boundary used by controls. This is not a DOM tree.
// Applications forward their platform events and supply viewport/surface metrics.
// JS listener edges live in managed properties/slots, not hidden C++ callbacks.
class NativeControlsInput {
  public:
    struct State;
    struct BindingOwner;
    enum class Target : unsigned { Window, Document, Surface };
    NativeControlsInput();
    JSValue target(Target which = Target::Surface) const;
    JSValue window() const { return target(Target::Window); }
    JSValue document() const { return target(Target::Document); }
    JSValue surface() const { return target(Target::Surface); }
    void setWindowSize(double width, double height);
    void setSurfaceBounds(double left, double top, double width, double height);
    std::string touchAction(Target which = Target::Surface) const;
    // Synchronous C++ event delivery. Source callback exceptions propagate to the
    // C++ caller; a window-procedure boundary must catch/report them.
    bool dispatch(Target which, const std::string& type, JSValue event = {}) const;
    void keyDown(const std::string& code, bool altKey = false) const;
    void keyUp(const std::string& code) const;
    void pointerMove(double pageX, double pageY) const;
    void pointerDown(int button) const;
    void pointerUp(int button) const;
    void pointerCancel() const;
    bool contextMenu() const;
  private:
    std::shared_ptr<State> state_;
    // Deliberately separate from State: target payloads must not retain their
    // own managed roots through State -> target -> State.
    std::shared_ptr<BindingOwner> binding_;
    friend void bindNativeControlsInput(const NativeControlsInput& input);
};

// One source window/document binding per native JSValue realm. No global fake
// DOM objects are installed. Missing bindings throw only when source reads them.
// A bound input and its copies keep that realm alive until clear/rebind or the
// last copy is destroyed. Bound inputs use the owning JSValue thread; clearing
// an earlier/replaced input cannot remove a newer input's bindings.
void bindNativeControlsInput(const NativeControlsInput& input);
void clearNativeControlsInput();
JSValue nativeControlsHostBinding(const std::string& name);

// Forward real Win32 window messages from the application's WndProc. This does
// not create/pump a window or inject input. Mouse, keyboard, context-menu and
// cancel messages are supported; touch/pen synthesis is outside this adapter.
// The default surface is the whole window client area; forwarding refreshes
// its bounds from GetClientRect. Keyboard mapping uses standard PC scan-code
// positions; unknown/OEM positions remain Unidentified.
// Return true only when a context menu was prevented. Other messages preserve
// the caller's normal DefWindowProc handling. hwnd is the real HWND as void*.
bool forwardNativeControlsWin32Message(NativeControlsInput& input, void* hwnd,
                                      std::uint32_t message, std::uintptr_t wParam,
                                      std::intptr_t lParam);
