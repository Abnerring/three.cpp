#include "loaders/GCodeLoader.h"

#include <cctype>
#include <cstdlib>
#include <unordered_map>

namespace {
std::string upper(std::string text) {
    for (auto& c : text) c = char(std::toupper(static_cast<unsigned char>(c)));
    return text;
}

// Strips everything from the first semicolon on each line, as upstream does globally.
std::string stripComments(const std::string& data) {
    std::string out;
    bool inComment = false;
    for (const char c : data) {
        if (inComment) {
            if (c == '\n') {
                inComment = false;
                out.push_back(c);
            }
            continue;
        }
        if (c == ';') {
            inComment = true;
            continue;
        }
        out.push_back(c);
    }
    return out;
}
}  // namespace

GCodeLoader::Result GCodeLoader::parse(const std::string& data) const {
    State state;
    Result result;
    std::size_t currentLayer = std::string::npos;

    const auto newLayer = [&](const State& line) {
        Layer layer;
        layer.z = line.z;
        result.layers.push_back(layer);
        currentLayer = result.layers.size() - 1;
    };

    const auto addSegment = [&](const State& p1, const State& p2) {
        if (currentLayer == std::string::npos) newLayer(p1);
        Layer& layer = result.layers[currentLayer];
        if (state.extruding) {
            layer.vertex.insert(layer.vertex.end(), {p1.x, p1.y, p1.z, p2.x, p2.y, p2.z});
        } else {
            layer.pathVertex.insert(layer.pathVertex.end(), {p1.x, p1.y, p1.z, p2.x, p2.y, p2.z});
        }
    };

    const auto absolute = [&](double v1, double v2) { return state.relative ? v1 + v2 : v2; };
    const auto absoluteExtrusion = [&](double v1, double v2) {
        const bool relative = state.extrusionOverride ? state.extrusionRelative : state.relative;
        return relative ? v1 + v2 : v2;
    };

    const std::string stripped = stripComments(data);
    std::size_t lineStart = 0;
    while (lineStart <= stripped.size()) {
        const std::size_t lineEnd = stripped.find('\n', lineStart);
        const std::string lineText = lineEnd == std::string::npos
                                         ? stripped.substr(lineStart)
                                         : stripped.substr(lineStart, lineEnd - lineStart);
        lineStart = lineEnd == std::string::npos ? stripped.size() + 1 : lineEnd + 1;

        // Upstream splits on a single space, so the first token may be empty for indented
        // lines and the command is then empty, which matches no branch.
        std::unordered_map<std::string, double> args;
        std::string cmd;
        {
            std::size_t tokenStart = 0;
            bool first = true;
            while (tokenStart <= lineText.size()) {
                const std::size_t space = lineText.find(' ', tokenStart);
                const std::string token = space == std::string::npos
                                              ? lineText.substr(tokenStart)
                                              : lineText.substr(tokenStart, space - tokenStart);
                if (first) {
                    cmd = upper(token);
                    first = false;
                } else if (!token.empty()) {
                    args[std::string(1, char(std::tolower(static_cast<unsigned char>(token[0]))))] =
                        std::strtod(token.substr(1).c_str(), nullptr);
                }
                if (space == std::string::npos) break;
                tokenStart = space + 1;
            }
        }

        if (cmd == "G0" || cmd == "G1") {
            State line = state;
            if (args.count("x")) line.x = absolute(state.x, args["x"]);
            if (args.count("y")) line.y = absolute(state.y, args["y"]);
            if (args.count("z")) line.z = absolute(state.z, args["z"]);
            if (args.count("e")) line.e = absoluteExtrusion(state.e, args["e"]);
            if (args.count("f")) line.f = absolute(state.f, args["f"]);

            // A layer starts when extrusion happens at a new Z.
            if (line.e > state.e) {
                state.extruding = true;
                if (currentLayer == std::string::npos || line.z != result.layers[currentLayer].z)
                    newLayer(line);
            }
            addSegment(state, line);
            state = line;
        } else if (cmd == "G2" || cmd == "G3") {
            // Arc moves are listed but not implemented upstream either.
        } else if (cmd == "G90") {
            state.relative = false;
            state.extrusionOverride = false;
        } else if (cmd == "G91") {
            state.relative = true;
            state.extrusionOverride = false;
        } else if (cmd == "M82") {
            state.extrusionOverride = true;
            state.extrusionRelative = false;
        } else if (cmd == "M83") {
            state.extrusionOverride = true;
            state.extrusionRelative = true;
        } else if (cmd == "G92") {
            if (args.count("x")) state.x = args["x"];
            if (args.count("y")) state.y = args["y"];
            if (args.count("z")) state.z = args["z"];
            if (args.count("e")) state.e = args["e"];
        }
    }

    // Concatenate exactly as upstream does for the non splitting path.
    for (const Layer& layer : result.layers) {
        result.vertex.insert(result.vertex.end(), layer.vertex.begin(), layer.vertex.end());
        result.pathVertex.insert(result.pathVertex.end(), layer.pathVertex.begin(),
                                 layer.pathVertex.end());
    }
    result.finalState = state;
    return result;
}
