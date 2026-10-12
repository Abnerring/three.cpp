#include "core/JSDataSnapshot.h"
#include <algorithm>
#include <functional>

std::optional<JsonValue> snapshotData(const JSValue& input) {
    std::vector<JSValue> active;
    std::function<std::optional<JsonValue>(JSValue)> copy = [&](JSValue value) -> std::optional<JsonValue> {
        switch (value.type()) {
        case JSValue::Type::Undefined: case JSValue::Type::Function: return std::nullopt;
        case JSValue::Type::Null: return JsonValue(nullptr);
        case JSValue::Type::Boolean: return JsonValue(value.toBoolean());
        case JSValue::Type::Number: return JsonValue(value.number());
        case JSValue::Type::String: return JsonValue(value.string());
        default: break;
        }
        if (std::any_of(active.begin(), active.end(), [&](const JSValue& ancestor) { return ancestor.sameValue(value); }))
            throw JSTypeError("Circular structure in native data snapshot");
        active.push_back(value);
        JsonValue result;
        if (value.type() == JSValue::Type::Array) {
            JsonValue::Array entries;
            const auto length=value.length(); entries.reserve(length);
            for (std::uint32_t i=0; i<length; ++i)
                entries.push_back(copy(value.get(std::to_string(i))).value_or(JsonValue(nullptr)));
            result=std::move(entries);
        } else {
            JsonValue::Object entries;
            for (const auto& name : value.keys())
                if (auto entry=copy(value.get(name))) entries[name]=std::move(*entry);
            result=std::move(entries);
        }
        active.pop_back(); return result;
    };
    return copy(input);
}
