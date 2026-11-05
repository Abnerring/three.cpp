#include "nodes/functions/BSDF/LTC.h"

#include "core/JSValue.h"
#include "nodes/core/Node.h"
#include "nodes/tsl/TSLBase.h"
#include "LTCFixtures.h"

#include <iostream>

int main() {
    try {
        const auto output = LTCFixtures::snapshot();
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
