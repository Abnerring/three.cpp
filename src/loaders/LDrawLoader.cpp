#include "loaders/LDrawLoader.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
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

namespace {
// The suffix a diagnostic appends to name its line, empty when the line has no number.
std::string lineSuffix(int lineNumber) {
    if (lineNumber < 0) return std::string();
    return " at line " + std::to_string(lineNumber);
}
}  // namespace

std::string LDrawLoader::normalizeColour(const std::string& token, int lineNumber) {
    // The hex form with a leading zero x is the other way the format writes a colour.
    if (token.rfind("0x", 0) == 0) return "#" + token.substr(2);
    // A token already in the hash form is kept as it stands.
    if (!token.empty() && token[0] == '#') return token;
    // Anything else is not a colour at all.
    throw std::runtime_error("THREE.LDrawLoader: Invalid color while parsing material" +
                             lineSuffix(lineNumber) + ".");
}

double LDrawLoader::alphaFromByte(int value) {
    // The byte is divided by its range and then clamped, so both ends saturate.
    const double fraction = value / 255.0;
    if (fraction < 0.0) return 0.0;
    if (fraction > 1.0) return 1.0;
    return fraction;
}

bool LDrawLoader::isTransparentAlpha(double alpha) {
    // Only an alpha below fully opaque is transparent.
    return alpha < 1.0;
}

std::array<std::array<double, 3>, 3> LDrawLoader::readTriangle(LineParser& parser, bool ccw) {
    std::array<std::array<double, 3>, 3> vertices{};
    for (int i = 0; i < 3; ++i) vertices[static_cast<std::size_t>(i)] = parser.getVector();
    // A clockwise listing is the same triangle written the other way round.
    if (!ccw) {
        std::swap(vertices[0], vertices[2]);
    }
    return vertices;
}

std::array<std::array<double, 3>, 4> LDrawLoader::readQuad(LineParser& parser, bool ccw) {
    std::array<std::array<double, 3>, 4> vertices{};
    for (int i = 0; i < 4; ++i) vertices[static_cast<std::size_t>(i)] = parser.getVector();
    // The reversal turns the four around, which keeps the same quad wound the other way.
    if (!ccw) {
        std::swap(vertices[0], vertices[3]);
        std::swap(vertices[1], vertices[2]);
    }
    return vertices;
}

bool LDrawLoader::isDoubleSided(bool certified, bool cull) {
    // A file that never certified its winding cannot be culled safely, and neither can one that
    // certified but asked not to cull.
    return !certified || !cull;
}

int LDrawLoader::triangleFaceCount(bool doubleSided) {
    // A face that shows from both sides contributes two records.
    return doubleSided ? 2 : 1;
}

std::string LDrawLoader::normalizeFileName(const std::string& raw) {
    // The whitespace around the name is not part of it.
    std::size_t begin = 0;
    while (begin < raw.size() && (raw[begin] == ' ' || raw[begin] == '\t')) ++begin;
    std::size_t last = raw.size();
    while (last > begin && (raw[last - 1] == ' ' || raw[last - 1] == '\t')) --last;

    // Every backslash becomes a forward slash, since the files are written on systems that use
    // the former as a separator.
    std::string name = raw.substr(begin, last - begin);
    for (char& c : name) {
        if (c == '\\') c = '/';
    }
    return name;
}

std::string LDrawLoader::resolveFileName(const std::string& name,
                                         const std::vector<std::string>& mappedNames,
                                         const std::vector<std::string>& mappedPaths) {
    // A name the caller mapped is used exactly as they gave it.
    for (std::size_t i = 0; i < mappedNames.size() && i < mappedPaths.size(); ++i) {
        if (mappedNames[i] == name) return mappedPaths[i];
    }
    // Otherwise the two standard folders are looked for, and the folder itself is kept as part
    // of the path rather than replaced.
    if (name.rfind("s/", 0) == 0) return "parts/" + name;
    if (name.rfind("48/", 0) == 0) return "p/" + name;
    return name;
}

