#include <iostream>
#include "math/MathUtils.h"
#include "math/Vector2.h"

int main()
{
    for (size_t i = 0; i < 10; i++)
    {
        std::cout << "CREAT UUID " + std::to_string(i) + " :" <<  MathUtils::generateUUID() << std::endl;

        std::cout << "seededRandom " + std::to_string(i) + " :" << MathUtils::seededRandom() << std::endl;
    }

    std::cout << "Hello, THREE.CPP!" << std::endl;
    return 0;
}
