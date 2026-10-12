#include "animation/PropertyBinding.h"
#include "core/NativeObject.h"
#include <cmath>
#include <iostream>
#include <regex>

namespace {
using Function = PropertyBinding::Function;
template <auto Method> const Function& function() {
    static const Function result = [](PropertyBinding& binding, AnimationArray buffer, JSValue offset) {
        (binding.*Method)(std::move(buffer), std::move(offset));
    };
    return result;
}
JSValue enumeration(std::initializer_list<const char*> names) {
    auto value = JSValue::object(); int index = 0;
    for (auto name : names) value.set(name, index++);
    return value;
}
const JSValue& bindingTypes() { static auto value = enumeration({"Direct", "EntireArray", "ArrayElement", "HasFromToArray"}); return value; }
const JSValue& versioningTypes() { static auto value = enumeration({"None", "NeedsUpdate", "MatrixWorldNeedsUpdate"}); return value; }
const SharedArray<Function>& defaultGetters() {
    static const SharedArray<Function> value{function<&PropertyBinding::_getValue_direct>(), function<&PropertyBinding::_getValue_array>(),
        function<&PropertyBinding::_getValue_arrayElement>(), function<&PropertyBinding::_getValue_toArray>()};
    return value;
}
const SharedArray<SharedArray<Function>>& defaultSetters() {
    static const SharedArray<SharedArray<Function>> value{
        {function<&PropertyBinding::_setValue_direct>(), function<&PropertyBinding::_setValue_direct_setNeedsUpdate>(), function<&PropertyBinding::_setValue_direct_setMatrixWorldNeedsUpdate>()},
        {function<&PropertyBinding::_setValue_array>(), function<&PropertyBinding::_setValue_array_setNeedsUpdate>(), function<&PropertyBinding::_setValue_array_setMatrixWorldNeedsUpdate>()},
        {function<&PropertyBinding::_setValue_arrayElement>(), function<&PropertyBinding::_setValue_arrayElement_setNeedsUpdate>(), function<&PropertyBinding::_setValue_arrayElement_setMatrixWorldNeedsUpdate>()},
        {function<&PropertyBinding::_setValue_fromArray>(), function<&PropertyBinding::_setValue_fromArray_setNeedsUpdate>(), function<&PropertyBinding::_setValue_fromArray_setMatrixWorldNeedsUpdate>()}};
    return value;
}
JSValue get(JSValue target, JSValue key) {
    if (target.isNull() || target.isUndefined()) throw JSTypeError("Property access on null/undefined");
    return target.get(key.toString());
}
void put(JSValue target, JSValue key, JSValue value) {
    if (target.isNull() || target.isUndefined()) throw JSTypeError("Property assignment on null/undefined");
    target.set(key.toString(), value);
}
std::string append(const std::string& prefix, JSValue value) { return Interpolant_detail::add(prefix, value).toString(); }
void report(const std::string& message) { std::cerr << "THREE." << message << '\n'; }
void advance(JSValue& index) {
    const auto value = index.toNumber(), next = value + 1;
    if (!std::isfinite(value) || next == value) throw JSRangeError("PropertyBinding loop cannot progress");
    index = next;
}
void checkUnreachableLength(const JSValue& count) {
    // The source loops use strict i !== n, with numeric i starting at zero.
    if (count.type() != JSValue::Type::Number || !std::isfinite(count.number()) ||
        count.number() < 0 || std::trunc(count.number()) != count.number())
        throw JSRangeError("PropertyBinding loop cannot reach its endpoint");
}
std::wstring units(const std::string& text) {
    std::wstring result;
    for (std::size_t i = 0; i < text.size();) {
        auto cp = static_cast<unsigned char>(text[i++]);
        std::uint32_t value = cp;
        int extra = 0;
        if (cp >= 0xf0) { value &= 7; extra = 3; }
        else if (cp >= 0xe0) { value &= 15; extra = 2; }
        else if (cp >= 0xc0) { value &= 31; extra = 1; }
        while (extra-- && i < text.size()) value = (value << 6) | (static_cast<unsigned char>(text[i++]) & 63);
        if (value > 0xffff) { value -= 0x10000; result.push_back(wchar_t(0xd800 + (value >> 10))); result.push_back(wchar_t(0xdc00 + (value & 1023))); }
        else result.push_back(wchar_t(value));
    }
    return result;
}
std::string text(const std::wstring& units) {
    std::string result;
    for (std::size_t i = 0; i < units.size(); ++i) {
        auto cp = std::uint32_t(units[i]);
        if (cp >= 0xd800 && cp <= 0xdbff && i + 1 < units.size() && units[i + 1] >= 0xdc00 && units[i + 1] <= 0xdfff)
            cp = 0x10000 + ((cp - 0xd800) << 10) + (std::uint32_t(units[++i]) - 0xdc00);
        if (cp < 0x80) result += char(cp);
        else if (cp < 0x800) { result += char(0xc0 | (cp >> 6)); result += char(0x80 | (cp & 63)); }
        else if (cp < 0x10000) { result += char(0xe0 | (cp >> 12)); result += char(0x80 | ((cp >> 6) & 63)); result += char(0x80 | (cp & 63)); }
        else { result += char(0xf0 | (cp >> 18)); result += char(0x80 | ((cp >> 12) & 63)); result += char(0x80 | ((cp >> 6) & 63)); result += char(0x80 | (cp & 63)); }
    }
    return result;
}
bool whitespace(wchar_t c) {
    return (c >= 9 && c <= 13) || c == 0x20 || c == 0xa0 || c == 0x1680 ||
        (c >= 0x2000 && c <= 0x200a) || c == 0x2028 || c == 0x2029 || c == 0x202f || c == 0x205f || c == 0x3000 || c == 0xfeff;
}
JSValue search(JSValue children, JSValue nodeName) {
    for (double i = 0; i < children.get("length").toNumber(); ++i) {
        const auto child = children.get(JSValue(i).toString());
        if (child.get("name").strictEquals(nodeName) || child.get("uuid").strictEquals(nodeName)) return child;
        const auto result = search(child.get("children"), nodeName);
        if (result.toBoolean()) return result;
        if (i + 1 == i) throw JSRangeError("PropertyBinding search cannot progress");
    }
    return nullptr;
}
std::optional<std::size_t> tableIndex(JSValue index, std::size_t size) {
    const auto key = index.toString();
    for (std::size_t i = 0; i < size; ++i) if (key == std::to_string(i)) return i;
    return {};
}
void compositeLoop(JSValue bindings, JSValue group, const char* method, std::vector<JSValue> arguments = {}) {
    auto index = group.get("nCachedObjects_");
    const auto count = bindings.get("length");
    for (; !index.strictEquals(count); advance(index)) {
        const auto binding = get(bindings, index);
        const auto callable = binding.get(method);
        callable.call(binding, arguments);
    }
}
}

