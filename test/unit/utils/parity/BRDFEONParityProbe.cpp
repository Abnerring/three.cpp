#include "BRDFEONScenarios.h"
#include <iostream>

int main() {
    try {
        std::cout << BRDFEONScenarios::all().toJSON()->dump() << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
