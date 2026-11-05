#include "transpiler/AST.h"

#include "catch.hpp"

TEST_CASE("ASTNode carries the pinned base flags and helpers") {
    ASTNode node;
    REQUIRE(node.isASTNode);
    REQUIRE(node.parent == nullptr);
    REQUIRE(node.linker.reference == nullptr);
    REQUIRE(node.linker.accesses.empty());
    REQUIRE(node.linker.assignments.empty());
    REQUIRE_FALSE(node.isNumericExpression());
    REQUIRE_FALSE(node.hasAssignment());
    REQUIRE_FALSE(node.getType().has_value());
    REQUIRE(node.getProgram() == nullptr);
    REQUIRE(node.getParent().empty());
}

TEST_CASE("ASTNode hasAssignment walks the parent chain") {
    ASTNode parent;
    ASTNode child;
    child.parent = &parent;
    REQUIRE_FALSE(child.hasAssignment());
    parent.isAssignmentField = true;
    REQUIRE(child.hasAssignment());
}

TEST_CASE("ASTNode getParent collects the ancestors nearest first") {
    ASTNode grandparent;
    ASTNode parent;
    ASTNode child;
    parent.parent = &grandparent;
    child.parent = &parent;
    const auto ancestors = child.getParent();
    REQUIRE(ancestors.size() == 2);
    REQUIRE(ancestors[0] == &parent);
    REQUIRE(ancestors[1] == &grandparent);
}

TEST_CASE("AST leaf nodes keep their pinned fields and type") {
    Comment comment("// hello");
    REQUIRE(comment.isComment);
    REQUIRE(comment.comment == "// hello");
    REQUIRE_FALSE(comment.getType().has_value());

    Uniform uniform("float", "uTime");
    REQUIRE(uniform.isUniform);
    REQUIRE(uniform.type == "float");
    REQUIRE(uniform.name == "uTime");
    REQUIRE(uniform.getType().has_value());
    REQUIRE(*uniform.getType() == "float");

    Varying varying("vec3", "vColor");
    REQUIRE(varying.isVarying);
    REQUIRE(varying.name == "vColor");
    REQUIRE(*varying.getType() == "vec3");
}
TEST_CASE("AST container nodes adopt their children and report the program root") {
    Uniform first("float", "uA");
    Uniform second("float", "uB");
    std::vector<ASTNode*> statements{&first, &second};
    Program program(statements);

    REQUIRE(program.isProgram);
    REQUIRE(program.isProgramNode());
    REQUIRE(program.body.size() == 2);
    REQUIRE(first.parent == &program);
    REQUIRE(second.parent == &program);
    // A child climbs to the program root.
    REQUIRE(first.getProgram() == &program);
    REQUIRE(program.getProgram() == &program);

    Expression expression(&first);
    REQUIRE(expression.isExpression);
    REQUIRE(expression.expression == &first);
    REQUIRE(first.parent == &expression);
    REQUIRE(expression.getProgram() == nullptr);

    Ternary ternary(&first, &second, &first);
    REQUIRE(ternary.isTernary);
    REQUIRE(ternary.cond == &first);
    REQUIRE(ternary.left == &second);
    REQUIRE(ternary.right == &first);
    REQUIRE(first.parent == &ternary);
    REQUIRE(second.parent == &ternary);
}
TEST_CASE("AST variable declarations compute the assignment predicate from their value") {
    VariableDeclaration bare("float", "x");
    REQUIRE(bare.isVariableDeclaration);
    REQUIRE(bare.type == "float");
    REQUIRE(bare.name == "x");
    REQUIRE(bare.value == nullptr);
    REQUIRE_FALSE(bare.isAssignmentValue());
    REQUIRE(*bare.getType() == "float");

    Uniform initializer("float", "one");
    VariableDeclaration assigned("float", "y", &initializer);
    REQUIRE(assigned.isAssignmentValue());
    REQUIRE(initializer.parent == &assigned);
    // A child of an assignment-carrying declaration inherits hasAssignment through the parent chain.
    REQUIRE(assigned.hasAssignment());
}

TEST_CASE("AST function parameter and declaration keep their payloads and adopt children") {
    FunctionParameter parameter("vec3", "p", std::string("in"));
    REQUIRE(parameter.isFunctionParameter);
    REQUIRE(parameter.type == "vec3");
    REQUIRE(parameter.name == "p");
    REQUIRE(parameter.qualifier.has_value());
    REQUIRE(*parameter.qualifier == "in");
    REQUIRE(parameter.immutable == true);

    Uniform firstStatement("float", "uA");
    Uniform secondStatement("float", "uB");
    std::vector<ASTNode*> params{&parameter};
    std::vector<ASTNode*> body{&firstStatement, &secondStatement};
    FunctionDeclaration function("void", "main", params, body);
    REQUIRE(function.isFunctionDeclaration);
    REQUIRE(function.name == "main");
    REQUIRE(function.params.size() == 1);
    REQUIRE(function.body.size() == 2);
    REQUIRE(parameter.parent == &function);
    REQUIRE(firstStatement.parent == &function);
    REQUIRE(secondStatement.parent == &function);
    REQUIRE(firstStatement.getProgram() == nullptr);
}

