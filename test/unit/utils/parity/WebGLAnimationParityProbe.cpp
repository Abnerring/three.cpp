#include "WebGLAnimationFixture.h"
#include <iostream>
int main() {
    for (int mode = 0; mode < 19; ++mode)
        for (int failAt = 0; failAt <= 32; ++failAt)
            std::cout << WebGLAnimationParity::scenario(mode, failAt).dump() << '\n';
    std::cout << WebGLAnimationParity::metadata().dump() << '\n';
}
