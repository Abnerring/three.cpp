#include "transpiler/TranspilerUtils.h"

#include <regex>
#include <string>

namespace {
// /^(i?int)$/
const std::regex& intTypePattern() {
    static const std::regex pattern("^(i?int)$");
    return pattern;
}
// /^(i|u)?vec([234])$/
const std::regex& vectorTypePattern() {
    static const std::regex pattern("^(i|u)?vec([234])$");
    return pattern;
}
// /^(void|bool|float|u?int|mat[234]|mat[234]x[234]|(u|i|b)?vec[234])$/
const std::regex& builtinTypePattern() {
    static const std::regex pattern(
        "^(void|bool|float|u?int|mat[234]|mat[234]x[234]|(u|i|b)?vec[234])$");
    return pattern;
}
// /^(true|false|-?(\d|\.\d))/
const std::regex& primitivePattern() {
    static const std::regex pattern("^(true|false|-?(\\d|\\.\\d))");
    return pattern;
}
}  // namespace

bool TranspilerUtils::isExpression(const JSValue& statement) {
    // return st.isFunctionDeclaration !== true && st.isFor !== true && ... && st.isStructDefinition !== true;
    auto value = statement;
    if (value.get("isFunctionDeclaration").strictEquals(true)) return false;
    if (value.get("isFor").strictEquals(true)) return false;
    if (value.get("isWhile").strictEquals(true)) return false;
    if (value.get("isConditional").strictEquals(true)) return false;
    if (value.get("isSwitch").strictEquals(true)) return false;
    if (value.get("isStructDefinition").strictEquals(true)) return false;
    return true;
}

bool TranspilerUtils::isPrimitive(const std::string& value) {
    return std::regex_search(value, primitivePattern());
}

bool TranspilerUtils::isBuiltinType(const std::string& type) {
    return std::regex_match(type, builtinTypePattern());
}

std::string TranspilerUtils::toFloatType(const std::string& type) {
    if (std::regex_match(type, intTypePattern())) return "float";
    std::smatch match;
    if (std::regex_match(type, match, vectorTypePattern())) {
        // vecMatch[ 2 ] is the component count digit.
        return "vec" + match[2].str();
    }
    return type;
}
