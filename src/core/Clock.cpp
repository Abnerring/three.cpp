#include "core/Clock.h"
#include "math/Interpolant.h"

Clock::Clock(JSValue value, NativeTimeSource source)
    : autoStart(value.isUndefined() ? JSValue(true) : std::move(value)), timeSource_(std::move(source)) {
    timeSource_.warning("THREE.Clock: This module has been deprecated. Please use THREE.Timer instead.");
}
void Clock::start() {
    startTime = timeSource_.nowMilliseconds();
    oldTime = startTime;
    elapsedTime = 0;
    running = true;
}
void Clock::stop() {
    getElapsedTime();
    running = false;
    autoStart = false;
}
JSValue Clock::getElapsedTime() {
    getDelta();
    return elapsedTime;
}
double Clock::getDelta() {
    if (autoStart.toBoolean() && !running.toBoolean()) {
        start();
        return 0;
    }
    double diff = 0;
    if (running.toBoolean()) {
        const auto newTime = timeSource_.nowMilliseconds();
        const auto previous = oldTime;
        diff = Interpolant_detail::subtract(newTime, previous) / 1000;
        oldTime = newTime;
        const auto elapsed = elapsedTime;
        elapsedTime = Interpolant_detail::add(elapsed, diff);
    }
    return diff;
}
