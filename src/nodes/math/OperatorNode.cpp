// Native managed port of pinned src/nodes/math/OperatorNode.js.
#include "nodes/math/OperatorNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"
#include "constants.h"
#include <algorithm>
#include <limits>
#include <tuple>

namespace {
using namespace UtilityNodeRuntime;
bool comparison(const JSValue& op) {
    return op.strictEquals("==") || op.strictEquals("!=") || op.strictEquals("<") ||
           op.strictEquals(">") || op.strictEquals("<=") || op.strictEquals(">=");
}
bool bitwise(const JSValue& op) {
    return op.strictEquals("~") || op.strictEquals("&") || op.strictEquals("|") ||
           op.strictEquals("^") || op.strictEquals(">>") || op.strictEquals("<<");
}
JSValue maximum(const JSValue& a, const JSValue& b) {
    const auto x = a.toNumber(); const auto y = b.toNumber();
    if (std::isnan(x) || std::isnan(y)) return std::numeric_limits<double>::quiet_NaN();
    if (x == 0 && y == 0) return std::signbit(x) && std::signbit(y) ? -0.0 : 0.0;
    return x > y ? x : y;
}
template<class F> JSValue formatted(const JSValue& builder, F arguments) {
    const auto method = invoke(builder, "format");
    return method(arguments());
}
std::string binary(const JSValue& a, const JSValue& op, const JSValue& b, bool parens = true) {
    const auto left = a.toString(); const auto middle = op.toString(); const auto right = b.toString();
    return (parens ? "( " : "") + left + " " + middle + " " + right + (parens ? " )" : "");
}
std::string function(const JSValue& method, const JSValue& a, const JSValue& b) {
    const auto name = method.toString(); const auto left = a.toString(); const auto right = b.toString();
    return name + "( " + left + ", " + right + " )";
}
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string(); const auto builder = arg(args, 0);
    if (name == "getOperatorMethod") {
        const auto callback = invoke(builder, "getMethod"); const auto op = self.get("op");
        return callback({captures[2].get(op), arg(args, 1)});
    }
    if (name == "generateNodeType") {
        const auto output = arg(args, 1, nullptr), op = self.get("op");
        const auto aNode = self.get("aNode"), bNode = self.get("bNode");
        const auto typeA = invoke(aNode, "getNodeType")({builder});
        const auto typeB = bNode.toBoolean() ? invoke(bNode, "getNodeType")({builder}) : JSValue(nullptr);
        if (typeA.strictEquals("void") || typeB.strictEquals("void")) return output.toBoolean() ? output : JSValue("void");
        if (bitwise(op)) return invoke(builder, "getIntegerType")({typeA});
        if (op.strictEquals("&&") || op.strictEquals("||") || op.strictEquals("^^")) return "bool";
        if (op.strictEquals("!")) {
            const auto length = invoke(builder, "getTypeLength")({typeA});
            return greater(length, 1) ? JSValue("bvec" + length.toString()) : JSValue("bool");
        }
        if (comparison(op)) {
            const auto a = invoke(builder, "getTypeLength")({typeA}); const auto b = invoke(builder, "getTypeLength")({typeB});
            const auto length = maximum(a, b);
            return greater(length, 1) ? JSValue("bvec" + length.toString()) : JSValue("bool");
        }
        if (typeB.isNull()) return typeA;
        if (invoke(builder, "isMatrix")({typeA}).toBoolean()) {
            if (invoke(builder, "isScalar")({typeB}).toBoolean()) return typeA;
            if (invoke(builder, "isVector")({typeB}).toBoolean()) return invoke(builder, "getVectorFromMatrix")({typeA});
            if (invoke(builder, "isMatrix")({typeB}).toBoolean()) return typeA;
        } else if (invoke(builder, "isMatrix")({typeB}).toBoolean()) {
            if (invoke(builder, "isScalar")({typeA}).toBoolean()) return typeB;
            if (invoke(builder, "isVector")({typeA}).toBoolean()) return invoke(builder, "getVectorFromMatrix")({typeB});
        }
        const auto bLength = invoke(builder, "getTypeLength")({typeB}); const auto aLength = invoke(builder, "getTypeLength")({typeA});
        const auto type = greater(bLength, aLength) ? typeB : typeA;
        const auto promoted = invoke(builder, "getPromotedComponentType")({aNode, bNode});
        return invoke(builder, "changeComponentType")({type, promoted});
    }
    if (name == "generate") {
        const auto output = arg(args, 1), op = self.get("op");
        const auto aNode = self.get("aNode"), bNode = self.get("bNode");
        const auto type = invoke(self, "getNodeType")({builder, output});
        JSValue typeA = nullptr, typeB = nullptr;
        if (!type.strictEquals("void")) {
            typeA = invoke(aNode, "getNodeType")({builder});
            typeB = bNode.toBoolean() ? invoke(bNode, "getNodeType")({builder}) : JSValue(nullptr);
            if (comparison(op)) {
                const auto a = invoke(builder, "getTypeLength")({typeA}); const auto b = invoke(builder, "getTypeLength")({typeB});
                const auto length = maximum(a, b); const auto getType = invoke(builder, "getTypeFromLength");
                const auto promoted = invoke(builder, "getPromotedComponentType")({aNode, bNode});
                typeB = getType({length, promoted}); typeA = typeB;
            } else if (op.strictEquals(">>") || op.strictEquals("<<")) {
                typeA = type; typeB = invoke(builder, "changeComponentType")({typeB, "uint"});
            } else if (invoke(builder, "isMatrix")({typeA}).toBoolean()) {
                if (invoke(builder, "isScalar")({typeB}).toBoolean()) typeB = "float";
                else if (invoke(builder, "isVector")({typeB}).toBoolean()) typeB = invoke(builder, "getVectorFromMatrix")({typeA});
                else if (invoke(builder, "isMatrix")({typeB}).toBoolean()) {}
                else { typeB = type; typeA = typeB; }
            } else if (invoke(builder, "isMatrix")({typeB}).toBoolean()) {
                if (invoke(builder, "isScalar")({typeA}).toBoolean()) typeA = "float";
                else if (invoke(builder, "isVector")({typeA}).toBoolean()) typeA = invoke(builder, "getVectorFromMatrix")({typeB});
                else { typeB = type; typeA = typeB; }
            } else { typeB = type; typeA = typeB; }
        } else { typeB = type; typeA = typeB; }
        const auto a = invoke(aNode, "build")({builder, typeA});
        const auto b = bNode.toBoolean() ? invoke(bNode, "build")({builder, typeB}) : JSValue(nullptr);
        const auto fn = invoke(builder, "getFunctionOperator")({op});
        if (!output.strictEquals("void")) {
            const auto glsl = builder.get("renderer").get("coordinateSystem").strictEquals(WebGLCoordinateSystem);
            if (comparison(op)) {
                if (glsl && invoke(builder, "isVector")({typeA}).toBoolean())
                    return formatted(builder, [&] { return Args{function(invoke(self, "getOperatorMethod")({builder, output}), a, b), type, output}; });
                return formatted(builder, [&] { return Args{binary(a, op, b), type, output}; });
            }
            if (op.strictEquals("%")) {
                if (invoke(builder, "isInteger")({typeB}).toBoolean()) return formatted(builder, [&] { return Args{binary(a, "%", b), type, output}; });
                return formatted(builder, [&] { return Args{function(invoke(self, "getOperatorMethod")({builder, type}), a, b), type, output}; });
            }
            if (op.strictEquals("!")) {
                if (glsl && invoke(builder, "isVector")({typeA}).toBoolean())
                    return formatted(builder, [&] { return Args{"not( " + a.toString() + " )", output}; });
                return formatted(builder, [&] { const auto o = op.toString(); return Args{"( " + o + " " + a.toString() + " )", typeA, output}; });
            }
            if (op.strictEquals("~")) return formatted(builder, [&] { const auto o = op.toString(); return Args{"( " + o + " " + a.toString() + " )", typeA, output}; });
            if (fn.toBoolean()) return formatted(builder, [&] { return Args{function(fn, a, b), type, output}; });
            if (invoke(builder, "isMatrix")({typeA}).toBoolean() && invoke(builder, "isScalar")({typeB}).toBoolean())
                return formatted(builder, [&] { return Args{binary(b, op, a), type, output}; });
            if (invoke(builder, "isScalar")({typeA}).toBoolean() && invoke(builder, "isMatrix")({typeB}).toBoolean())
                return formatted(builder, [&] { return Args{binary(a, op, b, false), type, output}; });
            auto snippet = binary(a, op, b);
            if (!glsl && type.strictEquals("bool") && invoke(builder, "isVector")({typeA}).toBoolean() && invoke(builder, "isVector")({typeB}).toBoolean()) snippet = "all" + snippet;
            return formatted(builder, [&] { return Args{snippet, type, output}; });
        }
        if (!typeA.strictEquals("void")) {
            if (fn.toBoolean()) return formatted(builder, [&] { return Args{function(fn, a, b), type, output}; });
            if (invoke(builder, "isMatrix")({typeA}).toBoolean() && invoke(builder, "isScalar")({typeB}).toBoolean())
                return formatted(builder, [&] { return Args{binary(b, op, a, false), type, output}; });
            return formatted(builder, [&] { return Args{binary(a, op, b, false), type, output}; });
        }
        return {};
    }
    if (name == "serialize" || name == "deserialize") {
        superMethod(self, name.c_str(), captures, {builder});
        if (name == "serialize") { auto data = builder; data.set("op", self.get("op")); }
        else self.set("op", builder.get("op"));
        return {};
    }
    throw std::logic_error("Unknown OperatorNode method");
}
JSValue firstBinding(const JSValue& values) {
    auto iterator = getJSIterator(values); JSValue result;
    try { const auto value = stepJSIterator(iterator); if (value) result = *value; }
    catch (...) { if (!iterator.done) closeJSIterator(iterator, std::current_exception()); throw; }
    if (!iterator.done) closeJSIterator(iterator); return result;
}
}

