#include "catch.hpp"
#include "animation/AnimationUtils.h"
#include "core/JSValue.h"
#include "loaders/AnimationLoader.h"
#include <string>

// The parse is asserted through the array it returns, which is the surface the pinned class also
// exposes. Two things matter here: the JSON text goes through Animation_detail::parseJSON (a
// JsonValue built from a std::string is a string value, not parsed JSON), and array elements are
// read with a string index, because an integer index selects the name overload of get.
namespace {
JSValue parse(const std::string& json) {
    AnimationLoader loader;
    return loader.parse(Animation_detail::parseJSON(json));
}
double lengthOf(const JSValue& clips) { return clips.get("length").toNumber(); }
JSValue clipAt(const JSValue& clips, int index) { return clips.get(std::to_string(index)); }
}

TEST_CASE("AnimationLoader parses an empty list", "[animationloader]") {
    const auto clips = parse("[]");
    REQUIRE(lengthOf(clips) == 0);
}

TEST_CASE("AnimationLoader parses one clip", "[animationloader]") {
    const auto clips = parse("[{\"name\":\"walk\",\"duration\":2.5,\"tracks\":[]}]");
    REQUIRE(lengthOf(clips) == 1);
    REQUIRE(clipAt(clips, 0).get("name").toString() == "walk");
    REQUIRE(clipAt(clips, 0).get("duration").toNumber() == 2.5);
}

TEST_CASE("AnimationLoader parses several clips in order", "[animationloader]") {
    const auto clips = parse("[{\"name\":\"a\",\"duration\":1,\"tracks\":[]},{\"name\":\"b\",\"duration\":2,\"tracks\":[]}]");
    REQUIRE(lengthOf(clips) == 2);
    REQUIRE(clipAt(clips, 0).get("name").toString() == "a");
    REQUIRE(clipAt(clips, 1).get("name").toString() == "b");
    REQUIRE(clipAt(clips, 1).get("duration").toNumber() == 2.0);
}