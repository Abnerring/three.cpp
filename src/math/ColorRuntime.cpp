#include "math/Color.h"
#include "core/JSModuleObject.h"
#include "core/JSRegExp.h"
#include "core/JSStringEncoding.h"
#include "utils.h"
#include <cmath>
#include <limits>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, unsigned i) { return i < args.size() ? args[i] : JSValue{}; }
JSPropertyDescriptor data(JSValue value) { JSPropertyDescriptor d; d.value = value; d.writable = d.enumerable = d.configurable = true; return d; }
JSValue add(const JSValue& a, const JSValue& b) {
    const auto left = a.toPrimitive(); const auto right = b.toPrimitive();
    if (left.type() == JSValue::Type::String || right.type() == JSValue::Type::String) return left.toString() + right.toString();
    return left.toNumber() + right.toNumber();
}
double product(const JSValue& a, const JSValue& b) { const auto left = a.toNumber(); const auto right = b.toNumber(); return left * right; }
double minimum(double a, double b) {
    if (std::isnan(a) || std::isnan(b)) return std::numeric_limits<double>::quiet_NaN();
    if (a == 0 && b == 0) return std::signbit(a) || std::signbit(b) ? -0.0 : 0.0;
    return a < b ? a : b;
}
double clamp(const JSValue& value) {
    const auto n = minimum(1, value.toNumber());
    if (std::isnan(n)) return n;
    return n <= 0 ? 0.0 : n;
}
std::uint32_t uint32(double value) {
    if (!std::isfinite(value) || value == 0) return 0;
    const auto remainder = std::fmod(std::trunc(value), 4294967296.0);
    return static_cast<std::uint32_t>(remainder < 0 ? remainder + 4294967296.0 : remainder);
}
JSValue exec(const char* pattern, const JSValue& value) {
    const auto regexp = createNativeRegExp(pattern); const auto fn = regexp.get("exec"); return fn.call(regexp, {value});
}
std::string numberText(const JSValue& value) {
    const auto text = jsUTF16(value.toString()); std::size_t start = 0;
    while (start < text.size() && jsWhitespace(text[start])) ++start;
    return jsUTF8(std::u16string_view(text).substr(start));
}
double parseFloat(const JSValue& value) {
    const auto text = numberText(value); std::size_t index = 0;
    if (text.empty()) return std::numeric_limits<double>::quiet_NaN();
    if (text[index] == '+' || text[index] == '-') ++index;
    if (text.compare(index, 8, "Infinity") == 0) return text[0] == '-' ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    bool digits = false;
    while (index < text.size() && text[index] >= '0' && text[index] <= '9') { ++index; digits = true; }
    if (index < text.size() && text[index] == '.') {
        ++index; while (index < text.size() && text[index] >= '0' && text[index] <= '9') { ++index; digits = true; }
    }
    if (!digits) return std::numeric_limits<double>::quiet_NaN();
    if (index < text.size() && (text[index] == 'e' || text[index] == 'E')) {
        const auto exponent = index++; if (index < text.size() && (text[index] == '+' || text[index] == '-')) ++index;
        const auto start = index; while (index < text.size() && text[index] >= '0' && text[index] <= '9') ++index;
        if (start == index) index = exponent;
    }
    return JSValue(text.substr(0, index)).toNumber();
}
double parseInt(const JSValue& value, unsigned radix) {
    const auto text = numberText(value); std::size_t index = 0; bool negative = false;
    if (index < text.size() && (text[index] == '+' || text[index] == '-')) { negative = text[index] == '-'; ++index; }
    if (radix == 16 && index + 1 < text.size() && text[index] == '0' && (text[index + 1] == 'x' || text[index + 1] == 'X')) index += 2;
    const auto start = index;
    while (index < text.size()) {
        const auto c = text[index]; const unsigned digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : 16;
        if (digit >= radix) break; ++index;
    }
    if (start == index) return std::numeric_limits<double>::quiet_NaN();
    const auto result = JSValue((radix == 16 ? "0x" : "") + text.substr(start, index - start)).toNumber();
    return negative ? -result : result;
}
void convert(const JSValue& self, const JSValue& space) {
    const auto management = nativeColorManagement(); const auto fn = management.get("colorSpaceToWorking"); fn.call(management, {self, space});
}
void alpha(const JSValue& value, const JSValue& style) {
    if (value.isUndefined()) return;
    if (parseFloat(value) < 1) ThreeUtils::warn({add(add("Color: Alpha component of ", style), " will be ignored.")});
}
double hue(double p, double q, double t) {
    if (t < 0) t += 1;
    if (t > 1) t -= 1;
    if (t < 1.0 / 6.0) return p + (q - p) * 6 * t;
    if (t < 1.0 / 2.0) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * 6 * (2.0 / 3.0 - t);
    return p;
}
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string(); const auto first = arg(args, 0);
    if (name == "set") {
        if (arg(args, 1).isUndefined() && arg(args, 2).isUndefined()) {
            if (first.toBoolean() && first.get("isColor").toBoolean()) { const auto fn = self.get("copy"); fn.call(self, {first}); }
            else if (first.type() == JSValue::Type::Number) { const auto fn = self.get("setHex"); fn.call(self, {first}); }
            else if (first.type() == JSValue::Type::String) { const auto fn = self.get("setStyle"); fn.call(self, {first}); }
        } else { const auto fn = self.get("setRGB"); fn.call(self, {first, arg(args, 1), arg(args, 2)}); }
        return self;
    }
    if (name == "setScalar" || name == "copy") {
        for (const auto* field : {"r", "g", "b"}) self.set(field, name == "copy" ? first.get(field) : first);
        return self;
    }
    if (name == "clone") {
        const auto ctor = self.get("constructor"); const auto r = self.get("r"), g = self.get("g"), b = self.get("b");
        return ctor.construct({r, g, b});
    }
    if (name == "setRGB" || name == "setHSL") {
        auto space = arg(args, 3); if (space.isUndefined()) space = nativeColorManagement().get("workingColorSpace");
        if (name == "setRGB") {
            self.set("r", first); self.set("g", arg(args, 1)); self.set("b", arg(args, 2));
        } else {
            const auto h = std::fmod(std::fmod(first.toNumber(), 1) + 1, 1);
            const auto s = clamp(arg(args, 1)), l = clamp(arg(args, 2));
            if (s == 0) { self.set("b", l); self.set("g", l); self.set("r", l); }
            else {
                const auto p = l <= 0.5 ? l * (1 + s) : l + s - (l * s), q = (2 * l) - p;
                self.set("r", hue(q, p, h + 1.0 / 3.0)); self.set("g", hue(q, p, h)); self.set("b", hue(q, p, h - 1.0 / 3.0));
            }
        }
        convert(self, space); return self;
    }
    if (name == "setHex") {
        const auto rawSpace = arg(args, 1), space = rawSpace.isUndefined() ? JSValue(SRGBColorSpace) : rawSpace;
        const auto value = uint32(std::floor(first.toNumber()));
        self.set("r", ((value >> 16) & 255) / 255.0); self.set("g", ((value >> 8) & 255) / 255.0); self.set("b", (value & 255) / 255.0);
        convert(self, space); return self;
    }
    if (name == "setColorName") {
        const auto rawSpace = arg(args, 1), space = rawSpace.isUndefined() ? JSValue(SRGBColorSpace) : rawSpace;
        const auto lower = first.get("toLowerCase").call(first); const auto hex = captures[1].get(lower);
        if (!hex.isUndefined()) { const auto fn = self.get("setHex"); fn.call(self, {hex, space}); }
        else ThreeUtils::warn({add("Color: Unknown color ", first)});
        return self;
    }
    if (name == "setStyle") {
        const auto rawSpace = arg(args, 1), space = rawSpace.isUndefined() ? JSValue(SRGBColorSpace) : rawSpace;
        const auto model = exec("^(\\w+)\\(([^\\)]*)\\)", first);
        if (model.toBoolean()) {
            const auto modelName = model.get("1"), components = model.get("2"); JSValue color;
            if (modelName.strictEquals("rgb") || modelName.strictEquals("rgba")) {
                color = exec("^\\s*(\\d+)\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)\\s*(?:,\\s*(\\d*\\.?\\d+)\\s*)?$", components);
                if (color.toBoolean()) {
                    alpha(color.get("4"), first); const auto fn = self.get("setRGB");
                    const auto r = minimum(255, parseInt(color.get("1"), 10)) / 255, g = minimum(255, parseInt(color.get("2"), 10)) / 255, b = minimum(255, parseInt(color.get("3"), 10)) / 255;
                    return fn.call(self, {r, g, b, space});
                }
                color = exec("^\\s*(\\d*\\.?\\d+)\\%\\s*,\\s*(\\d*\\.?\\d+)\\%\\s*,\\s*(\\d*\\.?\\d+)\\%\\s*(?:,\\s*(\\d*\\.?\\d+)\\s*)?$", components);
                if (color.toBoolean()) {
                    alpha(color.get("4"), first); const auto fn = self.get("setRGB");
                    const auto r = minimum(100, parseFloat(color.get("1"))) / 100, g = minimum(100, parseFloat(color.get("2"))) / 100, b = minimum(100, parseFloat(color.get("3"))) / 100;
                    return fn.call(self, {r, g, b, space});
                }
            } else if (modelName.strictEquals("hsl") || modelName.strictEquals("hsla")) {
                color = exec("^\\s*([+-]?\\d*\\.?\\d+)\\s*,\\s*(\\d*\\.?\\d+)\\%\\s*,\\s*(\\d*\\.?\\d+)\\%\\s*(?:,\\s*(\\d*\\.?\\d+)\\s*)?$", components);
                if (color.toBoolean()) {
                    alpha(color.get("4"), first); const auto fn = self.get("setHSL");
                    const auto h = parseFloat(color.get("1")) / 360, s = parseFloat(color.get("2")) / 100, l = parseFloat(color.get("3")) / 100;
                    return fn.call(self, {h, s, l, space});
                }
            } else ThreeUtils::warn({add("Color: Unknown color model ", first)});
        } else {
            const auto match = exec("^\\#([A-Fa-f\\d]+)$", first);
            if (match.toBoolean()) {
                const auto hex = match.get("1"), size = hex.get("length");
                if (size.strictEquals(3)) {
                    const auto fn = self.get("setRGB");
                    const auto r = parseInt(hex.get("charAt").call(hex, {0}), 16) / 15;
                    const auto g = parseInt(hex.get("charAt").call(hex, {1}), 16) / 15;
                    const auto b = parseInt(hex.get("charAt").call(hex, {2}), 16) / 15;
                    return fn.call(self, {r, g, b, space});
                }
                if (size.strictEquals(6)) { const auto fn = self.get("setHex"); return fn.call(self, {parseInt(hex, 16), space}); }
                ThreeUtils::warn({add("Color: Invalid hex color ", first)});
            } else if (first.toBoolean() && first.get("length").toNumber() > 0) {
                const auto fn = self.get("setColorName"); return fn.call(self, {first, space});
            }
        }
        return self;
    }
    if (name == "applyMatrix3") {
        const auto r = self.get("r"), g = self.get("g"), b = self.get("b"), elements = first.get("elements");
        for (unsigned i = 0; i < 3; ++i) {
            const auto x = product(elements.get(JSValue(i)), r), y = product(elements.get(JSValue(i + 3)), g), z = product(elements.get(JSValue(i + 6)), b);
            self.set(std::string(1, "rgb"[i]), (x + y) + z);
        }
        return self;
    }
    if (name == "equals") {
        for (const auto* field : {"r", "g", "b"}) { const auto left = first.get(field); if (!left.strictEquals(self.get(field))) return false; }
        return true;
    }
    if (name == "fromArray" || name == "toArray") {
        auto array = first; if (name == "toArray" && array.isUndefined()) array = JSValue::array();
        const auto raw = arg(args, 1), offset = raw.isUndefined() ? JSValue(0) : raw;
        for (unsigned i = 0; i < 3; ++i) {
            const auto key = i == 0 ? offset : add(offset, i); const std::string field(1, "rgb"[i]);
            if (name == "toArray") array.set(key, self.get(field)); else self.set(field, array.get(key));
        }
        return name == "toArray" ? array : self;
    }
    throw std::logic_error("Unknown managed Color method");
}
JSValue keywords() {
    auto result = JSValue::object();
    for (const auto& [name, value] : std::initializer_list<std::pair<const char*, unsigned>>{
#include "ColorKeywords.inc"
    }) result.defineProperty(name, data(value));
    return result;
}
}
JSValue nativeColorConstructor() {
    static thread_local JSWeakValue weak; static thread_local bool initializing = false;
    if (const auto ctor = weak.lock(); ctor && ctor->isConstructible()) return *ctor;
    JSModuleInitializationScope scope(initializing, "Color");
    auto ctor = JSValue::constructor([](const JSValue& receiver, const Args& args, const Args&) -> JSValue {
        auto self = receiver; self.set("isColor", true); self.set("r", 1); self.set("g", 1); self.set("b", 1);
        const auto fn = self.get("set"); return fn.call(self, {arg(args, 0), arg(args, 1), arg(args, 2)});
    }, {}, "Color", 3);
    auto prototype = ctor.get("prototype"); const auto names = keywords();
    for (const auto& [key, arity] : std::initializer_list<std::pair<const char*, unsigned>>{
        {"set",3},{"setScalar",1},{"setHex",1},{"setRGB",3},{"setHSL",3},{"setStyle",1},{"setColorName",1},
        {"clone",0},{"copy",1},{"applyMatrix3",1},{"equals",1},{"fromArray",1},{"toArray",0}}) {
        JSPropertyDescriptor d; d.value = JSValue::function(method, {key, names}, key, arity);
        d.writable = d.configurable = true; prototype.defineProperty(key, d);
    }
    ctor.set("NAMES", names); ctor.retainForHeapLifetime(); weak = JSWeakValue(ctor); return ctor;
}
