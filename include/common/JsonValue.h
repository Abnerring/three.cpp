#pragma once

#include <cmath>
#include <iomanip>
#include <locale>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

// Ordinary copies are independent values. sharedCopy() explicitly models a JS object alias.
class JsonValue {
  public:
    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue>;
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

    JsonValue() : value_(std::make_shared<Storage>(nullptr)) {}
    JsonValue(std::nullptr_t) : JsonValue() {}
    JsonValue(bool value) : value_(std::make_shared<Storage>(value)) {}
    template <class T,
              std::enable_if_t<std::is_arithmetic_v<T> && !std::is_same_v<T, bool>, int> = 0>
    JsonValue(T value) : value_(std::make_shared<Storage>(static_cast<double>(value))) {}
    JsonValue(const char* value) : value_(std::make_shared<Storage>(std::string(value))) {}
    JsonValue(std::string value) : value_(std::make_shared<Storage>(std::move(value))) {}
    JsonValue(Array value) : value_(std::make_shared<Storage>(std::move(value))) {}
    JsonValue(Object value) : value_(std::make_shared<Storage>(std::move(value))) {}
    JsonValue(const JsonValue& other) : value_(std::make_shared<Storage>(other.storage())) {}
    JsonValue& operator=(const JsonValue& other) {
        if (this != &other)
            value_ = std::make_shared<Storage>(other.storage());
        return *this;
    }
    JsonValue(JsonValue&&) noexcept = default;
    JsonValue& operator=(JsonValue&&) noexcept = default;

    JsonValue sharedCopy() const { return JsonValue(value_, SharedTag{}); }
    bool isNull() const { return std::holds_alternative<std::nullptr_t>(storage()); }
    bool isObject() const { return std::holds_alternative<Object>(storage()); }
    bool isArray() const { return std::holds_alternative<Array>(storage()); }
    const Storage& storage() const {
        static const Storage nullValue{nullptr};
        return value_ ? *value_ : nullValue;
    }
    template <class T> const T& get() const { return std::get<T>(storage()); }
    template <class T> T& get() { return std::get<T>(mutableStorage()); }
    JsonValue& operator[](const std::string& key) {
        if (isNull())
            mutableStorage() = Object{};
        return get<Object>()[key];
    }
    const JsonValue& at(const std::string& key) const { return get<Object>().at(key); }
    JsonValue& at(const std::string& key) { return get<Object>().at(key); }
    JsonValue& operator[](std::size_t index) { return get<Array>().at(index); }
    const JsonValue& operator[](std::size_t index) const { return get<Array>().at(index); }
    bool contains(const std::string& key) const {
        return isObject() && get<Object>().contains(key);
    }
    std::size_t size() const {
        if (isObject())
            return get<Object>().size();
        if (isArray())
            return get<Array>().size();
        return 0;
    }
    bool operator==(const JsonValue& other) const { return storage() == other.storage(); }
    JsonValue copyJSON() const {
        return std::visit(
            [](const auto& value) -> JsonValue {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, double>) {
                    if (!std::isfinite(value))
                        return nullptr;
                    return value == 0 ? 0.0 : value;
                } else if constexpr (std::is_same_v<T, Array>) {
                    Array result;
                    result.reserve(value.size());
                    for (const auto& element : value)
                        result.push_back(element.copyJSON());
                    return result;
                } else if constexpr (std::is_same_v<T, Object>) {
                    Object result;
                    for (const auto& [key, element] : value)
                        result.emplace(key, element.copyJSON());
                    return result;
                } else
                    return value;
            },
            storage());
    }
    std::string dump() const {
        std::ostringstream stream;
        stream.imbue(std::locale::classic());
        write(stream);
        return stream.str();
    }

  private:
    struct SharedTag {};
    std::shared_ptr<Storage> value_;
    JsonValue(std::shared_ptr<Storage> value, SharedTag) : value_(std::move(value)) {}
    Storage& mutableStorage() {
        if (!value_)
            value_ = std::make_shared<Storage>(nullptr);
        return *value_;
    }
    static void quote(std::ostream& stream, const std::string& value) {
        stream << '"';
        for (unsigned char c : value) {
            switch (c) {
            case '"':
                stream << "\\\"";
                break;
            case '\\':
                stream << "\\\\";
                break;
            case '\b':
                stream << "\\b";
                break;
            case '\f':
                stream << "\\f";
                break;
            case '\n':
                stream << "\\n";
                break;
            case '\r':
                stream << "\\r";
                break;
            case '\t':
                stream << "\\t";
                break;
            default:
                if (c < 0x20)
                    stream << "\\u" << std::hex << std::setw(4) << std::setfill('0') << int(c)
                           << std::dec;
                else
                    stream << c;
            }
        }
        stream << '"';
    }
    void write(std::ostream& stream) const {
        std::visit(
            [&](const auto& value) {
                using T = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<T, std::nullptr_t>)
                    stream << "null";
                else if constexpr (std::is_same_v<T, bool>)
                    stream << (value ? "true" : "false");
                else if constexpr (std::is_same_v<T, double>) {
                    if (value == 0)
                        stream << '0';
                    else if (std::isfinite(value))
                        stream << std::setprecision(17) << value;
                    else
                        stream << "null";
                } else if constexpr (std::is_same_v<T, std::string>)
                    quote(stream, value);
                else if constexpr (std::is_same_v<T, Array>) {
                    stream << '[';
                    bool first = true;
                    for (const auto& element : value) {
                        if (!first)
                            stream << ',';
                        element.write(stream);
                        first = false;
                    }
                    stream << ']';
                } else {
                    stream << '{';
                    bool first = true;
                    for (const auto& [key, element] : value) {
                        if (!first)
                            stream << ',';
                        quote(stream, key);
                        stream << ':';
                        element.write(stream);
                        first = false;
                    }
                    stream << '}';
                }
            },
            storage());
    }
};
