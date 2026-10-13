#include "NativeMaterialsMathFixture.h"
#include <iostream>
using namespace MaterialsMathFixture;
int main() {
    try {
        A out;
        for (unsigned seed = 0; seed < 128; ++seed)
            for (bool native : {false, true})
                out.push_back(numeric(seed, native));
        for (unsigned seed = 0; seed < 64; ++seed)
            for (unsigned mode = 0; mode < 7; ++mode)
                out.push_back(colorNumeric(seed, mode));
        for (unsigned kind = 0; kind < 8; ++kind)
            for (unsigned mode = 0; mode < 5; ++mode)
                out.push_back(observed(kind, mode));
        for (unsigned mode = 0; mode < 3; ++mode)
            out.push_back(reentry(mode));
        for (unsigned mode = 0; mode < 4; ++mode)
            out.push_back(chaining(mode));
        for (unsigned seed = 0; seed < 1000; ++seed)
            out.push_back(trig(seed));
        for (unsigned mode = 0; mode < 6; ++mode)
            out.push_back(colorProtocol(mode));
        for (unsigned mode = 0; mode < 4; ++mode)
            out.push_back(textureNative(mode));
        std::cout << JsonValue(out).dump();
    } catch (const std::exception& e) {
        std::cerr << e.what();
        return 1;
    }
}