JSValue nativeOperatorNodeConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto constructor = weak.lock(); constructor && constructor->isConstructible()) return *constructor;
    auto vectorOperators = JSValue::object();
    for (const auto& [key, value] : std::initializer_list<std::pair<const char*, const char*>>{{"==","equal"},{"!=","notEqual"},{"<","lessThan"},{">","greaterThan"},{"<=","lessThanEqual"},{">=","greaterThanEqual"},{"%","mod"}}) own(vectorOperators,key,value);
    auto constructor = makeConstructor("OperatorNode", 3, +[](const JSValue& newTarget, const Args& args, const Args& captures) -> JSValue {
        auto self = superConstruct(newTarget, captures); const auto op = arg(args,0); auto a = arg(args,1), b = arg(args,2);
        if (args.size() > 3) {
            const auto constructor = captures[0].get("constructor"); auto final = constructor.construct({op,a,b});
            for (std::size_t i = 3; i + 1 < args.size(); ++i) final = constructor.construct({op,final,args[i]});
            a = final; b = args.back();
        }
        self.set("op",op);self.set("aNode",a);self.set("bNode",b);self.set("isOperatorNode",true);return self;
    }, {{"getOperatorMethod",2},{"generateNodeType",1},{"generate",2},{"serialize",1},{"deserialize",1}}, method, {vectorOperators});
    weak = JSWeakValue(constructor); return constructor;
}

