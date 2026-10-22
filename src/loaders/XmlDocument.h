#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

// A small XML reader for the addon formats, since the port carries no DOM. It handles the
// subset those files use: elements with attributes, self closing elements, text content,
// comments and the declaration line. Anything malformed is reported rather than guessed at.
struct XmlNode {
    std::string name;
    std::map<std::string, std::string> attributes;
    // Everything between the tags that is not itself a tag, trimmed.
    std::string text;
    std::vector<std::shared_ptr<XmlNode>> children;

    // The first child with this name, or null.
    std::shared_ptr<XmlNode> child(const std::string& childName) const;

    // Every child with this name, in document order.
    std::vector<std::shared_ptr<XmlNode>> childrenNamed(const std::string& childName) const;

    // An attribute value, or the fallback when it is absent.
    std::string attributeOr(const std::string& key, const std::string& fallback = "") const;
};

class XmlDocument {
  public:
    // Parses text into a root element. Throws std::runtime_error when a tag is unterminated,
    // when a closing tag does not match the open one, or when no element is found.
    static std::shared_ptr<XmlNode> parse(const std::string& text);
};

