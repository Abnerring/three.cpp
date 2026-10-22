#include "nodes/core/NodeError.h"
#include "nodes/core/StackTrace.h"

#include <cstdio>
#include "catch.hpp"

TEST_CASE("getFilteredStack parses the Chrome and Firefox frame shapes") {
    const std::string stack =
        "Error\n"
        "    at userFn (app.js:12:4)\n"
        "    at Node.js:1:1\n"
        "    at TSL/graph.js?t=123:7:2\n"
        "    userFn@main.js:3:9\n";
    const auto frames = getFilteredStack(stack);
    REQUIRE(frames.size() == 3);
    REQUIRE(frames[0].fn == "userFn");
    REQUIRE(frames[0].file == "app.js");
    REQUIRE(frames[0].line == 12);
    REQUIRE(frames[0].column == 4);
    // Node.js is on the ignore list, so the third line is dropped entirely.
    REQUIRE(frames[1].file == "graph.js");  // query stripped, path reduced to the file name
    REQUIRE(frames[1].line == 7);
    // Firefox shape: the lazy group before the at sign also keeps the leading indentation,
    // which is what the upstream regex produces too.
    REQUIRE(frames[2].fn == "    userFn");
    REQUIRE(frames[2].file == "main.js");
    REQUIRE(frames[2].line == 3);
    REQUIRE(frames[2].column == 9);
}

TEST_CASE("getFilteredStack drops unparsable lines") {
    REQUIRE(getFilteredStack("").empty());
    REQUIRE(getFilteredStack("Error\n    not a frame\n").empty());
}

TEST_CASE("StackTrace formats the top frame and the full error") {
    StackTrace empty;
    REQUIRE(empty.stack().empty());
    REQUIRE(empty.getLocation() == "[Unknown location]");
    REQUIRE(empty.getError("boom") == "boom");

    StackTrace trace("    at userFn (app.js:12:4)\n    at other (b.js:2:3)\n");
    REQUIRE(trace.stack().size() == 2);
    REQUIRE(trace.getLocation() == "\"userFn()\" at \"app.js:12\"");
    REQUIRE(trace.getError("boom") == "boom\n    at userFn (app.js:12:4)\n    at other (b.js:2:3)");
}

TEST_CASE("NodeError carries its name and optional stack trace") {
    NodeError plain("no graph");
    REQUIRE(plain.name == "NodeError");
    REQUIRE(std::string(plain.what()) == "no graph");
    REQUIRE_FALSE(plain.stackTrace.has_value());

    NodeError traced("bad node", StackTrace("    at fn (a.js:1:1)\n"));
    REQUIRE(traced.stackTrace.has_value());
    REQUIRE(traced.stackTrace->stack().size() == 1);
}