namespace { JSWeakValue& operatorLexicalBindings(){static thread_local JSWeakValue weak;return weak;} }
JSValue nativeOperatorNodeModule() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;
    if (const auto module = weak.lock(); module && module->isObject()) return *module;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing,"OperatorNode exports");
    const auto tsl = nativeTSLCore(), constructor = nativeOperatorNodeConstructor(); auto bindings = JSValue::object(); own(bindings,"default",constructor);
    const auto proxy = nativeTSLCoreBinding("nodeProxyIntent");
    for (const auto& [name, op, count, variadic] : std::initializer_list<std::tuple<const char*, const char*, unsigned, bool>>{
        {"add","+",2,true},{"sub","-",2,true},{"mul","*",2,true},{"div","/",2,true},{"mod","%",2,false},
        {"equal","==",2,false},{"notEqual","!=",2,false},{"lessThan","<",2,false},{"greaterThan",">",2,false},{"lessThanEqual","<=",2,false},{"greaterThanEqual",">=",2,false},
        {"and","&&",2,true},{"or","||",2,true},{"not","!",1,false},{"xor","^^",2,false},{"bitAnd","&",2,false},{"bitNot","~",1,false},{"bitOr","|",2,false},{"bitXor","^",2,false},{"shiftLeft","<<",2,false},{"shiftRight",">>",2,false}}) {
        auto fn = proxy.call({}, {constructor,op});
        fn = invoke(fn,"setParameterLength")(variadic ? Args{count,std::numeric_limits<double>::infinity()} : Args{count});
        fn = invoke(fn,"setName")({name}); own(bindings,name,fn);
    }
    for (const auto* name : {"incrementBefore","decrementBefore","increment","decrement"}) {
        const auto callback = JSValue::function(+[](const JSValue&, const Args& args, const Args& captures) -> JSValue {
            const auto a = firstBinding(arg(args,0)); const auto name = captures[0].string(); JSValue temp;
            if (name == "increment" || name == "decrement") temp = invoke(captures[1].call({}, {a}),"toConst")();
            invoke(a, name == "incrementBefore" || name == "increment" ? "addAssign" : "subAssign")({1});
            return name == "increment" || name == "decrement" ? temp : a;
        }, {name,nativeTSLCoreBinding("int")},"",1);
        own(bindings,name,nativeTSLCoreBinding("Fn").call({}, {callback}));
    }
    const auto chaining = nativeTSLCoreBinding("addMethodChaining");
    for (const auto* name : {"add","sub","mul","div","mod","equal","notEqual","lessThan","greaterThan","lessThanEqual","greaterThanEqual","and","or","not","xor","bitAnd","bitNot","bitOr","bitXor","shiftLeft","shiftRight","incrementBefore","decrementBefore","increment","decrement"}) chaining.call({}, {name,bindings.get(name)});
    auto module = JSValue::object(); auto names = bindings.ownPropertyNames(); std::sort(names.begin(),names.end());
    for (const auto& name : names) own(module,name.c_str(),bindings.get(name));
    bindings.retainForHeapLifetime();operatorLexicalBindings()=JSWeakValue(bindings);
    module.retainForHeapLifetime(); weak = JSWeakValue(module); return module;
}
JSValue nativeOperatorNodeBinding(const std::string& name){const auto keeper=nativeOperatorNodeModule();return operatorLexicalBindings().lock()->get(name);}

OperatorNode::OperatorNode(const JSValue& op, const JSValue& a, const JSValue& b, const std::vector<JSValue>& params) {
    std::vector<JSValue> args{op,a,b};args.insert(args.end(),params.begin(),params.end());object_=constructor().construct(args);
}
JSValue OperatorNode::getOperatorMethod(const JSValue& builder,const JSValue& output) const { return object_.get("getOperatorMethod").call(object_,{builder,output}); }
JSValue OperatorNode::generateNodeType(const JSValue& builder,const JSValue& output) const { return object_.get("generateNodeType").call(object_,{builder,output}); }
JSValue OperatorNode::generate(const JSValue& builder,const JSValue& output) const { return object_.get("generate").call(object_,{builder,output}); }
void OperatorNode::serialize(const JSValue& data) const { object_.get("serialize").call(object_,{data}); }
void OperatorNode::deserialize(const JSValue& data) const { object_.get("deserialize").call(object_,{data}); }
