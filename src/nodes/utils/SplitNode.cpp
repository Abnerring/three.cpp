#include "nodes/utils/SplitNode.h"

#include <algorithm>

namespace {
const char* kComponents = "xyzw";
}

int SplitNode::vectorLength(const std::string& components) {
    int length = static_cast<int>(components.size());
    for (const char c : components) {
        const char* found = std::find(kComponents, kComponents + 4, c);
        const int index = (found == kComponents + 4) ? -1 : static_cast<int>(found - kComponents);
        length = std::max(index + 1, length);
    }
    return length;
}

bool SplitNode::swizzleIsIdentity(int nodeTypeLength, const std::string& components) {
    // As many names as the value is wide, and the first that many in order.
    if (static_cast<int>(components.size()) != nodeTypeLength) return false;
    return components == std::string(kComponents, static_cast<std::size_t>(nodeTypeLength));
}

bool SplitNode::componentsAreRead(int nodeTypeLength) {
    // A one part value ignores them entirely.
    return nodeTypeLength > 1;
}

bool SplitNode::isCacheable() {
    return false;
}

#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
JSValue method(const JSValue& self,const Args& args,const Args& c) {
    const auto name=c[0].string();const auto builder=arg(args,0);
    if(name=="isCacheable")return false;
    if(name=="getScope")return invoke(self.get("node"),"getScope")();
    if(name=="getVectorLength") {
        auto length=self.get("components").get("length");
        each(self.get("components"),[&](const JSValue& component) {
            const auto index=invoke(c[2],"indexOf")({component});
            const auto next=add(index,1).toNumber();const auto previous=length.toNumber();
            length=std::isnan(next)||std::isnan(previous)?std::numeric_limits<double>::quiet_NaN(): next==0&&previous==0 ? (std::signbit(next)&&std::signbit(previous)?-0.0:0.0): std::max(next,previous);
        });return length;
    }
    if(name=="getComponentType") {
        const auto getComponent=invoke(builder,"getComponentType");
        return getComponent({invoke(self.get("node"),"getNodeType")({builder})});
    }
    if(name=="generateNodeType") {
        const auto getType=invoke(builder,"getTypeFromLength");const auto length=self.get("components").get("length");
        return getType({length,invoke(self,"getComponentType")({builder})});
    }
    if(name=="generate") {
        const auto node=self.get("node");const auto getLength=invoke(builder,"getTypeLength");
        const auto length=getLength({invoke(node,"getNodeType")({builder})});JSValue snippet=nullptr;
        if(greater(length,1)) {
            JSValue type=nullptr;const auto componentLength=invoke(self,"getVectorLength")();
            if(greaterEqual(componentLength,length)) {
                const auto getType=invoke(builder,"getTypeFromLength");const auto width=invoke(self,"getVectorLength")();
                type=getType({width,invoke(self,"getComponentType")({builder})});
            }
            const auto nodeSnippet=invoke(node,"build")({builder,type});
            if(self.get("components").get("length").strictEquals(length)) {
                const auto components=self.get("components");const auto slice=invoke(c[3],"slice");
                if(components.strictEquals(slice({0,self.get("components").get("length")}))) return invoke(builder,"format")({nodeSnippet,type,arg(args,1)});
            }
            const auto format=invoke(builder,"format");const auto prefix=nodeSnippet.toString();const auto suffix=self.get("components").toString();
            snippet=format({prefix+"."+suffix,invoke(self,"getNodeType")({builder}),arg(args,1)});
        } else snippet=invoke(node,"build")({builder,arg(args,1)});
        return snippet;
    }
    if(name=="serialize"||name=="deserialize") {
        superMethod(self,name.c_str(),c,{builder});
        if(name=="serialize"){auto data=builder;data.set("components",self.get("components"));}
        else {auto receiver=self;receiver.set("components",builder.get("components"));}return {};
    }
    throw std::logic_error("Unknown SplitNode method");
}
}
JSValue nativeSplitNodeConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto components=nativeNodeConstantsBinding("vectorComponents");const auto text=invoke(components,"join")({""});
    auto constructor=makeConstructor("SplitNode",1,+[](const JSValue& newTarget,const Args& args,const Args& c)->JSValue {
        auto self=superConstruct(newTarget,c);self.set("node",arg(args,0));self.set("components",arg(args,1,"x"));self.set("isSplitNode",true);return self;
    },{{"isCacheable",0},{"getVectorLength",0},{"getComponentType",1},{"generateNodeType",1},{"getScope",0},{"generate",2},{"serialize",1},{"deserialize",1}},method,{components,text});
    weak=JSWeakValue(constructor);return constructor;
}
