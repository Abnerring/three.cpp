#include "transpiler/Linker.h"

#include "catch.hpp"

#include <stdexcept>

TEST_CASE("Linker records reads and writes on the resolved node") {
    Uniform uniform("float", "uTime");
    Accessor read("uTime");
    Accessor write("uTime");
    Operator assignment("=", &write, &read);
    Program program;
    program.body = {&uniform, &assignment};

    Linker linker;
    linker.process(&program);
    REQUIRE(read.linker.reference == &uniform);
    REQUIRE(write.linker.reference == &uniform);
    REQUIRE(uniform.linker.accesses.size() == 2);
    REQUIRE(uniform.linker.assignments.size() == 1);
    REQUIRE(uniform.linker.assignments[0] == &assignment);
}

TEST_CASE("Linker reports a block mismatch when removing") {
    Linker linker;
    Program first;
    Program second;
    linker.addBlock(&first);
    REQUIRE_THROWS_AS(linker.removeBlock(&second), std::runtime_error);
    linker.removeBlock(&first);
    REQUIRE(linker.block == nullptr);
}

TEST_CASE("Linker keeps function parameters in their own block") {
    FunctionParameter parameter("float", "p");
    Accessor inside("p");
    FunctionDeclaration function("void", "main", {&parameter}, {&inside});
    Program program;
    program.body = {&function};

    Linker linker;
    linker.process(&program);
    REQUIRE(inside.linker.reference == &parameter);
    REQUIRE(parameter.linker.accesses.size() == 1);
}

TEST_CASE("Linker follows loop conditions and function call arguments") {
    Uniform uniform("float", "uValue");
    Accessor loopCondition("uValue");
    Accessor callArgument("uValue");
    For loop(nullptr, &loopCondition, nullptr, {});
    FunctionCall call("doSomething", {&callArgument});
    Program program;
    program.body = {&uniform, &loop, &call};

    Linker linker;
    linker.process(&program);
    REQUIRE(loopCondition.linker.reference == &uniform);
    REQUIRE(callArgument.linker.reference == &uniform);
    // Each accessor records itself once on the resolved node.
    REQUIRE(uniform.linker.accesses.size() == 2);
}

TEST_CASE("Linker covers the conditional, ternary and accessor element branches") {
    Uniform uniform("float", "uValue");

    Accessor ternaryCond("uValue");
    Accessor ternaryLeft("uValue");
    Accessor ternaryRight("uValue");
    Ternary ternary(&ternaryCond, &ternaryLeft, &ternaryRight);

    Accessor conditionalCond("uValue");
    Accessor bodyAccess("uValue");
    Conditional conditional(&conditionalCond, {&bodyAccess});

    Accessor objectAccess("uValue");
    Accessor staticAccess("uValue");
    Accessor dynamicAccess("uValue");
    StaticElement staticElement(&staticAccess);
    DynamicElement dynamicElement(&dynamicAccess);
    std::vector<ASTNode*> elementList{&staticElement, &dynamicElement};
    AccessorElements elements(&objectAccess, elementList);

    std::vector<ASTNode*> programBody{&uniform, &ternary, &conditional, &elements};
    Program program;
    program.body = programBody;

    Linker linker;
    linker.process(&program);
    REQUIRE(ternaryCond.linker.reference == &uniform);
    REQUIRE(ternaryLeft.linker.reference == &uniform);
    REQUIRE(ternaryRight.linker.reference == &uniform);
    REQUIRE(conditionalCond.linker.reference == &uniform);
    REQUIRE(bodyAccess.linker.reference == &uniform);
    REQUIRE(objectAccess.linker.reference == &uniform);
    REQUIRE(staticAccess.linker.reference == &uniform);
    REQUIRE(dynamicAccess.linker.reference == &uniform);
    // Three ternary accessors, two conditional accessors and three element accessors.
    REQUIRE(uniform.linker.accesses.size() == 8);
}