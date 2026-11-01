#include "math/MathUtils.h"
#include "core/JSGlobal.h"
#include "core/JSNumber.h"
#include "core/JSPromise.h"
#include "core/JSError.h"
#include "animation/AnimationUtils.h"
#include <iostream>
#include <iterator>
#include <cmath>
namespace {
using Args=std::vector<JSValue>;
void push(JSValue a,const JSValue&v){a.set(std::to_string(a.length()),v);}
JSValue array(const Args&a){auto out=JSValue::array();for(const auto&v:a)push(out,v);return out;}
JSValue scalar(const JSValue&v){if(v.type()==JSValue::Type::Number){if(std::isnan(v.number()))return "#NaN";if(std::isinf(v.number()))return v.number()>0?"#Infinity":"#-Infinity";if(v.number()==0&&std::signbit(v.number()))return "#-0";}return v;}
JSValue caught(){try{throw;}catch(const JSThrownValue&e){return e.value;}catch(...){return nativeExceptionValue(std::current_exception());}}
JSValue encode(const JSValue&e){return (isNativeError(e)||(e.isObject()&&e.has("name")&&e.has("message")))?JSValue("#"+e.get("name").toString()):e;}
}
int main(int argc,char**){try{if(argc!=1)throw std::invalid_argument("JSON stdin only");const std::string input(std::istreambuf_iterator<char>(std::cin),{});const auto q=Animation_detail::parseJSON(input);auto global=nativeGlobalObject(),originalMath=nativeGlobalBinding("Math"),events=JSValue::array(),state=JSValue::object(),token=JSValue::object();token.set("tag","failure");state.set("events",events);state.set("gets",0);state.set("request",q);state.set("token",token);
 auto first=JSValue::object(originalMath),second=JSValue::object(originalMath);state.set("current",first);state.set("second",second);for(const auto&entry:std::initializer_list<std::pair<JSValue,int>>{{first,1},{second,2}}){auto object=entry.first;JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{auto s=c[0];const auto which=c[1];const auto n=s.get("gets").number()+1;s.set("gets",n);push(s.get("events"),"PI:"+which.toString());const auto q=s.get("request");if(q.get("throwAt").strictEquals(n))throw JSThrownValue(s.get("token"));if(q.get("symbolAt").strictEquals(n))return JSValue::symbol("pi");if(q.get("kind").strictEquals("replaceOnConvert")&&which.strictEquals(1)){auto result=JSValue::object();result.set(JSValue::wellKnownSymbol("toPrimitive"),JSValue::function(+[](const JSValue&,const Args&a,const Args&c)->JSValue{auto s=c[0];push(s.get("events"),"convert:"+a[0].toString());s.set("current",s.get("second"));return 3;},{s}));return result;}if(q.get("kind").strictEquals("coercion"))return which.strictEquals(1)?JSValue("6"):JSValue(nullptr);return which.strictEquals(1)?3:6;},{state,entry.second});object.defineProperty("PI",d);}
 if(q.get("kind").strictEquals("twoValues")){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{auto s=c[0];const auto n=s.get("gets").number()+1;s.set("gets",n);push(s.get("events"),"PI:"+JSValue(n).toString());return n==1?3:6;},{state});first.defineProperty("PI",d);}
 JSPropertyDescriptor math;math.configurable=true;math.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{push(c[0].get("events"),"Math");return c[0].get("current");},{state});global.defineProperty("Math",math);auto numberPrototype=nativeNumberPrototype();const auto previous=numberPrototype.get("toString");numberPrototype.set("toString",JSValue::function(+[](const JSValue&self,const Args&a,const Args&c)->JSValue{push(c[0],"Number.toString");return c[1].call(self,a);},{events,previous}));
 auto out=JSValue::object();try{const auto fn=q.get("factory").strictEquals("generateUUID")?nativeMathUtilsGenerateUUID():nativeMathUtilsClamp();const auto DEG=nativeMathUtilsBinding("DEG2RAD"),RAD=nativeMathUtilsBinding("RAD2DEG");out.set("constants",array({scalar(DEG),scalar(RAD)}));out.set("identity",fn.sameValue(nativeMathUtilsBinding("MathUtils").get(q.get("factory").isUndefined()?JSValue("clamp"):q.get("factory"))));out.set("constantStable",nativeMathUtilsBinding("degToRad").call({}, {1}).strictEquals(DEG));}catch(...){const auto firstError=caught();out.set("error",encode(firstError));try{nativeMathUtilsModule();}catch(...){const auto secondError=caught();out.set("secondError",encode(secondError));out.set("sameError",firstError.sameValue(secondError));}}
 out.set("events",events);std::cout<<out.toJSON()->dump();
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
