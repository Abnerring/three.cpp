#pragma once

#include "transpiler/AST.h"

#include <map>
#include <string>
#include <vector>

// Native port of examples/jsm/transpiler/Linker.js from the pinned three.js revision
// (see upstream.json).
//
// The linker resolves accessor names against the enclosing block chain, records the resolved node on
// the accessor linker record and registers reads and writes on the resolved node. Blocks nest for
// function declarations and a local property hides the same name from an outer block.
//
// Recorded deviation: the pinned switch case keeps an array of conditions (or null for the default
// branch). The ported AST still carries a single condition pointer, so a multi condition case is not
// linked yet; the linker links that one condition when present.
class LinkerBlock {
  public:
    ASTNode* node = nullptr;
    LinkerBlock* parent = nullptr;
    std::map<std::string, ASTNode*> properties;

    LinkerBlock(ASTNode* node = nullptr, LinkerBlock* parent = nullptr);
    void setProperty(const std::string& name, ASTNode* value);
    ASTNode* getProperty(const std::string& name) const;
};

class Linker {
  public:
    LinkerBlock* block = nullptr;

    Linker() = default;
    Linker(const Linker&) = delete;
    Linker& operator=(const Linker&) = delete;
    ~Linker();

    void addBlock(ASTNode* node);
    void removeBlock(ASTNode* node);

    void processVariables(ASTNode* node);
    void processUniform(ASTNode* node);
    void processVarying(ASTNode* node);
    std::string evalProperty(ASTNode* node) const;
    void processExpression(ASTNode* node);
    void processBody(const std::vector<ASTNode*>& body);
    void processConditional(ASTNode* node);
    void processForWhile(ASTNode* node);
    void processSwitch(ASTNode* node);
    void processFunction(ASTNode* node);
    void process(ASTNode* ast);
};