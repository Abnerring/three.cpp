#include "loaders/PDBLoader.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <limits>

namespace {
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string lower(const std::string& text) {
    std::string out = text;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return char(std::tolower(c)); });
    return out;
}

std::string capitalize(const std::string& text) {
    if (text.empty()) return {};
    std::string out = text;
    out[0] = char(std::toupper(static_cast<unsigned char>(out[0])));
    for (std::size_t i = 1; i < out.size(); ++i)
        out[i] = char(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

std::string slice(const std::string& text, std::size_t start, std::size_t end) {
    if (start >= text.size()) return {};
    return text.substr(start, std::min(end, text.size()) - start);
}

double toNumber(const std::string& text) {
    const std::string trimmed = trim(text);
    if (trimmed.empty()) return std::numeric_limits<double>::quiet_NaN();
    return std::strtod(trimmed.c_str(), nullptr);
}

int toInt(const std::string& text) {
    const std::string trimmed = trim(text);
    // Upstream uses parseInt, which yields not a number on an empty field; zero stands in
    // here and the caller skips it exactly as upstream skips a falsy end atom.
    return trimmed.empty() ? 0 : std::atoi(trimmed.c_str());
}

std::string bondKey(int start, int end) {
    return "s" + std::to_string(std::min(start, end)) + "e" + std::to_string(std::max(start, end));
}
}  // namespace

const std::unordered_map<std::string, std::array<int, 3>>& PDBLoader::cpkTable() {
    static const std::unordered_map<std::string, std::array<int, 3>> table{
    {"h", {255, 255, 255}},
    {"he", {217, 255, 255}},
    {"li", {204, 128, 255}},
    {"be", {194, 255, 0}},
    {"b", {255, 181, 181}},
    {"c", {144, 144, 144}},
    {"n", {48, 80, 248}},
    {"o", {255, 13, 13}},
    {"f", {144, 224, 80}},
    {"ne", {179, 227, 245}},
    {"na", {171, 92, 242}},
    {"mg", {138, 255, 0}},
    {"al", {191, 166, 166}},
    {"si", {240, 200, 160}},
    {"p", {255, 128, 0}},
    {"s", {255, 255, 48}},
    {"cl", {31, 240, 31}},
    {"ar", {128, 209, 227}},
    {"k", {143, 64, 212}},
    {"ca", {61, 255, 0}},
    {"sc", {230, 230, 230}},
    {"ti", {191, 194, 199}},
    {"v", {166, 166, 171}},
    {"cr", {138, 153, 199}},
    {"mn", {156, 122, 199}},
    {"fe", {224, 102, 51}},
    {"co", {240, 144, 160}},
    {"ni", {80, 208, 80}},
    {"cu", {200, 128, 51}},
    {"zn", {125, 128, 176}},
    {"ga", {194, 143, 143}},
    {"ge", {102, 143, 143}},
    {"as", {189, 128, 227}},
    {"se", {255, 161, 0}},
    {"br", {166, 41, 41}},
    {"kr", {92, 184, 209}},
    {"rb", {112, 46, 176}},
    {"sr", {0, 255, 0}},
    {"y", {148, 255, 255}},
    {"zr", {148, 224, 224}},
    {"nb", {115, 194, 201}},
    {"mo", {84, 181, 181}},
    {"tc", {59, 158, 158}},
    {"ru", {36, 143, 143}},
    {"rh", {10, 125, 140}},
    {"pd", {0, 105, 133}},
    {"ag", {192, 192, 192}},
    {"cd", {255, 217, 143}},
    {"in", {166, 117, 115}},
    {"sn", {102, 128, 128}},
    {"sb", {158, 99, 181}},
    {"te", {212, 122, 0}},
    {"i", {148, 0, 148}},
    {"xe", {66, 158, 176}},
    {"cs", {87, 23, 143}},
    {"ba", {0, 201, 0}},
    {"la", {112, 212, 255}},
    {"ce", {255, 255, 199}},
    {"pr", {217, 255, 199}},
    {"nd", {199, 255, 199}},
    {"pm", {163, 255, 199}},
    {"sm", {143, 255, 199}},
    {"eu", {97, 255, 199}},
    {"gd", {69, 255, 199}},
    {"tb", {48, 255, 199}},
    {"dy", {31, 255, 199}},
    {"ho", {0, 255, 156}},
    {"er", {0, 230, 117}},
    {"tm", {0, 212, 82}},
    {"yb", {0, 191, 56}},
    {"lu", {0, 171, 36}},
    {"hf", {77, 194, 255}},
    {"ta", {77, 166, 255}},
    {"w", {33, 148, 214}},
    {"re", {38, 125, 171}},
    {"os", {38, 102, 150}},
    {"ir", {23, 84, 135}},
    {"pt", {208, 208, 224}},
    {"au", {255, 209, 35}},
    {"hg", {184, 184, 208}},
    {"tl", {166, 84, 77}},
    {"pb", {87, 89, 97}},
    {"bi", {158, 79, 181}},
    {"po", {171, 92, 0}},
    {"at", {117, 79, 69}},
    {"rn", {66, 130, 150}},
    {"fr", {66, 0, 102}},
    {"ra", {0, 125, 0}},
    {"ac", {112, 171, 250}},
    {"th", {0, 186, 255}},
    {"pa", {0, 161, 255}},
    {"u", {0, 143, 255}},
    {"np", {0, 128, 255}},
    {"pu", {0, 107, 255}},
    {"am", {84, 92, 242}},
    {"cm", {120, 92, 227}},
    {"bk", {138, 79, 227}},
    {"cf", {161, 54, 212}},
    {"es", {179, 31, 212}},
    {"fm", {179, 31, 186}},
    {"md", {179, 13, 166}},
    {"no", {189, 13, 135}},
    {"lr", {199, 0, 102}},
    {"rf", {204, 0, 89}},
    {"db", {209, 0, 79}},
    {"sg", {217, 0, 69}},
    {"bh", {224, 0, 56}},
    {"hs", {230, 0, 46}},
    {"mt", {235, 0, 38}},
    {"ds", {235, 0, 38}},
    {"rg", {235, 0, 38}},
    {"cn", {235, 0, 38}},
    {"uut", {235, 0, 38}},
    {"uuq", {235, 0, 38}},
    {"uup", {235, 0, 38}},
    {"uuh", {235, 0, 38}},
    {"uus", {235, 0, 38}},
    {"uuo", {235, 0, 38}},
    };
    return table;
}

PDBLoader::Result PDBLoader::parse(const std::string& text) const {
    Result result;
    std::unordered_map<std::string, std::size_t> bondHash;
    // Serial numbers are one based and not necessarily contiguous, so keep a map.
    std::vector<int> atomBySerial;

    std::size_t lineStart = 0;
    while (lineStart <= text.size()) {
        const std::size_t lineEnd = text.find('\n', lineStart);
        const std::string line =
            lineEnd == std::string::npos ? text.substr(lineStart) : text.substr(lineStart, lineEnd - lineStart);
        lineStart = lineEnd == std::string::npos ? text.size() + 1 : lineEnd + 1;

        const std::string record = slice(line, 0, 6);
        if (slice(line, 0, 4) == "ATOM" || record == "HETATM") {
            Atom atom;
            atom.x = toNumber(slice(line, 30, 38));
            atom.y = toNumber(slice(line, 38, 46));
            atom.z = toNumber(slice(line, 46, 54));
            atom.index = toInt(slice(line, 6, 11));

            std::string symbol = lower(trim(slice(line, 76, 78)));
            if (symbol.empty()) symbol = lower(trim(slice(line, 12, 14)));
            atom.symbol = symbol;
            atom.element = capitalize(symbol);
            const auto found = cpkTable().find(symbol);
            if (found != cpkTable().end()) atom.cpk.assign(found->second.begin(), found->second.end());

            result.atoms.push_back(atom);
            if (atomBySerial.size() <= std::size_t(std::max(atom.index, 0)))
                atomBySerial.resize(std::size_t(atom.index) + 1, -1);
            atomBySerial[std::size_t(atom.index)] = int(result.atoms.size()) - 1;
        } else if (record == "CONECT") {
            const int satom = toInt(slice(line, 6, 11));
            for (int column = 11; column <= 26; column += 5) {
                const int eatom = toInt(slice(line, std::size_t(column), std::size_t(column + 5)));
                if (eatom == 0) continue;  // upstream skips a falsy end atom
                const std::string key = bondKey(satom, eatom);
                if (bondHash.find(key) != bondHash.end()) continue;  // already recorded
                // The stored indices are one based minus one, matching upstream.
                result.bonds.push_back({satom - 1, eatom - 1, 1});
                bondHash[key] = result.bonds.size() - 1;
            }
        }
    }
    return result;
}

std::string PDBLoader::elementOf(const std::string& line) {
    // The two columns near the end come first, trimmed of their spaces.
    const auto slice = [&](std::size_t from, std::size_t count) {
        if (line.size() <= from) return std::string();
        const std::string piece = line.substr(from, count);
        const std::size_t first = piece.find_first_not_of(' ');
        if (first == std::string::npos) return std::string();
        const std::size_t last = piece.find_last_not_of(' ');
        return piece.substr(first, last - first + 1);
    };
    std::string element = slice(76, 2);
    // Only when those are empty does the earlier pair count.
    if (element.empty()) element = slice(12, 2);
    // The answer is lower cased whatever the file wrote.
    for (char& c : element) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return element;
}

const std::array<std::size_t, 4>& PDBLoader::bondColumns() {
    // Four partners, five columns apart.
    static const std::array<std::size_t, 4> columns{11, 16, 21, 26};
    return columns;
}

int PDBLoader::atomIndex(int oneBased) {
    // The format numbers atoms from one, so the first atom is zero here.
    return oneBased - 1;
}