std::array<double, 16> LDrawLoader::referenceMatrix(const ReferenceLine& reference) {
    std::array<double, 16> matrix{};
    // The nine numbers are a rotation written row by row, so a column of the stored matrix
    // gathers one number from each row.
    matrix[0] = reference.rotation[0];
    matrix[1] = reference.rotation[3];
    matrix[2] = reference.rotation[6];
    matrix[3] = 0;
    matrix[4] = reference.rotation[1];
    matrix[5] = reference.rotation[4];
    matrix[6] = reference.rotation[7];
    matrix[7] = 0;
    matrix[8] = reference.rotation[2];
    matrix[9] = reference.rotation[5];
    matrix[10] = reference.rotation[8];
    matrix[11] = 0;
    // The position is the fourth column.
    matrix[12] = reference.position[0];
    matrix[13] = reference.position[1];
    matrix[14] = reference.position[2];
    matrix[15] = 1;
    return matrix;
}

namespace {
// A double truncated towards zero and taken as a thirty two bit integer, wrapping the way the
// double bitwise not of the source language does.
std::int32_t truncateToInt32(double value) {
    const double truncated = value < 0 ? -std::floor(-value) : std::floor(value);
    // The conversion below wraps rather than saturating, which is what the source does.
    const auto wrapped = static_cast<std::uint32_t>(static_cast<std::int64_t>(truncated));
    return static_cast<std::int32_t>(wrapped);
}
std::string joinNumbers(const std::array<double, 3>& v, double multiplier) {
    return std::to_string(truncateToInt32(v[0] * multiplier)) + "," +
           std::to_string(truncateToInt32(v[1] * multiplier)) + "," +
           std::to_string(truncateToInt32(v[2] * multiplier));
}
}  // namespace

LDrawLoader::NormalizedRay LDrawLoader::toNormalizedRay(const std::array<double, 3>& v0,
                                                       const std::array<double, 3>& v1) {
    NormalizedRay ray;
    // The direction is the segment narrowed to unit length.
    std::array<double, 3> direction{v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]};
    const double length = std::sqrt(direction[0] * direction[0] +
                                    direction[1] * direction[1] +
                                    direction[2] * direction[2]);
    if (length > 0.0) {
        for (double& component : direction) component /= length;
    }
    ray.direction = direction;

    // The origin is the foot of the perpendicular from the world origin onto the line, which is
    // the first vertex moved back along the direction by its own projection.
    const double scalar = v0[0] * direction[0] + v0[1] * direction[1] + v0[2] * direction[2];
    ray.origin = {v0[0] - direction[0] * scalar, v0[1] - direction[1] * scalar,
                  v0[2] - direction[2] * scalar};
    return ray;
}

std::string LDrawLoader::hashVertex(const std::array<double, 3>& v, double multiplier) {
    return joinNumbers(v, multiplier);
}

std::string LDrawLoader::hashEdge(const std::array<double, 3>& v0, const std::array<double, 3>& v1,
                                  double multiplier) {
    // The order matters, so an edge and its reverse are two keys.
    return hashVertex(v0, multiplier) + "_" + hashVertex(v1, multiplier);
}

std::array<double, 3> LDrawLoader::faceNormal(const std::array<double, 3>& v0,
                                             const std::array<double, 3>& v1,
                                             const std::array<double, 3>& v2) {
    // The two edges leaving the first vertex.
    const std::array<double, 3> a{v1[0] - v0[0], v1[1] - v0[1], v1[2] - v0[2]};
    const std::array<double, 3> b{v2[0] - v0[0], v2[1] - v0[1], v2[2] - v0[2]};
    // Their cross product is perpendicular to the face.
    std::array<double, 3> normal{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
                                 a[0] * b[1] - a[1] * b[0]};
    const double length = std::sqrt(normal[0] * normal[0] + normal[1] * normal[1] +
                                    normal[2] * normal[2]);
    // A face with no area has no direction.
    if (length == 0.0) return {0.0, 0.0, 0.0};
    for (double& component : normal) component /= length;
    return normal;
}

bool LDrawLoader::isSmoothPair(double dotOfFaceNormals) {
    // The absolute value is compared, so two faces that face apart still smooth together.
    const double magnitude = dotOfFaceNormals < 0 ? -dotOfFaceNormals : dotOfFaceNormals;
    return magnitude >= kHardEdgeCosine;
}

std::string LDrawLoader::cacheKey(const std::string& fileName) {
    // Lookups ignore case, so the key is the name lower cased.
    std::string key = fileName;
    for (char& c : key) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return key;
}

bool LDrawLoader::isPassthroughCode(const std::string& code, bool forEdge) {
    // A face asks with sixteen, an edge with twenty four; the other one is an ordinary code.
    if (forEdge) return code == kMainEdgeColourCode;
    return code == kMainColourCode;
}

