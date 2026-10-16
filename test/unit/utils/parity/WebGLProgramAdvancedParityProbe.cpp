#include "WebGLProgramAdvancedFixture.h"
#include <iostream>
int main() {
    try {
        auto realm = JSValue::object();
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty())
                continue;
            auto c = Animation_detail::parseJSON(line);
            std::cout << ProgramAdvancedFixture::scenario(c).toJSON()->dump() << '\n';
            c = {};
            realm.collectGarbage();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
