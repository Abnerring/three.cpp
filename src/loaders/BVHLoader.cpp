#include "loaders/BVHLoader.h"

#include <cctype>
#include <cstdlib>
#include <stdexcept>

namespace {
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string upper(const std::string& text) {
    std::string out = text;
    for (auto& c : out) c = char(std::toupper(static_cast<unsigned char>(c)));
    return out;
}

std::vector<std::string> splitWhitespace(const std::string& text) {
    std::vector<std::string> parts;
    std::size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
        if (i >= text.size()) break;
        const std::size_t start = i;
        while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i]))) ++i;
        parts.push_back(text.substr(start, i - start));
    }
    return parts;
}

[[noreturn]] void fail(const char* message) {
    throw std::runtime_error(std::string("THREE.BVHLoader: ") + message);
}

std::shared_ptr<BVHLoader::Node> readNode(const std::vector<std::string>& lines,
                                          std::size_t& cursor,
                                          std::vector<std::shared_ptr<BVHLoader::Node>>& list) {
    if (cursor >= lines.size()) fail("Unexpected end of hierarchy");
    auto node = std::make_shared<BVHLoader::Node>();
    list.push_back(node);

    // The declaration line names the node; END SITE has no name of its own.
    auto tokens = splitWhitespace(trim(lines[cursor++]));
    if (tokens.size() >= 2 && upper(tokens[0]) == "END" && upper(tokens[1]) == "SITE") {
        node->type = "ENDSITE";
        node->name = "ENDSITE";
    } else {
        node->type = tokens.empty() ? std::string() : upper(tokens[0]);
        node->name = tokens.size() > 1 ? tokens[1] : std::string();
    }

    if (cursor >= lines.size() || trim(lines[cursor]) != "{") fail("Expected opening { after type and name");
    ++cursor;

    tokens = splitWhitespace(trim(lines[cursor++]));
    if (tokens.empty() || tokens[0] != "OFFSET") fail("Expected OFFSET");
    if (tokens.size() != 4) fail("Invalid number of values for OFFSET");
    node->offset[0] = std::strtod(tokens[1].c_str(), nullptr);
    node->offset[1] = std::strtod(tokens[2].c_str(), nullptr);
    node->offset[2] = std::strtod(tokens[3].c_str(), nullptr);

    // An end site carries no channel declaration.
    if (node->type != "ENDSITE") {
        if (cursor >= lines.size()) fail("Expected CHANNELS definition");
        tokens = splitWhitespace(trim(lines[cursor++]));
        if (tokens.empty() || tokens[0] != "CHANNELS") fail("Expected CHANNELS definition");
        const int count = tokens.size() > 1 ? std::atoi(tokens[1].c_str()) : 0;
        for (int i = 0; i < count && static_cast<std::size_t>(2 + i) < tokens.size(); ++i)
            node->channels.push_back(tokens[static_cast<std::size_t>(2 + i)]);
    }

    // Children follow until the closing brace.
    while (cursor < lines.size()) {
        const std::string line = trim(lines[cursor]);
        if (line == "}") {
            ++cursor;
            break;
        }
        node->children.push_back(readNode(lines, cursor, list));
    }
    return node;
}
}  // namespace

std::vector<std::string> BVHLoader::splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find('\n', start);
        std::string line = end == std::string::npos ? text.substr(start) : text.substr(start, end - start);
        // A trailing carriage return is stripped so comparisons see the token alone.
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return lines;
}

BVHLoader::Hierarchy BVHLoader::parseHierarchy(const std::vector<std::string>& lines,
                                               std::size_t& cursor) {
    if (cursor >= lines.size() || trim(lines[cursor]) != "HIERARCHY") fail("HIERARCHY expected.");
    ++cursor;

    Hierarchy hierarchy;
    hierarchy.root = readNode(lines, cursor, hierarchy.list);
    return hierarchy;
}

BVHLoader::Motion BVHLoader::parseMotion(const std::vector<std::string>& lines, std::size_t& cursor,
                                          std::size_t channelCount) {
    if (cursor >= lines.size() || trim(lines[cursor]) != "MOTION") fail("MOTION expected.");
    ++cursor;

    // Frames: <count>
    if (cursor >= lines.size()) fail("Failed to read number of frames.");
    auto tokens = splitWhitespace(trim(lines[cursor++]));
    if (tokens.size() < 2 || tokens[0] != "Frames:") fail("Failed to read number of frames.");
    Motion motion;
    motion.numFrames = std::atoi(tokens[1].c_str());
    if (motion.numFrames < 0) fail("Failed to read number of frames.");

    // Frame Time: <seconds>
    if (cursor >= lines.size()) fail("Failed to read frame time.");
    tokens = splitWhitespace(trim(lines[cursor++]));
    if (tokens.size() < 3 || tokens[0] != "Frame" || tokens[1] != "Time:")
        fail("Failed to read frame time.");
    motion.frameTime = std::strtod(tokens[2].c_str(), nullptr);

    motion.times.reserve(static_cast<std::size_t>(motion.numFrames));
    motion.values.reserve(static_cast<std::size_t>(motion.numFrames) * channelCount);

    for (int i = 0; i < motion.numFrames; ++i) {
        if (cursor >= lines.size()) fail("Failed to read frame data.");
        const auto frameTokens = splitWhitespace(trim(lines[cursor++]));
        if (frameTokens.size() < channelCount) fail("Failed to read frame data.");
        // The frame time is the index times the duration, as upstream computes it.
        motion.times.push_back(i * motion.frameTime);
        for (std::size_t k = 0; k < channelCount; ++k)
            motion.values.push_back(std::strtod(frameTokens[k].c_str(), nullptr));
    }
    return motion;
}