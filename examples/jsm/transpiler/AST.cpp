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
FunctionCall::FunctionCall(std::string name, std::vector<ASTNode*> params)
    : name(std::move(name)), params(std::move(params)) {
    initialize();
}

std::optional<std::string> FunctionCall::getType() const {
    // if ( isBuiltinType( this.name ) ) return this.name;
    if (TranspilerUtils::isBuiltinType(name)) return name;
    // if ( this.linker.reference ) return this.linker.reference.getType();
    if (linker.reference != nullptr) return linker.reference->getType();
    return ASTNode::getType();
}

void FunctionCall::initialize() {
    for (auto* parameter : params) adopt(parameter);
}

Return::Return(ASTNode* value) : value(value) { initialize(); }

void Return::initialize() { adopt(value); }

Accessor::Accessor(std::string property) : property(std::move(property)) { initialize(); }

std::optional<std::string> Accessor::getType() const {
    if (linker.reference != nullptr) return linker.reference->getType();
    return ASTNode::getType();
}

StaticElement::StaticElement(ASTNode* value) : value(value) { initialize(); }

void StaticElement::initialize() { adopt(value); }

DynamicElement::DynamicElement(ASTNode* value) : value(value) { initialize(); }

void DynamicElement::initialize() { adopt(value); }

AccessorElements::AccessorElements(ASTNode* object, std::vector<ASTNode*> elements)
    : object(object), elements(std::move(elements)) {
    initialize();
}

void AccessorElements::initialize() {
    adopt(object);
    for (auto* element : elements) adopt(element);
}

For::For(ASTNode* initialization, ASTNode* condition, ASTNode* afterthought,
         std::vector<ASTNode*> body)
    : initialization(initialization),
      condition(condition),
      afterthought(afterthought),
      body(std::move(body)) {
    initialize();
}

void For::initialize() {
    adopt(initialization);
    adopt(condition);
    adopt(afterthought);
    for (auto* statement : body) adopt(statement);
}

While::While(ASTNode* condition, std::vector<ASTNode*> body, bool doWhile)
    : condition(condition), body(std::move(body)), doWhile(doWhile) {
    initialize();
}

void While::initialize() {
    adopt(condition);
    for (auto* statement : body) adopt(statement);
}

Switch::Switch(ASTNode* discriminant, std::vector<ASTNode*> cases)
    : discriminant(discriminant), cases(std::move(cases)) {
    initialize();
}

void Switch::initialize() {
    adopt(discriminant);
    for (auto* switchCase : cases) adopt(switchCase);
}

SwitchCase::SwitchCase(std::vector<ASTNode*> body, ASTNode* conditions)
    : body(std::move(body)), conditions(conditions), isDefault(conditions == nullptr) {
    initialize();
}

void SwitchCase::initialize() {
    adopt(conditions);
    for (auto* statement : body) adopt(statement);
}

StructMember::StructMember(std::string type, std::string name)
    : type(std::move(type)), name(std::move(name)) {}

StructDefinition::StructDefinition(std::string name, std::vector<StructMember*> members)
    : name(std::move(name)), members(std::move(members)) {}