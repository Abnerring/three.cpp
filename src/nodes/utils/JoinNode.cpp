#include "nodes/utils/JoinNode.h"

int JoinNode::joinedWidth(const std::vector<int>& partWidths) {
    // The sum, with no check that a type of that width exists.
    int total = 0;
    for (const int width : partWidths) total += width;
    return total;
}

JoinNode::Overrun JoinNode::checkOverrun(int length, int maxLength, int partWidth) {
    // The first check uses greater or equal and stops the loop; the second uses greater and cuts
    // the part down.
    if (length >= maxLength) return Overrun::AlreadyFull;
    if (length + partWidth > maxLength) return Overrun::Truncated;
    return Overrun::None;
}

int JoinNode::truncatedWidth(int length, int maxLength) {
    return maxLength - length;
}

bool JoinNode::printsMessage(Overrun overrun) {
    // Both of the overruns report, and the ordinary case does not.
    return overrun != Overrun::None;
}

std::string JoinNode::alreadyFullMessage(const std::string& type) {
    return "TSL: Length of parameters exceeds maximum length of function '" + type + "()' type.";
}

std::string JoinNode::truncatedMessage(const std::string& type) {
    return "TSL: Length of '" + type + "()' data exceeds maximum length of output type.";
}

#include "nodes/utils/UtilityNodeRuntime.h"
#include "utils.h"
namespace {
using namespace UtilityNodeRuntime;
JSValue method(const JSValue& self,const Args& args,const Args& c) {
    const auto name=c[0].string(); const auto builder=arg(args,0);
    if(name=="generateNodeType") {
        if(!self.get("nodeType").isNull()) return invoke(builder,"getVectorType")({self.get("nodeType")});
        const auto getType=invoke(builder,"getTypeFromLength");
        const auto reduce=invoke(self.get("nodes"),"reduce");
        const auto callback=JSValue::function(+[](const JSValue&,const Args& a,const Args& captures)->JSValue {
            const auto count=arg(a,0); const auto getLength=invoke(captures[0],"getTypeLength");
            const auto nodeType=invoke(arg(a,1),"getNodeType")({captures[0]}); return add(count,getLength({nodeType}));
        },{builder},"",2);
        return getType({reduce({callback,0})});
    }
    if(name=="generate") {
        const auto type=invoke(self,"getNodeType")({builder});const auto maximum=invoke(builder,"getTypeLength")({type});
        const auto nodes=self.get("nodes"),primitive=invoke(builder,"getComponentType")({type});auto snippets=JSValue::array();JSValue length=0;
        auto iterator=getJSIterator(nodes);
        while(const auto input=stepJSIterator(iterator)) {
            bool closing=false;
            try {
                if(greaterEqual(length,maximum)) {
                    ThreeUtils::error({"TSL: Length of parameters exceeds maximum length of function '"+type.toString()+"()' type.",self.get("stackTrace")});
                    closing=true;closeJSIterator(iterator);break;
                }
                auto inputType=invoke(*input,"getNodeType")({builder}),width=invoke(builder,"getTypeLength")({inputType});JSValue snippet;
                const auto sum=add(length,width);
                if(greater(sum,maximum)) {
                    ThreeUtils::error({"TSL: Length of '"+type.toString()+"()' data exceeds maximum length of output type.",self.get("stackTrace")});
                    const auto a=maximum.toNumber(); width=a-length.toNumber();inputType=invoke(builder,"getTypeFromLength")({width});
                }
                length=add(length,width);snippet=invoke(*input,"build")({builder,inputType});
                const auto component=invoke(builder,"getComponentType")({inputType});
                if(!component.strictEquals(primitive)) {
                    const auto target=invoke(builder,"getTypeFromLength")({width,primitive});snippet=invoke(builder,"format")({snippet,inputType,target});
                }
                invoke(snippets,"push")({snippet});
            }catch(...){if(closing)throw;closeJSIterator(iterator,std::current_exception());}
        }
        const auto prefix=invoke(builder,"getType")({type}).toString();const auto joined=invoke(snippets,"join")({", "});
        return invoke(builder,"format")({prefix+"( "+joined.toString()+" )",type,arg(args,1)});
    }
    throw std::logic_error("Unknown JoinNode method");
}
}
JSValue nativeJoinNodeConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    auto constructor=makeConstructor("JoinNode",0,+[](const JSValue& newTarget,const Args& args,const Args& c)->JSValue {
        const auto nodes=arg(args,0,JSValue::array()),type=arg(args,1,nullptr);auto self=superConstruct(newTarget,c,{type});self.set("nodes",nodes);return self;
    },{{"generateNodeType",1},{"generate",2}},method);weak=JSWeakValue(constructor);return constructor;
}
