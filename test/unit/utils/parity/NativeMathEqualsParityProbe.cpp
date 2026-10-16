#include "core/NativeObject.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "math/Color.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "utils.h"
#include <iostream>
#include <cmath>

namespace {
using Args=std::vector<JSValue>;using A=JsonValue::Array;using O=JsonValue::Object;
JSValue array(){return JSValue::array();}
void push(JSValue array,JSValue value){array.get("push").call(array,{value});}
JSValue native(unsigned kind){
    if(kind==0)return nativeVector(std::make_shared<Vector2>());
    if(kind==1)return nativeVector(std::make_shared<Vector3>());
    if(kind==2)return nativeVector(std::make_shared<Vector4>());
    if(kind==3)return nativeColor(std::make_shared<Color>());
    if(kind==4)return nativeMatrix(std::make_shared<Matrix3>());
    return nativeMatrix(std::make_shared<Matrix4>());
}
const char* components(unsigned kind){return kind==0?"xy":kind==2?"xyzw":kind==3?"rgb":"xyz";}
unsigned size(unsigned kind){return kind<4?unsigned(std::string(components(kind)).size()):kind==4?9:16;}
JSValue values(unsigned count){auto result=array();for(unsigned i=0;i<count;++i)push(result,.25+i);return result;}
void getter(JSValue object,const std::string& key,JSValue state,const std::string& label,
            JSValue data,const std::string& index,unsigned mode,bool element) {
    JSPropertyDescriptor d;d.configurable=d.enumerable=true;
    d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue {
        auto state=c[0];push(state.get("trace"),c[1]);
        const auto mode=c[4].number();
        if(mode==9&&c[5].toBoolean())throw JSRangeError("observable getter failure");
        if(mode==8&&c[5].toBoolean()) {
            auto replacement=JSValue::object();JSPropertyDescriptor d;d.value=replacement;
            d.writable=d.enumerable=d.configurable=true;
            state.get("self").defineProperty("elements",d);state.get("other").defineProperty("elements",d);
        }
        if(mode==10&&c[1].strictEquals(state.get("firstOther"))) {
            auto values=state.get("otherValues");values.set(state.get("lastIndex").toString(),99);
        }
        if(mode==11)state.collectGarbage();
        return c[2].get(c[3].string());
    },{state,label,data,index,mode,element});
    object.defineProperty(key,d);
}
JsonValue scenario(unsigned kind,unsigned mode){
    auto owner=native(kind),fn=owner.get("equals"),state=JSValue::object();
    state.set("trace",array());auto a=JSValue::object(),b=JSValue::object();
    auto av=values(size(kind)),bv=values(size(kind));
    state.set("self",a);state.set("other",b);state.set("otherValues",bv);
    state.set("lastIndex",size(kind)-1);
    if(mode==1)bv.set("0",9);if(mode==2)bv.set(JSValue(size(kind)-1),99);
    if(mode==3){av.set("0",std::nan(""));bv.set("0",std::nan(""));}
    if(mode==4){av.set("0",-0.);bv.set("0",0.);}
    if(mode==5)bv.set("0","0.25");
    if(kind<4){
        const std::string fields=components(kind);state.set("firstOther","other."+fields.substr(0,1));
        for(unsigned i=0;i<fields.size();++i){const std::string key(1,fields[i]);getter(a,key,state,"self."+key,av,std::to_string(i),mode,true);getter(b,key,state,"other."+key,bv,std::to_string(i),mode,true);}
    } else {
        auto ae=JSValue::object(),be=JSValue::object();state.set("firstOther","other[0]");
        for(unsigned i=0;i<size(kind);++i){auto key=std::to_string(i);getter(ae,key,state,"self["+key+"]",av,key,mode,true);getter(be,key,state,"other["+key+"]",bv,key,mode,true);}
        auto at=JSValue::object(),bt=JSValue::object();at.set("value",ae);bt.set("value",be);
        getter(a,"elements",state,"self.elements",at,"value",mode,false);getter(b,"elements",state,"other.elements",bt,"value",mode,false);
    }
    if(mode==6)b={};if(mode==7)a=nullptr;
    JsonValue result;
    try{result=fn.call(a,{b}).toBoolean();}
    catch(const JSRangeError&){result=O{{"error","RangeError"}};}
    catch(const JSTypeError&){result=O{{"error","TypeError"}};}
    return O{{"kind",kind},{"mode",mode},{"result",result},{"trace",*state.get("trace").toJSON()}};
}
JsonValue metadata(unsigned kind){
    auto fn=native(kind).get("equals");A names;for(const auto& name:fn.ownPropertyNames())names.emplace_back(name);
    A descriptors;
    for(const auto* name:{"name","length"}){auto d=fn.getOwnPropertyDescriptor(name);descriptors.emplace_back(A{d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)});}
    return O{{"metadata",kind},{"name",fn.get("name").toString()},{"length",fn.get("length").number()},
        {"own",names},{"descriptors",descriptors},{"constructible",fn.isConstructible()},
        {"source",fn.get("toString").call(fn).toString()}};
}
}
int main(){
    try{for(unsigned k=0;k<6;++k){for(unsigned m=0;m<12;++m)std::cout<<scenario(k,m).dump()<<'\n';std::cout<<metadata(k).dump()<<'\n';}}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
