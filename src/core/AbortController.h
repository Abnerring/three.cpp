#pragma once
#include "core/JSValue.h"

// Native cancellation objects. All callbacks execute synchronously on the
// owning JSValue thread; I/O workers must post completion to that thread.
class JSAbortException : public std::exception {
  public:
    explicit JSAbortException(JSValue value):reason(std::move(value)) {}
    const char* what() const noexcept override{return "AbortSignal.throwIfAborted";}
    JSValue reason;
};
JSValue createNativeAbortController();
JSValue createNativeAbortSignalAny(JSValue signals);
// Register an abort algorithm independently of event propagation. Called at
// most once, before abort event listeners. Already-aborted signals run it now.
void addNativeAbortAlgorithm(JSValue signal,JSValue callback);
