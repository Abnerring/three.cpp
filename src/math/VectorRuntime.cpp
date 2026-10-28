#include "math/Vector2.h"
#include "math/Vector3.h"
#include "core/JSModuleObject.h"
#include "core/JSGlobal.h"
#include <cmath>

namespace {
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, std::size_t i) { return i < args.size() ? args[i] : JSValue{}; }
JSValue add(JSValue a, JSValue b) { const auto x = a.toPrimitive(), y = b.toPrimitive(); if (x.type() == JSValue::Type::String || y.type() == JSValue::Type::String) return x.toString() + y.toString(); return x.toNumber() + y.toNumber(); }
double product(const JSValue& a, const JSValue& b) { const auto left = a.toNumber(); const auto right = b.toNumber(); return left * right; }
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string(); const auto count = static_cast<unsigned>(captures[1].number()); const auto source = arg(args, 0);
    if (name == "get width" || name == "get height") return self.get(name == "get width" ? "x" : "y");
    if (name == "set width" || name == "set height") { self.set(name == "set width" ? "x" : "y", source); return {}; }
    if (name == "clone") { const auto ctor = self.get("constructor"); Args values; for (unsigned i = 0; i < count; ++i) values.push_back(self.get(std::string(1, "xyz"[i]))); return ctor.construct(values); }
    if (name == "set" || name == "setScalar" || name == "copy") {
        auto z = arg(args, 2); if (name == "set" && count == 3 && z.isUndefined()) z = self.get("z");
        for (unsigned i = 0; i < count; ++i) { const std::string key(1, "xyz"[i]); self.set(key, name == "copy" ? source.get(key) : name == "setScalar" ? source : i == 2 ? z : arg(args, i)); } return self;
    }
    if (name == "toArray" || name == "fromArray") {
        auto array = source; if (name == "toArray" && array.isUndefined()) array = JSValue::array(); const auto raw = arg(args, 1), offset = raw.isUndefined() ? JSValue(0) : raw;
        for (unsigned i = 0; i < count; ++i) { const auto index = i ? add(offset, i) : offset; const std::string key(1, "xyz"[i]); if (name == "toArray") array.set(index, self.get(key)); else self.set(key, array.get(index)); }
        return name == "toArray" ? array : self;
    }
    if (name == "equals") { for (unsigned i = 0; i < count; ++i) { const std::string key(1, "xyz"[i]); const auto left = source.get(key); if (!left.strictEquals(self.get(key))) return false; } return true; }
    if(name=="multiplyScalar"){
        for(unsigned i=0;i<count;++i){const std::string key(1,"xyz"[i]);self.set(key,product(self.get(key),source));}return self;
    }
    if(name=="divideScalar"){
        const auto multiply=self.get("multiplyScalar");return multiply.call(self,{1/source.toNumber()});
    }
    if(name=="normalize"){
        const auto divide=self.get("divideScalar");auto length=self.get("length").call(self);if(!length.toBoolean())length=1;return divide.call(self,{length});
    }
    if(name=="fromBufferAttribute"){
        const auto index=arg(args,1);
        for(unsigned i=0;i<count;++i){const std::string component(1,"xyz"[i]);const std::string method=std::string("get")+"XYZ"[i];const auto callback=source.get(method);self.set(component,callback.call(source,{index}));}return self;
    }
    if(name=="applyNormalMatrix"){
        const auto result=self.get("applyMatrix3").call(self,{source});return result.get("normalize").call(result);
    }
    if(name=="transformDirection"){
        const auto x=self.get("x"),y=self.get("y"),z=self.get("z"),e=source.get("elements");
        for(unsigned i=0;i<3;++i){const auto a=product(e.get(std::to_string(i)),x),b=product(e.get(std::to_string(i+4)),y);const auto ab=a+b;const auto c=product(e.get(std::to_string(i+8)),z);self.set(std::string(1,"xyz"[i]),ab+c);}
        return self.get("normalize").call(self);
    }
    if(name=="length"){
        const auto math=nativeGlobalBinding("Math"),sqrt=math.get("sqrt");
        double result=0;
        for(unsigned i=0;i<count;++i){const std::string key(1,"xyz"[i]);const auto left=self.get(key),right=self.get(key);const auto squared=product(left,right);result=i?result+squared:squared;}
        return sqrt.call(math,{result});
    }
    if(name=="setFromMatrixColumn"){
        const auto fn=self.get("fromArray"),elements=source.get("elements");const auto offset=arg(args,1).toNumber()*4;
        return fn.call(self,{elements,offset});
    }
    if(name=="setFromMatrixPosition"){
        const auto elements=source.get("elements");for(unsigned i=0;i<3;++i)self.set(std::string(1,"xyz"[i]),elements.get(std::to_string(12+i)));return self;
    }
    if(name=="setFromMatrixScale"){
        JSValue scales[3];
        for(unsigned i=0;i<3;++i){const auto column=self.get("setFromMatrixColumn").call(self,{source,i});scales[i]=column.get("length").call(column);}
        for(unsigned i=0;i<3;++i)self.set(std::string(1,"xyz"[i]),scales[i]);return self;
    }
    if(name=="applyMatrix4"){
        const auto x=self.get("x"),y=self.get("y"),z=self.get("z"),e=source.get("elements");
        const auto a=product(e.get("3"),x),b=product(e.get("7"),y);const auto ab=a+b;const auto c=product(e.get("11"),z);const auto abc=ab+c;
        const auto denominator=add(abc,e.get("15")).toNumber();const auto w=1/denominator;
        for(unsigned i=0;i<3;++i){
            const auto a=product(e.get(std::to_string(i)),x),b=product(e.get(std::to_string(i+4)),y);const auto ab=a+b;
            const auto c=product(e.get(std::to_string(i+8)),z);const auto abc=ab+c;const auto sum=add(abc,e.get(std::to_string(i+12)));
            self.set(std::string(1,"xyz"[i]),product(sum,w));
        }return self;
    }
    if (name == "applyMatrix3") {
        const auto x = self.get("x"), y = self.get("y"); const auto z = count == 3 ? self.get("z") : JSValue{}; const auto elements = source.get("elements");
        for (unsigned i = 0; i < count; ++i) {
            const auto a = product(elements.get(JSValue(i)), x);
            const auto b = product(elements.get(JSValue(i + 3)), y);
            const auto c = elements.get(JSValue(i + 6));
            self.set(std::string(1, "xyz"[i]), count == 3 ? JSValue((a + b) + product(c, z)) : add(a + b, c));
        }
        return self;
    }
    throw std::logic_error("Unknown managed vector method");
}
JSValue constructor(unsigned count) {
    static thread_local JSWeakValue weak[2]; static thread_local bool initializing[2]{};
    const auto index = count - 2; if (const auto ctor = weak[index].lock(); ctor && ctor->isConstructible()) return *ctor;
    const auto name = count == 2 ? "Vector2" : "Vector3"; JSModuleInitializationScope scope(initializing[index], name);
    auto ctor = JSValue::constructor([](const JSValue& receiver, const Args& args, const Args& captures) -> JSValue { auto self = receiver; const auto count = static_cast<unsigned>(captures[0].number()); for (unsigned i = 0; i < count; ++i) { const auto value = arg(args, i); self.set(std::string(1, "xyz"[i]), value.isUndefined() ? JSValue(0) : value); } return {}; }, {count}, name, 0);
    auto prototype = ctor.get("prototype");
    if (count == 2) for (const auto* key : {"width", "height"}) { JSPropertyDescriptor d; d.configurable = true; d.getter = JSValue::function(method, {std::string("get ") + key, count}, std::string("get ") + key, 0); d.setter = JSValue::function(method, {std::string("set ") + key, count}, std::string("set ") + key, 1); prototype.defineProperty(key, d); }
    std::vector<std::pair<const char*,unsigned>> methods{{"set",count},{"setScalar",1},{"clone",0},{"copy",1},{"multiplyScalar",1},{"applyMatrix3",1},{"fromBufferAttribute",2},{"divideScalar",1},{"normalize",0}};
    if(count==3)for(const auto& entry:std::initializer_list<std::pair<const char*,unsigned>>{{"applyMatrix4",1},{"applyNormalMatrix",1},{"transformDirection",1}})methods.push_back(entry);
    methods.emplace_back("length",0);methods.emplace_back("equals",1);methods.emplace_back("fromArray",1);methods.emplace_back("toArray",0);
    if(count==3)for(const auto& entry:std::initializer_list<std::pair<const char*,unsigned>>{{"setFromMatrixPosition",1},{"setFromMatrixScale",1},{"setFromMatrixColumn",2}})methods.insert(methods.end()-3,entry);
    for (const auto& [methodName, arity] : methods) { JSPropertyDescriptor d; d.value = JSValue::function(method, {methodName, count}, methodName, arity); d.writable = d.configurable = true; prototype.defineProperty(methodName, d); }
    prototype.set(std::string("is") + name, true); ctor.retainForHeapLifetime(); weak[index] = JSWeakValue(ctor); return ctor;
}
}
JSValue nativeVector2Constructor() { return constructor(2); }
JSValue nativeVector3Constructor() { return constructor(3); }
