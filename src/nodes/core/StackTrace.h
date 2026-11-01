#pragma once
#include "core/JSValue.h"
#include <string>
#include <vector>

// Port of src/nodes/core/StackTrace.js at the pinned revision.
//
// Default capture reads the current global Error constructor and its stack.
// Translated native execution supplies real source frames through JSSourceTrace.
struct NodeStackFrame {
    std::string fn;
    std::string file;
    int line = 0;
    int column = 0;
};

// Parses a stack string in the Chrome ("at fn (file.js:1:2)" or "at file.js:1:2") or
// Firefox ("fn@file.js:1:2") shapes, drops unparsable lines and the four ignored files.
std::vector<NodeStackFrame> getFilteredStack(const std::string& stack);

JSValue nativeStackTraceConstructor();
// Hosts may override capture explicitly; undefined restores the upstream
// new Error().stack path. Source frames are recorded at actual execution sites.
void setNodeStackCaptureFunction(const JSValue& callback);
class StackTraceUnsupported : public std::logic_error {
  public:
    using std::logic_error::logic_error;
};

class StackTrace {
  public:
    static constexpr bool isStackTrace = true;

    explicit StackTrace(const std::string& stackMessage = {});

    const std::vector<NodeStackFrame>& stack() const { return _stack; }

    // Formatted location of the top frame, or "[Unknown location]".
    std::string getLocation() const;
    // The message followed by one "at" line per frame.
    std::string getError(const std::string& message) const;

  private:
    std::vector<NodeStackFrame> _stack;
};
