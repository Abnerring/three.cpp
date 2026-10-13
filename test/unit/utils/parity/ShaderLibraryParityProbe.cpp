#include "core/JSValue.h"
#include "renderers/shaders/ShaderChunk.h"
#include "renderers/shaders/ShaderLib.h"
#include "renderers/shaders/UniformsLib.h"
#include <cmath>
#include <iostream>
void emit(JsonValue v) {
    std::cout << v.dump() << '\n';
}
#include "ShaderLibraryFixture.h"
#include "ShaderStrings.generated.h"
int main(int argc, char** argv) {
    try {
        if (argc == 3) {
            emit(cloneAssignmentCase(std::stoi(argv[2])));
            return 0;
        }
        if (argc == 4 && std::string(argv[1]) == "--enum") {
            emit(enumerationCase(std::stoi(argv[2]), std::stoi(argv[3])));
            return 0;
        }
        if (argc == 4) {
            emit(libraryInitCase(std::stoi(argv[2]), std::stoi(argv[3])));
            return 0;
        }

        shaderStringCases();
        auto chunks = nativeShaderChunk();
        Graph c;
        emit(JsonValue::Object{{"kind", "chunk"}, {"value", c.encode(chunks)}});
        auto uniforms = nativeUniformsLib(), lib = nativeShaderLib();
        Graph g;
        const auto u = g.encode(uniforms), l = g.encode(lib);
        emit(JsonValue::Object{{"kind", "library"}, {"uniforms", u}, {"lib", l}});
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
