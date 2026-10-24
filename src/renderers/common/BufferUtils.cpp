#include "renderers/common/BufferUtils.h"
#include "renderers/common/Constants.h"
#include <cmath>

namespace {
JSValue padded(const JSValue& length, double alignment) {
    const auto padding = std::fmod(alignment - std::fmod(length.toNumber(),alignment),alignment);
    // The left operand is converted again, after the remainder expression.
    const auto primitive = length.toPrimitive();
    if (primitive.type() == JSValue::Type::String) return primitive.string() + JSValue(padding).toString();
    return primitive.toNumber() + padding;
}
}
JSValue getFloatLength(const JSValue& length) { return padded(length,GPU_CHUNK_BYTES); }
JSValue getStrideLength(const JSValue& length) { return padded(length,4); }
JSValue getVectorLength(const JSValue& count, const JSValue& vectorLength) {
    const auto stride = getStrideLength(vectorLength.isUndefined() ? JSValue(4) : vectorLength);
    const auto a = stride.toNumber(); const auto b = count.toNumber(); return getFloatLength(a * b);
}
JSValue nativeBufferUtils() {
    static thread_local JSWeakValue weak;
    if (const auto module = weak.lock(); module && module->isObject()) return *module;
    auto module = JSValue::object();
    for (const auto* name : {"getFloatLength","getVectorLength","getStrideLength"}) {
        module.set(name,JSValue::function([](const JSValue&,const std::vector<JSValue>& args,const std::vector<JSValue>& c)->JSValue{
            const auto value=args.empty()?JSValue{}:args[0]; const auto name=c[0].string();
            if(name=="getFloatLength")return getFloatLength(value);
            if(name=="getStrideLength")return getStrideLength(value);
            return getVectorLength(value,args.size()>1?args[1]:JSValue{});
        },{name},name,1));
    }
    module.retainForHeapLifetime(); weak=JSWeakValue(module); return module;
}
