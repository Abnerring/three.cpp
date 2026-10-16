#include "CompilerInternal.h"
#include <glslang/MachineIndependent/iomapper.h>
#include <algorithm>
#include <map>
#include <stdexcept>
namespace threecppEssl {
using namespace glslang;
namespace {
struct Symbols : TIntermTraverser {
    std::map<std::string, TIntermSymbol*> uniforms;
    std::map<std::string, TIntermSymbol*> samplers;
    long long maximum = 0;
    bool instanceId = false, fragCoord = false;
    Symbols() : TIntermTraverser(true, false, false, false, true) {}
    void visitSymbol(TIntermSymbol* node) override {
        maximum = std::max(maximum, node->getId());
        instanceId |= node->getQualifier().builtIn == EbvInstanceId;
        fragCoord |= node->getQualifier().builtIn == EbvFragCoord;
        if (node->getQualifier().storage != EvqUniform || node->getType().isBuiltIn()) return;
        const std::string name = node->getName().c_str();
        if (node->getType().getBasicType() == EbtBlock) throw std::runtime_error("Existing user uniform blocks are outside this prototype: " + name);
        if (node->getType().containsOpaque()) {
            if (!node->getType().isOpaque()) throw std::runtime_error("Mixed opaque uniform structure requires flattening: " + name);
            samplers.emplace(name, node);
        } else uniforms.emplace(name, node);
    }
};

// Change expression nodes and linker declarations, not their source spelling.
// Both stages receive the same std140 member schema, including shared matrices.
class UniformBlockRewrite {
    EShLanguage stage;
    long long blockId;
    TType* blockType;
    std::map<long long, int> members;
    bool glBuiltins;
    bool usesBackend;
    int baseInstanceMember=-1, framebufferHeightMember=-1;
    TIntermTyped* typed(TIntermNode* node) {
        if (!node) return nullptr;
        auto result = rewrite(node)->getAsTyped();
        if (!result) throw std::runtime_error("Expected typed AST child");
        return result;
    }
    TIntermSymbol* blockSymbol() {
        return new TIntermSymbol(blockId, "threeUniforms", stage, *blockType);
    }
    static TIntermTyped* component(TIntermTyped* input, int index) {
        auto value=new TIntermBinary(EOpIndexDirect);value->setLeft(input);
        TConstUnionArray number(1);number[0].setIConst(index);
        value->setRight(new TIntermConstantUnion(number,TType(EbtInt,EvqConst,EpqHigh)));
        value->setType(TType(EbtFloat,EvqTemporary,EpqHigh));return value;
    }
    TIntermTyped* backendMember(int index,TBasicType type) {
        if(index<0)throw std::runtime_error("Backend uniform schema missing");
        auto value=new TIntermBinary(EOpIndexDirectStruct);value->setLeft(blockSymbol());
        TConstUnionArray number(1);number[0].setIConst(index);
        value->setRight(new TIntermConstantUnion(number,TType(EbtInt,EvqConst,EpqHigh)));
        value->setType(TType(type,EvqUniform,EpqHigh));return value;
    }
public:
    UniformBlockRewrite(EShLanguage s, const Symbols& symbols,
                        const std::map<std::string, TIntermSymbol*>& schema, bool adapt) : stage(s), blockId(symbols.maximum + 1), glBuiltins(adapt), usesBackend(adapt && (symbols.instanceId || symbols.fragCoord)) {
        auto fields = new TTypeList;
        int index = 0;
        for (const auto& [name, symbol] : schema) {
            auto memberType = symbol->getType().clone();
            memberType->setFieldName(TString(name.c_str()));
            memberType->getQualifier().layoutBinding = TQualifier::layoutBindingEnd;
            memberType->getQualifier().layoutSet = TQualifier::layoutSetEnd;
            memberType->getQualifier().layoutLocation = TQualifier::layoutLocationEnd;
            fields->push_back({memberType, symbol->getLoc()});
            if (auto found = symbols.uniforms.find(name); found != symbols.uniforms.end()) members.emplace(found->second->getId(), index);
            if(name=="threecppBackendBaseInstance")baseInstanceMember=index;
            if(name=="threecppBackendFramebufferHeight")framebufferHeightMember=index;
            ++index;
        }
        TQualifier qualifier; qualifier.clear();
        qualifier.storage = EvqUniform;
        qualifier.layoutPacking = ElpStd140;
        qualifier.layoutMatrix = ElmColumnMajor;
        qualifier.layoutBinding = 0; qualifier.layoutSet = 0;
        blockType = new TType(fields, TString("ThreeUniforms"), qualifier);
    }
    TIntermNode* rewrite(TIntermNode* node) {
        if (!node) return nullptr;
        if (auto symbol = node->getAsSymbolNode()) {
            if(glBuiltins && symbol->getQualifier().builtIn==EbvInstanceId) {
                auto result=new TIntermBinary(EOpSub);result->setLeft(symbol);
                result->setRight(backendMember(baseInstanceMember,EbtInt));result->setType(symbol->getType());return result;
            }
            if(glBuiltins && symbol->getQualifier().builtIn==EbvFragCoord) {
                auto y=new TIntermBinary(EOpSub);y->setLeft(backendMember(framebufferHeightMember,EbtFloat));
                y->setRight(component(symbol,1));y->setType(TType(EbtFloat,EvqTemporary,EpqHigh));
                auto result=new TIntermAggregate(EOpConstructVec4);result->setType(symbol->getType());
                result->getSequence().push_back(component(symbol,0));result->getSequence().push_back(y);
                result->getSequence().push_back(component(symbol,2));result->getSequence().push_back(component(symbol,3));return result;
            }
            auto found = members.find(symbol->getId());
            if (found == members.end()) return node;
            auto expression = new TIntermBinary(EOpIndexDirectStruct);
            expression->setLeft(blockSymbol());
            TConstUnionArray index(1); index[0].setIConst(found->second);
            expression->setRight(new TIntermConstantUnion(index, TType(EbtInt, EvqConst, EpqHigh)));
            expression->setType(symbol->getType()); expression->setLoc(symbol->getLoc());
            return expression;
        }
        if (auto binary = node->getAsBinaryNode()) {
            binary->setLeft(typed(binary->getLeft())); binary->setRight(typed(binary->getRight()));
        } else if (auto unary = node->getAsUnaryNode()) {
            unary->setOperand(typed(unary->getOperand()));
            if(glBuiltins && (unary->getOp()==EOpDPdy || unary->getOp()==EOpDPdyFine || unary->getOp()==EOpDPdyCoarse)) {
                auto result=new TIntermUnary(EOpNegative);result->setOperand(unary);result->setType(unary->getType());result->setLoc(unary->getLoc());return result;
            }
        }
        else if (auto aggregate = node->getAsAggregate()) {
            if (aggregate->getOp() == EOpLinkerObjects) {
                auto& sequence = aggregate->getSequence();
                sequence.erase(std::remove_if(sequence.begin(), sequence.end(), [this](TIntermNode* child) {
                    auto symbol = child->getAsSymbolNode(); return symbol && members.contains(symbol->getId());
                }), sequence.end());
                if (!members.empty() || usesBackend) sequence.push_back(blockSymbol());
            } else for (auto& child : aggregate->getSequence()) child = rewrite(child);
        } else if (auto selection = node->getAsSelectionNode()) {
            auto result = new TIntermSelection(typed(selection->getCondition()), rewrite(selection->getTrueBlock()), rewrite(selection->getFalseBlock()), selection->getType());
            if (!selection->getShortCircuit()) result->setNoShortCircuit();
            if (selection->getFlatten()) result->setFlatten();
            if (selection->getDontFlatten()) result->setDontFlatten();
            result->setLoc(selection->getLoc()); return result;
        } else if (auto loop = node->getAsLoopNode()) {
            auto result = new TIntermLoop(rewrite(loop->getBody()), rewrite(loop->getTest()), typed(loop->getTerminal()), loop->testFirst());
            if (loop->getUnroll()) result->setUnroll();
            if (loop->getDontUnroll()) result->setDontUnroll();
            result->setLoopDependency(loop->getLoopDependency());
            result->setMinIterations(loop->getMinIterations()); result->setMaxIterations(loop->getMaxIterations());
            result->setIterationMultiple(loop->getIterationMultiple()); result->setPeelCount(loop->getPeelCount());
            result->setPartialCount(loop->getPartialCount()); result->setMultipleWaitQueuesQCOM(loop->getMultipleWaitQueuesQCOM());
            result->setLoc(loop->getLoc()); return result;
        } else if (auto branch = node->getAsBranchNode()) branch->setExpression(typed(branch->getExpression()));
        else if (auto switchNode = node->getAsSwitchNode()) {
            auto result = new TIntermSwitch(typed(switchNode->getCondition()), rewrite(switchNode->getBody())->getAsAggregate());
            if (switchNode->getFlatten()) result->setFlatten();
            if (switchNode->getDontFlatten()) result->setDontFlatten();
            result->setLoc(switchNode->getLoc()); return result;
        } else if (auto declaration = node->getAsVariableDecl()) {
            auto result = new TIntermVariableDecl(declaration->getDeclSymbol(), rewrite(declaration->getInitNode()));
            result->setLoc(declaration->getLoc()); return result;
        } else if (!node->getAsConstantUnion()) throw std::runtime_error("Unmapped AST node kind");
        return node;
    }
};

struct SamplerBindings : TIntermTraverser {
    const std::map<std::string, unsigned>& bindings;
    explicit SamplerBindings(const std::map<std::string, unsigned>& b) : TIntermTraverser(true, false, false, false, true), bindings(b) {}
    void visitSymbol(TIntermSymbol* node) override {
        if (node->getQualifier().storage != EvqUniform || !node->getType().isOpaque()) return;
        auto found = bindings.find(node->getName().c_str());
        if (found == bindings.end()) throw std::runtime_error("Missing sampler binding");
        auto& q = node->getWritableType().getQualifier(); q.layoutSet = 0; q.layoutBinding = found->second;
    }
};

struct VulkanBuiltIns : TIntermTraverser {
    VulkanBuiltIns() : TIntermTraverser(true, false, false, false, true) {}
    void visitSymbol(TIntermSymbol* symbol) override {
        auto& qualifier=symbol->getWritableType().getQualifier();
        if (qualifier.builtIn==EbvVertexId) qualifier.builtIn=EbvVertexIndex;
        if (qualifier.builtIn==EbvInstanceId) qualifier.builtIn=EbvInstanceIndex;
    }
};

// Rename the linked original main into a private function and append a new entry.
// User reads/writes and early returns therefore finish in original GL coordinates.
void wrapVertexEntry(TIntermediate& intermediate) {
    auto root=intermediate.getTreeRoot()->getAsAggregate();
    if(!root)throw std::runtime_error("Vertex AST root is not an aggregate");
    TIntermAggregate* original=nullptr;
    std::map<std::string,TIntermAggregate*> functions;
    for(auto child:root->getSequence())if(auto fn=child->getAsAggregate();fn && fn->getOp()==EOpFunction) {
        functions.emplace(fn->getName().c_str(),fn);
        if(std::string(fn->getName().c_str())==intermediate.getEntryPointMangledName())original=fn;
    }
    if(!original)throw std::runtime_error("Original vertex entry missing");
    struct Position:TIntermTraverser {
        TIntermSymbol* symbol=nullptr;
        Position():TIntermTraverser(true,false,false,false,true){}
        void visitSymbol(TIntermSymbol* node)override{if(node->getQualifier().builtIn==EbvPosition)symbol=node;}
    } position;
    root->traverse(&position);if(!position.symbol)throw std::runtime_error("gl_Position symbol absent");
    std::string privateName="threecppNativeUserMain(";
    while(functions.contains(privateName))privateName="_"+privateName;
    const auto entryName=original->getName();original->setName(TString(privateName.c_str()));
    auto entry=new TIntermAggregate(EOpFunction);entry->setName(entryName);entry->setType(original->getType());
    auto parameters=new TIntermAggregate(EOpParameters);auto body=new TIntermAggregate(EOpSequence);
    auto call=new TIntermAggregate(EOpFunctionCall);call->setName(TString(privateName.c_str()));call->setType(original->getType());call->setUserDefined();body->getSequence().push_back(call);
    const auto component=[&](int index){
        auto result=new TIntermBinary(EOpIndexDirect);result->setLeft(position.symbol);
        TConstUnionArray number(1);number[0].setIConst(index);
        result->setRight(new TIntermConstantUnion(number,TType(EbtInt,EvqConst,EpqHigh)));
        result->setType(TType(EbtFloat,EvqTemporary,EpqHigh));return result;
    };
    auto y=new TIntermUnary(EOpNegative);y->setOperand(component(1));y->setType(TType(EbtFloat,EvqTemporary,EpqHigh));
    auto assignY=new TIntermBinary(EOpAssign);assignY->setLeft(component(1));assignY->setRight(y);assignY->setType(y->getType());body->getSequence().push_back(assignY);
    auto sum=new TIntermBinary(EOpAdd);sum->setLeft(component(2));sum->setRight(component(3));sum->setType(TType(EbtFloat,EvqTemporary,EpqHigh));
    TConstUnionArray half(1);half[0].setDConst(0.5);
    auto z=new TIntermBinary(EOpMul);z->setLeft(sum);z->setRight(new TIntermConstantUnion(half,TType(EbtFloat,EvqConst,EpqHigh)));z->setType(sum->getType());
    auto assignZ=new TIntermBinary(EOpAssign);assignZ->setLeft(component(2));assignZ->setRight(z);assignZ->setType(z->getType());body->getSequence().push_back(assignZ);
    entry->getSequence().push_back(parameters);entry->getSequence().push_back(body);root->getSequence().push_back(entry);
}


} // namespace
void mapAst(TProgram& program, VulkanEsslProgram& result, std::span<const VulkanEsslAttributeBinding> attributes) {
    constexpr bool adaptBuiltins=true;
        Symbols symbols[2];
        const EShLanguage stages[]{EShLangVertex, EShLangFragment};
        std::map<std::string, TIntermSymbol*> schema;
        std::map<std::string, unsigned> samplerBindings;
        for (int i=0;i<2;++i) {
            program.getIntermediate(stages[i])->getTreeRoot()->traverse(&symbols[i]);
            for (const auto& [name, symbol] : symbols[i].uniforms) {
                if (auto found=schema.find(name);found!=schema.end() && found->second->getType()!=symbol->getType()) throw std::runtime_error("Cross-stage uniform type mismatch: "+name);
                schema.emplace(name, symbol);
            }
            for (const auto& [name, symbol] : symbols[i].samplers) samplerBindings.emplace(name, 0);
        }
        if(adaptBuiltins) {
            const long long nextId=std::max(symbols[0].maximum,symbols[1].maximum)+2;
            if(schema.contains("threecppBackendBaseInstance")||schema.contains("threecppBackendFramebufferHeight"))throw std::runtime_error("Private backend schema name collision");
            schema.emplace("threecppBackendBaseInstance",new TIntermSymbol(nextId,"threecppBackendBaseInstance",EShLangVertex,TType(EbtInt,EvqUniform,EpqHigh)));
            schema.emplace("threecppBackendFramebufferHeight",new TIntermSymbol(nextId+1,"threecppBackendFramebufferHeight",EShLangFragment,TType(EbtFloat,EvqUniform,EpqHigh)));
        }
        unsigned binding = 1; for (auto& [name, slot] : samplerBindings) slot = binding++;
        for (const auto& [name, symbol] : schema)
            result.uniforms.push_back({name, symbol->getType().getCompleteString().c_str()});
        for (const auto& [name, slot] : samplerBindings) result.samplers.push_back({name,0,slot});
        result.phase = "map-AST";
        for (int i=0;i<2;++i) {
            auto& intermediate = *program.getIntermediate(stages[i]);
            UniformBlockRewrite rewrite(stages[i], symbols[i], schema,adaptBuiltins);
            intermediate.setTreeRoot(rewrite.rewrite(intermediate.getTreeRoot()));
            SamplerBindings samplerMap(samplerBindings); intermediate.getTreeRoot()->traverse(&samplerMap);
            VulkanBuiltIns builtinMap; intermediate.getTreeRoot()->traverse(&builtinMap);
            if (stages[i]==EShLangVertex && adaptBuiltins) wrapVertexEntry(intermediate);
            if (stages[i]==EShLangFragment) intermediate.setOriginUpperLeft();
            SpvVersion target; target.spv=EShTargetSpv_1_0; target.vulkan=EShTargetVulkan_1_0;
            intermediate.setSpv(target); intermediate.setAutoMapBindings(true); intermediate.setAutoMapLocations(true);
        }
        // Apply requested attribute bindings only to inputs without an explicit
        // original ESSL location. Never change varyings or SPIR-V builtins.
        struct Attributes:TIntermTraverser {
            std::map<std::string,unsigned> requested;
            Attributes(std::span<const VulkanEsslAttributeBinding> bindings):TIntermTraverser(true,false,false,false,true) {
                for(const auto& b:bindings) {
                    if(b.location>=TQualifier::layoutLocationEnd)throw std::invalid_argument("Attribute binding exceeds frontend location range");
                    requested.insert_or_assign(b.name,b.location);
                }
            }
            void visitSymbol(TIntermSymbol* symbol)override {
                auto& q=symbol->getWritableType().getQualifier();
                if(q.storage!=EvqVaryingIn||q.builtIn!=EbvNone||q.hasLocation())return;
                if(const auto found=requested.find(symbol->getName().c_str());found!=requested.end())q.layoutLocation=found->second;
            }
        } mappedAttributes(attributes);
        program.getIntermediate(EShLangVertex)->getTreeRoot()->traverse(&mappedAttributes);
        result.phase = "map-IO";
        TDefaultGlslIoResolver resolver(*program.getIntermediate(EShLangVertex));
        TGlslIoMapper ioMapper;
        if (!program.mapIO(&resolver, &ioMapper)) throw std::runtime_error(std::string(program.getInfoLog()) + program.getInfoDebugLog());

}
} // namespace threecppEssl