PropertyBinding::PropertyBinding(JSValue root, JSValue name, JSValue parsed)
    : path(name), parsedPath(parsed.toBoolean() ? parsed : parseTrackName(name)),
      node(findNode(root, parsedPath.get("nodeName"))), rootNode(root), BindingType(bindingTypes()), Versioning(versioningTypes()),
      GetterByBindingType(defaultGetters()), SetterByBindingTypeAndVersioning(defaultSetters()) {
    getValueFunction = function<&PropertyBinding::_getValue_unbound>();
    setValueFunction = function<&PropertyBinding::_setValue_unbound>();
}
std::string PropertyBinding::runtimeFunctionName(const Function& value) {
#define MATCH(NAME) if (value.sameIdentity(function<&PropertyBinding::NAME>())) return #NAME;
    MATCH(_getValue_unbound) MATCH(_setValue_unbound)
    MATCH(_getValue_unavailable) MATCH(_setValue_unavailable)
    MATCH(_getValue_direct) MATCH(_getValue_array) MATCH(_getValue_arrayElement) MATCH(_getValue_toArray)
    MATCH(_setValue_direct) MATCH(_setValue_array) MATCH(_setValue_arrayElement) MATCH(_setValue_fromArray)
    MATCH(_setValue_direct_setNeedsUpdate) MATCH(_setValue_direct_setMatrixWorldNeedsUpdate)
    MATCH(_setValue_array_setNeedsUpdate) MATCH(_setValue_array_setMatrixWorldNeedsUpdate)
    MATCH(_setValue_arrayElement_setNeedsUpdate) MATCH(_setValue_arrayElement_setMatrixWorldNeedsUpdate)
    MATCH(_setValue_fromArray_setNeedsUpdate) MATCH(_setValue_fromArray_setMatrixWorldNeedsUpdate)
#undef MATCH
    return {};
}
std::shared_ptr<PropertyBindingInterface> PropertyBinding::create(JSValue root, JSValue path, JSValue parsed) {
    if (root.toBoolean() && root.get("isAnimationObjectGroup").toBoolean()) return std::make_shared<Composite>(root, path, parsed);
    return std::make_shared<PropertyBinding>(root, path, parsed);
}
std::string PropertyBinding::sanitizeNodeName(JSValue name) {
    if (name.type() != JSValue::Type::String) throw JSTypeError("name.replace is not a function");
    std::wstring result;
    for (auto c : units(name.string())) {
        if (whitespace(c)) c = L'_';
        if (c != L'[' && c != L']' && c != L'.' && c != L':' && c != L'/') result += c;
    }
    return text(result);
}
JSValue PropertyBinding::parseTrackName(JSValue trackName) {
    const auto input = units(trackName.toString());
    // ECMAScript regexp on UTF-16 code units. Dot excludes all four JS line terminators.
    static const std::wregex expression(LR"(^((?:[^\[\]\.:/]+[/:])*)([^\[\]:/]+)?(?:\.([^\[\]\.:/]+)(?:\[([^\r\n\u2028\u2029]+)\])?)?\.([^\[\]\.:/]+)(?:\[([^\r\n\u2028\u2029]+)\])?$)");
    std::wsmatch match;
    if (!std::regex_match(input, match, expression)) throw std::runtime_error(append("THREE.PropertyBinding: Cannot parse trackName: ", trackName));
    auto result = JSValue::object();
    const char* keys[] = {"nodeName", "objectName", "objectIndex", "propertyName", "propertyIndex"};
    for (int i = 0; i < 5; ++i) result.set(keys[i], match[i + 2].matched ? JSValue(text(match[i + 2].str())) : JSValue{});
    const auto node = result.get("nodeName");
    if (node.toBoolean()) {
        const auto dot = node.string().rfind('.');
        if (dot != std::string::npos) {
            const auto object = node.string().substr(dot + 1);
            if (object == "material" || object == "materials" || object == "bones" || object == "map") {
                result.set("nodeName", node.string().substr(0, dot)); result.set("objectName", object);
            }
        }
    }
    return result;
}
JSValue PropertyBinding::findNode(JSValue root, JSValue name) {
    if (name.isUndefined() || name.strictEquals("") || name.strictEquals(".") || name.strictEquals(-1) ||
        name.strictEquals(root.get("name")) || name.strictEquals(root.get("uuid"))) return root;
    if (root.get("skeleton").toBoolean()) {
        const auto skeleton = root.get("skeleton"), method = skeleton.get("getBoneByName");
        const auto bone = method.call(skeleton, {name});
        if (!bone.isUndefined()) return bone;
    }
    if (root.get("children").toBoolean()) {
        const auto result = search(root.get("children"), name);
        if (result.toBoolean()) return result;
    }
    return nullptr;
}
void PropertyBinding::getValue(AnimationArray buffer, double offset) { getValueAt(std::move(buffer), offset); }
void PropertyBinding::setValue(AnimationArray buffer, double offset) { setValueAt(std::move(buffer), offset); }
void PropertyBinding::getValueAt(AnimationArray buffer, JSValue offset) { const auto method = getValueFunction; method(*this, buffer, offset); }
void PropertyBinding::setValueAt(AnimationArray buffer, JSValue offset) { const auto method = setValueFunction; method(*this, buffer, offset); }
void PropertyBinding::_getValue_unbound(AnimationArray buffer, JSValue offset) { bind(); getValueAt(buffer, offset); }
void PropertyBinding::_setValue_unbound(AnimationArray buffer, JSValue offset) { bind(); setValueAt(buffer, offset); }
void PropertyBinding::_getValue_direct(AnimationArray buffer, JSValue offset) {
    auto dest = nativeArray(buffer); const auto key = offset;
    const auto source = targetObject, property = propertyName;
    const auto value = get(source, property); put(dest, key, value);
}
void PropertyBinding::_getValue_array(AnimationArray buffer, JSValue offset) {
    auto dest = nativeArray(buffer); const auto source = resolvedProperty, count = source.get("length");
    checkUnreachableLength(count);
    for (double i = 0; !JSValue(i).strictEquals(count); ++i) {
        const auto index = offset.toNumber(); offset = index + 1;
        const auto value = source.get(JSValue(i).toString()); put(dest, index, value);
    }
}
void PropertyBinding::_getValue_arrayElement(AnimationArray buffer, JSValue offset) {
    auto dest = nativeArray(buffer); const auto key = offset;
    const auto source = resolvedProperty, index = propertyIndex; const auto value = get(source, index); put(dest, key, value);
}
void PropertyBinding::_getValue_toArray(AnimationArray buffer, JSValue offset) {
    const auto source = resolvedProperty, method = source.get("toArray"); method.call(source, {nativeArray(buffer), offset});
}
void PropertyBinding::_setValue_direct(AnimationArray buffer, JSValue offset) {
    auto dest = targetObject; const auto key = propertyName;
    const auto value = get(nativeArray(buffer), offset); put(dest, key, value);
}
void PropertyBinding::_setValue_array(AnimationArray buffer, JSValue offset) {
    auto dest = resolvedProperty; const auto count = dest.get("length"); auto source = nativeArray(buffer);
    checkUnreachableLength(count);
    for (double i = 0; !JSValue(i).strictEquals(count); ++i) {
        const auto index = offset.toNumber(); offset = index + 1;
        const auto value = source.get(JSValue(index).toString()); dest.set(JSValue(i).toString(), value);
    }
}
void PropertyBinding::_setValue_arrayElement(AnimationArray buffer, JSValue offset) {
    auto dest = resolvedProperty; const auto key = propertyIndex;
    const auto value = get(nativeArray(buffer), offset); put(dest, key, value);
}
void PropertyBinding::_setValue_fromArray(AnimationArray buffer, JSValue offset) {
    const auto dest = resolvedProperty, method = dest.get("fromArray"); method.call(dest, {nativeArray(buffer), offset});
}
#define VERSIONED(NAME, SUFFIX, FLAG) \
void PropertyBinding::_setValue_##NAME##_##SUFFIX(AnimationArray buffer, JSValue offset) { \
    PropertyBinding::_setValue_##NAME(buffer, offset); auto target = targetObject; target.set(FLAG, true); \
}
VERSIONED(direct, setNeedsUpdate, "needsUpdate")
VERSIONED(direct, setMatrixWorldNeedsUpdate, "matrixWorldNeedsUpdate")
VERSIONED(array, setNeedsUpdate, "needsUpdate")
VERSIONED(array, setMatrixWorldNeedsUpdate, "matrixWorldNeedsUpdate")
VERSIONED(arrayElement, setNeedsUpdate, "needsUpdate")
VERSIONED(arrayElement, setMatrixWorldNeedsUpdate, "matrixWorldNeedsUpdate")
VERSIONED(fromArray, setNeedsUpdate, "needsUpdate")
VERSIONED(fromArray, setMatrixWorldNeedsUpdate, "matrixWorldNeedsUpdate")
#undef VERSIONED

