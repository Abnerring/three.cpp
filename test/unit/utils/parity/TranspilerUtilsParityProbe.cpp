#include "transpiler/TranspilerUtils.h"

#include "core/JSValue.h"

#include <iostream>
#include <string>
#include <vector>

int main() {
    try {
        auto output = JSValue::object(nullptr);

        // isExpression over a battery of statements.
        auto statements = JSValue::array();
        std::size_t statementIndex = 0;
        auto addStatement = [&](const JSValue& statement) {
            auto entry = JSValue::object(nullptr);
            entry.set("input", statement);
            entry.set("result", JSValue(TranspilerUtils::isExpression(statement)));
            statements.set(statementIndex++, entry);
        };
        addStatement(JSValue::object(nullptr));
        {
            auto statement = JSValue::object(nullptr);
            statement.set("isExpression", true);
            addStatement(statement);
        }
        for (const char* flag : {"isFunctionDeclaration", "isFor", "isWhile", "isConditional",
                                 "isSwitch", "isStructDefinition"}) {
            auto statement = JSValue::object(nullptr);
            statement.set(flag, true);
            addStatement(statement);
        }
        {
            auto statement = JSValue::object(nullptr);
            statement.set("isFor", JSValue(1));
            addStatement(statement);
        }
        output.set("statements", statements);

        // isPrimitive and isBuiltinType over string batteries.
        const std::vector<std::string> values{"true", "false", "123", "-7", ".5", "identifier", "",
                                             " true", "0x10", "1e3", "-.25", "falsy"};
        const std::vector<std::string> types{"void", "bool", "float", "int", "uint", "mat2", "mat3",
                                            "mat4", "mat3x2", "mat4x4", "vec2", "vec3", "vec4",
                                            "ivec2", "uvec4", "bvec3", "vec5", "mat5", "float2",
                                            "", "int2", "vec2x2"};

        auto primitives = JSValue::array();
        std::size_t index = 0;
        for (const auto& value : values) {
            auto entry = JSValue::object(nullptr);
            entry.set("input", JSValue(value));
            entry.set("result", JSValue(TranspilerUtils::isPrimitive(value)));
            primitives.set(index++, entry);
        }
        output.set("primitives", primitives);

        auto builtins = JSValue::array();
        index = 0;
        for (const auto& type : types) {
            auto entry = JSValue::object(nullptr);
            entry.set("input", JSValue(type));
            entry.set("result", JSValue(TranspilerUtils::isBuiltinType(type)));
            builtins.set(index++, entry);
        }
        output.set("builtins", builtins);

        auto floats = JSValue::array();
        index = 0;
        for (const auto& type : types) {
            auto entry = JSValue::object(nullptr);
            entry.set("input", JSValue(type));
            entry.set("result", JSValue(TranspilerUtils::toFloatType(type)));
            floats.set(index++, entry);
        }
        output.set("floats", floats);

        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}
