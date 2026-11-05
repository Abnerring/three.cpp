#include "utils/ColorUtils.h"

#include "core/JSValue.h"
#include "math/Color.h"

#include <array>
#include <iostream>

int main() {
    try {
        const std::array<double, 12> temperatures{
            500, 1000, 1500, 1900, 2000, 3000, 6500, 6600, 10000, 20000, 40000, 50000};

        auto samples = JSValue::array();
        std::size_t index = 0;
        for (const double kelvin : temperatures) {
            Color color;
            ColorUtils::setKelvin(color, kelvin);
            auto sample = JSValue::object(nullptr);
            sample.set("kelvin", JSValue(kelvin));
            sample.set("r", JSValue(color.r));
            sample.set("g", JSValue(color.g));
            sample.set("b", JSValue(color.b));
            sample.set("hex", JSValue(color.getHex()));
            samples.set(index++, sample);
        }

        auto output = JSValue::object(nullptr);
        output.set("samples", samples);
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
