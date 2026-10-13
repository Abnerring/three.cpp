#include "core/JSValue.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args = std::vector<JSValue>;
using O = JsonValue::Object;
using A = JsonValue::Array;
JSValue tuple(const Args& args) {
    auto result = JSValue::array();
    for (const auto& value : args) result.set(std::to_string(result.length()), value);
    return result;
}
void append(JSValue events, const Args& args) { events.set(std::to_string(events.length()), tuple(args)); }
JsonValue encode(const JSValue& value) {
    if (value.isUndefined()) return "@undefined";
    if (value.isNull()) return nullptr;
    if (value.isCallable()) return "@function";
    if (value.type() == JSValue::Type::Boolean) return value.toBoolean();
    if (value.type() == JSValue::Type::String) return value.string();
    if (value.type() == JSValue::Type::Number) {
        const auto n = value.number();
        if (std::isnan(n)) return "@NaN";
        if (std::isinf(n)) return n < 0 ? "@-Infinity" : "@Infinity";
        if (n == 0 && std::signbit(n)) return "@-0";
        return n;
    }
    if (value.type() == JSValue::Type::Array) {
        A result; for (unsigned i = 0; i < value.length(); ++i) result.push_back(encode(value.get(std::to_string(i))));
        return result;
    }
    O result; for (const auto& key : value.keys()) result[key] = encode(value.get(key)); return result;
}
JsonValue snapshot(const JSValue& object) {
    if (!object.isObject()) return encode(object);
    O result;
    for (const auto& key : object.ownPropertyNames()) {
        const auto d = *object.getOwnPropertyDescriptor(key);
        O entry{{"enumerable", d.enumerable.value_or(false)}, {"configurable", d.configurable.value_or(false)}};
        if (d.value) { entry["value"] = encode(*d.value); entry["writable"] = d.writable.value_or(false); }
        else { entry["get"] = d.getter && d.getter->isCallable(); entry["set"] = d.setter && d.setter->isCallable(); }
        result[key] = std::move(entry);
    }
    return result;
}
std::string error(const auto& fn) {
    try { fn(); return ""; }
    catch (const JSTypeError&) { return "TypeError"; }
    catch (const JSRangeError&) { return "RangeError"; }
    catch (...) { return "Error"; }
}
JSValue intrinsic() { return JSValue::array().get("shift"); }
void property(JSValue object, const std::string& key, const JSValue& value, bool writable = true, bool configurable = true) {
    JSPropertyDescriptor d; d.value = value; d.writable = writable; d.enumerable = true; d.configurable = configurable;
    object.defineProperty(key, d);
}
void access(JSValue target, std::string key, JSValue receiver, JSValue events, JSValue value, int read = 0, int write = 0) {
    auto state = JSValue::object(); state.set("entered", false);
    JSPropertyDescriptor d; d.configurable = d.enumerable = true;
    d.getter = JSValue::function([](const JSValue& self, const Args&, const Args& c)->JSValue {
        auto o = c[0], state = c[5]; const auto action = c[4].toNumber();
        append(c[1], {"get:" + c[2].string(), self.sameValue(o)});
        if (action == 1) throw JSRangeError("getter");
        if (action == 2) o.erase("2");
        if (action == 3) o.set("4", 99);
        if (action == 4) o.set("length", 0);
        if (action == 5 && !state.get("entered").toBoolean()) {
            state.set("entered", true); const auto result = intrinsic().call(o); append(c[1], {"inner", result});
        }
        if (action == 6) { auto p = JSValue::object(); p.set("1", 88); o.setPrototype(p); }
        if (action == 7) { append(c[1], {"gc"}); o.collectGarbage(); }
        return c[3];
    }, {receiver, events, key, value, read, state});
    if (write != -1) d.setter = JSValue::function([](const JSValue& self, const Args& a, const Args& c)->JSValue {
        auto o = c[0], state = c[4]; append(c[1], {"set:" + c[2].string(), a[0], self.sameValue(o)});
        if (c[3].strictEquals(1)) throw JSRangeError("setter");
        if (c[3].strictEquals(2) && !state.get("entered").toBoolean()) {
            state.set("entered", true); const auto result = intrinsic().call(o); append(c[1], {"inner", result});
        }
        return {};
    }, {receiver, events, key, write, state});
    target.defineProperty(key, d);
}
JsonValue execute(const std::string& kind, int mode, JSValue o, JSValue events) {
    JSValue result;
    const auto failure = error([&] { result = intrinsic().call(o, {"ignored"}); });
    return O{{"kind", kind}, {"mode", mode}, {"error", failure}, {"result", encode(result)}, {"props", snapshot(o)}, {"events", encode(events)}};
}
JsonValue scenario(int mode, bool array) {
    auto o = array ? JSValue::array() : JSValue::object(), events = JSValue::array();
    o.set("length", 4); o.set("0", -0.); o.set("2", NAN); o.set("3", JSValue());
    if (mode == 1) { auto p = JSValue::object(o.getPrototype()); p.set("1", 9); o.setPrototype(p); }
    if (mode == 2) { auto p = JSValue::object(o.getPrototype()); o.erase("0"); access(p,"0",o,events,12); o.setPrototype(p); }
    if (mode == 3) access(o,"0",o,events,12);
    if (mode == 4) access(o,"1",o,events,13,2);
    if (mode == 5) access(o,"1",o,events,13,3);
    if (mode == 6) access(o,"1",o,events,13,4);
    if (mode == 7) access(o,"2",o,events,14,1);
    if (mode == 8) access(o,"0",o,events,12,0,1);
    if (mode == 9) access(o,"1",o,events,13,0,-1);
    if (mode == 10) property(o,"1",9,false);
    if (mode == 11) { JSPropertyDescriptor d; d.configurable=false; o.defineProperty("3",d); }
    if (mode == 12) { JSPropertyDescriptor d; d.configurable=false; o.defineProperty("0",d); }
    if (mode == 13) { JSPropertyDescriptor d; d.writable=false; o.defineProperty("length",d); }
    if (mode == 14) o.preventExtensions();
    if (mode == 15) { auto p = JSValue::object(o.getPrototype()); access(p,"1",o,events,13); o.setPrototype(p); }
    if (mode == 16) { auto p = JSValue::object(o.getPrototype()); property(p,"1",9,false); o.setPrototype(p); }
    if (mode == 17) access(o,"length",o,events,4);
    if (mode == 18) access(o,"length",o,events,4,1);
    if (mode == 19) access(o,"length",o,events,4,0,1);
    if (mode == 20) access(o,"0",o,events,12,5);
    if (mode == 21) access(o,"0",o,events,12,0,2);
    if (mode == 22) access(o,"0",o,events,12,6);
    if (mode == 23) access(o,"2",o,events,14,7);
    return execute(array?"array":"object",mode,o,events);
}
JsonValue sparse(int mask, bool array, bool inherited) {
    auto o = array ? JSValue::array() : JSValue::object(); o.set("length",6);
    auto p = JSValue::object(o.getPrototype()); if(inherited)o.setPrototype(p);
    for(int i=0;i<6;++i)if(mask&(1<<i)) {
        auto target=inherited&&(i%2)?p:o;target.set(std::to_string(i),i==0?JSValue(-0.):i==2?JSValue(NAN):i==3?JSValue():JSValue(i));
    }
    return execute(std::string(array?"sparse-array":"sparse-object")+(inherited?"-inherited":""),mask,o,JSValue::array());
}
JsonValue lengthCase(int mode) {
    auto o=JSValue::object(),events=JSValue::array();o.set("0",-0.);o.set("1",7);o.set("2",8);
    const Args lengths{JSValue(),nullptr,false,true,0,-0.,-1,-INFINITY,NAN,0.9,1.9,3.9," 3.8 ","invalid"};
    if(mode<int(lengths.size()))o.set("length",lengths[mode]);
    else if(mode<18){
        auto value=JSValue::object();value.set("tag","length");
        value.set("valueOf",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
            append(c[0],{"valueOf"});auto o=c[1];
            if(c[2].strictEquals(15))throw JSRangeError("length");
            if(c[2].strictEquals(16))o.set("1",42);
            if(c[2].strictEquals(17)){o.set("length",0);return 3.8;}
            return 3.8;
        },{events,o,mode}));o.set("length",value);
    }else{
        o.set("length",mode==18?4294967295.:mode==19?9007199254740991.:INFINITY);
        access(o,"0",o,events,0,1);
    }
    return execute("length",mode,o,events);
}
JsonValue primitive(int mode) {
    const Args values{JSValue(),nullptr,false,true,0,-0.,42,NAN,"","a","ab","\xF0\x9F\x98\x80"};
    return execute("primitive",mode,values[mode],JSValue::array());
}
JsonValue meta() {
    const auto fn=intrinsic();
    return O{{"kind","metadata"},{"name",encode(fn.isObject()?fn.get("name"):JSValue())},
        {"length",encode(fn.isObject()?fn.get("length"):JSValue())},{"own",snapshot(fn)},
        {"construct",error([&]{fn.construct();})},
        {"prototypeDescriptor",snapshot(JSValue::array().getPrototype()).at("shift")}};
}
}
int main() {
    A result;
    // Baseline failure remains structured even though the method is absent.
    for(bool array:{false,true})for(int mode=0;mode<24;++mode)if(!array||mode<17||mode>19)result.push_back(scenario(mode,array));
    for(bool array:{false,true})for(bool inherited:{false,true})for(int mask=0;mask<64;++mask)result.push_back(sparse(mask,array,inherited));
    for(int mode=0;mode<21;++mode)result.push_back(lengthCase(mode));
    for(int mode=0;mode<12;++mode)result.push_back(primitive(mode));
    if(intrinsic().isCallable())result.push_back(meta());
    else result.push_back(O{{"kind","metadata"},{"missing",true}});
    std::cout<<JsonValue(result).dump()<<'\n';
}
