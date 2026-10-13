#include "LayersUniformsFixture.h"
#include <iostream>
int main() {
    std::cout << JsonValue(LayersUniformsFixture::run()).dump();
}
