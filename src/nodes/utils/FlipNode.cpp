#include "nodes/utils/FlipNode.h"

const std::vector<std::string>& FlipNode::componentNames() {
    static const std::vector<std::string> names{"x", "y", "z", "w"};
    return names;
}

std::vector<bool> FlipNode::flipped(int length, const std::string& components) {
    std::vector<bool> out(static_cast<std::size_t>(length), false);
    std::size_t componentIndex = 0;
    const auto& names = componentNames();
    for (int i = 0; i < length; ++i) {
        const std::string& name = names[static_cast<std::size_t>(i)];
        // The walk looks for the next name in the list at this position; if it does not match,
        // the component is left alone and the walk does not advance.
        if (componentIndex < components.size() && name[0] == components[componentIndex]) {
            out[static_cast<std::size_t>(i)] = true;
            componentIndex += 1;
        }
    }
    return out;
}

double FlipNode::flip(double value) {
    return 1.0 - value;
}

bool FlipNode::isInWalkOrder(const std::string& components) {
    // The names must appear in the order the walk visits them.
    const auto& names = componentNames();
    int last = -1;
    for (const char c : components) {
        int found = -1;
        for (std::size_t i = 0; i < names.size(); ++i) {
            if (names[i][0] == c) found = static_cast<int>(i);
        }
        if (found <= last) return false;
        last = found;
    }
    return true;
}

#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
JSValue method(const JSValue& self,const Args& args,const Args& c) {
    const auto name=c[0].string();const auto builder=arg(args,0);
    if(name=="generateNodeType")return invoke(self.get("sourceNode"),"getNodeType")({builder});
    if(name=="generate") {
        const auto components=self.get("components"),source=self.get("sourceNode");
        const auto type=invoke(self,"getNodeType")({builder}),snippet=invoke(source,"build")({builder});
        const auto cache=invoke(builder,"getVarFromNode")({self}),property=invoke(builder,"getPropertyName")({cache});
        const auto addLine=invoke(builder,"addLineFlowCode");addLine({add(add(property," = "),snippet),self});
        const auto length=invoke(builder,"getTypeLength")({type});auto snippets=JSValue::array();double componentIndex=0;
        for(double i=0;less(i,length);++i) {
            const auto component=c[2].get(JSValue(i));
            if(component.strictEquals(components.get(JSValue(componentIndex)))) {
                const auto push=invoke(snippets,"push");push({add("1.0 - ",add(add(property,"."),component))});++componentIndex;
            }else {const auto push=invoke(snippets,"push");push({add(add(property,"."),component)});}
        }
        const auto prefix=invoke(builder,"getType")({type}).toString();const auto joined=invoke(snippets,"join")({", "});return prefix+"( "+joined.toString()+" )";
    }
    throw std::logic_error("Unknown FlipNode method");
}
}
JSValue nativeFlipNodeConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto components=nativeNodeConstants().get("vectorComponents");
    auto constructor=makeConstructor("FlipNode",2,+[](const JSValue& newTarget,const Args& args,const Args& c)->JSValue {
        auto self=superConstruct(newTarget,c);self.set("sourceNode",arg(args,0));self.set("components",arg(args,1));return self;
    },{{"generateNodeType",1},{"generate",1}},method,{components});weak=JSWeakValue(constructor);return constructor;
}
