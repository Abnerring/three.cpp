#include "core/JSObject.h"
#include "core/JSNumber.h"
#include "core/JSBoolean.h"
#include "core/JSSymbol.h"
#include "core/JSStringEncoding.h"
#include <cmath>
#include <iostream>
#include <string>

namespace {
using Args=std::vector<JSValue>;using O=JsonValue::Object;using A=JsonValue::Array;
JSValue call(const JSValue& o,const char* name,const Args& args={}){return o.get(name).call(o,args);}
void own(JSValue o,const JSValue& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;o.defineProperty(key,d);}
JSValue array(const Args& values={}){auto a=JSValue::array();for(std::size_t i=0;i<values.size();++i)own(a,std::to_string(i),values[i]);return a;}
void push(JSValue a,const JSValue& v){own(a,std::to_string(a.length()),v);}
JsonValue encode(const JSValue& v,const Args& symbols){
    if(v.isUndefined())return O{{"type","undefined"}};if(v.isNull())return nullptr;if(v.isCallable())return O{{"type","function"}};
    if(v.isSymbol()){for(std::size_t i=0;i<symbols.size();++i)if(v.sameValue(symbols[i]))return O{{"symbol",i}};throw std::runtime_error("Unexpected Symbol identity in output");}
    if(v.type()==JSValue::Type::Boolean)return v.toBoolean();
    if(v.type()==JSValue::Type::Number){const auto n=v.number();if(std::isnan(n))return O{{"number","NaN"}};if(n==0&&std::signbit(n))return O{{"number","-0"}};if(!std::isfinite(n))return O{{"number",n<0?"-Infinity":"Infinity"}};return n;}
    if(v.type()==JSValue::Type::String){A units;for(const auto c:jsUTF16(v.string()))units.push_back(c);return O{{"stringUTF16",units}};}
    A values;if(v.type()==JSValue::Type::Array){for(unsigned i=0;i<v.length();++i)values.push_back(encode(v.get(JSValue(i)),symbols));return O{{"array",values}};}
    for(const auto& key:v.ownPropertyKeys()){const auto d=v.getOwnPropertyDescriptor(key);if(d&&d->enumerable.value_or(false))values.push_back(A{encode(key,symbols),encode(v.get(key),symbols)});}return O{{"object",values}};
}
JsonValue snapshot(const JSValue& object,const Args& symbols){A result;for(const auto& key:object.ownPropertyKeys()){const auto d=object.getOwnPropertyDescriptor(key);O descriptor;if(d->value){descriptor["value"]=encode(*d->value,symbols);descriptor["writable"]=*d->writable;}else{descriptor["get"]=encode(*d->getter,symbols);descriptor["set"]=encode(*d->setter,symbols);}descriptor["enumerable"]=*d->enumerable;descriptor["configurable"]=*d->configurable;result.push_back(A{encode(key,symbols),descriptor});}return result;}
const char* labels[]={"undefined","null","false","true","zero","negative-zero","number","NaN","Infinity","empty-string","string","astral-string","symbol","object","array"};
JSValue input(unsigned index,const JSValue& symbol){switch(index){case 0:return {};case 1:return nullptr;case 2:return false;case 3:return true;case 4:return 0;case 5:return -0.;case 6:return 3;case 7:return std::nan("");case 8:return INFINITY;case 9:return "";case 10:return "ab";case 11:return "a\xf0\x9f\x98\x80" "b";case 12:return symbol;case 13:{auto v=JSValue::object();own(v,"k",5);return v;}default:return array({5,6});}}
JSValue descriptorValue(double value){auto d=JSValue::object();own(d,"value",value);own(d,"writable",true);own(d,"enumerable",true);own(d,"configurable",true);return d;}
JSValue keyObject(JSValue log,const JSValue& symbol,unsigned mode){auto key=JSValue::object();JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"get");c[0].collectGarbage();if(c[2].number()==2)throw JSRangeError("key getter");if(c[2].number()==4)return 7;return JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{push(c[0],args[0]);c[0].collectGarbage();if(c[2].number()==3)throw JSRangeError("key call");if(c[2].number()==5)return JSValue::object();return c[2].number()==0?JSValue("0"):c[1];},c);},{log,symbol,mode});key.defineProperty(JSSymbol::toPrimitive(),d);return key;}
template<class F>JsonValue attempt(F&& f,const Args& symbols){try{return encode(f(),symbols);}catch(const JSTypeError&){return O{{"error","TypeError"}};}catch(const JSRangeError&){return O{{"error","RangeError"}};}}
void emit(const JsonValue& v){std::cout<<v.dump()<<'\n';}
void primitiveCases(const JSValue& object){
    const char* methods[]={"assign-target","assign-source","keys","values","getPrototypeOf","defineProperties-source","getOwnPropertyNames","getOwnPropertyDescriptor","getOwnPropertyDescriptors","defineProperty-target","defineProperties-target"};
    for(unsigned i=0;i<15;++i)for(const auto* method:methods){
        const auto symbol=JSSymbol::create("fixture"),sourceSymbol=JSSymbol::create("source");const Args symbols{symbol,sourceSymbol};auto value=input(i,symbol);const std::string m=method;
        JsonValue result;try{
            JSValue output;
            if(m=="assign-target"){auto source=JSValue::object();own(source,"a",1);own(source,sourceSymbol,2);output=call(object,"assign",{value,source});result=O{{"value",encode(output,symbols)},{"ownDescriptors",snapshot(output,symbols)},{"valueOfSame",call(output,"valueOf").sameValue(value)},{"sameTarget",output.sameValue(value)}};}
            else if(m=="assign-source")result=encode(call(object,"assign",{JSValue::object(),value}),symbols);
            else if(m=="getPrototypeOf"){output=call(object,"getPrototypeOf",{value});JSValue expected;if(i==2||i==3)expected=nativeBooleanPrototype();else if(i>=4&&i<=8)expected=nativeNumberPrototype();else if(i==12)expected=nativeSymbolPrototype();else expected=value.toObject().getPrototype();result=O{{"object",output.isObject()},{"samePrototype",output.sameValue(expected)}};}
            else if(m=="defineProperties-source")result=encode(call(object,"defineProperties",{JSValue::object(),value}),symbols);
            else if(m=="getOwnPropertyDescriptor")result=encode(call(object,method,{value,"0"}),symbols);
            else if(m=="defineProperty-target")result=encode(call(object,"defineProperty",{value,"x",descriptorValue(7)}),symbols);
            else if(m=="defineProperties-target"){auto properties=JSValue::object();own(properties,"x",descriptorValue(7));result=encode(call(object,"defineProperties",{value,properties}),symbols);}
            else result=encode(call(object,method,{value}),symbols);
        }catch(const JSTypeError&){result=O{{"error","TypeError"}};}catch(const JSRangeError&){result=O{{"error","RangeError"}};}
        emit(O{{"kind","primitive"},{"input",labels[i]},{"method",method},{"result",result}});
    }
    for(unsigned i=0;i<15;++i)for(const auto* method:{"getOwnPropertyDescriptor","defineProperty"})for(unsigned mode=0;mode<6;++mode){
        const auto symbol=JSSymbol::create("fixture"),keySymbol=JSSymbol::create("key");const Args symbols{symbol,keySymbol};auto value=input(i,symbol),log=array(),key=keyObject(log,keySymbol,mode);
        const auto result=attempt([&]{return std::string(method)=="defineProperty"?call(object,method,{value,key,descriptorValue(9)}):call(object,method,{value,key});},symbols);
        emit(O{{"kind","dynamic-key"},{"input",labels[i]},{"method",method},{"mode",mode},{"result",result},{"events",encode(log,symbols)}});
    }
}
JSValue inheritedDescriptor(const JSValue& log,const JSValue& state,const std::string& label,const std::string& fail,double value){
    auto prototype=JSValue::object(),descriptor=JSValue::object(prototype);own(state,label,descriptor);
    for(const auto* field:{"enumerable","configurable","value","writable"}){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],array({c[2],self.sameValue(c[1].get(c[3]))}));c[0].collectGarbage();if(c[2].strictEquals(c[4]))throw JSRangeError("descriptor getter");return c[5];},{log,state,label+"."+field,label,fail,std::string(field)=="value"?JSValue(value):JSValue(true)});prototype.defineProperty(field,d);}return descriptor;
}
void descriptorCases(const JSValue& object){
    for(const auto* method:{"defineProperty","defineProperties"})for(const auto* failure:{"none","first.enumerable","first.value","first.writable","second.value"}){
        auto log=array(),state=JSValue::object(),target=JSValue::object();own(target,"prior",1);const auto first=inheritedDescriptor(log,state,"first",failure,7),second=inheritedDescriptor(log,state,"second",failure,9),keySymbol=JSSymbol::create("key");const Args symbols{keySymbol};auto key=keyObject(log,keySymbol,0),properties=JSValue::object();own(properties,"a",first);own(properties,"b",second);
        const auto result=attempt([&]{return std::string(method)=="defineProperty"?call(object,method,{target,key,first}):call(object,method,{target,properties});},symbols);
        emit(O{{"kind","inherited-descriptor"},{"method",method},{"failure",failure},{"result",result},{"target",snapshot(target,symbols)},{"events",encode(log,symbols)}});
    }
}
void proxyCases(const JSValue& object){
    for(const auto* method:{"assign","keys","values","getOwnPropertyNames","getOwnPropertyDescriptors","defineProperties"})for(unsigned mode=0;mode<2;++mode){
        auto log=array(),base=JSValue::object(),state=JSValue::object();const auto symbol=JSSymbol::create("source");const Args symbols{symbol};const bool define=std::string(method)=="defineProperties";
        JSPropertyDescriptor a;a.enumerable=a.configurable=true;a.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{push(c[0],array({"a.get",self.sameValue(c[1].get("proxy"))}));c[0].collectGarbage();if(c[3].toBoolean()){auto base=c[2];base.erase("b");}return c[4];},{log,state,base,bool(mode),define?descriptorValue(1):JSValue(1)});base.defineProperty("a",a);
        own(base,"b",define?descriptorValue(2):JSValue(2));JSPropertyDescriptor hidden;hidden.value=define?descriptorValue(3):JSValue(3);hidden.configurable=true;base.defineProperty("hidden",hidden);own(base,symbol,define?descriptorValue(4):JSValue(4));
        auto handler=JSValue::object();own(handler,"ownKeys",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[1],"ownKeys");c[1].collectGarbage();Args keys=c[0].ownPropertyKeys();return array(keys);},{base,log}));
        own(handler,"getOwnPropertyDescriptor",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{push(c[1],array({"descriptor",args[1]}));c[1].collectGarbage();return call(c[2],"getOwnPropertyDescriptor",{c[0],args[1]});},{base,log,object}));
        own(handler,"get",JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{push(c[1],array({"get",args[1]}));c[1].collectGarbage();return c[0].get(args[1],args[2]);},{base,log}));
        const auto proxy=JSValue::proxy(base,handler);own(state,"proxy",proxy);const auto result=attempt([&]{if(std::string(method)=="assign"||std::string(method)=="defineProperties")return call(object,method,{JSValue::object(),proxy});return call(object,method,{proxy});},symbols);
        emit(O{{"kind","proxy-source"},{"method",method},{"mode",mode},{"result",result},{"events",encode(log,symbols)}});
    }
    auto prototype=JSValue::object(),base=JSValue::object(prototype),handler=JSValue::object(),log=array();
    own(handler,"getPrototypeOf",JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[1],"getPrototypeOf");c[1].collectGarbage();return c[0];},{prototype,log}));
    const auto proxy=JSValue::proxy(base,handler),result=call(object,"getPrototypeOf",{proxy});emit(O{{"kind","proxy-prototype"},{"samePrototype",result.sameValue(prototype)},{"events",encode(log,{})}});
    auto symbolPrototype=nativeSymbolPrototype(),sentinelLog=array();const auto saved=*symbolPrototype.getOwnPropertyDescriptor("constructor");JSPropertyDescriptor constructor;constructor.configurable=true;constructor.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"unexpected constructor getter");return 9;},{sentinelLog});symbolPrototype.defineProperty("constructor",constructor);const auto symbol=JSSymbol::create("sentinel");
    auto symbolResult=call(object,"getPrototypeOf",{symbol});const auto names=call(object,"getOwnPropertyNames",{symbol});symbolPrototype.defineProperty("constructor",saved);
    emit(O{{"kind","symbol-prototype-getter"},{"samePrototype",symbolResult.sameValue(symbolPrototype)},{"ownNames",encode(names,{})},{"events",encode(sentinelLog,{})}});
}
void lifetime(){for(unsigned round=0;round<3;++round){JSWeakValue boxWeak,keyWeak,symbolWeak;{auto object=JSValue::intrinsicObjectConstructor(),symbol=JSSymbol::create("lifetime"),log=array(),key=keyObject(log,symbol,1);auto box=call(object,"assign",{symbol,JSValue::object()});call(object,"getOwnPropertyDescriptor",{symbol,key});boxWeak=JSWeakValue(box);keyWeak=JSWeakValue(key);symbolWeak=JSWeakValue(symbol);log.collectGarbage();if(boxWeak.expired()||keyWeak.expired()||symbolWeak.expired())throw std::runtime_error("Active Object argument or result lost during GC");}JSValue::nativeSafepoint();if(!boxWeak.expired()||!keyWeak.expired()||!symbolWeak.expired()||JSValue::hasActiveHeap())throw std::runtime_error("Object ToObject graph did not retire");}std::cout<<"3 actual Object ToObject GC/retirement rounds passed\n";}
}
int main(int argc,char** argv){try{if(argc==2&&std::string(argv[1])=="--lifetime"){lifetime();return 0;}if(argc!=1)throw std::runtime_error("Usage: ObjectToObjectParityProbe [--lifetime]");const auto object=JSValue::intrinsicObjectConstructor();primitiveCases(object);descriptorCases(object);proxyCases(object);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
