#include "nodes/core/StackTrace.h"

#include <regex>
#include <sstream>

namespace {
// Files whose frames say nothing useful about the node graph.
const std::regex ignored[] = {std::regex("^StackTrace\\.js$"), std::regex("^TSLCore\\.js$"),
                              std::regex("^.*Node\\.js$"), std::regex("^three\\.webgpu.*\\.js$")};

std::string stripPath(const std::string& file) {
    const auto slash = file.find_last_of('/');
    return slash == std::string::npos ? file : file.substr(slash + 1);
}
}  // namespace

std::vector<NodeStackFrame> getFilteredStack(const std::string& stack) {
    // Same shape as upstream: an optional "at fn (" prefix, an optional "fn@" prefix, then
    // file:line:column.
    static const std::regex pattern(
        R"((?:at\s+(.+?)\s+\()?(?:(.+?)@)?([^@\s()]+):(\d+):(\d+))");
    std::vector<NodeStackFrame> frames;
    std::istringstream lines(stack);
    std::string line;
    while (std::getline(lines, line)) {
        std::smatch match;
        if (!std::regex_search(line, match, pattern)) continue;
        NodeStackFrame frame;
        frame.fn = match[1].matched ? match[1].str() : (match[2].matched ? match[2].str() : "");
        std::string file = match[3].str();
        const auto query = file.find('?');  // strip Vite/HMR suffixes
        if (query != std::string::npos) file = file.substr(0, query);
        frame.file = stripPath(file);
        frame.line = std::stoi(match[4].str());
        frame.column = std::stoi(match[5].str());
        bool skip = false;
        for (const auto& pattern2 : ignored)
            if (std::regex_match(frame.file, pattern2)) skip = true;
        if (!skip) frames.push_back(std::move(frame));
    }
    return frames;
}

StackTrace::StackTrace(const std::string& stackMessage) {
    if (!stackMessage.empty()) _stack = getFilteredStack(stackMessage);
}

std::string StackTrace::getLocation() const {
    if (_stack.empty()) return "[Unknown location]";
    const auto& main = _stack.front();
    const std::string fnName = main.fn.empty() ? std::string() : '"' + main.fn + "()\" at ";
    return fnName + '"' + main.file + ":" + std::to_string(main.line) + '"';
}

std::string StackTrace::getError(const std::string& message) const {
    if (_stack.empty()) return message;
    std::string out = message + "\n";
    for (std::size_t i = 0; i < _stack.size(); ++i) {
        const auto& frame = _stack[i];
        const std::string location =
            frame.file + ":" + std::to_string(frame.line) + ":" + std::to_string(frame.column);
        out += frame.fn.empty() ? "    at " + location : "    at " + frame.fn + " (" + location + ")";
        if (i + 1 != _stack.size()) out += "\n";
    }
    return out;
}
