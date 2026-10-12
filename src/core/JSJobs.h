#pragma once
#include "core/JSValue.h"
#include <cstddef>

// The native event loop calls this checkpoint on the JSValue owning thread.
// Worker threads must post native data to that loop, never JSValue handles.
void queueMicrotask(JSValue callback);
bool hasPendingMicrotasks();
// True while this thread is executing a microtask checkpoint.
bool isDrainingMicrotasks();
std::size_t drainMicrotasks();
