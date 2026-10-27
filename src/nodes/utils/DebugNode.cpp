#include "nodes/utils/DebugNode.h"
#include "nodes/utils/UtilityNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"
#include "core/JSRegExp.h"
#include "utils.h"

std::string DebugNode::title(const std::string& shaderStage) {
    return "--- TSL debug - " + shaderStage + " shader ---";
}

std::string DebugNode::border(const std::string& titleText) {
    // As many dashes as the title is long, which already includes its own end dashes.
    return std::string(jsUTF16(titleText).size(), '-');
}

std::string DebugNode::stripLeadingTabs(const std::string& code) {
    // Only a tab at the beginning of a line is removed. Spaces are left alone, and so is a tab
    // that comes after something else.
    std::u16string out;
    const auto units=jsUTF16(code);out.reserve(units.size());
    bool atLineStart = true;
    for (const auto c : units) {
        if (atLineStart && c == '\t') {atLineStart=false;continue;}
        out.push_back(c);
        atLineStart = (c == '\n'||c=='\r'||c==0x2028||c==0x2029);
    }
    return jsUTF8(out);
}

std::string DebugNode::block(const std::string& shaderStage, const std::string& code,
                             const std::string& snippet) {
    const std::string titleText = title(shaderStage);
    const std::string borderText = border(titleText);
    std::string out;
    out += "// #" + titleText + "#\n";
    out += stripLeadingTabs(code) + "\n";
    out += "/* ... */ " + snippet + " /* ... */\n";
    out += "// #" + borderText + "#\n";
    return out;
}

namespace {
using namespace UtilityNodeRuntime;
JSWeakValue& debugLexical(){static thread_local JSWeakValue weak;return weak;}
JSValue debugMethod(const JSValue& self,const Args& args,const Args& c){
    const auto name=c[0],builder=arg(args,0);
    if(name.strictEquals("generateNodeType"))return invoke(self.get("node"),"getNodeType")({builder});
    if(name.strictEquals("setup")||name.strictEquals("analyze"))return invoke(self.get("node"),"build")({builder});
    const auto callback=self.get("callback"),snippet=invoke(self.get("node"),"build")({builder});
    if(!callback.isNull())callback.call({}, {builder,snippet});
    else{
        const auto prefix=add("--- TSL debug - ",builder.get("shaderStage"));const auto title=add(prefix," shader ---");
        const auto repeat=invoke("-","repeat");const auto border=repeat({title.get("length")});
        auto code=add(add("// #",title),"#\n");
        const auto replace=invoke(builder.get("flow").get("code"),"replace");const auto pattern=createNativeRegExp("^\t","mg");const auto cleaned=replace({pattern,""});
        code=add(code,add(cleaned,"\n"));const auto middle=add(add("/* ... */ ",snippet)," /* ... */\n");code=add(code,middle);
        code=add(code,add(add("// #",border),"#\n"));ThreeUtils::log({code});
    }
    return snippet;
}
}
JSValue nativeDebugNodeConstructor(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    auto constructor=makeConstructor("DebugNode",1,+[](const JSValue& target,const Args& args,const Args& c)->JSValue{auto self=superConstruct(target,c);self.set("node",arg(args,0));self.set("callback",arg(args,1,nullptr));return self;},
        {{"generateNodeType",1},{"setup",1},{"analyze",1},{"generate",1}},debugMethod);weak=JSWeakValue(constructor);return constructor;
}
JSValue nativeDebugNodeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"DebugNode exports");
    const auto tsl=nativeTSLCore(),constructor=nativeDebugNodeConstructor();auto bindings=JSValue::object();
    own(bindings,"debug",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{
        const auto node=c[1].call({}, {arg(args,0)});return invoke(c[0].construct({node,arg(args,1,nullptr)}),"toStack")();
    },{constructor,nativeTSLCoreBinding("nodeObject")},"debug",1));own(bindings,"default",constructor);
    nativeTSLCoreBinding("addMethodChaining").call({}, {"debug",bindings.get("debug")});auto module=JSValue::object();
    for(const auto& key:bindings.ownPropertyNames())own(module,key.c_str(),bindings.get(key));bindings.retainForHeapLifetime();debugLexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeDebugNodeBinding(const std::string& name){const auto keeper=nativeDebugNodeModule();return debugLexical().lock()->get(name);}
DebugNode::DebugNode(const JSValue& node,const JSValue& callback):object_(constructor().construct({node,callback})){}
