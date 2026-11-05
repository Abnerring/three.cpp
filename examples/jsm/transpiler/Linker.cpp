#include "transpiler/Linker.h"

#include <stdexcept>

namespace {
template <class T>
T* as(ASTNode* node) {
    return dynamic_cast<T*>(node);
}
}  // namespace

LinkerBlock::LinkerBlock(ASTNode* node, LinkerBlock* parent) : node(node), parent(parent) {}

void LinkerBlock::setProperty(const std::string& name, ASTNode* value) { properties[name] = value; }

ASTNode* LinkerBlock::getProperty(const std::string& name) const {
    const auto found = properties.find(name);
    if (found == properties.end()) return parent != nullptr ? parent->getProperty(name) : nullptr;
    return found->second;
}

Linker::~Linker() {
    while (block != nullptr) {
        LinkerBlock* parent = block->parent;
        delete block;
        block = parent;
    }
}

void Linker::addBlock(ASTNode* node) { block = new LinkerBlock(node, block); }

void Linker::removeBlock(ASTNode* node) {
    if (block == nullptr || block->node != node) {
        throw std::runtime_error("THREE.Linker: No block to remove or block mismatch.");
    }
    LinkerBlock* parent = block->parent;
    delete block;
    block = parent;
}

void Linker::processVariables(ASTNode* node) {
    if (auto* declaration = as<VariableDeclaration>(node)) {
        block->setProperty(declaration->name, declaration);
        if (declaration->value != nullptr) processExpression(declaration->value);
    } else if (auto* uniform = as<Uniform>(node)) {
        block->setProperty(uniform->name, uniform);
    } else if (auto* varying = as<Varying>(node)) {
        block->setProperty(varying->name, varying);
    }
}

void Linker::processUniform(ASTNode* node) {
    if (auto* uniform = as<Uniform>(node)) block->setProperty(uniform->name, uniform);
}

void Linker::processVarying(ASTNode* node) {
    if (auto* varying = as<Varying>(node)) block->setProperty(varying->name, varying);
}

std::string Linker::evalProperty(ASTNode* node) const {
    if (auto* accessor = as<Accessor>(node)) return accessor->property;
    return {};
}

void Linker::processExpression(ASTNode* node) {
    if (node == nullptr || block == nullptr) return;
    if (auto* accessor = as<Accessor>(node)) {
        ASTNode* property = block->getProperty(evalProperty(accessor));
        if (property != nullptr) {
            accessor->linker.reference = property;
            property->linker.accesses.push_back(accessor);
        }
        return;
    }
    if (as<Number>(node) != nullptr || as<String>(node) != nullptr) return;
    if (auto* op = as<Operator>(node)) {
        processExpression(op->left);
        processExpression(op->right);
        if (op->isAssignmentValue()) {
            ASTNode* property = block->getProperty(evalProperty(op->left));
            if (property != nullptr) property->linker.assignments.push_back(op);
        }
        return;
    }
    if (auto* call = as<FunctionCall>(node)) {
        for (auto* parameter : call->params) processExpression(parameter);
        return;
    }
    if (auto* returned = as<Return>(node)) {
        if (returned->value != nullptr) processExpression(returned->value);
        return;
    }
    if (as<Discard>(node) != nullptr || as<Break>(node) != nullptr || as<Continue>(node) != nullptr) {
        return;
    }
    if (auto* elements = as<AccessorElements>(node)) {
        processExpression(elements->object);
        for (auto* element : elements->elements) {
            if (auto* staticElement = as<StaticElement>(element)) {
                processExpression(staticElement->value);
            } else if (auto* dynamicElement = as<DynamicElement>(element)) {
                processExpression(dynamicElement->value);
            }
        }
        return;
    }
    if (auto* element = as<DynamicElement>(node)) {
        processExpression(element->value);
        return;
    }
    if (auto* element = as<StaticElement>(node)) {
        processExpression(element->value);
        return;
    }
    if (as<For>(node) != nullptr || as<While>(node) != nullptr) {
        processForWhile(node);
        return;
    }
    if (as<Switch>(node) != nullptr) {
        processSwitch(node);
        return;
    }
    if (as<VariableDeclaration>(node) != nullptr) {
        processVariables(node);
        return;
    }
    if (as<Uniform>(node) != nullptr) {
        processUniform(node);
        return;
    }
    if (as<Varying>(node) != nullptr) {
        processVarying(node);
        return;
    }
    if (auto* ternary = as<Ternary>(node)) {
        processExpression(ternary->cond);
        processExpression(ternary->left);
        processExpression(ternary->right);
        return;
    }
    if (as<Conditional>(node) != nullptr) {
        processConditional(node);
        return;
    }
    if (auto* unary = as<Unary>(node)) {
        processExpression(unary->expression);
        if (unary->isAssignmentValue()) {
            if (unary->parent == nullptr || !unary->parent->hasAssignment()) unary->after = false;
            ASTNode* property = block->getProperty(evalProperty(unary->expression));
            if (property != nullptr) property->linker.assignments.push_back(unary);
        }
    }
}

void Linker::processBody(const std::vector<ASTNode*>& body) {
    for (auto* statement : body) processExpression(statement);
}

void Linker::processConditional(ASTNode* node) {
    auto* conditional = as<Conditional>(node);
    if (conditional == nullptr) return;
    processExpression(conditional->cond);
    processBody(conditional->body);
    ASTNode* current = conditional;
    while (auto* currentConditional = as<Conditional>(current)) {
        auto* elseConditional = as<Conditional>(currentConditional->elseConditional);
        if (elseConditional == nullptr) break;
        if (elseConditional->cond != nullptr) processExpression(elseConditional->cond);
        processBody(elseConditional->body);
        current = elseConditional;
    }
}

void Linker::processForWhile(ASTNode* node) {
    if (auto* loop = as<For>(node)) {
        if (loop->initialization != nullptr) processExpression(loop->initialization);
        if (loop->condition != nullptr) processExpression(loop->condition);
        if (loop->afterthought != nullptr) processExpression(loop->afterthought);
        processBody(loop->body);
        return;
    }
    if (auto* whileLoop = as<While>(node)) {
        if (whileLoop->condition != nullptr) processExpression(whileLoop->condition);
        processBody(whileLoop->body);
    }
}

void Linker::processSwitch(ASTNode* node) {
    auto* switchNode = as<Switch>(node);
    if (switchNode == nullptr) return;
    processExpression(switchNode->discriminant);
    for (auto* caseNode : switchNode->cases) {
        auto* switchCase = as<SwitchCase>(caseNode);
        if (switchCase == nullptr) continue;
        if (!switchCase->isDefault && switchCase->conditions != nullptr) {
            processExpression(switchCase->conditions);
        }
        processBody(switchCase->body);
    }
}

void Linker::processFunction(ASTNode* node) {
    auto* function = as<FunctionDeclaration>(node);
    if (function == nullptr) return;
    addBlock(function);
    for (auto* parameter : function->params) {
        if (auto* functionParameter = as<FunctionParameter>(parameter)) {
            block->setProperty(functionParameter->name, functionParameter);
        }
    }
    processBody(function->body);
    removeBlock(function);
}

void Linker::process(ASTNode* ast) {
    auto* program = as<Program>(ast);
    if (program == nullptr) return;
    addBlock(program);
    for (auto* statement : program->body) {
        if (as<FunctionDeclaration>(statement) != nullptr) {
            processFunction(statement);
        } else {
            processExpression(statement);
        }
    }
    removeBlock(program);
}