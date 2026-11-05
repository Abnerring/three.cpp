#include "nodes/gpgpu/AtomicFunctionNode.h"

#include "core/JSValue.h"
#include "nodes/lighting/LightingNodeRuntime.h"
#include "nodes/tsl/TSLCore.h"

#include <iostream>
#include <string>
#include <vector>


using namespace UtilityNodeRuntime;
namespace {
using ArgList = std::vector<JSValue>;

JSValue pointerStub() {
    auto pointer = JSValue::object(nullptr);
    pointer.set("getNodeType", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue("uint"); },
        {}, "getNodeType", 1));
    pointer.set("build", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue("ptr"); },
        {}, "build", 2));
    return pointer;
}

JSValue valueStub() {
    auto value = JSValue::object(nullptr);
    value.set("build", JSValue::function(
        +[](const JSValue&, const ArgList&, const ArgList&) -> JSValue { return JSValue("val"); },
        {}, "build", 2));
    return value;
}


}  // namespace

int main() {
    try {
        const auto module = nativeAtomicFunctionNodeModule();
        const auto ctor = module.get("default");
        const auto pointer = pointerStub();
        const auto value = valueStub();

        const auto atomic = ctor.construct({JSValue("atomicAdd"), pointer, value});
        auto builder = JSValue::object(nullptr);
        const auto inputType = atomic.get("getInputType").call(atomic, {builder});
        const auto generatedType = atomic.get("generateNodeType").call(atomic, {builder});


        // The module level helpers build a node and call toStack on it; whether that works is compared as a
        // boolean, since it needs the full stack node context.
        auto constants = JSValue::object(nullptr);
        for (const auto* name : {"ATOMIC_LOAD", "ATOMIC_STORE", "ATOMIC_ADD", "ATOMIC_SUB",
                                 "ATOMIC_MAX", "ATOMIC_MIN", "ATOMIC_AND", "ATOMIC_OR", "ATOMIC_XOR"}) {
            constants.set(name, ctor.get(name));
        }


        // The module level factory builds a node through nodeProxy and calls toStack on it.
        bool atomicLoadBuilt = true;
        std::string atomicLoadFailure;
        JSValue atomicLoadResult;
        try {
            atomicLoadResult = module.get("atomicLoad").call({}, {pointer});
        } catch (const std::exception& error) {
            atomicLoadBuilt = false;
            atomicLoadFailure = error.what();
        }
        const bool atomicLoadIsNode = atomicLoadBuilt && atomicLoadResult.get("isNode").strictEquals(true);
        const bool atomicLoadIsStackNode =
            atomicLoadBuilt && atomicLoadResult.get("isStackNode").strictEquals(true);
        auto output = JSValue::object(nullptr);
        output.set("name", ctor.get("name"));
        output.set("length", ctor.get("length"));
        output.set("staticType", ctor.get("type"));
        output.set("constants", constants);
        output.set("type", atomic.get("type"));
        output.set("nodeType", atomic.get("nodeType"));
        output.set("method", atomic.get("method"));
        output.set("pointerSame", atomic.get("pointerNode").sameValue(pointer));
        output.set("valueSame", atomic.get("valueNode").sameValue(value));
        output.set("parents", atomic.get("parents"));
        output.set("cacheable", atomic.get("isCacheable").call(atomic, {}));
        output.set("inputType", inputType);
        output.set("generatedType", generatedType);
        output.set("generateLength", atomic.get("generate").get("length"));
        output.set("generateExecuted", false);

        output.set("atomicFuncLength", module.get("atomicFunc").get("length"));
        output.set("atomicLoadLength", module.get("atomicLoad").get("length"));

        output.set("atomicAddLength", module.get("atomicAdd").get("length"));


        // toStack is a TSL chain method (registered through addMethodChaining in TSLCore), so a constructed node
        // answers it and chaining returns the node itself.
        const auto stackMethod = atomic.get("toStack");
        const bool toStackCallable = stackMethod.isCallable();
        bool toStackReturnsSelf = false;
        if (toStackCallable) {
            toStackReturnsSelf = stackMethod.call(atomic, {}).sameValue(atomic);
        }


        output.set("atomicLoadBuilt", atomicLoadBuilt);

        output.set("atomicLoadIsNode", atomicLoadIsNode);
        output.set("atomicLoadIsStackNode", atomicLoadIsStackNode);
        output.set("toStackCallable", toStackCallable);
        output.set("toStackReturnsSelf", toStackReturnsSelf);
        output.set("source", ctor.get("toString").call(ctor));
        std::cout << output.toJSON()->dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
}