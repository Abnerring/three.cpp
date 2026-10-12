#include "catch.hpp"
#include "core/OrderedMap.h"
#include "core/JSValue.h"
#include <numeric>

namespace {
template<class T> std::vector<std::string> orderedKeys(const OrderedMap<T>& values) {
    std::vector<std::string> keys;
    for (const auto& [key, value] : values) keys.push_back(key);
    return keys;
}
}

TEST_CASE("Ordered map preserves numeric ordering and stable values while indexes grow", "[core][orderedmap]") {
    OrderedMap<int> values{{"tail", -1}, {"01", -2}, {"4294967295", -3}};
    auto* stable = &values.at("tail");
    for (int i = 32767; i >= 0; --i) values[std::to_string(i)] = i;
    REQUIRE(&values.at("tail") == stable);
    REQUIRE(values.size() == 32771);
    int expected = 0;
    for (const auto& [key, value] : values) {
        if (expected == 32768) break;
        REQUIRE(key == std::to_string(expected));
        REQUIRE(value == expected++);
    }
    values.erase("0"); values.erase("tail"); values["tail"] = 7;
    auto keys = orderedKeys(values);
    REQUIRE(keys.front() == "1");
    REQUIRE(keys[keys.size()-3] == "01");
    REQUIRE(keys[keys.size()-2] == "4294967295");
    REQUIRE(keys.back() == "tail");
    values["0"] = 19;
    REQUIRE(values.begin()->first == "0");
    REQUIRE_FALSE(values.contains("missing"));
}

TEST_CASE("Ordered map copy move erase and clear keep independent lookup indexes", "[core][orderedmap]") {
    OrderedMap<int> original{{"b", 4}, {"3", 3}, {"1", 1}, {"a", 5}};
    auto copied = original;
    original.erase("1"); original["2"] = 2;
    REQUIRE(copied.at("1") == 1);
    REQUIRE_FALSE(copied.contains("2"));
    auto* stable = &copied.at("3");
    auto retained = copied.find("3");
    OrderedMap<int> moved(std::move(copied));
    REQUIRE(&moved.at("3") == stable);
    REQUIRE(copied.empty());
    REQUIRE(retained == moved.find("3"));
    REQUIRE(std::prev(moved.cend())->first == "a");
    REQUIRE(std::next(retained, 3) == moved.end());
    copied["9"] = 9;
    REQUIRE_FALSE(moved.contains("9"));
    original = moved;
    REQUIRE(orderedKeys(original) == std::vector<std::string>{"1","3","b","a"});
    original["b"] = 17;
    REQUIRE(moved.at("b") == 4);
    original = std::move(moved);
    REQUIRE(&original.at("3") == stable);
    REQUIRE(moved.empty());
    moved["fresh"] = 1;
    original.swap(moved);
    REQUIRE(&moved.at("3") == stable);
    REQUIRE(original.at("fresh") == 1);
    REQUIRE(retained == moved.find("3"));
    REQUIRE(std::next(retained, 3) == moved.cend());
    moved.clear(); moved["first"] = 8; moved["0"] = 0;
    REQUIRE(orderedKeys(moved) == std::vector<std::string>{"0","first"});
}

TEST_CASE("Large managed JSON arrays round trip and retain sparse mutation semantics", "[core][runtime][orderedmap]") {
    JsonValue::Array input;
    for (int i = 0; i < 8192; ++i) input.emplace_back(i);
    auto array = JSValue::fromJSON(input);
    REQUIRE(array.length() == input.size());
    REQUIRE(*array.toJSON() == JsonValue(input));
    array.erase("17");
    REQUIRE(array.length() == input.size());
    REQUIRE(array.get("17").isUndefined());
    array.set("17", -17);
    REQUIRE(array.keys()[17] == "17");
    array.set("length", 32);
    REQUIRE(array.length() == 32);
    REQUIRE(array.keys().size() == 32);
    REQUIRE(array.get("17").number() == -17);
    REQUIRE(array.get("32").isUndefined());
}
