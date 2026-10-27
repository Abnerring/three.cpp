#include "nodes/utils/SetNode.h"

const std::vector<std::string>& SetNode::componentNames() {
    static const std::vector<std::string> names{"x", "y", "z", "w"};
    return names;
}

std::vector<bool> SetNode::replaced(int length, const std::string& components) {
    std::vector<bool> out(static_cast<std::size_t>(length), false);
    const auto& names = componentNames();
    for (int i = 0; i < length; i += 1) {
        const std::string& name = names[static_cast<std::size_t>(i)];
        if (!components.empty() && name[0] == components[0]) {
            // The replacement value has as many parts as there are names, so it fills the whole
            // span the names cover; every position in it is replaced, and the walk jumps over
            // the rest.
            for (std::size_t k = 0; k < components.size(); ++k) {
                const std::size_t at = static_cast<std::size_t>(i) + k;
                if (at < out.size()) out[at] = true;
            }
            i += static_cast<int>(components.size()) - 1;
        }
    }
    return out;
}

int SetNode::producedCount(int length, const std::string& components) {
    const auto& names = componentNames();
    int produced = 0;
    for (int i = 0; i < length; i += 1) {
        const std::string& name = names[static_cast<std::size_t>(i)];
        if (!components.empty() && name[0] == components[0]) {
            produced += 1;
            i += static_cast<int>(components.size()) - 1;
        } else {
            produced += 1;
        }
    }
    return produced;
}

bool SetNode::isContiguousAndOrdered(const std::string& components) {
    // The names must be a run of the component names with no gap and no reordering.
    if (components.empty()) return false;
    int last = -1;
    for (std::size_t k = 0; k < components.size(); ++k) {
        int found = -1;
        for (std::size_t i = 0; i < componentNames().size(); ++i) {
            if (componentNames()[i][0] == components[k]) found = static_cast<int>(i);
        }
        if (k == 0) {
            last = found;
        } else {
            if (found != last + 1) return false;
            last = found;
        }
    }
    return true;
}

int SetNode::targetWidth(const std::string& components) {
    return static_cast<int>(components.size());
}

#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
JSValue method(const JSValue& self,const Args& args,const Args& c) {
    const auto name=c[0].string();const auto builder=arg(args,0);
    if(name=="generateNodeType")return invoke(self.get("sourceNode"),"getNodeType")({builder});
    if(name=="generate") {
        const auto source=self.get("sourceNode"),components=self.get("components"),target=self.get("targetNode");
        const auto sourceType=invoke(self,"getNodeType")({builder});const auto getComponent=invoke(builder,"getComponentType");
        const auto componentType=getComponent({invoke(target,"getNodeType")({builder})});
        const auto targetType=invoke(builder,"getTypeFromLength")({components.get("length"),componentType});
        const auto targetSnippet=invoke(target,"build")({builder,targetType});const auto sourceSnippet=invoke(source,"build")({builder,sourceType});
        const auto length=invoke(builder,"getTypeLength")({sourceType});auto snippets=JSValue::array();
        for(double i=0;less(i,length);++i) {
            const auto component=c[2].get(JSValue(i));
            if(component.strictEquals(components.get("0"))) {
                invoke(snippets,"push")({targetSnippet});const auto count=components.get("length").toNumber()-1;i+=count;
            } else {const auto push=invoke(snippets,"push");push({add(add(sourceSnippet,"."),component)});}
        }
        const auto prefix=invoke(builder,"getType")({sourceType}).toString();const auto joined=invoke(snippets,"join")({", "});return prefix+"( "+joined.toString()+" )";
    }
    throw std::logic_error("Unknown SetNode method");
}
}
JSValue nativeSetNodeConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto components=nativeNodeConstantsBinding("vectorComponents");
    auto constructor=makeConstructor("SetNode",3,+[](const JSValue& newTarget,const Args& args,const Args& c)->JSValue {
        auto self=superConstruct(newTarget,c);self.set("sourceNode",arg(args,0));self.set("components",arg(args,1));self.set("targetNode",arg(args,2));return self;
    },{{"generateNodeType",1},{"generate",1}},method,{components});weak=JSWeakValue(constructor);return constructor;
}
