#include "loaders/XmlDocument.h"

#include <cctype>
#include <stdexcept>

namespace {
std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

[[noreturn]] void fail(const char* message) {
    throw std::runtime_error(std::string("XmlDocument: ") + message);
}

// Reads a name: letters, digits, and the punctuation XML allows in them.
std::string readName(const std::string& text, std::size_t& i) {
    const std::size_t start = i;
    while (i < text.size()) {
        const char c = text[i];
        const bool allowed = std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' ||
                             c == ':' || c == '.';
        if (!allowed) break;
        ++i;
    }
    if (i == start) fail("expected a tag name");
    return text.substr(start, i - start);
}
}  // namespace

std::shared_ptr<XmlNode> XmlNode::child(const std::string& childName) const {
    for (const auto& node : children)
        if (node && node->name == childName) return node;
    return nullptr;
}

std::vector<std::shared_ptr<XmlNode>> XmlNode::childrenNamed(const std::string& childName) const {
    std::vector<std::shared_ptr<XmlNode>> out;
    for (const auto& node : children)
        if (node && node->name == childName) out.push_back(node);
    return out;
}

std::string XmlNode::attributeOr(const std::string& key, const std::string& fallback) const {
    const auto found = attributes.find(key);
    return found == attributes.end() ? fallback : found->second;
}

std::shared_ptr<XmlNode> XmlDocument::parse(const std::string& text) {
    std::shared_ptr<XmlNode> root;
    // The open element stack, so a closing tag can be matched against its opener.
    std::vector<std::shared_ptr<XmlNode>> stack;
    std::string pendingText;

    const auto flushText = [&]() {
        if (stack.empty()) {
            pendingText.clear();
            return;
        }
        const std::string trimmed = trim(pendingText);
        if (!trimmed.empty()) {
            // Text is kept with the surrounding whitespace removed, which is what the addon
            // formats rely on for numbers.
            stack.back()->text += trimmed;
        }
        pendingText.clear();
    };

    std::size_t i = 0;
    while (i < text.size()) {
        if (text[i] != '<') {
            pendingText.push_back(text[i]);
            ++i;
            continue;
        }

        // Comments and the declaration carry no element, so they are skipped whole.
        if (text.compare(i, 4, "<!--") == 0) {
            const std::size_t end = text.find("-->", i + 4);
            if (end == std::string::npos) fail("unterminated comment");
            i = end + 3;
            continue;
        }
        if (text.compare(i, 2, "<?") == 0) {
            const std::size_t end = text.find("?>", i + 2);
            if (end == std::string::npos) fail("unterminated declaration");
            i = end + 2;
            continue;
        }

        ++i;
        if (i >= text.size()) fail("unterminated tag");

        // A closing tag pops the matching element.
        if (text[i] == '/') {
            ++i;
            const std::string name = readName(text, i);
            while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
            if (i >= text.size() || text[i] != '>') fail("unterminated closing tag");
            ++i;
            flushText();
            if (stack.empty() || stack.back()->name != name) fail("mismatched closing tag");
            stack.pop_back();
            continue;
        }

        flushText();
        const std::string name = readName(text, i);
        auto node = std::make_shared<XmlNode>();
        node->name = name;

        bool selfClosing = false;
        while (i < text.size()) {
            while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
            if (i >= text.size()) fail("unterminated tag");
            if (text[i] == '/') {
                selfClosing = true;
                ++i;
                continue;
            }
            if (text[i] == '>') {
                ++i;
                break;
            }
            // A bare word is not an attribute; stop so the caller sees the malformed tag.
            if (!std::isalpha(static_cast<unsigned char>(text[i])) && text[i] != '_')
                fail("expected an attribute name");
            const std::string key = readName(text, i);
            while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
            if (i >= text.size() || text[i] != '=') fail("expected an equals sign");
            ++i;
            while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i]))) ++i;
            if (i >= text.size() || (text[i] != '"' && text[i] != '\'')) fail("expected a quoted value");
            const char quote = text[i++];
            const std::size_t end = text.find(quote, i);
            if (end == std::string::npos) fail("unterminated attribute value");
            node->attributes[key] = text.substr(i, end - i);
            i = end + 1;
        }

        if (stack.empty()) {
            if (!root) root = node;
            else fail("more than one root element");
        } else {
            stack.back()->children.push_back(node);
        }
        if (!selfClosing) stack.push_back(node);
    }

    if (!stack.empty()) fail("unclosed element");
    if (!root) fail("no element found");
    return root;
}