std::string LDrawLoader::resolveColourCode(const std::string& code, const std::string& parentCode,
                                           bool forEdge) {
    // A passthrough takes the colour of whatever contains it.
    if (isPassthroughCode(code, forEdge)) return parentCode;
    return code;
}

std::string LDrawLoader::edgeColourCode(const std::string& code) {
    // The edge of a face that inherits its colour inherits the edge colour instead.
    if (code == kMainColourCode) return kMainEdgeColourCode;
    return code;
}

std::vector<std::string> LDrawLoader::splitLines(const std::string& text) {
    // The two character ending is the only thing rewritten, and only when it appears at all.
    std::string normalised = text;
    if (normalised.find("\r\n") != std::string::npos) {
        std::string collapsed;
        collapsed.reserve(normalised.size());
        for (std::size_t i = 0; i < normalised.size(); ++i) {
            if (normalised[i] == '\r' && i + 1 < normalised.size() && normalised[i + 1] == '\n') {
                continue;
            }
            collapsed.push_back(normalised[i]);
        }
        normalised = std::move(collapsed);
    }

    std::vector<std::string> lines;
    std::size_t at = 0;
    while (true) {
        const std::size_t newline = normalised.find('\n', at);
        if (newline == std::string::npos) {
            lines.push_back(normalised.substr(at));
            break;
        }
        lines.push_back(normalised.substr(at, newline - at));
        at = newline + 1;
    }
    return lines;
}

LDrawLoader::EmbeddedSplit LDrawLoader::splitEmbeddedFiles(const std::vector<std::string>& lines) {
    EmbeddedSplit split;
    // A header only begins the embedded section when it is not the first line, because the
    // first one names the file being read rather than a file inside it.
    const std::string header = "0 FILE ";
    bool parsing = false;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        if (!parsing) {
            if (i > 0 && line.compare(0, header.size(), header) == 0) {
                parsing = true;
                EmbeddedFile file;
                file.name = line.substr(header.size());
                split.files.push_back(file);
            } else {
                // Everything up to here is the body.
                split.bodyLineCount = i + 1;
            }
            continue;
        }
        if (line.compare(0, header.size(), header) == 0) {
            // A header closes the previous file and opens the next.
            EmbeddedFile file;
            file.name = line.substr(header.size());
            split.files.push_back(file);
        } else {
            // The line is kept together with the newline that followed it.
            split.files.back().text += line;
            split.files.back().text += '\n';
        }
    }
    return split;
}

bool LDrawLoader::applyBfcToken(const std::string& token, BfcState& state) {
    if (token == "CERTIFY" || token == "NOCERTIFY") {
        // Either word sets the flag to what it says and, whatever that is, puts the winding
        // back to counter clockwise.
        state.certified = token == "CERTIFY";
        state.ccw = true;
        return true;
    }
    if (token == "CW" || token == "CCW") {
        state.ccw = token == "CCW";
        return true;
    }
    if (token == "INVERTNEXT") {
        state.inverted = true;
        return true;
    }
    if (token == "CLIP" || token == "NOCLIP") {
        state.cull = token == "CLIP";
        return true;
    }
    // Anything else is reported rather than silently ignored.
    return false;
}

LDrawLoader::FinishType LDrawLoader::finishTypeFromToken(const std::string& token) {
    // The words are compared without regard to case, as the directive reader does.
    std::string upper = token;
    for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    if (upper == "CHROME") return FinishType::Chrome;
    if (upper == "PEARLESCENT") return FinishType::Pearlescent;
    if (upper == "RUBBER") return FinishType::Rubber;
    if (upper == "MATTE_METALLIC") return FinishType::MatteMetallic;
    if (upper == "METAL") return FinishType::Metal;
    // A colour that names no finish has the plain one.
    return FinishType::Default;
}

bool LDrawLoader::isPartType(const std::string& type) {
    // Both words are compared exactly and with their case, so a lower case spelling and a
    // plural do not count.
    return type == "Part" || type == "Unofficial_Part";
}

bool LDrawLoader::isPrimitiveType(const std::string& type) {
    // The primitive test looks for the word anywhere, without regard to case.
    std::string lower = type;
    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (lower.find("primitive") != std::string::npos) return true;
    // The subpart word is exact and case sensitive.
    return type == "Subpart";
}