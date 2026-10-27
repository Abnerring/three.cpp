#include "animation/AnimationUtils.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/Sphere.h"
#include <cmath>
#include <iostream>
#include <limits>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& a,unsigned i){return i<a.size()?a[i]:JSValue{};}
JSValue call(const JSValue& self,const char* key,const Args& args={}){return self.get(key).call(self,args);}
JSValue array(const Args& values={}){auto out=JSValue::array();for(const auto& v:values)call(out,"push",{v});return out;}
JSValue ctor(const JSValue& name){if(name.strictEquals("Vector2"))return nativeVector2Constructor();if(name.strictEquals("Vector3"))return nativeVector3Constructor();if(name.strictEquals("Vector4"))return nativeVector4Constructor();if(name.strictEquals("Matrix3"))return nativeMatrix3Constructor();if(name.strictEquals("Matrix4"))return nativeMatrix4Constructor();return nativeSphereConstructor();}
JSValue normal(const JSValue& v){
    if(v.isUndefined())return "#undefined";if(v.isCallable())return "#function";
    if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return "#NaN";if(std::isinf(n))return n>0?"#Infinity":"#-Infinity";if(n==0&&std::signbit(n))return "#-0";}
    if(!v.isObject())return v;if(v.get("fixtureTag").toBoolean())return "@"+v.get("fixtureTag").toString();
    auto out=v.type()==JSValue::Type::Array?JSValue::array(v.length()):JSValue::object();for(const auto& k:v.keys())out.set(k,normal(v.get(k)));return out;
}
JSValue decode(const JSValue& v){
    if(!v.isObject())return v;const auto tag=v.get("tag");if(tag.strictEquals("undefined"))return {};if(tag.strictEquals("NaN"))return std::numeric_limits<double>::quiet_NaN();if(tag.strictEquals("Infinity"))return std::numeric_limits<double>::infinity();if(tag.strictEquals("-Infinity"))return -std::numeric_limits<double>::infinity();if(tag.strictEquals("-0"))return -0.0;
    if(v.type()==JSValue::Type::Array){auto out=array();for(unsigned i=0;i<v.length();++i)call(out,"push",{decode(v.get(std::to_string(i)))});return out;}
    if(tag.strictEquals("instance")){
        const auto C=ctor(v.get("class")),values=decode(v.get("values"));Args a;for(unsigned i=0;i<values.length();++i)a.push_back(values.get(std::to_string(i)));
        if(v.get("class").toString().starts_with("Matrix")){const auto instance=C.construct();if(!values.isUndefined())call(instance,"fromArray",{values});return instance;}
        return C.construct(a);
    }return v;
}
JSValue error(){try{throw;}catch(const JSTypeError&){return "#TypeError";}catch(const JSRangeError&){return "#RangeError";}catch(...){return "#Error";}}
void event(JSValue state,const Args& values){call(state.get("events"),"push",{array(values)});}
JSValue primitive(JSValue state,const std::string& label,JSValue value){
    auto out=JSValue::object();out.set("fixtureTag",label);out.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{
        event(c[0],{"primitive",c[1],arg(a,0)});if(c[0].get("request").get("throwPrimitive").strictEquals(c[1]))throw JSTypeError("primitive");return c[2];
    },{state,label,value},"",1));return out;
}
JSValue observed(JSValue state,const std::string& label,JSValue storage){
    auto handler=JSValue::object();handler.set("get",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{
        const auto key=arg(a,1);event(c[0],{c[1].toString()+".get",normal(key)});if(c[0].get("request").get("throwGet").strictEquals(c[1].toString()+"."+key.toString()))throw JSTypeError("get");return arg(a,0).get(key,arg(a,2));
    },{state,label},"get",3));
    handler.set("set",JSValue::function(+[](const JSValue&,const Args& a,const Args& c)->JSValue{
        const auto key=arg(a,1);event(c[0],{c[1].toString()+".set",normal(key),normal(arg(a,2))});if(c[0].get("request").get("throwSet").strictEquals(c[1].toString()+"."+key.toString()))throw JSTypeError("set");auto target=arg(a,0);target.set(key,arg(a,2));return true;
    },{state,label},"set",4));return JSValue::proxy(storage,handler);
}
struct Fixture{JSValue value,storage;};
Fixture matrix(JSValue state,const char* label,const JSValue& values,bool coerce){
    auto storage=decode(values);if(coerce)for(unsigned i=0;i<storage.length();++i)storage.set(std::to_string(i),primitive(state,std::string(label)+"."+std::to_string(i),storage.get(std::to_string(i))));
    auto proxy=observed(state,label,storage),object=JSValue::object();JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{event(c[0],{c[1].toString()+".elements"});return c[2];},{state,label,proxy},"get elements",0);object.defineProperty("elements",d);return {object,storage};
}
JSValue run(const JSValue& r){
    const auto C=ctor(r.get("class")),methodName=r.get("method");
    if(r.get("kind").strictEquals("metadata")){
        const auto d=C.get("prototype").getOwnPropertyDescriptor(methodName.toString());const auto fn=C.get("prototype").get(methodName);auto out=JSValue::object();out.set("name",fn.get("name"));out.set("length",fn.get("length"));out.set("constructible",fn.isConstructible());out.set("writable",d->writable.value_or(false));out.set("enumerable",d->enumerable.value_or(false));out.set("configurable",d->configurable.value_or(false));return out;
    }
    auto state=JSValue::object();state.set("events",array());state.set("request",r);auto out=JSValue::object();JSValue self,snapshot;Args args;
    if(r.get("kind").strictEquals("protocol")){
        const bool coerce=r.get("coerce").toBoolean();const auto first=matrix(state,"a",r.get("a"),coerce),second=matrix(state,"b",r.get("b"),coerce);
        if(r.get("class").toString().starts_with("Matrix")){
            auto target=matrix(state,"self",r.get("self"),coerce);self=target.value;self.setPrototype(C.get("prototype"));snapshot=target.storage;
            if(methodName.strictEquals("multiplyMatrices"))args={r.get("alias").strictEquals("left")?self:first.value,r.get("alias").strictEquals("right")?self:second.value};
            else if(methodName.strictEquals("multiply")||methodName.strictEquals("premultiply")||methodName.strictEquals("setFromMatrix4")||methodName.strictEquals("getNormalMatrix"))args={first.value};
        }else{
            auto storage=JSValue::object(C.get("prototype"));const auto values=decode(r.get("self"));const unsigned width=r.get("class").strictEquals("Vector2")?2:r.get("class").strictEquals("Vector3")?3:4;
            for(unsigned i=0;i<width;++i){const std::string key(1,"xyzw"[i]);const auto v=values.get(std::to_string(i));storage.set(key,coerce?primitive(state,"self."+key,v):v);}self=observed(state,"self",storage);snapshot=storage;
            if(methodName.strictEquals("multiplyScalar"))args={coerce?primitive(state,"scalar",decode(r.get("scalar"))):decode(r.get("scalar"))};
            else if(methodName.strictEquals("setFromMatrixColumn"))args={first.value,decode(r.get("index"))};
            else if(!methodName.strictEquals("length"))args={first.value};
        }
    }else{
        self=decode(r.get("self"));snapshot=self;const auto values=r.get("args");for(unsigned i=0;i<values.length();++i)args.push_back(decode(values.get(std::to_string(i))));
        if(r.get("alias").strictEquals("left"))args[0]=self;else if(r.get("alias").strictEquals("right"))args[1]=self;
    }
    try{const auto value=C.get("prototype").get(methodName).call(self,args);out.set("result",value.sameValue(self)?JSValue("@self"):normal(value));}catch(...){out.set("result",error());}
    out.set("remaining",normal(snapshot));out.set("events",state.get("events"));return out;
}
}
int main(){std::string line;while(std::getline(std::cin,line)){try{std::cout<<run(Animation_detail::parseJSON(line)).toJSON()->dump()<<'\n';}catch(const std::exception& e){auto out=JSValue::object();out.set("probeError",e.what());std::cout<<out.toJSON()->dump()<<'\n';}}}
