#include "renderers/webgl/WebGLAnimation.h"
#include <iostream>

namespace {
using Args = std::vector<JSValue>;
struct Sample {
    std::string method;
    JSValue keep;
    std::vector<JSWeakValue> refs;
};
Sample prepare(const char* method) {
    auto animation = nativeWebGLAnimationConstructor().call({});
    const auto loop = JSValue::function(+[](const JSValue&, const Args&, const Args&) -> JSValue { return {}; });
    const auto oldContext = JSValue::object(), context = JSValue::object(), requestId = JSValue::object();
    auto first = oldContext;
    first.set("requestAnimationFrame", JSValue::function(
        +[](const JSValue&, const Args&, const Args& captures) -> JSValue { return captures[0]; }, {requestId}));
    animation.get("setAnimationLoop").call({}, {loop});
    animation.get("setContext").call({}, {first});
    animation.get("start").call({});
    animation.get("setContext").call({}, {context});
    return {method, animation.get(method), {JSWeakValue(animation), JSWeakValue(oldContext),
        JSWeakValue(context), JSWeakValue(loop), JSWeakValue(requestId)}};
}
JsonValue alive(const Sample& sample) {
    JsonValue::Array result;
    for (const auto& weak : sample.refs) result.emplace_back(!weak.expired());
    return result;
}
}
int main() {
    auto witness = JSValue::object();
    std::vector<Sample> samples;
    for (const auto* method : {"start", "stop", "setAnimationLoop", "setContext"}) samples.push_back(prepare(method));
    witness.collectGarbage();
    JsonValue::Array result;
    for (const auto& sample : samples) result.emplace_back(JsonValue::Object{{"method", sample.method}, {"before", alive(sample)}});
    for (auto& sample : samples) sample.keep = {};
    witness.collectGarbage();
    for (std::size_t i = 0; i < samples.size(); ++i) result[i]["after"] = alive(samples[i]);
    std::cout << JsonValue(result).dump() << '\n';
}
