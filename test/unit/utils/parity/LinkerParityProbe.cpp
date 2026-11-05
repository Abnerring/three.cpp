#include "transpiler/Linker.h"

#include "core/JSValue.h"

#include <iostream>
#include <memory>
#include <vector>

int main() {
    try {
        Uniform uniform("float", "uTime");
        Accessor read("uTime");
        Accessor write("uTime");
        Operator assignment("=", &write, &read);
        VariableDeclaration declaration("float", "localValue", &read);
        FunctionParameter parameter("float", "p");
        Accessor parameterAccess("p");
        For loop(nullptr, &read, nullptr, {&parameterAccess});
        Program program;
        program.body = {&uniform, &declaration, &assignment};

        Linker linker;
        linker.process(&program);

        // A function gets its own block for parameters.
        FunctionDeclaration function("void", "main", {&parameter}, {&parameterAccess});
        Program functionProgram;
        functionProgram.body = {&function};
        Linker functionLinker;
        functionLinker.process(&functionProgram);

        auto output = JSValue::object(nullptr);
        output.set("uniformAccesses", JSValue(static_cast<double>(uniform.linker.accesses.size())));
        output.set("uniformAssignments",
                   JSValue(static_cast<double>(uniform.linker.assignments.size())));
        output.set("readReferenceIsUniform", JSValue(read.linker.reference == &uniform));
        output.set("writeReferenceIsUniform", JSValue(write.linker.reference == &uniform));
        output.set("declarationReferenceIsUniform",
                   JSValue(declaration.linker.reference == nullptr));
        output.set("parameterReferenceIsParameter",
                   JSValue(parameterAccess.linker.reference == &parameter));
        output.set("parameterAccesses",
                   JSValue(static_cast<double>(parameter.linker.accesses.size())));
        output.set("blockIsNullAfterProcess", JSValue(linker.block == nullptr));
        output.set("functionBlockIsNullAfterProcess", JSValue(functionLinker.block == nullptr));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}