#include "Three.Legacy.h"

#include "core/JSValue.h"

#include <iostream>
#include <string>

int main() {
    try {
        const auto module = nativeThreeLegacyModule();
        auto names = JSValue::array();
        std::size_t index = 0;
        for (const auto& name : module.ownPropertyNames()) names.set(index++, JSValue(name));

        auto output = JSValue::object(nullptr);
        output.set("isObject", module.isObject());
        output.set("ownKeys", names);
        output.set("ownKeyCount", JSValue(static_cast<double>(module.ownPropertyNames().size())));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
