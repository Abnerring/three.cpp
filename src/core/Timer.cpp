#include "core/Timer.h"
#include "math/Interpolant.h"

Timer::Timer(NativeTimeSource source)
    : timeSource_(std::move(source)), lifetime_(std::make_shared<Lifetime>(Lifetime{this})) {
    _startTime = timeSource_.nowMilliseconds();
}
Timer::~Timer() {
    lifetime_->timer = nullptr;
    // Explicit dispose() retains observable upstream removal behavior. Native
    // destruction also releases the current subscription without throwing.
    try { disconnect(); } catch (...) {}
}
void Timer::connect(std::shared_ptr<NativeVisibilitySource> document) {
    _document = document;
    if (!document) throw JSTypeError("Cannot read properties of null (reading 'hidden')");
    if (!document->hidden().isUndefined()) {
        const std::weak_ptr<Lifetime> weak = lifetime_;
        _pageVisibilityHandler = std::make_shared<std::function<void()>>([weak] {
            const auto lifetime = weak.lock();
            if (!lifetime || !lifetime->timer) return;
            auto& timer = *lifetime->timer;
            const auto source = timer._document;
            if (!source) throw JSTypeError("Cannot read properties of null (reading 'hidden')");
            if (source->hidden().strictEquals(false)) timer.reset();
        });
        const auto handler = _pageVisibilityHandler;
        document->addVisibilityListener(handler, false);
    }
}
void Timer::disconnect() {
    if (_pageVisibilityHandler) {
        const auto source = _document;
        const auto handler = _pageVisibilityHandler;
        if (!source) throw JSTypeError("Cannot read properties of null (reading 'removeEventListener')");
        source->removeVisibilityListener(handler);
        _pageVisibilityHandler.reset();
    }
    _document.reset();
}
double Timer::getDelta() const { const auto delta = _delta; return delta.toNumber() / 1000; }
double Timer::getElapsed() const { const auto elapsed = _elapsed; return elapsed.toNumber() / 1000; }
JSValue Timer::getTimescale() const { return _timescale; }
Timer& Timer::setTimescale(JSValue value) { _timescale = std::move(value); return *this; }
Timer& Timer::reset() {
    const auto now = timeSource_.nowMilliseconds();
    const auto start = _startTime;
    _currentTime = Interpolant_detail::subtract(now, start);
    return *this;
}
void Timer::dispose() { disconnect(); }
Timer& Timer::update(JSValue timestamp) {
    bool hidden = false;
    if (_pageVisibilityHandler) {
        const auto source = _document;
        if (!source) throw JSTypeError("Cannot read properties of null (reading 'hidden')");
        hidden = source->hidden().strictEquals(true);
    }
    if (hidden) {
        _delta = 0;
    } else {
        _previousTime = _currentTime;
        const auto now = timestamp.isUndefined() ? timeSource_.nowMilliseconds() : timestamp;
        const auto start = _startTime;
        _currentTime = Interpolant_detail::subtract(now, start);
        const auto current = _currentTime, previous = _previousTime;
        const auto elapsed = Interpolant_detail::subtract(current, previous);
        const auto scale = _timescale;
        _delta = Interpolant_detail::multiply(elapsed, scale);
        const auto total = _elapsed, delta = _delta;
        _elapsed = Interpolant_detail::add(total, delta);
    }
    return *this;
}
