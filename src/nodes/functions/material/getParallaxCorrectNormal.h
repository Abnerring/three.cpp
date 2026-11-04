#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeGetParallaxCorrectNormal();
JSValue nativeGetParallaxCorrectNormalModule();
JSValue nativeGetParallaxCorrectNormalBinding(const std::string& name);
