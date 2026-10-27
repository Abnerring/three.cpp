#include "nodes/utils/ConvertNode.h"

int ConvertNode::typeLength(const std::string& type) {
    // A three part vector counts three and a float counts one; a matrix counts its rows times
    // its columns. Only the shapes the source distinguishes are needed here.
    if (type == "float" || type == "int" || type == "uint" || type == "bool" || type == "half") {
        return 1;
    }
    if (type.size() > 3 && type.compare(0, 3, "vec") == 0) {
        return type[3] - '0';
    }
    if (type.size() > 3 && type.compare(0, 3, "mat") == 0) {
        const int n = type[3] - '0';
        const int m = type.size() > 4 ? type[4] - '0' : n;
        return n * m;
    }
    return 1;
}

std::vector<std::string> ConvertNode::parts(const std::string& convertTo) {
    std::vector<std::string> out;
    std::string current;
    for (const char c : convertTo) {
        if (c == '|') {
            out.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    out.push_back(current);
    return out;
}

std::string ConvertNode::choose(const std::string& requestType, const std::string& convertTo) {
    const int requestLength = typeLength(requestType);
    std::string chosen;
    bool first = true;
    for (const auto& part : parts(convertTo)) {
        // The first part is kept because there is nothing chosen yet, whatever its length.
        if (first || typeLength(part) == requestLength) {
            chosen = part;
            first = false;
        }
    }
    return chosen;
}

#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
JSValue method(const JSValue& self, const Args& args, const Args& c) {
    const auto name=c[0].string(); const auto builder=arg(args,0);
    if(name=="generateNodeType") {
        const auto requestType=invoke(self.get("node"),"getNodeType")({builder}); JSValue result=nullptr;
        const auto overloads=invoke(self.get("convertTo"),"split")({"|"});
        each(overloads,[&](const JSValue& type) {
            if(result.isNull()) result=type;
            else { const auto request=invoke(builder,"getTypeLength")({requestType}); const auto output=invoke(builder,"getTypeLength")({type}); if(request.strictEquals(output)) result=type; }
        }); return result;
    }
    if(name=="serialize"||name=="deserialize") {
        superMethod(self,name.c_str(),c,{builder});
        if(name=="serialize") {auto data=builder;data.set("convertTo",self.get("convertTo"));}
        else {auto receiver=self;receiver.set("convertTo",builder.get("convertTo"));} return {};
    }
    if(name=="generate") {
        const auto node=self.get("node"), type=invoke(self,"getNodeType")({builder});
        const auto snippet=invoke(node,"build")({builder,type});return invoke(builder,"format")({snippet,type,arg(args,1)});
    }
    throw std::logic_error("Unknown ConvertNode method");
}
}
JSValue nativeConvertNodeConstructor() {
    using namespace UtilityNodeRuntime;
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    auto constructor=makeConstructor("ConvertNode",2,+[](const JSValue& newTarget,const Args& args,const Args& c)->JSValue {
        auto self=superConstruct(newTarget,c);self.set("node",arg(args,0));self.set("convertTo",arg(args,1));return self;
    },{{"generateNodeType",1},{"serialize",1},{"deserialize",1},{"generate",2}},method);
    weak=JSWeakValue(constructor);return constructor;
}
