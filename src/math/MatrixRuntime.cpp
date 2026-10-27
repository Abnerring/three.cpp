#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "core/JSModuleObject.h"
#include "math/JSNumberMath.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace {
#include "math/Matrix4RotationSource.inc"
using Args = std::vector<JSValue>;
JSValue arg(const Args& args, unsigned index) { return index < args.size() ? args[index] : JSValue{}; }
JSValue add(const JSValue& a, const JSValue& b) {
    const auto left = a.toPrimitive(); const auto right = b.toPrimitive();
    if (left.type() == JSValue::Type::String || right.type() == JSValue::Type::String)
        return left.toString() + right.toString();
    return left.toNumber() + right.toNumber();
}
double product(const JSValue& a,const JSValue& b){const auto left=a.toNumber();const auto right=b.toNumber();return left*right;}
double difference(const JSValue& a,const JSValue& b,const JSValue& c,const JSValue& d){const auto first=product(a,b);const auto second=product(c,d);return first-second;}
double triple(const JSValue& a,double b,const JSValue& c,double d,const JSValue& e,double f,bool subtractSecond=false,bool subtractThird=false){
    const auto first=product(a,b),second=product(c,d);const auto partial=subtractSecond?first-second:first+second;const auto third=product(e,f);return subtractThird?partial-third:partial+third;
}
Args identity(unsigned width) {
    Args values; for (unsigned i = 0; i < width * width; ++i) values.emplace_back(i % (width + 1) == 0 ? 1 : 0);
    return values;
}
JSValue rotationFromEuler(const JSValue& self,const Args& args,const Args&) {
    auto te=self.get("elements");
    const auto euler=arg(args,0);
    const auto x=euler.get("x"),y=euler.get("y"),z=euler.get("z");
    const auto a=JSNumberMath::cos(x.toNumber()),b=JSNumberMath::sin(x.toNumber());
    const auto c=JSNumberMath::cos(y.toNumber()),d=JSNumberMath::sin(y.toNumber());
    const auto e=JSNumberMath::cos(z.toNumber()),f=JSNumberMath::sin(z.toNumber());
    if(euler.get("order").strictEquals("XYZ")) {
        const auto ae=a*e,af=a*f,be=b*e,bf=b*f;
        te.set("0",c*e);te.set("4",-c*f);te.set("8",d);
        te.set("1",af+be*d);te.set("5",ae-bf*d);te.set("9",-b*c);
        te.set("2",bf-ae*d);te.set("6",be+af*d);te.set("10",a*c);
    } else if(euler.get("order").strictEquals("YXZ")) {
        const auto ce=c*e,cf=c*f,de=d*e,df=d*f;
        te.set("0",ce+df*b);te.set("4",de*b-cf);te.set("8",a*d);
        te.set("1",a*f);te.set("5",a*e);te.set("9",-b);
        te.set("2",cf*b-de);te.set("6",df+ce*b);te.set("10",a*c);
    } else if(euler.get("order").strictEquals("ZXY")) {
        const auto ce=c*e,cf=c*f,de=d*e,df=d*f;
        te.set("0",ce-df*b);te.set("4",-a*f);te.set("8",de+cf*b);
        te.set("1",cf+de*b);te.set("5",a*e);te.set("9",df-ce*b);
        te.set("2",-a*d);te.set("6",b);te.set("10",a*c);
    } else if(euler.get("order").strictEquals("ZYX")) {
        const auto ae=a*e,af=a*f,be=b*e,bf=b*f;
        te.set("0",c*e);te.set("4",be*d-af);te.set("8",ae*d+bf);
        te.set("1",c*f);te.set("5",bf*d+ae);te.set("9",af*d-be);
        te.set("2",-d);te.set("6",b*c);te.set("10",a*c);
    } else if(euler.get("order").strictEquals("YZX")) {
        const auto ac=a*c,ad=a*d,bc=b*c,bd=b*d;
        te.set("0",c*e);te.set("4",bd-ac*f);te.set("8",bc*f+ad);
        te.set("1",f);te.set("5",a*e);te.set("9",-b*e);
        te.set("2",-d*e);te.set("6",ad*f+bc);te.set("10",ac-bd*f);
    } else if(euler.get("order").strictEquals("XZY")) {
        const auto ac=a*c,ad=a*d,bc=b*c,bd=b*d;
        te.set("0",c*e);te.set("4",-f);te.set("8",d*e);
        te.set("1",ac*f+bd);te.set("5",a*e);te.set("9",ad*f-bc);
        te.set("2",bc*f-ad);te.set("6",b*e);te.set("10",bd*f+ac);
    }
    for(const auto* key:{"3","7","11","12","13","14"})te.set(key,0);
    te.set("15",1);
    return self;
}
JSValue method(const JSValue& receiver, const Args& args, const Args& captures) {
    auto self = receiver; const auto name = captures[0].string();
    const auto width = static_cast<unsigned>(captures[1].number()), count = width * width;
    if (width == 4 && name == "makeRotationFromEuler") return rotationFromEuler(self, args, {});
    if (name == "identity") {
        const auto fn = self.get("set"); fn.call(self, identity(width)); return self;
    }
    if (name == "clone") {
        // Matrix3 uses this.constructor; Matrix4 closes over its class binding.
        const auto ctor = width == 3 ? self.get("constructor") : nativeMatrix4Constructor();
        const auto matrix = ctor.construct(); const auto fn = matrix.get("fromArray");
        const auto elements = self.get("elements"); return fn.call(matrix, {elements});
    }
    if (name == "set") {
        auto elements = self.get("elements");
        for (unsigned i = 0; i < count; ++i) {
            const auto index = width == 3 ? i : (i % width) * width + i / width;
            const auto parameter = width == 3 ? (i % width) * width + i / width : i;
            elements.set(std::to_string(index), arg(args, parameter));
        }
        return self;
    }
    if (name == "copy" || name == "equals") {
        auto elements = self.get("elements"); const auto source = arg(args, 0).get("elements");
        for (unsigned i = 0; i < count; ++i) {
            const auto key = std::to_string(i);
            if (name == "copy") elements.set(key, source.get(key));
            else { const auto left = elements.get(key); if (!left.strictEquals(source.get(key))) return false; }
        }
        return name == "copy" ? self : JSValue(true);
    }
    if(name=="multiply"||name=="premultiply"){
        const auto fn=self.get("multiplyMatrices");return fn.call(self,name=="multiply"?Args{self,arg(args,0)}:Args{arg(args,0),self});
    }
    if(name=="multiplyMatrices"){
        const auto ae=arg(args,0).get("elements");const auto be=arg(args,1).get("elements");auto te=self.get("elements");
        JSValue a[4][4],b[4][4];
        for(unsigned row=0;row<width;++row)for(unsigned column=0;column<width;++column)a[row][column]=ae.get(std::to_string(row+column*width));
        for(unsigned row=0;row<width;++row)for(unsigned column=0;column<width;++column)b[row][column]=be.get(std::to_string(row+column*width));
        for(unsigned row=0;row<width;++row)for(unsigned column=0;column<width;++column){
            const auto first=product(a[row][0],b[0][column]);const auto second=product(a[row][1],b[1][column]);
            const auto partial=first+second;const auto third=product(a[row][2],b[2][column]);auto value=partial+third;
            if(width==4){const auto fourth=product(a[row][3],b[3][column]);value=value+fourth;}
            te.set(std::to_string(row+column*width),value);
        }
        return self;
    }
    if(width==3&&name=="setFromMatrix4"){
        const auto me=arg(args,0).get("elements"),fn=self.get("set");Args values;
        for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)values.push_back(me.get(std::to_string(row+col*4)));
        fn.call(self,values);return self;
    }
    if(width==3&&name=="getNormalMatrix"){
        const auto set=self.get("setFromMatrix4").call(self,{arg(args,0)});const auto inverse=set.get("invert").call(set);return inverse.get("transpose").call(inverse);
    }
    if(name=="transpose"){
        auto elements=self.get("elements");
        const auto swap=[&](unsigned a,unsigned b){const auto value=elements.get(std::to_string(a));elements.set(std::to_string(a),elements.get(std::to_string(b)));elements.set(std::to_string(b),value);};
        if(width==3){swap(1,3);swap(2,6);swap(5,7);}else{swap(1,4);swap(2,8);swap(6,9);swap(3,12);swap(7,13);swap(11,14);}return self;
    }
    if(width==3&&name=="invert"){
        auto te=self.get("elements");JSValue n[9];for(unsigned i=0;i<9;++i)n[i]=te.get(std::to_string(i));
        const auto t11=difference(n[8],n[4],n[5],n[7]),t12=difference(n[5],n[6],n[8],n[3]),t13=difference(n[7],n[3],n[4],n[6]);
        const auto det=triple(n[0],t11,n[1],t12,n[2],t13);
        if(det==0)return self.get("set").call(self,Args(9,0));
        const auto inverse=1/det;
        te.set("0",t11*inverse);te.set("1",difference(n[2],n[7],n[8],n[1])*inverse);te.set("2",difference(n[5],n[1],n[2],n[4])*inverse);
        te.set("3",t12*inverse);te.set("4",difference(n[8],n[0],n[2],n[6])*inverse);te.set("5",difference(n[2],n[3],n[5],n[0])*inverse);
        te.set("6",t13*inverse);te.set("7",difference(n[1],n[6],n[7],n[0])*inverse);te.set("8",difference(n[4],n[0],n[1],n[3])*inverse);return self;
    }
    if(width==4&&name=="invert"){
        auto te=self.get("elements");JSValue n[16];for(unsigned i=0;i<16;++i)n[i]=te.get(std::to_string(i));
        const auto t1=difference(n[0],n[5],n[1],n[4]),t2=difference(n[0],n[6],n[2],n[4]),t3=difference(n[0],n[7],n[3],n[4]);
        const auto t4=difference(n[1],n[6],n[2],n[5]),t5=difference(n[1],n[7],n[3],n[5]),t6=difference(n[2],n[7],n[3],n[6]);
        const auto t7=difference(n[8],n[13],n[9],n[12]),t8=difference(n[8],n[14],n[10],n[12]),t9=difference(n[8],n[15],n[11],n[12]);
        const auto t10=difference(n[9],n[14],n[10],n[13]),t11=difference(n[9],n[15],n[11],n[13]),t12=difference(n[10],n[15],n[11],n[14]);
        const auto det=t1*t12-t2*t11+t3*t10+t4*t9-t5*t8+t6*t7;
        if(det==0)return self.get("set").call(self,Args(16,0));
        const auto inverse=1/det;
        te.set("0",triple(n[5],t12,n[6],t11,n[7],t10,true)*inverse);
        te.set("1",triple(n[2],t11,n[1],t12,n[3],t10,true,true)*inverse);
        te.set("2",triple(n[13],t6,n[14],t5,n[15],t4,true)*inverse);
        te.set("3",triple(n[10],t5,n[9],t6,n[11],t4,true,true)*inverse);
        te.set("4",triple(n[6],t9,n[4],t12,n[7],t8,true,true)*inverse);
        te.set("5",triple(n[0],t12,n[2],t9,n[3],t8,true)*inverse);
        te.set("6",triple(n[14],t3,n[12],t6,n[15],t2,true,true)*inverse);
        te.set("7",triple(n[8],t6,n[10],t3,n[11],t2,true)*inverse);
        te.set("8",triple(n[4],t11,n[5],t9,n[7],t7,true)*inverse);
        te.set("9",triple(n[1],t9,n[0],t11,n[3],t7,true,true)*inverse);
        te.set("10",triple(n[12],t5,n[13],t3,n[15],t1,true)*inverse);
        te.set("11",triple(n[9],t3,n[8],t5,n[11],t1,true,true)*inverse);
        te.set("12",triple(n[5],t8,n[4],t10,n[6],t7,true,true)*inverse);
        te.set("13",triple(n[0],t10,n[1],t8,n[2],t7,true)*inverse);
        te.set("14",triple(n[13],t2,n[12],t4,n[14],t1,true,true)*inverse);
        te.set("15",triple(n[8],t4,n[9],t2,n[10],t1,true)*inverse);return self;
    }
    if(width==4&&name=="getMaxScaleOnAxis"){
        const auto elements=self.get("elements");double scales[3];
        for(unsigned col=0;col<3;++col){double sum=0;for(unsigned row=0;row<3;++row){const auto key=std::to_string(col*4+row);const auto left=elements.get(key),right=elements.get(key);const auto square=product(left,right);sum=row?sum+square:square;}scales[col]=sum;}
        if(std::isnan(scales[0])||std::isnan(scales[1])||std::isnan(scales[2]))return std::numeric_limits<double>::quiet_NaN();
        return std::sqrt(std::max({scales[0],scales[1],scales[2]}));
    }
    const auto rawOffset = arg(args, 1), offset = rawOffset.isUndefined() ? JSValue(0) : rawOffset;
    auto array = arg(args, 0);
    if (name == "fromArray") {
        for (unsigned i = 0; i < count; ++i) {
            // The property reference is evaluated before the index and RHS on every iteration.
            auto elements = self.get("elements");
            const auto value = array.get(add(i, offset).toPropertyKey());
            elements.set(std::to_string(i), value);
        }
        return self;
    }
    if (name == "toArray") {
        if (array.isUndefined()) array = JSValue::array();
        const auto elements = self.get("elements");
        for (unsigned i = 0; i < count; ++i) {
            const auto key = i == 0 ? offset : add(offset, i);
            const auto value = elements.get(std::to_string(i)); array.set(key, value);
        }
        return array;
    }
    throw std::logic_error("Unknown managed matrix method");
}
JSValue constructor(unsigned width) {
    static thread_local JSWeakValue weak[2]; static thread_local bool initializing[2]{};
    const auto index = width - 3;
    if (const auto ctor = weak[index].lock(); ctor && ctor->isConstructible()) return *ctor;
    const auto name = width == 3 ? "Matrix3" : "Matrix4";
    JSModuleInitializationScope scope(initializing[index], name);
    auto ctor = JSValue::constructor([](const JSValue& receiver, const Args& args, const Args& captures) -> JSValue {
        auto self = receiver; const auto width = static_cast<unsigned>(captures[0].number()), count = width * width;
        auto elements = JSValue::array(count); const auto values = identity(width);
        for (unsigned i = 0; i < count; ++i) {
            JSPropertyDescriptor d; d.value = values[i]; d.writable = d.enumerable = d.configurable = true;
            elements.defineProperty(std::to_string(i), d);
        }
        self.set("elements", elements);
        if (!arg(args, 0).isUndefined()) {
            const auto fn = self.get("set"); Args values;
            for (unsigned i = 0; i < count; ++i) values.push_back(arg(args, i));
            fn.call(self, values);
        }
        return {};
    }, {width}, name, width * width);
    auto prototype = ctor.get("prototype");
    // Preserve source order among the implemented class methods.
    const auto methods = width == 3
        ? std::vector<std::pair<const char*, unsigned>>{{"set",9},{"identity",0},{"copy",1},{"setFromMatrix4",1},{"multiply",1},{"premultiply",1},{"multiplyMatrices",2},{"invert",0},{"transpose",0},{"getNormalMatrix",1},{"equals",1},{"fromArray",1},{"toArray",0},{"clone",0}}
        : std::vector<std::pair<const char*, unsigned>>{{"set",16},{"identity",0},{"clone",0},{"copy",1},{"makeRotationFromEuler",1},{"multiply",1},{"premultiply",1},{"multiplyMatrices",2},{"transpose",0},{"invert",0},{"getMaxScaleOnAxis",0},{"equals",1},{"fromArray",1},{"toArray",0}};
    for (const auto& [key, arity] : methods) {
        JSPropertyDescriptor d; d.value = JSValue::function(method, {key,width}, key, arity);
        if(width==4&&std::string(key)=="makeRotationFromEuler")d.value->setFunctionSourceText(matrix4RotationSource);
        d.writable = d.configurable = true; prototype.defineProperty(key, d);
    }
    prototype.set(std::string("is") + name, true);
    ctor.retainForHeapLifetime(); weak[index] = JSWeakValue(ctor); return ctor;
}
}
JSValue nativeMatrix3Constructor() { return constructor(3); }
JSValue nativeMatrix4Constructor() { return constructor(4); }
