#include "nodes/utils/MemberNode.h"

bool MemberNode::hasMember(bool structIsMember, bool structFoundItsMember, bool foundHere) {
    // A structure that is itself a member must first have found its own member.
    if (structIsMember && !structFoundItsMember) return false;
    return foundHere;
}

std::string MemberNode::memberType(bool found, const std::string& actualType) {
    // A member that is not there is a float rather than an error.
    if (!found) return kMissingMemberType;
    return actualType;
}

bool MemberNode::guardUsesOwnProperty() {
    // The guard asks about the node's own property even when a different name was passed in.
    return true;
}

std::string MemberNode::missingMemberWarning(const std::string& property) {
    return "TSL: Member \"" + property + "\" does not exist in struct.";
}

bool MemberNode::isCacheable() {
    return false;
}

#include "nodes/utils/UtilityNodeRuntime.h"
#include "utils.h"
namespace {
using namespace UtilityNodeRuntime;
JSValue method(const JSValue& self,const Args& args,const Args& c) {
    const auto name=c[0].string();const auto builder=arg(args,0);
    if(name=="isCacheable")return false;
    if(name=="getScope")return invoke(self.get("structNode"),"getScope")();
    if(name=="hasMember") {
        if(self.get("structNode").get("isMemberNode").toBoolean()) {
            if(invoke(self.get("structNode"),"hasMember")({builder}).strictEquals(false))return false;
        }
        return !invoke(self.get("structNode"),"getMemberType")({builder,self.get("property")}).strictEquals("void");
    }
    if(name=="generateNodeType") {
        if(invoke(self,"hasMember")({builder}).strictEquals(false))return "float";
        return invoke(self.get("structNode"),"getMemberType")({builder,self.get("property")});
    }
    if(name=="getMemberType") {
        if(invoke(self,"hasMember")({builder}).strictEquals(false))return "float";
        const auto type=invoke(self,"getNodeType")({builder}),structure=invoke(builder,"getStructTypeNode")({type});
        return invoke(structure,"getMemberType")({builder,arg(args,1)});
    }
    if(name=="generate") {
        if(invoke(self,"hasMember")({builder}).strictEquals(false)) {
            const auto property=self.get("property").toString();ThreeUtils::warn({"TSL: Member \""+property+"\" does not exist in struct.",self.get("stackTrace")});
            const auto type=invoke(self,"getNodeType")({builder});return invoke(builder,"generateConst")({type});
        }
        const auto property=invoke(self.get("structNode"),"build")({builder});return add(add(property,"."),self.get("property"));
    }
    throw std::logic_error("Unknown MemberNode method");
}
}
JSValue nativeMemberNodeConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    auto constructor=makeConstructor("MemberNode",2,+[](const JSValue& newTarget,const Args& args,const Args& c)->JSValue {
        auto self=superConstruct(newTarget,c);self.set("structNode",arg(args,0));self.set("property",arg(args,1));self.set("isMemberNode",true);return self;
    },{{"isCacheable",0},{"getScope",0},{"hasMember",1},{"generateNodeType",1},{"getMemberType",2},{"generate",1}},method);weak=JSWeakValue(constructor);return constructor;
}
