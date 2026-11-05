#include "transpiler/AST.h"

#include <utility>

bool ASTNode::hasAssignment() const {
    if (isAssignment) return true;
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