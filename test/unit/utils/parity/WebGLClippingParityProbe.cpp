#include "WebGLClippingFixture.h"
#include <iostream>
using namespace ClippingFixture;
int main() {
    try {
        const auto emit = [](JsonValue value) {
            std::cout << value.dump() << '\n';
            JSValue::object().collectGarbage();
        };
        for (unsigned m = 0; m < 10; ++m)
            for (unsigned p = 0; p < 8; ++p)
                for (unsigned mode = 0; mode < 12; ++mode)
                    emit(sequenceCase(m, p, mode));
        for (unsigned point = 0; point < 34; ++point)
            for (unsigned action = 0; action < 5; ++action)
                emit(observedCase(point, action));
        for (unsigned method = 0; method < 6; ++method)
            for (unsigned mode = 0; mode < 5; ++mode)
                emit(constructCase(method, mode));
        for (unsigned length = 0; length < 12; ++length)
            for (unsigned enabled = 0; enabled < 12; ++enabled)
                emit(initCase(length, enabled));
        for (unsigned mode = 0; mode < 24; ++mode)
            for (unsigned skip = 0; skip < 3; ++skip)
                emit(cacheCase(mode, skip));
        for (unsigned point = 0; point < 13; ++point)
            for (unsigned action = 0; action < 5; ++action)
                emit(methodCase(point, action));
        for (unsigned point = 0; point < 15; ++point)
            for (unsigned mode = 0; mode < 3; ++mode)
                emit(assignmentCase(point, mode));
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
