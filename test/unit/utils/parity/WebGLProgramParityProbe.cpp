#include "WebGLProgramFixture.h"
#include <iostream>
int main() {
    try {
        // Keep one source realm, while each input record can die after its case.
        auto realm = JSValue::object();
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty())
                continue;
            auto spec = Animation_detail::parseJSON(line);
            std::cout << ProgramFixture::scenario(spec).toJSON()->dump() << '\n';
            spec = {};
            realm.collectGarbage();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
