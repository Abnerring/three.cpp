#include "MeshStandardNodeMaterialFixtures.h"
#include "animation/AnimationUtils.h"

#include <iostream>
#include <iterator>

int main() {
    try {
        const std::string input(std::istreambuf_iterator<char>(std::cin), {});
        const auto requests = Animation_detail::parseJSON(input);
        // Match the module import that evaluates _defaultValues before cases.
        const auto module = nativeMeshStandardNodeMaterialModule();
        auto out = JSValue::array();
        for (unsigned i = 0; i < requests.length(); ++i)
            MeshStandardNodeMaterialFixtures::push(out, MeshStandardNodeMaterialFixtures::run(requests.get(JSValue(i))));
        std::cout << out.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
