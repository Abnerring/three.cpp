#include "core/JSJSON.h"
#include "core/JSBoolean.h"
#include "core/JSNumber.h"
#include "core/JSProxy.h"
#include "core/JSString.h"
#include "core/JSStringEncoding.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {
using Args = std::vector<JSValue>;
JSValue argument(const Args& args, std::size_t index) { return index < args.size() ? args[index] : JSValue{}; }
JSPropertyDescriptor dataDescriptor(const JSValue& value) {
    JSPropertyDescriptor descriptor;
    descriptor.value = value;
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    return descriptor;
}
std::uint64_t arrayLength(const JSValue& object) {
    const double number = object.get("length").toNumber();
    if (std::isnan(number) || number <= 0) return 0;
    return static_cast<std::uint64_t>(std::min(std::trunc(number), 9007199254740991.0));
}

// Source spans belong to the original UTF-16 text. Duplicate object names keep
// the last parse record, just as the final own property keeps the last value.
struct ParseRecord {
    JSValue value;
    std::size_t begin = 0, end = 0;
    std::vector<std::unique_ptr<ParseRecord>> elements;
    std::unordered_map<std::string, std::unique_ptr<ParseRecord>> entries;

    ParseRecord() = default;
    ParseRecord(const ParseRecord&) = delete;
    ParseRecord& operator=(const ParseRecord&) = delete;
    // Some standard-library unordered_map constructors allocate a sentinel.
    // Finish all empty-container construction before transferring any tree.
    ParseRecord(ParseRecord&& other) : ParseRecord() { swap(other); }
    ParseRecord& operator=(ParseRecord&& other) {
        if (this != &other) { ParseRecord incoming(std::move(other)); swap(incoming); }
        return *this;
    }
    void swap(ParseRecord& other) noexcept {
        using std::swap;
        swap(value, other.value); swap(begin, other.begin); swap(end, other.end);
        elements.swap(other.elements); entries.swap(other.entries);
    }
    ~ParseRecord() {
        // Intrusive destruction worklist: no allocations, no recursive tree
        // teardown, including duplicate-key replacement and parser exceptions.
        ParseRecord* pending = nullptr;
        const auto detach = [&](ParseRecord& record) noexcept {
            const auto enqueue = [&](std::unique_ptr<ParseRecord>& owner) noexcept {
                if (auto* child = owner.release()) {
                    child->destroyNext_ = pending; pending = child;
                }
            };
            for (auto& child : record.elements) enqueue(child);
            for (auto& [key, child] : record.entries) enqueue(child);
            record.elements.clear(); record.entries.clear();
        };
        detach(*this);
        while (pending) {
            auto* current = pending; pending = current->destroyNext_;
            current->destroyNext_ = nullptr;
            detach(*current);
            delete current; // Its destructor sees empty child containers.
        }
    }
  private:
    ParseRecord* destroyNext_ = nullptr;
};
class Parser {
    const std::u16string& source_;
    std::size_t offset_ = 0;
    bool snapshots_;
    [[noreturn]] void fail() const {
        throw JSJSONSyntaxError("Invalid JSON at UTF-16 position " + std::to_string(offset_));
    }
    void whitespace() {
        while (offset_ < source_.size() && (source_[offset_] == u' ' || source_[offset_] == u'\t' ||
               source_[offset_] == u'\n' || source_[offset_] == u'\r')) ++offset_;
    }
    bool consume(char16_t unit) {
        if (offset_ == source_.size() || source_[offset_] != unit) return false;
        ++offset_; return true;
    }
    bool digit() const { return offset_ < source_.size() && source_[offset_] >= u'0' && source_[offset_] <= u'9'; }
    std::string string() {
        if (!consume(u'"')) fail();
        std::u16string units;
        while (offset_ < source_.size()) {
            const auto unit = source_[offset_++];
            if (unit == u'"') return jsUTF8(units);
            if (unit < 0x20) fail();
            if (unit != u'\\') { units.push_back(unit); continue; }
            if (offset_ == source_.size()) fail();
            switch (source_[offset_++]) {
            case u'"': units.push_back(u'"'); break;
            case u'\\': units.push_back(u'\\'); break;
            case u'/': units.push_back(u'/'); break;
            case u'b': units.push_back(u'\b'); break;
            case u'f': units.push_back(u'\f'); break;
            case u'n': units.push_back(u'\n'); break;
            case u'r': units.push_back(u'\r'); break;
            case u't': units.push_back(u'\t'); break;
            case u'u': {
                unsigned value = 0;
                for (unsigned index = 0; index < 4; ++index) {
                    if (offset_ == source_.size()) fail();
                    const auto hex = source_[offset_++]; value <<= 4;
                    if (hex >= u'0' && hex <= u'9') value += hex - u'0';
                    else if (hex >= u'a' && hex <= u'f') value += hex - u'a' + 10;
                    else if (hex >= u'A' && hex <= u'F') value += hex - u'A' + 10;
                    else fail();
                }
                units.push_back(static_cast<char16_t>(value)); break;
            }
            default: fail();
            }
        }
        fail();
    }
    JSValue number() {
        const auto begin = offset_;
        consume(u'-');
        if (!consume(u'0')) {
            if (!digit()) fail();
            while (digit()) ++offset_;
        }
        if (consume(u'.')) { if (!digit()) fail(); while (digit()) ++offset_; }
        if (consume(u'e') || consume(u'E')) {
            if (!consume(u'+')) consume(u'-');
            if (!digit()) fail();
            while (digit()) ++offset_;
        }
        return JSValue(jsUTF8(std::u16string_view(source_).substr(begin, offset_ - begin))).toNumber();
    }
  public:
    Parser(const std::u16string& source, bool snapshots) : source_(source), snapshots_(snapshots) {}
    ParseRecord parse() {
        // Parsing is iterative: deeply nested valid text does not consume one
        // C++ call frame per container. All properties are own data properties.
        struct Frame {
            ParseRecord record;
            bool array, needsEntry = true;
            std::uint64_t index = 0;
            std::string key;
        };
        std::vector<Frame> frames;
        std::optional<ParseRecord> complete;
        const auto start = [&] {
            whitespace(); if (offset_ == source_.size()) fail();
            ParseRecord record; record.begin = offset_;
            if (source_[offset_] == u'"') record.value = string();
            else if (source_[offset_] == u'-' || digit()) record.value = number();
            else if (source_[offset_] == u'[' || source_[offset_] == u'{') {
                const bool array = source_[offset_++] == u'[';
                record.value = array ? JSValue::array() : JSValue::object();
                whitespace();
                if (!consume(array ? u']' : u'}')) {
                    frames.push_back(Frame{std::move(record), array}); return;
                }
            } else if (source_.compare(offset_, 4, u"true") == 0) { offset_ += 4; record.value = true; }
            else if (source_.compare(offset_, 5, u"false") == 0) { offset_ += 5; record.value = false; }
            else if (source_.compare(offset_, 4, u"null") == 0) { offset_ += 4; record.value = nullptr; }
            else fail();
            record.end = offset_; complete.emplace(std::move(record));
        };
        start();
        for (;;) {
            if (complete) {
                if (frames.empty()) {
                    whitespace(); if (offset_ != source_.size()) fail();
                    return std::move(*complete);
                }
                auto& parent = frames.back();
                const auto key = parent.array ? std::to_string(parent.index++) : parent.key;
                parent.record.value.defineProperty(key, dataDescriptor(complete->value));
                if (snapshots_) {
                    auto child = std::make_unique<ParseRecord>(std::move(*complete));
                    if (parent.array) parent.record.elements.push_back(std::move(child));
                    else parent.record.entries.insert_or_assign(key, std::move(child));
                }
                complete.reset(); parent.needsEntry = false;
            } else if (frames.back().needsEntry) {
                if (!frames.back().array) {
                    whitespace(); frames.back().key = string(); whitespace(); if (!consume(u':')) fail();
                }
                start();
            } else {
                whitespace();
                if (consume(frames.back().array ? u']' : u'}')) {
                    frames.back().record.end = offset_;
                    complete.emplace(std::move(frames.back().record)); frames.pop_back();
                } else {
                    if (!consume(u',')) fail();
                    frames.back().needsEntry = true;
                }
            }
        }
    }
};

