#include "loaders/LDrawLoader.h"

#include "catch.hpp"

#include <string>
#include <vector>

TEST_CASE("LDrawLoader splits a file into lines") {
    // The single character ending is the common case.
    const auto simple = LDrawLoader::splitLines("a\nb\nc");
    REQUIRE(simple.size() == 3);
    REQUIRE(simple[0] == "a");
    REQUIRE(simple[2] == "c");
}

TEST_CASE("LDrawLoader reduces the two character ending") {
    // Every carriage return before a newline is dropped.
    const auto lines = LDrawLoader::splitLines("a\r\nb\r\nc");
    REQUIRE(lines.size() == 3);
    REQUIRE(lines[0] == "a");
    REQUIRE(lines[1] == "b");
    // A carriage return with no newline after it is not an ending and stays.
    const auto lone = LDrawLoader::splitLines("a\rb");
    REQUIRE(lone.size() == 1);
    REQUIRE(lone[0] == "a\rb");
}

TEST_CASE("LDrawLoader keeps the trailing empty line") {
    // A text that ends with a newline has an empty last line, which is what splitting does.
    const auto lines = LDrawLoader::splitLines("a\n");
    REQUIRE(lines.size() == 2);
    REQUIRE(lines[0] == "a");
    REQUIRE(lines[1].empty());
    // An empty text gives one empty line rather than none.
    REQUIRE(LDrawLoader::splitLines("").size() == 1);
}

TEST_CASE("LDrawLoader finds the embedded files") {
    // The first header names the file itself, the second begins an embedded one.
    const std::vector<std::string> lines{"0 my model", "1 16 0 0 0 1 0 0 0 1 0 0 0 1 a.dat",
                                         "0 FILE inner.ldr", "0 inner comment", "3 16 0 0 0 1 0 0 0 1 0"};
    const auto split = LDrawLoader::splitEmbeddedFiles(lines);
    // The body is the two lines before the header.
    REQUIRE(split.bodyLineCount == 2);
    REQUIRE(split.files.size() == 1);
    REQUIRE(split.files[0].name == "inner.ldr");
    // The text keeps its newlines.
    REQUIRE(split.files[0].text == "0 inner comment\n3 16 0 0 0 1 0 0 0 1 0\n");
}

TEST_CASE("LDrawLoader starts a new file at each header") {
    const std::vector<std::string> lines{"0 outer", "0 FILE one.ldr", "a", "0 FILE two.ldr", "b"};
    const auto split = LDrawLoader::splitEmbeddedFiles(lines);
    REQUIRE(split.files.size() == 2);
    REQUIRE(split.files[0].name == "one.ldr");
    REQUIRE(split.files[0].text == "a\n");
    REQUIRE(split.files[1].name == "two.ldr");
    REQUIRE(split.files[1].text == "b\n");
    // Only the opening line is in the body.
    REQUIRE(split.bodyLineCount == 1);
}

TEST_CASE("LDrawLoader reports no embedded files when there are none") {
    const std::vector<std::string> lines{"0 just a model", "3 16 0 0 0 1 0 0 0 1 0"};
    const auto split = LDrawLoader::splitEmbeddedFiles(lines);
    REQUIRE(split.files.empty());
    // Every line belongs to the body.
    REQUIRE(split.bodyLineCount == 2);
}

TEST_CASE("LDrawLoader treats a header on the first line as the file name") {
    // The very first header names the file being read, so it opens nothing.
    const std::vector<std::string> lines{"0 FILE myself.ldr", "3 16 0 0 0 1 0 0 0 1 0"};
    const auto split = LDrawLoader::splitEmbeddedFiles(lines);
    REQUIRE(split.files.empty());
    REQUIRE(split.bodyLineCount == 2);
}
