// Direct port of three.js WebGLAttributes.js at
// 11738a9d55e502fb484cefffcea3a66dc44e9fe9. See THREEJS-LICENSE.
#include "renderers/webgl/WebGLAttributes.h"
#include "core/JSWeakMap.h"
#include "core/NativeArray.h"
#include <cmath>
#include <limits>

namespace {
using Args = std::vector<JSValue>;
JSValue argument(const Args& args, std::size_t index) { return index < args.size() ? args[index] : JSValue{}; }
void literal(JSValue object, const char* key, JSValue value) {
    JSPropertyDescriptor descriptor; descriptor.value = std::move(value);
    descriptor.writable = descriptor.enumerable = descriptor.configurable = true;
    object.defineProperty(key, descriptor);
}
JSValue add(const JSValue& left, const JSValue& right) {
    const auto a = left.toPrimitive(), b = right.toPrimitive();
    if (a.type() == JSValue::Type::String || b.type() == JSValue::Type::String) return a.toString() + b.toString();
    const auto x = a.toNumber(); const auto y = b.toNumber(); return x + y;
}
double subtract(const JSValue& left, const JSValue& right) {
    const auto x = left.toNumber(); const auto y = right.toNumber(); return x - y;
}
std::u16string utf16(const std::string& text) {
    std::u16string result;
    for (std::size_t i = 0; i < text.size();) {
        auto code = std::uint32_t(static_cast<unsigned char>(text[i++]));
        if (code >= 0xc0) {
            const auto count = code >= 0xf0 ? 3 : code >= 0xe0 ? 2 : 1;
            code &= count == 3 ? 7 : count == 2 ? 15 : 31;
            for (int j = 0; j < count; ++j) code = (code << 6) | (static_cast<unsigned char>(text[i++]) & 63);
        }
        if (code > 0xffff) { code -= 0x10000; result.push_back(char16_t(0xd800 + (code >> 10))); result.push_back(char16_t(0xdc00 + (code & 1023))); }
        else result.push_back(char16_t(code));
    }
    return result;
}
bool compare(const JSValue& left, const JSValue& right, bool equal = false) {
    const auto a = left.toPrimitive(JSValue::PreferredType::Number);
    const auto b = right.toPrimitive(JSValue::PreferredType::Number);
    if (a.type() == JSValue::Type::String && b.type() == JSValue::Type::String) {
        const auto x = utf16(a.string()), y = utf16(b.string()); return equal ? x <= y : x < y;
    }
    const auto x = a.toNumber(); const auto y = b.toNumber(); return equal ? x <= y : x < y;
}
bool instanceOf(const JSValue& value, std::size_t kind) {
    if (!value.isObject()) return false;
    const auto prototype = NativeArray_detail::constructor(kind).get("prototype");
    for (auto current = value.getPrototype(); !current.isNull(); current = current.getPrototype())
        if (current.sameValue(prototype)) return true;
    return false;
}
JSValue createBuffer(const JSValue& gl, JSValue attribute, const JSValue& bufferType) {
    const auto array = attribute.get("array");
    const auto usage = attribute.get("usage");
    const auto size = array.get("byteLength");
    const auto create = gl.get("createBuffer"); const auto buffer = create.call(gl);
    const auto bind = gl.get("bindBuffer"); bind.call(gl, {bufferType, buffer});
    const auto upload = gl.get("bufferData"); upload.call(gl, {bufferType, array, usage});
    const auto callback = attribute.get("onUploadCallback"); callback.call(attribute);
    JSValue type;
    if (instanceOf(array, 6)) type = gl.get("FLOAT");
    // Native Float16Array is not an existing runtime type. Do not substitute
    // arbitrary number arrays for its binary16 storage or instanceof identity.
    else if (instanceOf(array, 3)) type = gl.get(attribute.get("isFloat16BufferAttribute").toBoolean() ? "HALF_FLOAT" : "UNSIGNED_SHORT");
    else if (instanceOf(array, 2)) type = gl.get("SHORT");
    else if (instanceOf(array, 5)) type = gl.get("UNSIGNED_INT");
    else if (instanceOf(array, 4)) type = gl.get("INT");
    else if (instanceOf(array, 0)) type = gl.get("BYTE");
    else if (instanceOf(array, 1)) type = gl.get("UNSIGNED_BYTE");
    else if (instanceOf(array, 8)) type = gl.get("UNSIGNED_BYTE");
    else throw std::runtime_error(add("THREE.WebGLAttributes: Unsupported buffer data format: ", array).string());
    auto result = JSValue::object();
    literal(result, "buffer", buffer); literal(result, "type", type);
    literal(result, "bytesPerElement", array.get("BYTES_PER_ELEMENT"));
    literal(result, "version", attribute.get("version")); literal(result, "size", size);
    return result;
}
void updateBuffer(const JSValue& gl, const JSValue& buffer, JSValue attribute, const JSValue& bufferType) {
    const auto array = attribute.get("array"); auto ranges = attribute.get("updateRanges");
    const auto bind = gl.get("bindBuffer"); bind.call(gl, {bufferType, buffer});
    if (ranges.get("length").strictEquals(0)) {
        const auto upload = gl.get("bufferSubData"); upload.call(gl, {bufferType, 0, array});
    } else {
        const auto sort = ranges.get("sort");
        const auto comparator = JSValue::function(+[](const JSValue&, const Args& args, const Args&) -> JSValue {
            const auto a = argument(args, 0).get("start"); const auto b = argument(args, 1).get("start");
            return subtract(a, b);
        }, {}, "", 2);
        sort.call(ranges, {comparator});
        double mergeIndex = 0;
        for (double i = 1; compare(i, ranges.get("length")); ++i) {
            auto previous = ranges.get(JSValue(mergeIndex).toString()); const auto range = ranges.get(JSValue(i).toString());
            const auto start = range.get("start");
            const auto previousStart = previous.get("start"); const auto previousCount = previous.get("count");
            const auto end = add(add(previousStart, previousCount), 1);
            if (compare(start, end, true)) {
                const auto count = previous.get("count");
                const auto nextStart = range.get("start"); const auto nextCount = range.get("count");
                const auto nextEnd = add(nextStart, nextCount);
                const auto oldStart = previous.get("start"); const auto span = subtract(nextEnd, oldStart);
                const auto x = count.toNumber(); const auto y = JSValue(span).toNumber();
                const double maximum = std::isnan(x) || std::isnan(y) ? std::numeric_limits<double>::quiet_NaN()
                    : x == 0 && y == 0 ? (std::signbit(x) && std::signbit(y) ? -0.0 : 0.0) : x > y ? x : y;
                previous.set("count", maximum);
            } else {
                ++mergeIndex; ranges.set(JSValue(mergeIndex).toString(), range);
            }
        }
        ranges.set("length", mergeIndex + 1);
        const auto length = ranges.get("length");
        for (double i = 0; compare(i, length); ++i) {
            const auto range = ranges.get(JSValue(i).toString());
            const auto upload = gl.get("bufferSubData");
            const auto start = range.get("start"); const auto bytes = array.get("BYTES_PER_ELEMENT");
            const auto x = start.toNumber(); const auto y = bytes.toNumber(); const auto offset = x * y;
            const auto sourceStart = range.get("start"); const auto count = range.get("count");
            upload.call(gl, {bufferType, offset, array, sourceStart, count});
        }
        const auto clear = attribute.get("clearUpdateRanges"); clear.call(attribute);
    }
    const auto callback = attribute.get("onUploadCallback"); callback.call(attribute);
}
JSValue method(const JSValue&, const Args& args, const Args& captures) {
    const auto name = captures[0].string(); const auto gl = captures[1], buffers = captures[2];
    auto attribute = argument(args, 0);
    if (attribute.get("isInterleavedBufferAttribute").toBoolean()) attribute = attribute.get("data");
    if (name == "get") { const auto get = buffers.get("get"); return get.call(buffers, {attribute}); }
    if (name == "remove") {
        const auto get = buffers.get("get"); const auto data = get.call(buffers, {attribute});
        if (data.toBoolean()) {
            const auto remove = gl.get("deleteBuffer"); const auto buffer = data.get("buffer"); remove.call(gl, {buffer});
            const auto erase = buffers.get("delete"); erase.call(buffers, {attribute});
        }
        return {};
    }
    const auto bufferType = argument(args, 1);
    if (attribute.get("isGLBufferAttribute").toBoolean()) {
        const auto get = buffers.get("get"); const auto cached = get.call(buffers, {attribute});
        bool update = !cached.toBoolean();
        if (!update) { const auto previous = cached.get("version"); const auto next = attribute.get("version"); update = compare(previous, next); }
        if (update) {
            const auto set = buffers.get("set"); auto data = JSValue::object();
            literal(data, "buffer", attribute.get("buffer")); literal(data, "type", attribute.get("type"));
            literal(data, "bytesPerElement", attribute.get("elementSize")); literal(data, "version", attribute.get("version"));
            set.call(buffers, {attribute, data});
        }
        return {};
    }
    const auto get = buffers.get("get"); auto data = get.call(buffers, {attribute});
    if (data.isUndefined()) {
        const auto set = buffers.get("set"); const auto created = createBuffer(gl, attribute, bufferType);
        set.call(buffers, {attribute, created});
    } else {
        const auto previous = data.get("version"); const auto next = attribute.get("version");
        if (compare(previous, next)) {
            const auto oldSize = data.get("size"); const auto array = attribute.get("array"); const auto size = array.get("byteLength");
            if (!oldSize.strictEquals(size)) throw std::runtime_error("THREE.WebGLAttributes: The size of the buffer attribute's array buffer does not match the original size. Resizing buffer attributes is not supported.");
            const auto buffer = data.get("buffer"); updateBuffer(gl, buffer, attribute, bufferType);
            data.set("version", attribute.get("version"));
        }
    }
    return {};
}
}
WebGLAttributes::WebGLAttributes(JSValue gl) : object_(JSValue::object()) {
    const auto buffers = createNativeWeakMap();
    for (const auto& [name, arity] : std::initializer_list<std::pair<const char*, unsigned>>{{"get",1},{"remove",1},{"update",2}})
        literal(object_, name, JSValue::constructor(method, {name, gl, buffers}, name, arity, JSValue::ConstructorKind::Callable));
}
JSValue WebGLAttributes::get(JSValue attribute) { return object_.get("get").call(object_, {attribute}); }
JSValue WebGLAttributes::remove(JSValue attribute) { return object_.get("remove").call(object_, {attribute}); }
JSValue WebGLAttributes::update(JSValue attribute, JSValue bufferType) { return object_.get("update").call(object_, {attribute, bufferType}); }
