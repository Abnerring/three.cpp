#pragma once
#include "core/TimeSource.h"

class Clock {
  public:
    JSValue autoStart, startTime = 0, oldTime = 0, elapsedTime = 0, running = false;
    explicit Clock(JSValue autoStart = true, NativeTimeSource timeSource = {});
    virtual ~Clock() = default;
    virtual void start();
    virtual void stop();
    virtual JSValue getElapsedTime();
    virtual double getDelta();

  private:
    NativeTimeSource timeSource_;
};
