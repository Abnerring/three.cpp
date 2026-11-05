#include "transpiler/AST.h"

#include "core/JSValue.h"

#include <iostream>
#include <memory>
#include <vector>

namespace {
JSValue optionalString(const std::optional<std::string>& value) {
    return value.has_value() ? JSValue(*value) : JSValue(nullptr);
}
}  // namespace

int main() {
    try {
        Number floatOne(1.0);
        Number intTwo(2.0, "int");
        String text("hello");
        Uniform vec3Uniform("vec3", "uColor");
        Uniform intUniform("int", "uCount");

        auto operators = JSValue::object(nullptr);
        for (const char* op : {"+", "+=", ">>>=", "<"}) {
            Operator candidate(op, &floatOne, &intTwo);
            auto entry = JSValue::object(nullptr);
            entry.set("isAssignment", JSValue(candidate.isAssignmentValue()));
            entry.set("isNumeric", JSValue(candidate.isNumericExpression()));
            entry.set("type", optionalString(candidate.getType()));
            operators.set(op, entry);
        }
        Operator stringConcat("+", &floatOne, &text);
        Operator typeMismatch("+", &floatOne, &vec3Uniform);

        auto unaries = JSValue::object(nullptr);
        for (const char* op : {"++", "--", "-"}) {
            Unary candidate(op, &floatOne);
            auto entry = JSValue::object(nullptr);
            entry.set("isAssignment", JSValue(candidate.isAssignmentValue()));
            entry.set("isNumeric", JSValue(candidate.isNumericExpression()));
            unaries.set(op, entry);
        }

        VariableDeclaration bare("float", "x");
        VariableDeclaration withValue("float", "y", &floatOne);
        VariableDeclaration chained("float", "z", nullptr, &bare);

        FunctionCall builtinCall("float", {&intTwo});
        FunctionCall referencedCall("myVec3");
        referencedCall.linker.reference = &vec3Uniform;
        FunctionCall plainCall("myOther");

        Accessor accessor("x");
        accessor.linker.reference = &intUniform;
        Accessor plainAccessor("y");

        SwitchCase defaultCase({}, nullptr);
        SwitchCase conditionalCase({}, &intTwo);

        Uniform programStatement("float", "uA");
        std::vector<ASTNode*> statements{&programStatement};
        Program program(statements);

        auto output = JSValue::object(nullptr);
        output.set("operators", operators);
        output.set("stringConcatNumeric", JSValue(stringConcat.isNumericExpression()));
        output.set("typeMismatch", optionalString(typeMismatch.getType()));
        output.set("unaries", unaries);
        output.set("bareDeclarationAssignment", JSValue(bare.isAssignmentValue()));
        output.set("valueDeclarationAssignment", JSValue(withValue.isAssignmentValue()));
        output.set("chainedNextAssignment", JSValue(chained.isAssignmentValue()));
        output.set("childHasAssignment", JSValue(withValue.hasAssignment()));
        output.set("chainedChildHasAssignment", JSValue(bare.hasAssignment()));
        output.set("builtinCallType", optionalString(builtinCall.getType()));
        output.set("referencedCallType", optionalString(referencedCall.getType()));
        output.set("plainCallType", optionalString(plainCall.getType()));
        output.set("accessorType", optionalString(accessor.getType()));
        output.set("plainAccessorType", optionalString(plainAccessor.getType()));
        output.set("defaultCaseIsDefault", JSValue(defaultCase.isDefault));
        output.set("conditionalCaseIsDefault", JSValue(conditionalCase.isDefault));
        output.set("programChildIsProgram", JSValue(programStatement.getProgram() == &program));
        output.set("numberNumeric", JSValue(floatOne.isNumericExpression()));
        output.set("numberType", optionalString(floatOne.getType()));
        output.set("stringNodeType", optionalString(text.getType()));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}