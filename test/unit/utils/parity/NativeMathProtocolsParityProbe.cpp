#include "core/NativeObject.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Color.h"
#include "math/Matrix4.h"
#include <cmath>
#include <iostream>
namespace {
using Args=std::vector<JSValue>;using O=JsonValue::Object;using A=JsonValue::Array;
void data(JSValue o,std::string k,JSValue v){JSPropertyDescriptor d;d.value=v;d.writable=d.enumerable=d.configurable=true;o.defineProperty(k,d);}
JSValue arr(Args a={}){auto v=JSValue::array();for(std::size_t i=0;i<a.size();++i)v.set(std::to_string(i),a[i]);return v;}
void log(JSValue t,Args a){t.set(std::to_string(t.length()),arr(a));}
JsonValue encode(JSValue v){if(v.isUndefined())return "@undefined";if(v.isNull())return nullptr;if(v.isCallable())return "@function";if(v.type()==JSValue::Type::Number){auto n=v.number();if(std::isnan(n))return "@NaN";if(n==0&&std::signbit(n))return "@-0";return n;}if(v.type()==JSValue::Type::String)return v.string();if(v.type()==JSValue::Type::Boolean)return v.toBoolean();if(v.type()==JSValue::Type::Array){A out;for(unsigned i=0;i<v.length();++i)out.push_back(encode(v.get(std::to_string(i))));return out;}O out;for(const auto& k:v.keys())out[k]=encode(v.get(k));return out;}
template<class F>JsonValue attempt(F f){try{return O{{"value",encode(f())}};}catch(const JSTypeError&){return O{{"error","TypeError"}};}catch(const JSRangeError&){return O{{"error","RangeError"}};}}
void accessor(JSValue o,std::string key,JSValue store,JSValue trace,std::string label,int throws=-1){JSPropertyDescriptor d;d.enumerable=d.configurable=true;d.getter=JSValue::function([](const JSValue& self,const Args&,const Args& c)->JSValue{log(c[1],{"get",c[2],c[3],self.sameValue(c[4])});return c[0].get(c[3].string());},{store,trace,label,key,o});d.setter=JSValue::function([](const JSValue& self,const Args& a,const Args& c)->JSValue{log(c[1],{"set",c[2],c[3],a[0],self.sameValue(c[4])});if(c[5].strictEquals(c[3]))throw JSRangeError("write");auto store=c[0];store.set(c[3].string(),a[0]);return {};},{store,trace,label,key,o,throws<0?JSValue():JSValue(std::to_string(throws))});o.defineProperty(key,d);}
JSValue number(JSValue trace,std::string label,JSValue value){auto o=JSValue::object();data(o,"valueOf",JSValue::function([](const JSValue&,const Args&,const Args& c){log(c[0],{"number",c[1]});return c[2];},{trace,label,value}));return o;}
JsonValue vectors(int type,int mode){const char* comps=type==2?"rgb":type==0?"xy":"xyz";auto native=type==2?nativeColor(std::make_shared<Color>()):type==0?nativeVector(std::make_shared<Vector2>()):nativeVector(std::make_shared<Vector3>());const char* methods[]={"copy","fromArray","toArray","multiplyScalar","multiply","sub","addScaledVector","length","lengthSq","divideScalar","normalize","set","setFromMatrixPosition","setFromMatrixColumn","applyMatrix4","transformDirection"};const auto name=methods[mode];auto trace=JSValue::array(),self=JSValue::object(),source=JSValue::object(),store=JSValue::object(),sourceStore=JSValue::object();for(int i=0;comps[i];++i){const std::string key(1,comps[i]);data(store,key,i==0?JSValue(-0.):JSValue(i+2));data(sourceStore,key,number(trace,"source"+key,i+4));accessor(self,key,store,trace,"self");accessor(source,key,sourceStore,trace,"source");}for(const auto* method:methods){const auto fn=native.get(method);if(fn.isCallable())data(self,method,fn);}auto array=arr({7,8,9,10,11}),matrix=JSValue::object();Args values;for(int i=0;i<16;++i)values.emplace_back(i%5==0?1.:i*.05);data(matrix,"elements",arr(values));Args args;if(mode==0||mode==4||mode==5)args={source};if(mode==1)args={array,number(trace,"offset",1)};if(mode==2)args={array,"1"};if(mode==3||mode==9)args={number(trace,"scalar",3)};if(mode==6)args={source,number(trace,"scale",2)};if(mode==11)args={number(trace,"x",1),number(trace,"y",2)};if(mode>=12)args={matrix};if(mode==13)args.push_back(number(trace,"column",1));auto fn=native.get(name);const auto result=attempt([&]()->JSValue{const auto r=fn.call(self,args);return r.sameValue(self)?JSValue("self"):r;});return O{{"kind","vector"},{"type",type},{"method",name},{"result",result},{"store",encode(store)},{"array",encode(array)},{"trace",encode(trace)},{"name",encode(fn.get("name"))},{"length",encode(fn.get("length"))},{"constructible",fn.isConstructible()}};}
JsonValue matrices(int mode){const char* methods[]={"set","identity","copy","multiply","premultiply","multiplyMatrices","determinantAffine","extractRotation"};const auto name=methods[mode];auto trace=JSValue::array(),self=JSValue::object(),a=JSValue::object(),b=JSValue::object(),out=JSValue::array(),av=JSValue::array(),bv=JSValue::array(),stores=JSValue::array();auto native=nativeMatrix(std::make_shared<Matrix4>());for(int which=0;which<3;++which){auto store=JSValue::object();auto target=which==0?out:which==1?av:bv;for(int i=0;i<16;++i){data(store,std::to_string(i),which==0?0:JSValue(i%5==0?double(which):i*.1));accessor(target,std::to_string(i),store,trace,std::to_string(which));}stores.set(std::to_string(which),store);}data(self,"elements",out);data(a,"elements",av);data(b,"elements",bv);for(const auto* method:methods)data(self,method,native.get(method));data(a,"determinantAffine",native.get("determinantAffine"));Args args;if(mode==0)for(int i=0;i<16;++i)args.emplace_back(i+1);else if(mode==2||mode==3||mode==4||mode==7)args={a};else if(mode==5)args={a,b};if(mode==3||mode==4||mode==6){for(int i=0;i<16;++i)stores.get("0").set(std::to_string(i),i%5==0?JSValue(1):JSValue(i*.05));}const auto fn=native.get(name);const auto result=attempt([&]()->JSValue{const auto r=fn.call(self,args);return r.sameValue(self)?JSValue("self"):r;});return O{{"kind","matrix"},{"method",name},{"result",result},{"store",encode(stores.get("0"))},{"trace",encode(trace)},{"name",encode(fn.get("name"))},{"length",encode(fn.get("length"))},{"constructible",fn.isConstructible()}};}

JsonValue arrayOffsets(int type,int mode) {
    const char* comps=type==2?"rgb":type==0?"xy":"xyz";
    const int count=type==3?16:type==0?2:3;
    auto native=type==3?nativeMatrix(std::make_shared<Matrix4>()):type==2?nativeColor(std::make_shared<Color>()):type==0?nativeVector(std::make_shared<Vector2>()):nativeVector(std::make_shared<Vector3>());
    auto trace=JSValue::array(),self=JSValue::object(),store=JSValue::object(),elements=JSValue::array(),dest=JSValue::object(),offset=JSValue::object(),state=JSValue::object();
    data(state,"calls",0);
    for(int i=0;i<count;++i) {
        const auto key=type==3?std::to_string(i):std::string(1,comps[i]);data(store,key,i+1);
        accessor(type==3?elements:self,key,store,trace,type==3?"elements":"self");
    }
    if(type==3)data(self,"elements",elements);
    const Args captures{trace,store,state,type,mode,count};
    data(offset,"toString",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
        log(c[0],{"string","offset"});auto store=c[1];const auto type=int(c[3].number());const char* comps=type==2?"rgb":type==0?"xy":"xyz";
        for(int i=0;i<c[5].number();++i)store.set(type==3?std::to_string(i):std::string(1,comps[i]),99);
        if(c[4].number()==2)throw JSRangeError("first key");return "1";
    },captures));
    data(offset,"valueOf",JSValue::function([](const JSValue&,const Args&,const Args& c)->JSValue {
        auto state=c[2];const auto calls=state.get("calls").number()+1;state.set("calls",calls);log(c[0],{"number","offset",calls});
        auto store=c[1];const auto type=int(c[3].number());const char* comps=type==2?"rgb":type==0?"xy":"xyz";
        for(int i=0;i<c[5].number();++i)store.set(type==3?std::to_string(i):std::string(1,comps[i]),200+calls);
        if(c[4].number()==3)throw JSRangeError("later addition");return c[4].number()==1?JSValue("1"):JSValue(1);
    },captures));
    const auto result=attempt([&]{return native.get("toArray").call(self,{dest,offset});});
    return O{{"kind","arrayOffsets"},{"type",type},{"mode",mode},{"result",result},{"store",encode(store)},{"trace",encode(trace)}};
}
}
int main(){try{A results;for(int type=0;type<3;++type)for(int mode=0;mode<16;++mode){if(type==2&&mode!=0&&mode!=1&&mode!=2&&mode!=3)continue;if(type==0&&mode>=12)continue;results.push_back(vectors(type,mode));}for(int mode=0;mode<8;++mode)results.push_back(matrices(mode));for(int type=0;type<4;++type)for(int mode=0;mode<4;++mode)results.push_back(arrayOffsets(type,mode));std::cout<<JsonValue(results).dump();}catch(const std::exception& e){std::cerr<<e.what();return 1;}}

