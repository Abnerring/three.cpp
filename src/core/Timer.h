#pragma once
#include "core/TimeSource.h"

class Timer {
  public:
    JSValue _previousTime = 0, _currentTime = 0, _startTime;
    JSValue _delta = 0, _elapsed = 0, _timescale = 1;
    std::shared_ptr<NativeVisibilitySource> _document;
    NativeVisibilitySource::Listener _pageVisibilityHandler;

    explicit Timer(NativeTimeSource timeSource = {});
    virtual ~Timer();
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    virtual void connect(std::shared_ptr<NativeVisibilitySource> document);
    virtual void disconnect();
    double getDelta() const;
    double getElapsed() const;
    JSValue getTimescale() const;
    Timer& setTimescale(JSValue timescale);
    virtual Timer& reset();
    virtual void dispose();
    virtual Timer& update(JSValue timestamp = {});

  private:
    struct Lifetime { Timer* timer; };
    NativeTimeSource timeSource_;
    std::shared_ptr<Lifetime> lifetime_;
};
