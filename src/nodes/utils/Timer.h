#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeTimerModule();
JSValue nativeTimerBinding(const std::string& name);
JSValue nativeTimerTime();
JSValue nativeTimerDeltaTime();
JSValue nativeTimerFrameId();