JSValue revive(JSValue holder, const std::string& key, const JSValue& callback,
               const ParseRecord* record, const std::u16string& source) {
    const auto value = holder.get(key);
    auto context = JSValue::object();
    if (record && !record->value.sameValue(value)) record = nullptr;
    if (record && !value.isObject())
        context.defineProperty("source", dataDescriptor(jsUTF8(std::u16string_view(source).substr(record->begin, record->end - record->begin))));
    if (value.isObject()) {
        auto object = value;
        const auto apply = [&](const std::string& name, const ParseRecord* child) {
            const auto replacement = revive(object, name, callback, child, source);
            if (replacement.isUndefined()) static_cast<void>(object.erase(name));
            else static_cast<void>(object.defineOwnProperty(name, dataDescriptor(replacement)));
        };
        if (isNativeArray(value)) {
            const auto length = arrayLength(value);
            for (std::uint64_t index = 0; index < length; ++index)
                apply(std::to_string(index), record && index < record->elements.size() ? record->elements[index].get() : nullptr);
        } else {
            // Enumeration is completed before the first child callback.
            const auto keys = value.keys();
            for (const auto& name : keys) {
                const ParseRecord* child = nullptr;
                if (record) if (const auto found = record->entries.find(name); found != record->entries.end()) child = found->second.get();
                apply(name, child);
            }
        }
    }
    return callback.call(holder, {key, value, context});
}

