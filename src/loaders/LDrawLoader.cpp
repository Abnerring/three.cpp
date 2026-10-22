#include "loaders/LDrawLoader.h"

#include <cstdlib>
#include <utility>

LDrawLoader::LineParser::LineParser(std::string line, int lineNumber)
    : line_(std::move(line)), length_(line_.size()), index_(0), lineNumber_(lineNumber) {}

void LDrawLoader::LineParser::seekNonSpace() {
    // Stop on the first character that is neither a space nor a tab.
    while (index_ < length_) {
        const char c = line_[index_];
        if (c != ' ' && c != '\t') return;
        ++index_;
    }
}

std::string LDrawLoader::LineParser::getToken() {
    // The cursor is expected to stand on the first character of the token, so stepping over it
    // before measuring is what makes the token start there.
    const std::size_t start = index_;
    if (index_ < length_) ++index_;

    // Run to the next space or tab.
    while (index_ < length_) {
        const char c = line_[index_];
        if (c == ' ' || c == '\t') break;
        ++index_;
    }
    const std::size_t stop = index_;

    // Then skip the whitespace that follows.
    seekNonSpace();

    // A start past the end gives nothing, which is what a substring clamped to the line does.
    if (start >= length_) return std::string();
    return line_.substr(start, stop - start);
}

std::array<double, 3> LDrawLoader::LineParser::getVector() {
    // Three tokens, each converted as far as it can be read.
    std::array<double, 3> vector{};
    for (int i = 0; i < 3; ++i) {
        const std::string token = getToken();
        vector[static_cast<std::size_t>(i)] = token.empty() ? 0.0 : std::strtod(token.c_str(), nullptr);
    }
    return vector;
}

std::string LDrawLoader::LineParser::getRemainingString() const {
    if (index_ >= length_) return std::string();
    return line_.substr(index_);
}

bool LDrawLoader::LineParser::isAtTheEnd() const { return index_ >= length_; }

void LDrawLoader::LineParser::setToEnd() { index_ = length_; }

std::string LDrawLoader::LineParser::getLineNumberString() const {
    // Only a line that knows its number names it.
    if (lineNumber_ < 0) return std::string();
    return " at line " + std::to_string(lineNumber_);
}

namespace {
// A token read as a number, with an empty token standing for nothing.
double toNumber(const std::string& token) {
    return token.empty() ? 0.0 : std::strtod(token.c_str(), nullptr);
}
}  // namespace

LDrawLoader::ReferenceLine LDrawLoader::parseReferenceLine(LineParser& parser) {
    ReferenceLine reference;
    // The colour is kept as text because it may be a number or a name.
    reference.colorCode = parser.getToken();
    // Three numbers for the position, then nine for the rotation, written a row at a time.
    for (int i = 0; i < 3; ++i) {
        reference.position[static_cast<std::size_t>(i)] = toNumber(parser.getToken());
    }
    for (int i = 0; i < 9; ++i) {
        reference.rotation[static_cast<std::size_t>(i)] = toNumber(parser.getToken());
    }
    // Whatever remains is the part file.
    reference.fileName = parser.getToken();
    return reference;
}

LDrawLoader::MetaKind LDrawLoader::metaKind(const std::string& token) {
    // The six meta names the loader reacts to; every other comment names nothing.
    if (token == "!LDRAW_ORG") return MetaKind::LDrawOrg;
    if (token == "!COLOUR") return MetaKind::Colour;
    if (token == "!CATEGORY") return MetaKind::Category;
    if (token == "!KEYWORDS") return MetaKind::Keywords;
    if (token == "FILE") return MetaKind::File;
    if (token == "BFC") return MetaKind::Bfc;
    return MetaKind::None;
}

std::vector<std::string> LDrawLoader::parseKeywords(const std::string& rest) {
    std::vector<std::string> keywords;
    std::size_t at = 0;
    while (true) {
        const std::size_t comma = rest.find(',', at);
        const std::size_t stop = comma == std::string::npos ? rest.size() : comma;
        std::string piece = rest.substr(at, stop - at);
        // Each piece is trimmed of the whitespace around it.
        std::size_t begin = 0;
        while (begin < piece.size() && (piece[begin] == ' ' || piece[begin] == '\t')) ++begin;
        std::size_t last = piece.size();
        while (last > begin && (piece[last - 1] == ' ' || piece[last - 1] == '\t')) --last;
        keywords.push_back(piece.substr(begin, last - begin));

        if (comma == std::string::npos) break;
        at = comma + 1;
    }
    // Splitting always gives at least one piece, even an empty rest.
    return keywords;
}