#include "Line2NodeMaterialFixtures.h"
#include "animation/AnimationUtils.h"
#include <iostream>
#include <iterator>
int main() {
    try {
        const std::string input((std::istreambuf_iterator<char>(std::cin)), std::istreambuf_iterator<char>());
        const auto requests = Animation_detail::parseJSON(input);
        auto results = JSValue::array();
        for (std::size_t i = 0; i < requests.length(); ++i)
            Line2NodeMaterialFixtures::push(results, Line2NodeMaterialFixtures::run(requests.get(JSValue(i))));
        std::cout << results.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
