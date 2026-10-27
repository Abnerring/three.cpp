#include "nodes/core/SubBuildNode.h"

bool SubBuildNode::bracketsTheTypeQuery(bool typeWasGiven) {
    // A type that was given is returned before the bracket opens.
    return !typeWasGiven;
}

bool SubBuildNode::bracketsTheBuild() {
    return true;
}

bool SubBuildNode::balancedOnThrow() {
    // A plain remove after the call, with nothing to run it on the way out.
    return false;
}

bool SubBuildNode::isCacheable() {
    return false;
}

#include "nodes/core/Node.h"
#include "nodes/tsl/TSLCore.h"
#include "core/JSModuleObject.h"
#include "core/JSIterator.h"

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, unsigned i, const JSValue& fallback = {}) { return i < args.size() && !args[i].isUndefined() ? args[i] : fallback; }
struct Invocation { JSValue object, method; JSValue operator()(const Args& args = {}) const { return method.call(object, args); } };
Invocation invoke(const JSValue& object, const char* key) { return {object, object.get(key)}; }
JSWeakValue& lexicalBindings() { static thread_local JSWeakValue weak; return weak; }
JSValue method(const JSValue& self, const Args& args, const Args& captures) {
    const auto name = captures[0].string(); const auto builder = arg(args, 0);
    if (name == "isCacheable") return false;
    if (name == "generateNodeType") {
        if (!self.get("nodeType").isNull()) return self.get("nodeType");
        const auto add = invoke(builder, "addSubBuild"); add({self.get("name")});
        const auto type = invoke(self.get("node"), "getNodeType")({builder});
        invoke(builder, "removeSubBuild")(); return type;
    }
    if (name == "build") {
        auto rest = JSValue::array(); for (unsigned i = 1; i < args.size(); ++i) { JSPropertyDescriptor d; d.value = args[i]; d.writable = d.enumerable = d.configurable = true; rest.defineProperty(std::to_string(i-1),d); }
        const auto add = invoke(builder, "addSubBuild"); add({self.get("name")});
        const auto build = invoke(self.get("node"), "build"); Args params{builder}; auto iterator = getJSIterator(rest); while (const auto value = stepJSIterator(iterator)) params.push_back(*value);
        const auto data = build(params); invoke(builder, "removeSubBuild")(); return data;
    }
    throw std::logic_error("Unknown SubBuildNode method");
}
}
JSValue nativeSubBuildNodeConstructor() {
    static thread_local JSWeakValue weak; if (const auto value = weak.lock(); value && value->isConstructible()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing, "SubBuildNode");
    const auto base = nativeNodeConstructor(), tslKeeper = nativeTSLCore(); auto holder = JSValue::object(nullptr);
    auto ctor = JSValue::builtinConstructor([](const JSValue& target, const Args& args, const Args& captures) -> JSValue {
        auto self = captures[0].get("constructor").getPrototype().construct({arg(args,2,nullptr)},target);
        self.set("node",arg(args,0)); self.set("name",arg(args,1)); self.set("isSubBuildNode",true); return self;
    },{holder},"SubBuildNode",2);
    holder.set("constructor",ctor); ctor.setPrototype(base); auto prototype = ctor.get("prototype"); prototype.setPrototype(base.get("prototype"));
    JSPropertyDescriptor type; type.getter = JSValue::function([](const JSValue&,const Args&,const Args&) -> JSValue { return "SubBuild"; },{},"get type",0); type.configurable = true; ctor.defineProperty("type",type);
    for (const auto& [name,arity] : std::initializer_list<std::pair<const char*,unsigned>>{{"isCacheable",0},{"generateNodeType",1},{"build",1}}) {
        JSPropertyDescriptor d; d.value = JSValue::function(method,{name},name,arity); d.writable = d.configurable = true; prototype.defineProperty(name,d);
    }
    ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
JSValue nativeSubBuildNodeModule() {
    static thread_local JSWeakValue weak; if (const auto value = weak.lock(); value && value->isObject()) return *value;
    static thread_local bool initializing = false; JSModuleInitializationScope scope(initializing,"SubBuildNode exports");
    const auto ctor = nativeSubBuildNodeConstructor(), nodeObject = nativeTSLCoreBinding("nodeObject");
    const auto subBuild = JSValue::function([](const JSValue&,const Args& args,const Args& captures) -> JSValue {
        const auto node = captures[1].call({}, {arg(args,0)}); return captures[0].construct({node,arg(args,1),arg(args,2,nullptr)});
    },{ctor,nodeObject},"subBuild",2);
    auto bindings = JSValue::object(); bindings.set("default",ctor); bindings.set("subBuild",subBuild); auto exports = JSValue::object(); exports.set("default",ctor); exports.set("subBuild",subBuild);
    bindings.retainForHeapLifetime(); lexicalBindings() = JSWeakValue(bindings); exports.retainForHeapLifetime(); weak = JSWeakValue(exports); return exports;
}
JSValue nativeSubBuildNodeBinding(const std::string& name) { const auto keeper = nativeSubBuildNodeModule(); return lexicalBindings().lock()->get(name); }
SubBuildNode::SubBuildNode(const JSValue& node,const JSValue& name,const JSValue& nodeType) : object_(constructor().construct({node,name,nodeType})) {}
JSValue SubBuildNode::generateNodeType(const JSValue& builder) const { return invoke(object_,"generateNodeType")({builder}); }
JSValue SubBuildNode::build(const JSValue& builder,const std::vector<JSValue>& params) const { Args args{builder}; args.insert(args.end(),params.begin(),params.end()); return invoke(object_,"build")(args); }
