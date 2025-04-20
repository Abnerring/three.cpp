#include <iostream>
#include "math/MathUtils.h"
#include "math/Vector2.h"

int main()
{
    for (size_t i = 0; i < 10; i++)
    {
        std::cout << "CREAT UUID " + std::to_string(i) + " :" <<  MathUtils::generateUUID() << std::endl;

        std::cout << "seededRandom " + std::to_string(i) + " :" << MathUtils::seededRandom() << std::endl;

        std::cout << "random " + std::to_string(i) + " :" << MathUtils::randInt(10, 100) << std::endl;

    }

    Vector2 v1(1.0, 2.0);
    Vector2 v2(3.0, 4.0);

    auto v3 = v1 + v2;
    std::cout << "v1 + v2 = (" << v3.x() << ", " << v3.y() << ")" << std::endl;

    auto v4 = v1 / v2;
    std::cout << "v1 / v2 = (" << v4.x() << ", " << v4.y() << ")" << std::endl;

    std::cout << "Hello, THREE.CPP!" << std::endl;
    return 0;
}
