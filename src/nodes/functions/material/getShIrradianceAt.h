#pragma once

#include "core/JSValue.h"
#include <string>

JSValue nativeGetShIrradianceAt();
JSValue nativeGetShIrradianceAtModule();
JSValue nativeGetShIrradianceAtBinding(const std::string& name);
