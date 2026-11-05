#include "transpiler/AST.h"

#include <regex>
#include <string>

#include <utility>

#include "transpiler/TranspilerUtils.h"

bool ASTNode::hasAssignment() const {
    if (isAssignmentValue()) return true;
    if (parent == nullptr) return false;
    return parent->hasAssignment();
}

ASTNode* ASTNode::getProgram() {
    ASTNode* current = this;
    while (current->parent != nullptr) current = current->parent;
    return current->isProgramNode() ? current : nullptr;
}

std::vector<ASTNode*> ASTNode::getParent(std::vector<ASTNode*> parents) const {
    if (parent == nullptr) return parents;
    parents.push_back(parent);
    return parent->getParent(std::move(parents));
}

void ASTNode::adopt(ASTNode* child) {
    if (child != nullptr) child->parent = this;
}

Comment::Comment(std::string comment) : comment(std::move(comment)) { initialize(); }

Uniform::Uniform(std::string type, std::string name)
    : type(std::move(type)), name(std::move(name)) {
    initialize();
}

Varying::Varying(std::string type, std::string name)
    : type(std::move(type)), name(std::move(name)) {
    initialize();
}
Program::Program(std::vector<ASTNode*> body) : body(std::move(body)) { initialize(); }

void Program::initialize() {
    for (auto* statement : body) adopt(statement);
}

Expression::Expression(ASTNode* expression) : expression(expression) { initialize(); }

void Expression::initialize() { adopt(expression); }

Ternary::Ternary(ASTNode* cond, ASTNode* left, ASTNode* right)
    : cond(cond), left(left), right(right) {
    initialize();
}

void Ternary::initialize() {
    adopt(cond);
    adopt(left);
    adopt(right);
}
VariableDeclaration::VariableDeclaration(std::string type, std::string name, ASTNode* value,
                                         ASTNode* next, bool immutable)
    : type(std::move(type)), name(std::move(name)), value(value), next(next), immutable(immutable) {
    initialize();
}

void VariableDeclaration::initialize() {
    adopt(value);
    adopt(next);
}

FunctionParameter::FunctionParameter(std::string type, std::string name,
                                     std::optional<std::string> qualifier, bool immutable)
    : type(std::move(type)),
      name(std::move(name)),
      qualifier(std::move(qualifier)),
      immutable(immutable) {}

FunctionDeclaration::FunctionDeclaration(std::string type, std::string name,
                                         std::vector<ASTNode*> params,
                                         std::vector<ASTNode*> body)
    : type(std::move(type)), name(std::move(name)), params(std::move(params)), body(std::move(body)) {
    initialize();
}

void FunctionDeclaration::initialize() {
    for (auto* parameter : params) adopt(parameter);
    for (auto* statement : body) adopt(statement);
}
Operator::Operator(std::string type, ASTNode* left, ASTNode* right)
    : type(std::move(type)), left(left), right(right) {
    initialize();
}

bool Operator::isAssignmentValue() const {
    static const std::regex pattern("^(=|\\+=|-=|\\*=|/=|%=|<<=|>>=|>>>=|&=|\\^=|\\|=)$");
    return std::regex_match(type, pattern);
}

bool Operator::isNumericExpression() const {
    return left != nullptr && right != nullptr && left->isNumericExpression() &&
           right->isNumericExpression();
}

std::optional<std::string> Operator::getType() const {
    if (left == nullptr || right == nullptr) return std::nullopt;
    const auto leftType = left->getType();
    const auto rightType = right->getType();
    if (!leftType.has_value() || !rightType.has_value()) return std::nullopt;
    if (*leftType == *rightType) return leftType;
    if (TranspilerUtils::toFloatType(*leftType) == TranspilerUtils::toFloatType(*rightType)) {
        return TranspilerUtils::toFloatType(*leftType);
    }
    return std::nullopt;
}

void Operator::initialize() {
    adopt(left);
    adopt(right);
}

Unary::Unary(std::string type, ASTNode* expression, bool after)
    : type(std::move(type)), expression(expression), after(after) {
    initialize();
}

bool Unary::isAssignmentValue() const {
    static const std::regex pattern("^(\\+\\+|--)$");
    return std::regex_match(type, pattern);
}

bool Unary::isNumericExpression() const {
    return expression != nullptr && expression->isNumberNode();
}

void Unary::initialize() { adopt(expression); }

Number::Number(double value, std::string type) : type(std::move(type)), value(value) {}

String::String(std::string value) : value(std::move(value)) {}

Conditional::Conditional(ASTNode* cond, std::vector<ASTNode*> body)
    : cond(cond), body(std::move(body)) {
    initialize();
}

void Conditional::initialize() {
    adopt(cond);
    adopt(elseConditional);
    for (auto* statement : body) adopt(statement);
}