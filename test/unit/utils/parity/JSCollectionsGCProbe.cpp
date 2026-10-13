#include "core/JSMap.h"
#include "core/JSSet.h"
#include "core/JSSymbol.h"
#include <iostream>
namespace {
using Args=std::vector<JSValue>;using A=JsonValue::Array;using O=JsonValue::Object;
JSValue call(const JSValue& self,const char* name,const Args& args={}){return self.get(name).call(self,args);}
A alive(const std::vector<JSWeakValue>& refs){A result;for(const auto& ref:refs)result.push_back(!ref.expired());return result;}
}
int main(){
    auto witness=JSValue::object();
    for(bool map:{true,false})for(unsigned mode=0;mode<5;++mode){
        JSValue iterator;std::vector<JSWeakValue> refs;
        {
            auto object=map?createNativeMap():createNativeSet(),key=JSValue::object(),value=JSValue::object(),fresh=JSValue::object();
            key.set("name","old");fresh.set("name","new");value.set("name","value");
            refs={JSWeakValue(object),JSWeakValue(key),JSWeakValue(value),JSWeakValue(fresh)};
            call(object,map?"set":"add",{key,value});iterator=call(object,"keys");
            if(mode==1)call(iterator,"next");
            if(mode==2)call(object,"delete",{key});
            if(mode==3){call(object,"clear");call(object,map?"set":"add",{fresh,value});}
            if(mode==4){call(iterator,"next");call(object,"delete",{key});call(object,map?"set":"add",{key,value});}
        }
        witness.collectGarbage();const auto before=alive(refs);A yielded;
        while(true){const auto result=call(iterator,"next");if(result.get("done").toBoolean())break;yielded.push_back(result.get("value").get("name").string());}
        witness.collectGarbage();std::cout<<JsonValue(O{{"map",map},{"mode",mode},{"before",before},{"yielded",yielded},{"after",alive(refs)}}).dump()<<'\n';
    }
    JSWeakValue symbolRef;JSValue holder=JSValue::object();
    {auto symbol=JSSymbol::create("kept");symbolRef=JSWeakValue(symbol);holder.set(symbol,1);}
    witness.collectGarbage();const bool before=!symbolRef.expired();holder={};witness.collectGarbage();
    std::cout<<JsonValue(O{{"symbolBefore",before},{"symbolAfter",!symbolRef.expired()}}).dump()<<'\n';
}
