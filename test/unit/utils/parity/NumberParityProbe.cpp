#include "core/JSValue.h"
#ifndef NUMBER_BASELINE
#include "core/JSNumber.h"
#endif
#include <bit>
#include <cstdint>
#include <iostream>
#include <string>
int main(int argc, char** argv) {
    const auto realm = JSValue::object();
    std::string value, digits;
    while (std::cin >> value >> digits) {
        const auto number = JSValue(std::bit_cast<double>(std::stoull(value, nullptr, 16)));
        const auto fraction = JSValue(std::bit_cast<double>(std::stoull(digits, nullptr, 16)));
        try {
            std::cout << number.get("toFixed").call(number, {fraction}).toJSON()->dump() << '\n';
        } catch (const JSRangeError&) {
            std::cout << "{\"error\":\"RangeError\"}\n";
        } catch (const JSTypeError&) {
            std::cout << "{\"error\":\"TypeError\"}\n";
        } catch (const std::exception& e) {
            std::cout << JsonValue(JsonValue::Object{{"nativeError", e.what()}}).dump() << '\n';
        }
    }
}
