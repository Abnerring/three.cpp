#include "MeshSSSNodeMaterialScenarios.h"
#include <iostream>

int main() {
    try {
        std::cout << MeshSSSNodeMaterialScenarios::all().toJSON()->dump() << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
