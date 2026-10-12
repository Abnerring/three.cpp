#include "animation/PropertyBinding.h"
#include "core/NativeObject.h"
#include <iostream>
#include <sstream>
#include <limits>

namespace {
JSValue decode(JSValue value) {
    if (value.isObject() && value.hasOwn("$undefined")) return {};
    if (value.isObject() && value.hasOwn("$number")) {
        const auto s = value.get("$number").toString();
        if (s == "NaN") return std::numeric_limits<double>::quiet_NaN();
        if (s == "Infinity") return std::numeric_limits<double>::infinity();
        if (s == "-Infinity") return -std::numeric_limits<double>::infinity();
        return -0.;
    }
    if (value.isObject()) for (const auto& key : value.keys()) value.set(key, decode(value.get(key)));
    return value;
}
JsonValue encode(JSValue value) {
    if (value.isUndefined()) return "$undefined";
    if (value.isCallable()) return "$function";
    if (value.type() == JSValue::Type::Number) {
        const auto n = value.number();
        if (std::isnan(n)) return "$NaN";
        if (std::isinf(n)) return n < 0 ? "$-Infinity" : "$Infinity";
        if (n == 0 && std::signbit(n)) return "$-0";
        return n;
    }
    if (value.type() == JSValue::Type::Array) {
        JsonValue::Array result; for (std::uint32_t i = 0; i < value.length(); ++i) result.push_back(encode(value.get(std::to_string(i)))); return result;
    }
    if (value.isObject()) {
        JsonValue::Object result; for (const auto& key : value.keys()) result.emplace(key, encode(value.get(key))); return result;
    }
    return value.toJSON().value();
}
JSValue obj(std::initializer_list<std::pair<const char*, JSValue>> entries) {
    auto result = JSValue::object(); for (auto& [key, value] : entries) result.set(key, value); return result;
}
void push(JSValue array, JSValue value) { array.set(std::to_string(array.length()), value); }
JSValue list(std::initializer_list<JSValue> values) { return nativeArray(AnimationArray::array(values)); }
std::string errorClass(const std::exception& error) {
    if (dynamic_cast<const JSTypeError*>(&error)) return "TypeError";
    if (dynamic_cast<const JSRangeError*>(&error)) return "RangeError";
    return "Error";
}
JsonValue state(PropertyBinding& b, JSValue root, AnimationArray buffer) {
    JsonValue::Object properties;const auto wrapped=nativeArray(buffer);for(const auto& key:wrapped.keys())properties[key]=encode(wrapped.get(key));
    return JsonValue::Object{{"root", encode(root)}, {"parsed", encode(b.parsedPath)}, {"node", encode(b.node)},
        {"target", encode(b.targetObject)}, {"resolved", encode(b.resolvedProperty)}, {"propertyName", encode(b.propertyName)},
        {"propertyIndex", encode(b.propertyIndex)}, {"buffer", encode(wrapped)}, {"bufferProperties",properties}};
}
JsonValue normal(JSValue request) {
    auto root = request.get("root");
    if (request.get("typedProperty").toBoolean()) root.set("value", nativeArray(convertArray(root.get("value"), AnimationArrayType::Float32)));
    PropertyBinding binding(root, request.get("path"), request.get("parsed"));
    AnimationArray buffer = request.hasOwn("buffer") ? request.get("buffer") : list({99,98,97,96,95});
    if (request.get("typedBuffer").toBoolean()) buffer = convertArray(buffer, AnimationArrayType::Float64);
    JsonValue::Array results{state(binding, root, buffer)};
    auto actions = request.get("actions");
    for (std::uint32_t i = 0; i < actions.length(); ++i) {
        const auto action = actions.get(std::to_string(i)); const auto op = action.get("op").toString();
        std::string error;
        try {
            if (op == "get") binding.getValueAt(buffer, action.hasOwn("offset") ? action.get("offset") : JSValue(0));
            else if (op == "set") binding.setValueAt(buffer, action.hasOwn("offset") ? action.get("offset") : JSValue(0));
            else if (op == "bind") binding.bind();
            else if (op == "unbind") binding.unbind();
            else if (op == "buffer") buffer = action.get("value");
            else if (op == "replace") {
                auto object = root; auto keys = action.get("keys");
                for (std::uint32_t k = 0; k + 1 < keys.length(); ++k) object = object.get(keys.get(std::to_string(k)).toString());
                object.set(keys.get(std::to_string(keys.length() - 1)).toString(), action.get("value"));
            }
        } catch (const std::exception& exception) { error = errorClass(exception); }
        results.emplace_back(JsonValue::Object{{"error", error}, {"state", state(binding, root, buffer)}});
    }
    return results;
}
PropertyBinding* activeBinding = nullptr;
PropertyBindingComposite* activeComposite = nullptr;
JSPropertyDescriptor accessor(JSValue getter, JSValue setter = {}) {
    JSPropertyDescriptor d; d.getter = getter; d.setter = setter; d.enumerable = true; d.configurable = true; return d;
}
JsonValue callback(const std::string& name) {
    auto trace = JSValue::array(), root = obj({{"name","root"},{"isMaterial",true},{"value",1}});
    if (name == "sharedTables") {
        PropertyBinding a(root,".value"), b(root,".value");
        const auto previousGet=a.GetterByBindingType[0], previousSet=a.SetterByBindingTypeAndVersioning[0][1];
        const bool same=a.GetterByBindingType.sameIdentity(b.GetterByBindingType),sameRow=a.SetterByBindingTypeAndVersioning[0].sameIdentity(b.SetterByBindingTypeAndVersioning[0]);
        a.GetterByBindingType[0]=[trace](PropertyBinding&,AnimationArray buffer,JSValue offset){push(trace,"getShared");buffer.set(offset.toNumber(),77);};
        a.SetterByBindingTypeAndVersioning[0][1]=[trace](PropertyBinding&,AnimationArray,JSValue){push(trace,"setShared");};
        AnimationArray buffer=AnimationArray::array({0});b.getValue(buffer,0);b.setValue(buffer,0);
        a.GetterByBindingType=a.GetterByBindingType.slice();a.GetterByBindingType[0]=previousGet;a.getValue(buffer,0);
        b.GetterByBindingType[0]=previousGet;b.SetterByBindingTypeAndVersioning[0][1]=previousSet;
        return JsonValue::Object{{"trace",encode(trace)},{"same",same},{"sameRow",sameRow},{"separate",!a.GetterByBindingType.sameIdentity(b.GetterByBindingType)},{"buffer",encode(nativeArray(buffer))}};
    }
    if (name == "versionTarget") {
        auto other = JSValue::object();
        other.defineProperty("needsUpdate", accessor({}, JSValue::function([](const JSValue&, const auto& a, const auto& c)->JSValue {push(c[0],list({"newVersion",a[0]}));return {};},{trace})));
        root.defineProperty("value", accessor(JSValue::function([](const JSValue&,const auto&,const auto& c)->JSValue {push(c[0],"get");return 1;},{trace}),
            JSValue::function([](const JSValue&,const auto& a,const auto& c)->JSValue {push(c[0],list({"set",a[0]}));activeBinding->targetObject=c[1];return {};},{trace,other})));
        root.defineProperty("needsUpdate",accessor({},JSValue::function([](const JSValue&,const auto&,const auto& c)->JSValue {push(c[0],"oldVersion");return {};},{trace})));
        PropertyBinding binding(root,".value"); activeBinding=&binding; binding.setValue(AnimationArray::array({7}),0); activeBinding=nullptr;
        return encode(trace);
    }
    if (name == "bindReentry") {
        root.defineProperty("value",accessor(JSValue::function([](const JSValue&,const auto&,const auto& c)->JSValue {
            push(c[0],"get");activeBinding->setValue(AnimationArray::array({8}),0);push(c[0],"returned");return 1;
        },{trace})));
        PropertyBinding binding(root,".value"); activeBinding=&binding; binding.bind(); activeBinding=nullptr;
        return encode(trace);
    }
    if (name == "arrayLength") {
        auto values=list({1,2,3}), buffer=JSValue::array();root.set("value",values);
        buffer.defineProperty("0",accessor({},JSValue::function([](const JSValue&,const auto& a,const auto& c)->JSValue {push(c[0],a[0]);push(c[1],4);return {};},{trace,values})));
        PropertyBinding binding(root,".value");binding.getValue(buffer,0);
        return JsonValue::Object{{"trace",encode(trace)},{"buffer",encode(buffer)},{"values",encode(values)}};
    }
    if (name == "fromToArray") {
        AnimationArray buffer=Float64Array({3,4,5});auto vector=obj({{"x",1},{"y",2}}), wrapped=nativeArray(buffer);
        vector.set("fromArray",JSValue::function([](const JSValue& receiver,const auto& a,const auto& c)->JSValue {
            push(c[0],list({"from",receiver.sameValue(c[1]),a[0].sameValue(c[2]),a[1]}));auto dest=receiver;
            const auto n=a[1].toNumber();dest.set("x",a[0].get(JSValue(n).toString()));dest.set("y",a[0].get(JSValue(n+1).toString()));return receiver;
        },{trace,vector,wrapped}));
        vector.set("toArray",JSValue::function([](const JSValue& receiver,const auto& a,const auto& c)->JSValue {
            push(c[0],list({"to",receiver.sameValue(c[1]),a[0].sameValue(c[2]),a[1]}));auto dest=a[0];const auto n=a[1].toNumber();
            dest.set(JSValue(n).toString(),receiver.get("x"));dest.set(JSValue(n+1).toString(),receiver.get("y"));return dest;
        },{trace,vector,wrapped}));root.set("value",vector);
        PropertyBinding binding(root,".value");binding.setValue(buffer,1);binding.getValue(buffer,0);
        return JsonValue::Object{{"trace",encode(trace)},{"vector",encode(vector)},{"buffer",encode(wrapped)},{"needsUpdate",encode(root.get("needsUpdate"))}};
    }
    if (name == "findGetters") {
        auto child=obj({{"name","child"},{"uuid","id"},{"children",JSValue::array()}}),children=list({child});
        root.defineProperty("children",accessor(JSValue::function([](const JSValue&,const auto&,const auto& c)->JSValue {push(c[0],"children");return c[1];},{trace,children})));
        root.defineProperty("skeleton",accessor(JSValue::function([](const JSValue&,const auto&,const auto& c)->JSValue {push(c[0],"skeleton");return {};},{trace})));
        const auto found=PropertyBinding::findNode(root,"id");return JsonValue::Object{{"trace",encode(trace)},{"same",found.sameValue(child)}};
    }
    if (name == "lookupOrder") {
        auto material=obj({{"isMaterial",true},{"opacity",1}});
        root.defineProperty("material",accessor(JSValue::function([](const JSValue&,const auto&,const auto& c)->JSValue {push(c[0],"material");return c[1];},{trace,material})));
        material.defineProperty("opacity",accessor(JSValue::function([](const JSValue&,const auto&,const auto& c)->JSValue {push(c[0],"opacity");return 1;},{trace}),
            JSValue::function([](const JSValue&,const auto& a,const auto& c)->JSValue {push(c[0],list({"write",a[0]}));return {};},{trace})));
        PropertyBinding binding(root,".material.opacity");binding.getValue(AnimationArray::array({0}),0);binding.setValue(AnimationArray::array({7}),0);
        return encode(trace);
    }
    if (name == "offsetCoercion") {
        auto other=obj({{"value",8}}),buffer=JSValue::array(),offset=JSValue::object();
        offset.set("toString",JSValue::function([](const JSValue&,const auto&,const auto& c)->JSValue {push(c[0],"key");activeBinding->targetObject=c[1];return "1";},{trace,other}));
        root.defineProperty("value",accessor(JSValue::function([](const JSValue&,const auto&,const auto& c)->JSValue {push(c[0],"rootRead");return 1;},{trace})));
        PropertyBinding binding(root,".value");binding.bind();activeBinding=&binding;binding.getValueAt(buffer,offset);activeBinding=nullptr;
        return JsonValue::Object{{"trace",encode(trace)},{"buffer",encode(buffer)}};
    }
    if (name == "boneUndefined" || name == "boneNull" || name == "boneFound") {
        auto child=obj({{"name","child"},{"uuid","id"},{"children",JSValue::array()}}),skeleton=JSValue::object();
        skeleton.set("getBoneByName",JSValue::function([](const JSValue& receiver,const auto& a,const auto& c)->JSValue {
            push(c[0],list({"bone",receiver.sameValue(c[1]),a[0]}));return c[2];
        },{trace,skeleton,name=="boneUndefined"?JSValue{}:name=="boneNull"?JSValue(nullptr):child}));
        root.set("skeleton",skeleton);root.set("children",list({child}));const auto found=PropertyBinding::findNode(root,"id");
        return JsonValue::Object{{"trace",encode(trace)},{"found",encode(found)},{"same",found.sameValue(child)}};
    }
    throw std::runtime_error("Unknown callback fixture");
}
JsonValue composite(JSValue request) {
    auto trace=JSValue::array(),bindings=JSValue::array(),group=obj({{"isAnimationObjectGroup",true},{"nCachedObjects_",request.get("cached")}});
    for(int i=0;i<3;++i){
        auto b=obj({{"id",i}});
        for(const auto* method:{"bind","unbind","getValue","setValue"}) b.set(method,JSValue::function([](const JSValue& receiver,const auto& a,const auto& c)->JSValue {
            const auto id=receiver.get("id"),name=c[1];push(c[0],list({name,id}));
            if(name.strictEquals("bind")&&id.strictEquals(1)&&c[3].toBoolean()){auto g=c[2];g.set("nCachedObjects_",2);}
            if(name.strictEquals("getValue")){auto array=a[0];array.set(a[1].toString(),id.toNumber()+10);}
            if(name.strictEquals("setValue")) push(c[0],a[0].get(a[1].toString()));
            return {};
        },{trace,method,group,request.get("mutate")}));bindings.set(std::to_string(i),b);
    }
    group.set("subscribe_",JSValue::function([](const JSValue& receiver,const auto& a,const auto& c)->JSValue {push(c[0],list({"subscribe",receiver.sameValue(c[2]),a[0],a[1]}));return c[1];},{trace,bindings,group}));
    auto binding=PropertyBinding::create(group,".value");auto* c=dynamic_cast<PropertyBindingComposite*>(binding.get());
    AnimationArray buffer=Float64Array({7});c->getValue(buffer,0);c->setValue(buffer,0);c->unbind();
    return JsonValue::Object{{"trace",encode(trace)},{"buffer",encode(nativeArray(buffer))},{"cached",encode(group.get("nCachedObjects_"))}};
}
JsonValue run(JSValue request) {
    const auto kind=request.get("kind");
    if(kind.strictEquals("parse"))return encode(PropertyBinding::parseTrackName(request.get("value")));
    if(kind.strictEquals("sanitize"))return PropertyBinding::sanitizeNodeName(request.get("value"));
    if(kind.strictEquals("find"))return encode(PropertyBinding::findNode(request.get("root"),request.get("name")));
    if(kind.strictEquals("callback"))return callback(request.get("name").toString());
    if(kind.strictEquals("composite"))return composite(request);
    return normal(request);
}
}
int main(){
    const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto requests=decode(Animation_detail::parseJSON(input));
    JsonValue::Array result;
    for(std::uint32_t i=0;i<requests.length();++i){std::ostringstream log;auto* previous=std::cerr.rdbuf(log.rdbuf());JsonValue output;
        try{output=run(requests.get(std::to_string(i)));}catch(const std::exception& e){output=JsonValue::Object{{"error",errorClass(e)}};}
        std::cerr.rdbuf(previous);activeBinding=nullptr;activeComposite=nullptr;
        JsonValue::Array messages;std::istringstream lines(log.str());for(std::string line;std::getline(lines,line);)messages.emplace_back(line);
        result.emplace_back(JsonValue::Object{{"result",output},{"messages",messages}});
    }
    std::cout<<JsonValue(result).dump()<<'\n';
}
