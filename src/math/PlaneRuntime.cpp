// Managed plane operations used by Material serialization; see THREEJS-LICENSE.
#include "math/Plane.h"
#include "core/JSModuleObject.h"
namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
JSValue method(const JSValue& receiver,const Args& args,const Args& captures){
    auto self=receiver;const auto name=captures[0].string();const auto value=arg(args,0);
    if(name=="clone"){const auto clone=self.get("constructor").construct();return clone.get("copy").call(clone,{self});}
    if(name=="copy"||name=="fromJSON"){
        const auto normal=self.get("normal"),call=normal.get(name=="copy"?"copy":"fromArray");call.call(normal,{value.get("normal")});self.set("constant",value.get("constant"));return self;
    }
    if(name=="equals"){
        const auto normal=value.get("normal"),equals=normal.get("equals");if(!equals.call(normal,{self.get("normal")}).toBoolean())return false;
        const auto left=value.get("constant");return left.strictEquals(self.get("constant"));
    }
    auto result=JSValue::object();JSPropertyDescriptor d;d.writable=d.enumerable=d.configurable=true;
    const auto normal=self.get("normal");d.value=normal.get("toArray").call(normal);result.defineProperty("normal",d);d.value=self.get("constant");result.defineProperty("constant",d);return result;
}
}
JSValue nativePlaneConstructor(){
    static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isConstructible())return *value;
    const auto vector=nativeVector3Constructor();
    auto ctor=JSValue::constructor(+[](const JSValue& receiver,const Args& args,const Args& captures)->JSValue{
        auto self=receiver;const auto input=arg(args,0),normal=input.isUndefined()?captures[0].construct({1,0,0}):input;const auto constant=arg(args,1);
        self.set("isPlane",true);self.set("normal",normal);self.set("constant",constant.isUndefined()?JSValue(0):constant);return {};
    },{vector},"Plane",0);
    auto p=ctor.get("prototype");for(const auto& [name,arity]:std::initializer_list<std::pair<const char*,unsigned>>{{"copy",1},{"equals",1},{"clone",0},{"toJSON",0},{"fromJSON",1}}){JSPropertyDescriptor d;d.value=JSValue::function(method,{name},name,arity);d.writable=d.configurable=true;p.defineProperty(name,d);}
    ctor.retainForHeapLifetime();weak=JSWeakValue(ctor);return ctor;
}
