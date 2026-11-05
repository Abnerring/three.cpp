#include "transpiler/TranspilerUtils.h"

#include "core/JSValue.h"

#include "catch.hpp"

#include <string>

namespace {
JSValue flags(std::initializer_list<const char*> trueFlags) {
    auto statement = JSValue::object(nullptr);
    for (const char* flag : trueFlags) statement.set(flag, true);
    return statement;
}
}  // namespace

TEST_CASE("TranspilerUtils isExpression reads the statement flags") {
    REQUIRE(TranspilerUtils::isExpression(JSValue::object(nullptr)));
    // A plain expression statement has none of the declaration flags.
    auto expression = JSValue::object(nullptr);
    expression.set("isExpression", true);
    REQUIRE(TranspilerUtils::isExpression(expression));

    REQUIRE_FALSE(TranspilerUtils::isExpression(flags({"isFunctionDeclaration"})));
    REQUIRE_FALSE(TranspilerUtils::isExpression(flags({"isFor"})));
    REQUIRE_FALSE(TranspilerUtils::isExpression(flags({"isWhile"})));
    REQUIRE_FALSE(TranspilerUtils::isExpression(flags({"isConditional"})));
    REQUIRE_FALSE(TranspilerUtils::isExpression(flags({"isSwitch"})));
    REQUIRE_FALSE(TranspilerUtils::isExpression(flags({"isStructDefinition"})));
    // Only the strict value true disqualifies a statement.
    auto notStrict = JSValue::object(nullptr);
    notStrict.set("isFor", JSValue(1));
    REQUIRE(TranspilerUtils::isExpression(notStrict));
}

TEST_CASE("TranspilerUtils classifies literals and builtin types") {
    REQUIRE(TranspilerUtils::isPrimitive("true"));
    REQUIRE(TranspilerUtils::isPrimitive("false"));
    REQUIRE(TranspilerUtils::isPrimitive("123"));
    REQUIRE(TranspilerUtils::isPrimitive("-7"));
    REQUIRE(TranspilerUtils::isPrimitive(".5"));
    REQUIRE_FALSE(TranspilerUtils::isPrimitive("identifier"));
    REQUIRE_FALSE(TranspilerUtils::isPrimitive(""));
    REQUIRE_FALSE(TranspilerUtils::isPrimitive(" true"));

    REQUIRE(TranspilerUtils::isBuiltinType("void"));
    REQUIRE(TranspilerUtils::isBuiltinType("bool"));
    REQUIRE(TranspilerUtils::isBuiltinType("float"));
    REQUIRE(TranspilerUtils::isBuiltinType("int"));
    REQUIRE(TranspilerUtils::isBuiltinType("uint"));
    REQUIRE(TranspilerUtils::isBuiltinType("mat4"));
    REQUIRE(TranspilerUtils::isBuiltinType("mat3x2"));
    REQUIRE(TranspilerUtils::isBuiltinType("vec3"));
    REQUIRE(TranspilerUtils::isBuiltinType("ivec4"));
    REQUIRE(TranspilerUtils::isBuiltinType("uvec2"));
    REQUIRE(TranspilerUtils::isBuiltinType("bvec2"));
    REQUIRE_FALSE(TranspilerUtils::isBuiltinType("vec5"));
    REQUIRE_FALSE(TranspilerUtils::isBuiltinType("mat5"));
    REQUIRE_FALSE(TranspilerUtils::isBuiltinType("float2"));
}

TEST_CASE("TranspilerUtils normalises integer and integer vector types to float") {
    REQUIRE(TranspilerUtils::toFloatType("int") == "float");
    REQUIRE(TranspilerUtils::toFloatType("iint") == "float");
    REQUIRE(TranspilerUtils::toFloatType("vec2") == "vec2");
    REQUIRE(TranspilerUtils::toFloatType("ivec3") == "vec3");
    REQUIRE(TranspilerUtils::toFloatType("uvec4") == "vec4");
    // Anything else is answered unchanged.
    REQUIRE(TranspilerUtils::toFloatType("mat4") == "mat4");
    REQUIRE(TranspilerUtils::toFloatType("float") == "float");
    REQUIRE(TranspilerUtils::toFloatType("") == "");
}
