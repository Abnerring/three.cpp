#include "WebGLMaterialsFixture.h"
#include <iostream>
int main() {
    try {
        const std::string input(std::istreambuf_iterator<char>(std::cin), {});
        const auto cases = Animation_detail::parseJSON(input);
        for (unsigned i = 0; i < cases.length(); ++i) {
            const auto spec = cases.get(std::to_string(i));
            std::cout << (spec.get("construct").toBoolean()
                              ? MaterialsFixture::constructScenario(spec)
                          : spec.get("native").toBoolean() ? MaterialsFixture::nativeScenario(spec)
                                                           : MaterialsFixture::scenario(spec))
                             .dump()
                      << '\n';
            cases.collectGarbage();
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
