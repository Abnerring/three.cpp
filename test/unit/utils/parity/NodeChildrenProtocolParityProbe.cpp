#include "nodes/core/Node.h"
#include "core/JSGlobal.h"
#include "core/JSIterator.h"
#include "core/JSSet.h"
#include "core/JSPromise.h"
#include <iostream>
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args&a,unsigned i){return i<a.size()?a[i]:JSValue{};}
JSValue array(const Args&values={}){auto out=JSValue::array();for(const auto&v:values)out.set(std::to_string(out.length()),v);return out;}
void push(JSValue a,const JSValue&v){a.set(std::to_string(a.length()),v);}
void event(JSValue state,const Args&row){push(state.get("events"),array(row));}
void get(JSValue object,const JSValue&key,JSValue getter){JSPropertyDescriptor d;d.getter=getter;d.enumerable=d.configurable=true;object.defineProperty(key,d);}
JSValue tag(const JSValue&v,const JSValue&s){for(const auto*key:{"receiver","A","B","list","bag"})if(v.sameValue(s.get(key)))return key;if(v.isUndefined())return "undefined";if(v.isNull())return "null";if(v.type()==JSValue::Type::Number)return "number";if(v.type()==JSValue::Type::String)return "string";if(v.type()==JSValue::Type::Boolean)return "boolean";return "object";}
JSValue namesIterable(JSValue s){auto iterable=JSValue::object(),iterator=JSValue::object(),state=JSValue::object();state.set("index",0);get(iterable,JSValue::wellKnownSymbol("iterator"),JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{event(c[0],{"get:iterator"});return JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{event(c[0],{"call:iterator"});return c[1];},c);},{s,iterator}));
 get(iterator,"next",JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{event(c[0],{"get:next"});return JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{auto state=c[1];const auto i=int(state.get("index").number());state.set("index",i+1);event(c[0],{"next",i});auto r=JSValue::object();r.set("done",i>=3);if(i<3)r.set("value",i==0?"single":i==1?"_private":"bag");return r;},c);},{s,state}));
 get(iterator,"return",JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{event(c[0],{"get:return"});return JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{event(c[0],{"call:return"});return c[0].get("mode").strictEquals(13)?JSValue(17):JSValue::object();},c);},{s}));return iterable;}
struct Restore{JSValue object,key;JSPropertyDescriptor descriptor;};
void hook(JSValue object,const char*key,const char*kind,JSValue state,std::vector<Restore>&restores){const auto old=*object.getOwnPropertyDescriptor(key);const auto original=object.get(key);restores.push_back({object,key,old});get(object,key,JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{auto s=c[0];const auto kind=c[1],mode=s.get("mode");event(s,{"get:"+kind.string()});if((kind.strictEquals("names")&&mode.strictEquals(2))||(kind.strictEquals("proto")&&mode.strictEquals(4)))throw JSRangeError("getter");if(kind.strictEquals("array")&&mode.strictEquals(9))return 17;
 return JSValue::function(+[](const JSValue&self,const Args&a,const Args&c)->JSValue{auto s=c[0];const auto kind=c[1],mode=s.get("mode");event(s,{"call:"+kind.string(),self.sameValue(c[3]),tag(arg(a,0),s)});
  if((kind.strictEquals("names")&&mode.strictEquals(3))||(kind.strictEquals("proto")&&mode.strictEquals(5)))throw JSRangeError("call");
  if(kind.strictEquals("names")){
   if(mode.strictEquals(1))return array({"single","bag"});if(mode.strictEquals(11)||mode.strictEquals(12)||mode.strictEquals(13))return namesIterable(s);
   if(mode.strictEquals(14)&&!s.get("entered").toBoolean()){s.set("entered",true);const auto result=nativeNodeConstructor().get("prototype").get("_getChildren").call(JSValue::object(),{createNativeSet()});event(s,{"inner",result.length()});}
  }
  if(kind.strictEquals("proto")){if(mode.strictEquals(6))return c[3].get("prototype");if(mode.strictEquals(7)){auto receiver=s.get("receiver");receiver.set("single",s.get("B"));}}
  if(kind.strictEquals("array")&&mode.strictEquals(8))return 1;
  if(mode.strictEquals(15)){s.collectGarbage();event(s,{"gc",s.get("A").get("isNode")});}
  return c[2].call(self,a);
 },c);
},{state,kind,original,object}));}
JSValue run(int mode){auto s=JSValue::object(),receiver=JSValue::object(),a=JSValue::object(),b=JSValue::object(),bag=JSValue::object(),list=array({});a.set("isNode",true);b.set("isNode",true);a.set("label","A");b.set("label","B");list.set("0",b);list.set("length",2);bag.set("_skip",a);bag.set("leaf",b);
 s.set("mode",mode);s.set("receiver",receiver);s.set("A",a);s.set("B",b);s.set("list",list);s.set("bag",bag);s.set("events",array());
 get(receiver,"_private",JSValue::function(+[](const JSValue&self,const Args&,const Args&c)->JSValue{auto s=c[0];event(s,{"read:private",self.sameValue(s.get("receiver"))});const auto mode=s.get("mode");if(mode.strictEquals(11)||mode.strictEquals(13))throw JSRangeError("private");if(mode.strictEquals(16)){auto receiver=s.get("receiver");receiver.set("single",s.get("B"));}return s.get("A");},{s}));
 receiver.set("empty",nullptr);receiver.set("number",7);receiver.set("string","x");receiver.set("flag",true);receiver.set("single",a);receiver.set("list",list);receiver.set("bag",bag);
 std::vector<Restore> restores;const auto object=nativeGlobalBinding("Object"),arrayCtor=nativeGlobalBinding("Array");hook(object,"getOwnPropertyNames","names",s,restores);hook(object,"getPrototypeOf","proto",s,restores);hook(arrayCtor,"isArray","array",s,restores);
 const auto stringPrototype=nativeStringPrototype(),starts=stringPrototype.get("startsWith");const auto startsDescriptor=*stringPrototype.getOwnPropertyDescriptor("startsWith");
 if(mode==10){restores.push_back({stringPrototype,"startsWith",startsDescriptor});get(stringPrototype,"startsWith",JSValue::function(+[](const JSValue&self,const Args&,const Args&c)->JSValue{event(c[0],{"get:startsWith",self});return JSValue::function(+[](const JSValue&self,const Args&a,const Args&c)->JSValue{event(c[0],{"call:startsWith",self,arg(a,0)});return 1;},c);},{s}));}
 auto out=JSValue::object();out.set("mode",mode);std::string error;JSValue result;
 try{result=nativeNodeConstructor().get("prototype").get("_getChildren").call(receiver,{createNativeSet()});}catch(const JSTypeError&){error="TypeError";}catch(const JSRangeError&){error="RangeError";}catch(const std::exception&){error="Error";}
 for(auto i=restores.rbegin();i!=restores.rend();++i)i->object.defineProperty(i->key,i->descriptor);
 out.set("error",error);auto children=array();if(result.isObject())for(unsigned i=0;i<result.length();++i){const auto row=result.get(std::to_string(i));auto value=JSValue::object();value.set("property",row.get("property"));value.set("index",row.get("index").isUndefined()?JSValue("undefined"):row.get("index"));value.set("child",tag(row.get("childNode"),s));push(children,value);}out.set("children",children);out.set("events",s.get("events"));return out;}
}
int main(){try{const auto ctor=nativeNodeConstructor();auto result=array();for(int mode=0;mode<17;++mode)push(result,run(mode));std::cout<<result.toJSON()->dump();}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
