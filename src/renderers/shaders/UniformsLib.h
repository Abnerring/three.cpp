#pragma once
#include "core/JSValue.h"

// Mutable shared uniform definitions within the calling thread's native realm.
extern thread_local JSValue UniformsLib;