void PropertyBinding::bind() {
    auto target = node; const auto parsed = parsedPath;
    const auto objectName = parsed.get("objectName"), name = parsed.get("propertyName");
    auto index = parsed.get("propertyIndex");
    if (!target.toBoolean()) { target = findNode(rootNode, parsed.get("nodeName")); node = target; }
    getValueFunction = function<&PropertyBinding::_getValue_unavailable>();
    setValueFunction = function<&PropertyBinding::_setValue_unavailable>();
    if (!target.toBoolean()) { report(append("PropertyBinding: No target node found for track: ", path) + "."); return; }
    if (objectName.toBoolean()) {
        auto objectIndex = parsed.get("objectIndex");
        if (objectName.strictEquals("materials")) {
            if (!target.get("material").toBoolean()) { report("PropertyBinding: Can not bind to material as node does not have a material."); return; }
            if (!target.get("material").get("materials").toBoolean()) { report("PropertyBinding: Can not bind to material.materials as node.material does not have a materials array."); return; }
            target = target.get("material").get("materials");
        } else if (objectName.strictEquals("bones")) {
            if (!target.get("skeleton").toBoolean()) { report("PropertyBinding: Can not bind to bones as node does not have a skeleton."); return; }
            target = target.get("skeleton").get("bones");
            for (double i = 0; i < target.get("length").toNumber(); ++i) {
                if (target.get(JSValue(i).toString()).get("name").strictEquals(objectIndex)) { objectIndex = i; break; }
                if (i + 1 == i) throw JSRangeError("PropertyBinding bone search cannot progress");
            }
        } else if (objectName.strictEquals("map")) {
            if (target.has("map")) target = target.get("map");
            else {
                if (!target.get("material").toBoolean()) { report("PropertyBinding: Can not bind to material as node does not have a material."); return; }
                if (!target.get("material").get("map").toBoolean()) { report("PropertyBinding: Can not bind to material.map as node.material does not have a map."); return; }
                target = target.get("material").get("map");
            }
        } else {
            if (get(target, objectName).isUndefined()) { report("PropertyBinding: Can not bind to objectName of node undefined."); return; }
            target = get(target, objectName);
        }
        if (!objectIndex.isUndefined()) {
            if (get(target, objectIndex).isUndefined()) { report("PropertyBinding: Trying to bind to objectIndex of objectName, but is undefined."); return; }
            target = get(target, objectIndex);
        }
    }
    const auto property = get(target, name);
    if (property.isUndefined()) {
        const auto nodeName = parsed.get("nodeName");
        report(append(append("PropertyBinding: Trying to update property for track: ", nodeName) + ".", name) + " but it wasn't found."); return;
    }
    auto version = Versioning.get("None"); targetObject = target;
    if (target.get("isMaterial").strictEquals(true)) version = Versioning.get("NeedsUpdate");
    else if (target.get("isObject3D").strictEquals(true)) version = Versioning.get("MatrixWorldNeedsUpdate");
    auto type = BindingType.get("Direct");
    if (!index.isUndefined()) {
        if (name.strictEquals("morphTargetInfluences")) {
            if (!target.get("geometry").toBoolean()) { report("PropertyBinding: Can not bind to morphTargetInfluences because node does not have a geometry."); return; }
            if (!target.get("geometry").get("morphAttributes").toBoolean()) { report("PropertyBinding: Can not bind to morphTargetInfluences because node does not have a geometry.morphAttributes."); return; }
            if (!get(target.get("morphTargetDictionary"), index).isUndefined()) index = get(target.get("morphTargetDictionary"), index);
        }
        type = BindingType.get("ArrayElement"); resolvedProperty = property; propertyIndex = index;
    } else if (!property.get("fromArray").isUndefined() && !property.get("toArray").isUndefined()) {
        type = BindingType.get("HasFromToArray"); resolvedProperty = property;
    } else if (property.type() == JSValue::Type::Array) {
        type = BindingType.get("EntireArray"); resolvedProperty = property;
    } else propertyName = name;
    const auto getters = GetterByBindingType;
    auto getter = tableIndex(type, getters.size());
    getValueFunction = getter ? getters[*getter] : Function{};
    const auto setters = SetterByBindingTypeAndVersioning;
    const auto setter = tableIndex(type, setters.size());
    if (!setter) throw JSTypeError("Cannot read setter version from undefined");
    const auto versions = setters[*setter];
    const auto selectedVersion = tableIndex(version, versions.size());
    setValueFunction = selectedVersion ? versions[*selectedVersion] : Function{};
}
void PropertyBinding::unbind() {
    node = nullptr; getValueFunction = function<&PropertyBinding::_getValue_unbound>(); setValueFunction = function<&PropertyBinding::_setValue_unbound>();
}
PropertyBindingComposite::PropertyBindingComposite(JSValue group, JSValue path, JSValue optionalParsed) {
    const auto parsed = optionalParsed.toBoolean() ? optionalParsed : PropertyBinding::parseTrackName(path);
    _targetGroup = group;
    const auto method = group.get("subscribe_"); _bindings = method.call(group, {path, parsed});
}
void PropertyBindingComposite::getValue(AnimationArray buffer, double offset) { getValueAt(buffer, offset); }
void PropertyBindingComposite::setValue(AnimationArray buffer, double offset) { setValueAt(buffer, offset); }
void PropertyBindingComposite::getValueAt(AnimationArray buffer, JSValue offset) {
    bind(); const auto first = _targetGroup.get("nCachedObjects_");
    const auto binding = get(_bindings, first);
    if (!binding.isUndefined()) { const auto method = binding.get("getValue"); method.call(binding, {nativeArray(buffer), offset}); }
}
void PropertyBindingComposite::setValueAt(AnimationArray buffer, JSValue offset) {
    const auto bindings = _bindings; compositeLoop(bindings, _targetGroup, "setValue", {nativeArray(buffer), offset});
}
void PropertyBindingComposite::bind() { const auto bindings = _bindings; compositeLoop(bindings, _targetGroup, "bind"); }
void PropertyBindingComposite::unbind() { const auto bindings = _bindings; compositeLoop(bindings, _targetGroup, "unbind"); }
