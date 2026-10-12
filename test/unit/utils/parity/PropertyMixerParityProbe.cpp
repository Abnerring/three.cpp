#include "animation/PropertyMixer.h"
#include "math/JSNumberMath.h"
#include <bit>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>

using A = AnimationArray;
namespace {
JSValue decode(JSValue value) {
    if (value.isObject() && value.hasOwn("$undefined")) return {};
    if (value.isObject() && value.hasOwn("$number")) {
        const auto name = value.get("$number").string();
        if (name == "NaN") return std::numeric_limits<double>::quiet_NaN();
        if (name == "Infinity") return std::numeric_limits<double>::infinity();
        if (name == "-Infinity") return -std::numeric_limits<double>::infinity();
        return -0.;
    }
    if (value.isObject()) for (const auto& key : value.keys()) value.set(key, decode(value.get(key)));
    return value;
}
JsonValue encode(const JSValue& value) {
    if (value.isUndefined()) return "$undefined";
    if (value.type() == JSValue::Type::Number) {
        const auto n = value.number();
        if (std::isnan(n)) return "$NaN";
        if (std::isinf(n)) return n < 0 ? "$-Infinity" : "$Infinity";
        if (n == 0 && std::signbit(n)) return "$-0";
        return n;
    }
    if (value.type() == JSValue::Type::Array) {
        JsonValue::Array result;
        for (std::uint32_t i = 0; i < value.length(); ++i) result.push_back(encode(value.get(std::to_string(i))));
        return result;
    }
    if (value.isObject()) {
        JsonValue::Object result;
        for (const auto& key : value.keys()) result.emplace(key, encode(value.get(key)));
        return result;
    }
    return value.toJSON().value();
}
JsonValue dump(A array) {
    JsonValue::Array values, holes;
    for (std::size_t i = 0; i < array.size(); ++i) {
        values.push_back(encode(array.get(double(i))));
        if (const auto ordinary = std::get_if<JSValue>(&array.storage()); ordinary && !ordinary->hasOwn(std::to_string(i))) holes.push_back(i);
    }
    JsonValue::Object extras;
    if (const auto ordinary = std::get_if<JSValue>(&array.storage())) for (const auto& key : ordinary->keys()) {
        const auto numeric = JSValue(key).toNumber();
        if (!(numeric >= 0 && numeric < double(array.size()) && std::floor(numeric) == numeric && JSValue(numeric).toString() == key))
            extras.emplace(key, encode(ordinary->get(key)));
    }
    return JsonValue::Object{{"values", values}, {"holes", holes}, {"extras", extras}};
}
void assign(A array, JSValue values, double offset = 0) {
    for (std::uint32_t i = 0; i < values.length(); ++i) array.set(offset + i, values.get(std::to_string(i)));
}
struct Binding : PropertyBindingInterface {
    JSValue value;
    PropertyMixer* mixer = nullptr;
    JsonValue::Array events;
    std::function<void(A, double)> getHook, setHook;
    explicit Binding(JSValue original) : value(original) {}
    void event(const char* name, A array, double offset) {
        events.emplace_back(JsonValue::Array{name, encode(offset), encode(mixer->cumulativeWeight),
            encode(mixer->cumulativeWeightAdditive), array.sameIdentity(mixer->buffer)});
    }
    void getValue(A array, double offset) override {
        event("get", array, offset); assign(array, value, offset);
        const auto callback = getHook; if (callback) callback(array, offset);
    }
    void setValue(A array, double offset) override {
        event("set", array, offset);
        for (std::uint32_t i = 0, n = value.length(); i < n; ++i) value.set(std::to_string(i), array.get(offset + i));
        const auto callback = setHook; if (callback) callback(array, offset);
    }
};
JsonValue state(PropertyMixer& mixer, Binding& binding) {
    return JsonValue::Object{{"buffer", dump(mixer.buffer)}, {"kind", std::holds_alternative<JSValue>(mixer.buffer.storage()) ? "array" : std::holds_alternative<Float32Array>(mixer.buffer.storage()) ? "float32" : "float64"},
        {"size", encode(mixer.valueSize)}, {"weight", encode(mixer.cumulativeWeight)}, {"additive", encode(mixer.cumulativeWeightAdditive)},
        {"work", encode(mixer._workIndex)}, {"original", mixer._origIndex}, {"addIndex", mixer._addIndex},
        {"useCount", mixer.useCount}, {"referenceCount", mixer.referenceCount}, {"value", encode(binding.value)}, {"events", binding.events}};
}
JsonValue normal(JSValue request) {
    auto binding = std::make_shared<Binding>(request.hasOwn("original") ? request.get("original") : JSValue::array());
    PropertyMixer mixer(binding, request.get("type"), request.get("size").toNumber()); binding->mixer = &mixer;
    if (request.hasOwn("bufferKind")) {
        auto values = request.get("initialBuffer");
        if (request.get("bufferKind").string() == "array") mixer.buffer = values;
        else mixer.buffer = convertArray(values, request.get("bufferKind").string() == "float32" ? AnimationArrayType::Float32 : AnimationArrayType::Float64);
    }
    if (request.get("nullBinding").toBoolean()) mixer.binding.reset();
    JsonValue::Array states{state(mixer, *binding)};
    auto actions = request.get("actions");
    if (!actions.isUndefined()) for (std::uint32_t i = 0; i < actions.length(); ++i) {
        const auto action = actions.get(std::to_string(i)); const auto op = action.get("op").string();
        if (action.hasOwn("incoming")) assign(mixer.buffer, action.get("incoming"));
        if (op == "save") mixer.saveOriginalState();
        else if (op == "restore") mixer.restoreOriginalState();
        else if (op == "accumulate") mixer.accumulate(action.get("index"), action.get("weight"));
        else if (op == "add") mixer.accumulateAdditive(action.get("weight"));
        else if (op == "apply") mixer.apply(action.get("index"));
        else if (op == "set") {
            if (action.hasOwn("weight")) mixer.cumulativeWeight = action.get("weight");
            if (action.hasOwn("additive")) mixer.cumulativeWeightAdditive = action.get("additive");
            if (action.hasOwn("size")) mixer.valueSize = action.get("size").toNumber();
            if (action.hasOwn("origIndex")) mixer._origIndex = action.get("origIndex").toNumber();
            if (action.hasOwn("addIndex")) mixer._addIndex = action.get("addIndex").toNumber();
            if (action.hasOwn("workIndex")) mixer._workIndex = action.get("workIndex");
        } else {
            const auto d = action.get("destination").toNumber(), s = action.get("source").toNumber();
            const auto t = action.get("weight");
            const auto n = action.hasOwn("stride") ? action.get("stride").toNumber() : mixer.valueSize;
            if (op == "select") mixer._select(mixer.buffer, d, s, t, n);
            else if (op == "lerp") mixer._lerp(mixer.buffer, d, s, t, n);
            else if (op == "lerpAdditive") mixer._lerpAdditive(mixer.buffer, d, s, t, n);
            else if (op == "slerp") mixer._slerp(mixer.buffer, d, s, t);
            else if (op == "slerpAdditive") mixer._slerpAdditive(mixer.buffer, d, s, t, n);
            else if (op == "numericIdentity") mixer._setAdditiveIdentityNumeric();
            else if (op == "quaternionIdentity") mixer._setAdditiveIdentityQuaternion();
            else if (op == "otherIdentity") mixer._setAdditiveIdentityOther();
        }
        states.push_back(state(mixer, *binding));
    }
    return states;
}
PropertyMixer* activeMixer = nullptr;
void log(JSValue trace, JSValue value) { trace.set(std::to_string(trace.length()), value); }
JSValue weight(JSValue trace, const std::string& label, double value) {
    auto result = JSValue::object();
    result.set("valueOf", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
        log(c[0], c[1]); return c[2];
    }, {trace, label, value}));
    return result;
}
JsonValue callbacks(const std::string& kind) {
    auto binding = std::make_shared<Binding>(JSValue::fromJSON(JsonValue::Array{10}));
    PropertyMixer mixer(binding, "number", 1); binding->mixer = &mixer; activeMixer = &mixer;
    struct Clear { ~Clear() { activeMixer = nullptr; } } clear;
    auto trace = JSValue::array();
    const auto old = mixer.buffer;
    if (kind == "save") {
        auto replacement = std::make_shared<Binding>(JSValue::fromJSON(JsonValue::Array{90})); replacement->mixer = &mixer;
        binding->getHook = [&](A, double) { mixer.buffer = Float64Array({1,2,3,4,5}); mixer.binding = replacement; mixer.cumulativeWeight = 9; mixer.valueSize = 1; };
        mixer.saveOriginalState(); mixer.restoreOriginalState();
        return JsonValue::Object{{"state", state(mixer, *binding)}, {"old", dump(old)}, {"replacement", encode(replacement->value)}, {"replacementEvents", replacement->events}};
    }
    mixer.saveOriginalState(); binding->events.clear();
    if (kind == "mutable") {
        PropertyMixer other(nullptr, "number", 1);
        const bool sharedBuiltin = mixer._mixBufferRegion.sameIdentity(other._mixBufferRegion);
        mixer._mixBufferRegion = [trace, count = 0](PropertyMixer& m, A, double, double, JSValue, double) mutable {
            log(trace, ++count);
            if (count == 2) m._mixBufferRegion = [trace](PropertyMixer&, A, double, double, JSValue, double) { log(trace, 99); };
            log(trace, -count);
        };
        const auto copied = mixer._mixBufferRegion;
        mixer.cumulativeWeight = 1;
        for (int i = 0; i < 3; ++i) mixer.accumulate(0, 1);
        copied(mixer, mixer.buffer, 1, 0, .5, 1);
        mixer._setIdentity = [trace, count = 0](PropertyMixer& m) mutable {
            log(trace, 10 + ++count);
            if (count == 2) m._setIdentity = [trace](PropertyMixer&) { log(trace, 199); };
            log(trace, -10 - count);
        };
        for (int i = 0; i < 3; ++i) { mixer.cumulativeWeightAdditive = 0; mixer.accumulateAdditive(1); }
        return JsonValue::Object{{"state", state(mixer, *binding)}, {"trace", encode(trace)},
            {"sharedBuiltin", sharedBuiltin}, {"sameFunction", copied.sameIdentity(mixer._mixBufferRegion)}};
    }
    if (kind == "applyBinding") {
        auto replacement = std::make_shared<Binding>(JSValue::fromJSON(JsonValue::Array{90})); replacement->mixer = &mixer;
        mixer.buffer.set(0, 2); mixer.accumulate(0, .25);
        const auto originalMix = mixer._mixBufferRegion;
        mixer._mixBufferRegion = [&, originalMix](PropertyMixer& m, A b, double d, double s, JSValue t, double n) {
            log(trace, "mix"); m.binding = replacement; m.buffer = Float64Array({1,2,3,4,5}); originalMix(m,b,d,s,t,n);
        };
        binding->setHook = [&](A, double) { log(trace, mixer.cumulativeWeight); mixer.buffer.set(0, 8); mixer.accumulate(1, .75); };
        mixer.apply(0);
        return JsonValue::Object{{"state", state(mixer, *binding)}, {"old", dump(old)}, {"trace", encode(trace)}, {"replacementEvents", replacement->events}};
    }
    if (kind == "identity") {
        const auto originalIdentity = mixer._setIdentity;
        mixer._setIdentity = [&, originalIdentity](PropertyMixer& m) {
            log(trace, "identity"); m.buffer = Float64Array({3,4,5,6,7}); m.valueSize = 2; m.cumulativeWeightAdditive = 5; originalIdentity(m);
        };
        mixer.buffer.set(0, 2); mixer.accumulateAdditive(.5);
        return JsonValue::Object{{"state", state(mixer, *binding)}, {"old", dump(old)}, {"trace", encode(trace)}};
    }
    if (kind == "weights") {
        mixer.buffer.set(0, 2); mixer.accumulate(0, weight(trace,"first",.25));
        mixer.buffer.set(0, 6); mixer.accumulate(0, weight(trace,"second",.5));
        mixer.apply(0);
        mixer.buffer.set(0, 3); mixer.accumulateAdditive(weight(trace,"add",.25)); mixer.apply(1);
        return JsonValue::Object{{"state", state(mixer, *binding)}, {"trace", encode(trace)}};
    }
    if (kind == "index") {
        auto index = JSValue::object();
        index.set("valueOf", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            log(c[0], "index"); activeMixer->buffer = Float64Array({3,4,5,6,7}); activeMixer->valueSize = 2; return 0;
        }, {trace}));
        mixer.buffer.set(0, 2); mixer.accumulate(index, .5);
        return JsonValue::Object{{"state", state(mixer, *binding)}, {"old", dump(old)}, {"trace", encode(trace)}};
    }
    if (kind == "applyWeight") {
        auto value = weight(trace, "weight", .5);
        value.set("valueOf", JSValue::function([](const JSValue&, const auto&, const auto& c) -> JSValue {
            log(c[0], activeMixer->cumulativeWeight); activeMixer->cumulativeWeight = 9; return .5;
        }, {trace}));
        mixer.buffer.set(0, 2); mixer.accumulate(0, value); mixer.apply(0);
        return JsonValue::Object{{"state", state(mixer, *binding)}, {"trace", encode(trace)}};
    }
    throw std::runtime_error("Unknown callback case");
}
JsonValue mathCase(JSValue values) {
    const auto bits = [](double value) {
        if (std::isnan(value)) return std::string("$NaN");
        std::ostringstream result; result << std::hex << std::setfill('0') << std::setw(16) << std::bit_cast<std::uint64_t>(value);
        return result.str();
    };
    JsonValue::Array result;
    for (std::uint32_t i = 0; i < values.length(); ++i) {
        const auto value = values.get(std::to_string(i)).toNumber();
        result.emplace_back(JsonValue::Array{bits(JSNumberMath::sin(value)), bits(JSNumberMath::acos(value))});
    }
    return result;
}
}
int main() {
    const std::string text(std::istreambuf_iterator<char>(std::cin), {});
    const auto requests = decode(Animation_detail::parseJSON(text));
    JsonValue::Array result;
    for (std::uint32_t i = 0; i < requests.length(); ++i) {
        try {
            const auto request = requests.get(std::to_string(i));
            result.push_back(request.hasOwn("math") ? mathCase(request.get("math")) : request.hasOwn("callback") ? callbacks(request.get("callback").string()) : normal(request));
        }
        catch (const JSTypeError&) { result.emplace_back(JsonValue::Object{{"error","TypeError"}}); }
        catch (const JSRangeError&) { result.emplace_back(JsonValue::Object{{"error","RangeError"}}); }
        catch (const std::exception&) { result.emplace_back(JsonValue::Object{{"error","Error"}}); }
    }
    std::cout << JsonValue(result).dump() << '\n';
}
