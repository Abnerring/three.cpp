#include "nodes/utils/ArrayElementNode.h"

bool ArrayElementNode::indexKeepsType(bool isVector, bool isInteger) {
    // Only an integer that is not a vector.
    return !isVector && isInteger;
}

std::string ArrayElementNode::indexType(bool isVector, bool isInteger,
                                        const std::string& actualType) {
    // The type itself when it is kept, an unsigned integer otherwise.
    if (indexKeepsType(isVector, isInteger)) return actualType;
    return "uint";
}

std::string ArrayElementNode::nodeTypeFrom(const std::string& elementType) {
    return elementType;
}

ArrayElementNode::MemberLookup ArrayElementNode::memberLookupTarget() {
    // The array, not the element.
    return MemberLookup::Array;
}

bool ArrayElementNode::isCacheable() {
    return false;
}

#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
JSValue method(const JSValue& self, const Args& args, const Args& c) {
    const auto name=c[0].string(); const auto builder=arg(args,0);
    if(name=="isCacheable") return false;
    if(name=="getScope") return invoke(self.get("node"),"getScope")();
    if(name=="generateNodeType") return invoke(self.get("node"),"getElementType")({builder});
    if(name=="getMemberType") return invoke(self.get("node"),"getMemberType")({builder,arg(args,1)});
    if(name=="generate") {
        const auto indexType=invoke(self.get("indexNode"),"getNodeType")({builder});
        const auto snippet=invoke(self.get("node"),"build")({builder});
        const auto build=invoke(self.get("indexNode"),"build");
        const bool scalar=!invoke(builder,"isVector")({indexType}).toBoolean();
        const auto output=scalar && invoke(builder,"isInteger")({indexType}).toBoolean() ? indexType : JSValue("uint");
        const auto index=build({builder,output});
        const auto text=snippet.toString(); return text+"[ "+index.toString()+" ]";
    }
    throw std::logic_error("Unknown ArrayElementNode method");
}
}
JSValue nativeArrayElementNodeConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak; if(const auto value=weak.lock();value&&value->isConstructible()) return *value;
    auto constructor=makeConstructor("ArrayElementNode",2,+[](const JSValue& newTarget,const Args& args,const Args& c)->JSValue {
        auto self=superConstruct(newTarget,c); self.set("node",arg(args,0)); self.set("indexNode",arg(args,1)); self.set("isArrayElementNode",true); return self;
    },{{"isCacheable",0},{"getScope",0},{"generateNodeType",1},{"getMemberType",2},{"generate",1}},method);
    weak=JSWeakValue(constructor); return constructor;
}
