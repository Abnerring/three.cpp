#include "WebGLShaderFixture.h"
#include <iostream>
int main() {
    try {
        for (int variant = 0; variant < 14; ++variant)
            for (int construction = 0; construction < 5; ++construction)
                for (int point = -1; point < 22; ++point) {
                    std::cout << ShaderFixture::run(variant, construction, point).dump() << '\n';
                }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
