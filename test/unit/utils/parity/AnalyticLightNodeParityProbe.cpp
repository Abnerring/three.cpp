#include "nodes/lighting/AnalyticLightNode.h"

#include "core/JSValue.h"
#include "nodes/core/Node.h"

#include <iostream>

int main() {
    try {
        const auto module = nativeAnalyticLightNodeModule();
        const auto ctor = nativeAnalyticLightNodeConstructor();
        const auto proto = ctor.get("prototype");
        const auto getLightVector = proto.get("getLightVector");
        auto output = JSValue::object(nullptr);
        output.set("moduleSame", module.get("default").sameValue(ctor));
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("hasGetLightVector", getLightVector.isCallable());
        output.set("getLightVectorLength", getLightVector.get("length"));
        output.set("getLightVectorName", getLightVector.get("name"));
        output.set("getLightVectorSource", getLightVector.get("toString").call(getLightVector));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (...) {
        return 1;
    }
}