std::string quote(const std::string& input) {
    const auto units = jsUTF16(input);
    std::u16string output = u"\"";
    constexpr char16_t hex[] = u"0123456789abcdef";
    for (std::size_t index = 0; index < units.size(); ++index) {
        const auto unit = units[index];
        switch (unit) {
        case u'"': output += u"\\\""; continue;
        case u'\\': output += u"\\\\"; continue;
        case u'\b': output += u"\\b"; continue;
        case u'\f': output += u"\\f"; continue;
        case u'\n': output += u"\\n"; continue;
        case u'\r': output += u"\\r"; continue;
        case u'\t': output += u"\\t"; continue;
        default: break;
        }
        if (unit >= 0xd800 && unit <= 0xdbff && index + 1 < units.size() && units[index + 1] >= 0xdc00 && units[index + 1] <= 0xdfff) {
            output.push_back(unit); output.push_back(units[++index]);
        } else if (unit < 0x20 || (unit >= 0xd800 && unit <= 0xdfff)) {
            output += u"\\u"; for (int shift = 12; shift >= 0; shift -= 4) output.push_back(hex[(unit >> shift) & 15]);
        } else output.push_back(unit);
    }
    output.push_back(u'"'); return jsUTF8(output);
}
struct Stringifier {
    JSValue replacer;
    std::optional<std::vector<std::string>> properties;
    std::vector<JSValue> stack;
    std::string gap, indent;

    Stringifier(const JSValue& candidate, JSValue space) {
        if (candidate.isObject()) {
            // Match the pinned Node 26/V8 observable initialization order.
            // TC39 checks IsCallable first; V8 checks IsArray before space coercion.
            const bool array = isNativeArray(candidate);
            if (candidate.isCallable()) replacer = candidate;
            else if (array) {
                properties.emplace(); std::unordered_set<std::string> seen;
                const auto length = arrayLength(candidate);
                for (std::uint64_t index = 0; index < length; ++index) {
                    const auto value = candidate.get(std::to_string(index));
                    if (value.type() == JSValue::Type::String || value.type() == JSValue::Type::Number ||
                        isNativeStringObject(value) || isNativeNumberObject(value)) {
                        const auto name = value.toString();
                        if (seen.insert(name).second) properties->push_back(name);
                    }
                }
            }
        }
        if (isNativeNumberObject(space)) space = space.toNumber();
        else if (isNativeStringObject(space)) space = space.toString();
        if (space.type() == JSValue::Type::Number) {
            const auto number = space.number();
            if (!std::isnan(number) && number >= 1) gap.assign(static_cast<std::size_t>(std::min(10.0, std::trunc(number))), ' ');
        } else if (space.type() == JSValue::Type::String) {
            const auto units = jsUTF16(space.string()); gap = jsUTF8(std::u16string_view(units).substr(0, 10));
        }
    }
    std::optional<std::string> property(const JSValue& holder, const std::string& key) {
        auto value = holder.get(key);
        if (value.isObject()) {
            const auto method = value.get("toJSON");
            if (method.isCallable()) value = method.call(value, {key});
        }
        if (replacer.isCallable()) value = replacer.call(holder, {key, value});
        if (value.isObject()) {
            if (nativeJSONIsRawJSON(value)) return value.get("rawJSON").string();
            if (isNativeNumberObject(value)) value = value.toNumber();
            else if (isNativeStringObject(value)) value = value.toString();
            else if (isNativeBooleanObject(value)) value = nativeBooleanValue(value);
        }
        switch (value.type()) {
        case JSValue::Type::Undefined: case JSValue::Type::Function: case JSValue::Type::Symbol: return std::nullopt;
        case JSValue::Type::Null: return "null";
        case JSValue::Type::Boolean: return value.toBoolean() ? "true" : "false";
        case JSValue::Type::Number: return std::isfinite(value.number()) ? value.toString() : "null";
        case JSValue::Type::String: return quote(value.string());
        default: break;
        }
        if (std::any_of(stack.begin(), stack.end(), [&](const auto& prior) { return prior.sameValue(value); }))
            throw JSTypeError("Converting circular structure to JSON");
        stack.push_back(value); const auto previous = indent; indent += gap;
        const bool array = isNativeArray(value);
        std::vector<std::string> pieces;
        if (array) {
            const auto length = arrayLength(value);
            for (std::uint64_t index = 0; index < length; ++index) pieces.push_back(property(value, std::to_string(index)).value_or("null"));
        } else {
            const auto names = properties ? *properties : value.keys();
            for (const auto& name : names) if (auto result = property(value, name))
                pieces.push_back(quote(name) + (gap.empty() ? ":" : ": ") + *result);
        }
        std::string output(1, array ? '[' : '{');
        if (!pieces.empty()) {
            if (!gap.empty()) output += '\n' + indent;
            for (std::size_t index = 0; index < pieces.size(); ++index) {
                if (index) { output += ','; if (!gap.empty()) output += '\n' + indent; }
                output += pieces[index];
            }
            if (!gap.empty()) output += '\n' + previous;
        }
        output += array ? ']' : '}'; stack.pop_back(); indent = previous; return output;
    }
};
}

