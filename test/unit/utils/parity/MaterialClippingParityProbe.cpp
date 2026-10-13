#include "MaterialClippingFixture.h"
#include <iostream>
int main() {
    try {
        for (unsigned mode = 0; mode < 25; ++mode) {
            std::cout << MaterialClippingFixture::scenario(mode).dump() << '\n';
            JSValue::object().collectGarbage();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
