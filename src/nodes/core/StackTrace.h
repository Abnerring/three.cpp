#pragma once
#include <string>
#include <vector>

// Port of src/nodes/core/StackTrace.js at the pinned revision.
//
// The parsed representation and both formatting methods are portable; what is not is the
// default capture, because that reads a JavaScript engine stack string. The constructor
// therefore takes an explicit stack string, and the no-argument form yields an empty stack
// rather than inventing one.
struct NodeStackFrame {
    std::string fn;
    std::string file;
    int line = 0;
    int column = 0;
};

// Parses a stack string in the Chrome ("at fn (file.js:1:2)" or "at file.js:1:2") or
// Firefox ("fn@file.js:1:2") shapes, drops unparsable lines and the four ignored files.
std::vector<NodeStackFrame> getFilteredStack(const std::string& stack);

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