JSValue nativeJSONParse(JSValue text, JSValue reviver) {
    const auto source = jsUTF16(text.toString());
    auto parsed = Parser(source, reviver.isCallable()).parse();
    if (!reviver.isCallable()) return parsed.value;
    auto holder = JSValue::object(); holder.defineProperty("", dataDescriptor(parsed.value));
    return revive(holder, "", reviver, &parsed, source);
}
JSValue nativeJSONStringify(JSValue value, JSValue replacer, JSValue space) {
    Stringifier serializer(replacer, space);
    auto holder = JSValue::object(); holder.defineProperty("", dataDescriptor(value));
    const auto result = serializer.property(holder, "");
    return result ? JSValue(*result) : JSValue{};
}
bool nativeJSONIsRawJSON(const JSValue& value) {
    return value.isObject() && !value.isProxy() && value.getInternalSlot("JSON.IsRawJSON").strictEquals(true);
}
JSValue nativeJSONRawJSON(JSValue text) {
    const auto value = text.toString(); const auto source = jsUTF16(value);
    if (source.empty() || source.front() == u' ' || source.front() == u'\t' || source.front() == u'\r' || source.front() == u'\n' ||
        source.back() == u' ' || source.back() == u'\t' || source.back() == u'\r' || source.back() == u'\n')
        throw JSJSONSyntaxError("Raw JSON requires a primitive without surrounding whitespace");
    const auto parsed = Parser(source, false).parse();
    if (parsed.value.isObject()) throw JSJSONSyntaxError("Raw JSON must be a primitive");
    auto result = JSValue::object(nullptr); result.setInternalSlot("JSON.IsRawJSON", true);
    JSPropertyDescriptor descriptor; descriptor.value = value; descriptor.enumerable = true;
    result.defineProperty("rawJSON", descriptor); result.preventExtensions(); return result;
}
JSValue nativeJSONObject() {
    static thread_local JSWeakValue weak;
    if (const auto cached = weak.lock(); cached && cached->isObject()) return *cached;
    auto object = JSValue::object();
    const auto install = [&](const char* name, std::uint32_t length, JSValue::NativeFunction callback) {
        JSPropertyDescriptor descriptor; descriptor.value = JSValue::function(callback, {}, name, length);
        descriptor.writable = descriptor.configurable = true; object.defineProperty(name, descriptor);
    };
    install("parse", 2, +[](const JSValue&, const Args& args, const Args&) { return nativeJSONParse(argument(args, 0), argument(args, 1)); });
    install("stringify", 3, +[](const JSValue&, const Args& args, const Args&) { return nativeJSONStringify(argument(args, 0), argument(args, 1), argument(args, 2)); });
    install("rawJSON", 1, +[](const JSValue&, const Args& args, const Args&) { return nativeJSONRawJSON(argument(args, 0)); });
    install("isRawJSON", 1, +[](const JSValue&, const Args& args, const Args&) -> JSValue { return nativeJSONIsRawJSON(argument(args, 0)); });
    JSPropertyDescriptor tag; tag.value = "JSON"; tag.configurable = true;
    object.defineProperty(JSValue::wellKnownSymbol("toStringTag"), tag);
    object.retainForHeapLifetime(); weak = JSWeakValue(object); return object;
}
