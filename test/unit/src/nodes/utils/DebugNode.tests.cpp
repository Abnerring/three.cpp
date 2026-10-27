#include "nodes/utils/DebugNode.h"

#include "catch.hpp"

#include <string>

TEST_CASE("DebugNode's title carries its own end dashes") {
    // The two dashes at each end are part of the title rather than added by the frame, which is
    // what makes the border the same width as the title rather than two wider.
    const std::string titleText = DebugNode::title("fragment");
    REQUIRE(titleText == "--- TSL debug - fragment shader ---");
    REQUIRE(titleText.rfind("---", 0) == 0);
    REQUIRE(titleText.size() >= 3);
    REQUIRE(titleText.substr(titleText.size() - 3) == "---");
}

TEST_CASE("DebugNode's border is as long as its title") {
    const std::string titleText = DebugNode::title("fragment");
    const std::string borderText = DebugNode::border(titleText);
    REQUIRE(borderText.size() == titleText.size());
    // It is all dashes, not a mixture.
    REQUIRE(borderText.find_first_not_of('-') == std::string::npos);
    // A short title gives a short border, so the length really does follow the title.
    REQUIRE(DebugNode::border("abc") == "---");
    REQUIRE(DebugNode::border("").empty());
    // The vertex stage's title is shorter by two, so the border is too.
    REQUIRE(DebugNode::border(DebugNode::title("vertex")).size() + 2 ==
            DebugNode::border(DebugNode::title("fragment")).size());
}

TEST_CASE("DebugNode removes tabs at the start of lines only") {
    // A tab at a line start goes, and one after text stays.
    REQUIRE(DebugNode::stripLeadingTabs("\tfloat a;") == "float a;");
    REQUIRE(DebugNode::stripLeadingTabs("x\tfloat a;") == "x\tfloat a;");
    // /^\t/mg removes one tab per line, leaving the second tab.
    REQUIRE(DebugNode::stripLeadingTabs("\t\tfloat a;") == "\tfloat a;");
    // Spaces are not tabs and stay, which is the part worth pinning: a reader would expect both
    // to be treated the same.
    REQUIRE(DebugNode::stripLeadingTabs("    float a;") == "    float a;");
    // Each line is treated on its own.
    REQUIRE(DebugNode::stripLeadingTabs("\ta\n\tb") == "a\nb");
    REQUIRE(DebugNode::stripLeadingTabs("a\n\tb") == "a\nb");
    // A tab after a newline is at a line start, and one at the very beginning too.
    REQUIRE(DebugNode::stripLeadingTabs("\n\tx") == "\nx");
}

TEST_CASE("DebugNode's block has the four lines in order") {
    const std::string text = DebugNode::block("fragment", "\tfloat a;", "a");
    // The title line, the tidied code, the snippet and the border.
    REQUIRE(text.rfind("// #--- TSL debug - fragment shader ---#\n", 0) == 0);
    REQUIRE(text.find("float a;\n") != std::string::npos);
    REQUIRE(text.find("/* ... */ a /* ... */\n") != std::string::npos);
    // The closing border is the same length as the title.
    const std::string closing = "// #" + DebugNode::border(DebugNode::title("fragment")) + "#\n";
    REQUIRE(text.find(closing) != std::string::npos);
    REQUIRE(std::string(DebugNode::kTypeName) == "DebugNode");
}
