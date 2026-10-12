#pragma once
#include "core/JSValue.h"
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string_view>

// Milliseconds on a monotonic native clock. Applications can supply the same
// clock used by their frame scheduler; tests can advance it without sleeping.
struct NativeTimeSource {
    std::function<JSValue()> nowMilliseconds = []() -> JSValue {
        using Clock = std::chrono::steady_clock;
        static const auto origin = Clock::now();
        return std::chrono::duration<double, std::milli>(Clock::now() - origin).count();
    };
    std::function<void(std::string_view)> warning = [](std::string_view message) {
        std::clog << message << '\n';
    };
};

// Native window/application adapters implement this contract. An unavailable
// visibility API returns undefined; only literal true/false select the timer's
// hidden/resumed branches, as in three.js.
class NativeVisibilitySource {
  public:
    using Listener = std::shared_ptr<std::function<void()>>;
    virtual ~NativeVisibilitySource() = default;
    virtual JSValue hidden() const = 0;
    virtual void addVisibilityListener(const Listener& listener, bool capture) = 0;
    virtual void removeVisibilityListener(const Listener& listener) = 0;
};
