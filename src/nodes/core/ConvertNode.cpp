#include "nodes/core/ConvertNode.h"

int ConvertNode::typeLength(const std::string& type) {
    // A three part vector counts three and a float counts one; a matrix counts its rows times
    // its columns. Only the shapes the source distinguishes are needed here.
    if (type == "float" || type == "int" || type == "uint" || type == "bool" || type == "half") {
        return 1;
    }
    if (type.size() > 3 && type.compare(0, 3, "vec") == 0) {
        return type[3] - '0';
    }
    if (type.size() > 3 && type.compare(0, 3, "mat") == 0) {
        const int n = type[3] - '0';
        const int m = type.size() > 4 ? type[4] - '0' : n;
        return n * m;
    }
    return 1;
}

std::vector<std::string> ConvertNode::parts(const std::string& convertTo) {
    std::vector<std::string> out;
    std::string current;
    for (const char c : convertTo) {
        if (c == '|') {
            out.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    out.push_back(current);
    return out;
}

std::string ConvertNode::choose(const std::string& requestType, const std::string& convertTo) {
    const int requestLength = typeLength(requestType);
    std::string chosen;
    bool first = true;
    for (const auto& part : parts(convertTo)) {
        // The first part is kept because there is nothing chosen yet, whatever its length.
        if (first || typeLength(part) == requestLength) {
            chosen = part;
            first = false;
        }
    }
    return chosen;
}
