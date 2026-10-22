#include "loaders/XmlDocument.h"

#include "catch.hpp"

#include <string>

TEST_CASE("XmlDocument parses nested elements and text") {
    const auto root = XmlDocument::parse("<a><b>1</b><c>2</c></a>");
    REQUIRE(root != nullptr);
    REQUIRE(root->name == "a");
    REQUIRE(root->children.size() == 2);
    REQUIRE(root->children[0]->name == "b");
    REQUIRE(root->children[0]->text == "1");
    REQUIRE(root->children[1]->text == "2");
    // The lookup helpers find by name.
    REQUIRE(root->child("c") != nullptr);
    REQUIRE(root->child("missing") == nullptr);
    REQUIRE(root->childrenNamed("b").size() == 1);
}

TEST_CASE("XmlDocument reads attributes") {
    const auto root = XmlDocument::parse("<a x=\"1\" y='two'/>");
    REQUIRE(root->attributes.at("x") == "1");
    // Single quotes are accepted too.
    REQUIRE(root->attributes.at("y") == "two");
    REQUIRE(root->attributeOr("z", "fallback") == "fallback");
    // A self closing element has no children.
    REQUIRE(root->children.empty());
}

TEST_CASE("XmlDocument skips the declaration and comments") {
    const auto root = XmlDocument::parse("<?xml version=\"1.0\"?><!-- a note --><a>1</a>");
    REQUIRE(root->name == "a");
    REQUIRE(root->text == "1");
}

TEST_CASE("XmlDocument trims text and joins mixed runs") {
    const auto root = XmlDocument::parse("<a>\n  hello \n</a>");
    REQUIRE(root->text == "hello");
}

TEST_CASE("XmlDocument reports malformed documents") {
    REQUIRE_THROWS(XmlDocument::parse("<a><b></a>"));
    REQUIRE_THROWS(XmlDocument::parse("<a>"));
    REQUIRE_THROWS(XmlDocument::parse("no elements here"));
    REQUIRE_THROWS(XmlDocument::parse("<a x=1/>"));
}
