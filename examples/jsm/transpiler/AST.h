#pragma once

#include <optional>
#include <unordered_map>
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
    // The pinned base class has no such field but reads this.isAssignment defensively; subclasses
    // such as VariableDeclaration compute it from their payload, so it is an overridable predicate.
    bool isAssignmentField = false;
    virtual bool isAssignmentValue() const { return isAssignmentField; }

    virtual ~ASTNode() = default;

    virtual bool isNumericExpression() const { return false; }
    virtual bool isProgramNode() const { return false; }
    // The pinned numeric literal classes expose an isNumber property; Unary reads it.
    virtual bool isNumberNode() const { return false; }
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
// container and expression node slice: Program, Expression and Ternary.
class Program : public ASTNode {
  public:
    const bool isProgram = true;
    std::vector<ASTNode*> body;
    // A map of struct name to struct definition; the definition type lands with StructDefinition.
    std::unordered_map<std::string, ASTNode*> structTypes;

    explicit Program(std::vector<ASTNode*> body = {});
    bool isProgramNode() const override { return true; }
    void initialize() override;
};

class Expression : public ASTNode {
  public:
    const bool isExpression = true;
    ASTNode* expression = nullptr;
    explicit Expression(ASTNode* expression);
    void initialize() override;
};

class Ternary : public ASTNode {
  public:
    const bool isTernary = true;
    ASTNode* cond = nullptr;
    ASTNode* left = nullptr;
    ASTNode* right = nullptr;
    Ternary(ASTNode* cond, ASTNode* left, ASTNode* right);
    void initialize() override;
};
class VariableDeclaration : public ASTNode {
  public:
    const bool isVariableDeclaration = true;
    std::string type;
    std::string name;
    ASTNode* value = nullptr;
    ASTNode* next = nullptr;
    bool immutable = false;

    VariableDeclaration(std::string type, std::string name, ASTNode* value = nullptr,
                        ASTNode* next = nullptr, bool immutable = false);
    // get isAssignment() { return this.value !== null; }
    bool isAssignmentValue() const override { return value != nullptr; }
    std::optional<std::string> getType() const override { return type; }
    void initialize() override;
};

class FunctionParameter : public ASTNode {
  public:
    const bool isFunctionParameter = true;
    std::string type;
    std::string name;
    std::optional<std::string> qualifier;
    bool immutable = true;

    FunctionParameter(std::string type, std::string name,
                      std::optional<std::string> qualifier = std::nullopt, bool immutable = true);
    std::optional<std::string> getType() const override { return type; }
};

class FunctionDeclaration : public ASTNode {
  public:
    const bool isFunctionDeclaration = true;
    std::string type;
    std::string name;
    std::vector<ASTNode*> params;
    std::vector<ASTNode*> body;

    FunctionDeclaration(std::string type, std::string name, std::vector<ASTNode*> params = {},
                        std::vector<ASTNode*> body = {});
    std::optional<std::string> getType() const override { return type; }
    void initialize() override;
};
class Operator : public ASTNode {
  public:
    const bool isOperator = true;
    std::string type;
    ASTNode* left = nullptr;
    ASTNode* right = nullptr;

    Operator(std::string type, ASTNode* left, ASTNode* right);
    // get isAssignment() { return /^(=|\+=|-=|\*=|\/=|%=|<<=|>>=|>>>=|&=|\^=|\|=)$/.test( this.type ); }
    bool isAssignmentValue() const override;
    bool isNumericExpression() const override;
    std::optional<std::string> getType() const override;
    void initialize() override;
};

class Unary : public ASTNode {
  public:
    const bool isUnary = true;
    std::string type;
    ASTNode* expression = nullptr;
    bool after = false;

    Unary(std::string type, ASTNode* expression, bool after = false);
    // get isAssignment() { return /^(\+\+|--)$/.test( this.type ); }
    bool isAssignmentValue() const override;
    bool isNumericExpression() const override;
    void initialize() override;
};

class Number : public ASTNode {
  public:
    const bool isNumber = true;
    std::string type;
    double value = 0;

    explicit Number(double value, std::string type = "float");
    bool isNumberNode() const override { return true; }
    bool isNumericExpression() const override { return true; }
    std::optional<std::string> getType() const override { return type; }
};

class String : public ASTNode {
  public:
    const bool isString = true;
    std::string value;

    explicit String(std::string value);
};

class Conditional : public ASTNode {
  public:
    const bool isConditional = true;
    ASTNode* cond = nullptr;
    std::vector<ASTNode*> body;
    ASTNode* elseConditional = nullptr;

    explicit Conditional(ASTNode* cond = nullptr, std::vector<ASTNode*> body = {});
    void initialize() override;
};