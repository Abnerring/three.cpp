#include "nodes/core/DebugNode.h"

std::string DebugNode::title(const std::string& shaderStage) {
    return "--- TSL debug - " + shaderStage + " shader ---";
}

std::string DebugNode::border(const std::string& titleText) {
    // As many dashes as the title is long, which already includes its own end dashes.
    return std::string(titleText.size(), '-');
}

std::string DebugNode::stripLeadingTabs(const std::string& code) {
    // Only a tab at the beginning of a line is removed. Spaces are left alone, and so is a tab
    // that comes after something else.
    std::string out;
    out.reserve(code.size());
    bool atLineStart = true;
    for (const char c : code) {
        if (atLineStart && c == '\t') continue;
        out.push_back(c);
        atLineStart = (c == '\n');
    }
    return out;
}

std::string DebugNode::block(const std::string& shaderStage, const std::string& code,
                             const std::string& snippet) {
    const std::string titleText = title(shaderStage);
    const std::string borderText = border(titleText);
    std::string out;
    out += "// #" + titleText + "#\n";
    out += stripLeadingTabs(code) + "\n";
    out += "/* ... */ " + snippet + " /* ... */\n";
    out += "// #" + borderText + "#\n";
    return out;
}
