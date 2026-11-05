#pragma once

#include <optional>
#include <string>
#include <vector>

// Native port of examples/jsm/transpiler/AST.js from the pinned three.js revision
// (see upstream.json).
//
// First slice: the ASTNode base class plus the three simplest leaf nodes, Comment, Uniform and
// Varying. Each node keeps the pinned flags, the linker record (reference plus the access and
// assignment lists), the parent pointer and the helper accessors. The JavaScript initializer scans
// the object properties for child nodes; C++ has no property reflection here, so every subclass that
// owns children will register them explicitly through adopt() once those classes are ported.
class ASTNode {
  public:
    struct Linker {
        ASTNode* reference = nullptr;
        std::vector<ASTNode*> accesses;
        std::vector<ASTNode*> assignments;
    };

    const bool isASTNode = true;
    Linker linker;
    ASTNode* parent = nullptr;
    // Set by subclasses whose assignment semantics matter; the pinned base class has no such field
    // but reads this.isAssignment defensively.
    bool isAssignment = false;

    virtual ~ASTNode() = default;

    virtual bool isNumericExpression() const { return false; }
    virtual bool isProgramNode() const { return false; }
    virtual std::optional<std::string> getType() const { return std::nullopt; }

    // get hasAssignment() { if ( this.isAssignment === true ) return true; if ( parent === null ) return false; return parent.hasAssignment; }
    bool hasAssignment() const;

    // getProgram(): walk to the root and answer it only when it is a program.
    ASTNode* getProgram();

    // getParent( parents = [] ): push every ancestor, nearest first, like the pinned recursion.
    std::vector<ASTNode*> getParent(std::vector<ASTNode*> parents = {}) const;

    // initialize(): explicit child registration in C++; leaf nodes keep the empty default.
    virtual void initialize() {}

  protected:
    // Equivalent of the pinned initializer assigning parent on each discovered child.
    void adopt(ASTNode* child);
    void adopt(ASTNode& child) { adopt(&child); }
};

class Comment : public ASTNode {
  public:
    const bool isComment = true;
    std::string comment;
    explicit Comment(std::string comment);
};

class Uniform : public ASTNode {
  public:
    const bool isUniform = true;
    std::string type;
    std::string name;
    Uniform(std::string type, std::string name);
    std::optional<std::string> getType() const override { return type; }
};

class Varying : public ASTNode {
  public:
    const bool isVarying = true;
    std::string type;
    std::string name;
    Varying(std::string type, std::string name);
    std::optional<std::string> getType() const override { return type; }
};