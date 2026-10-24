#include "renderers/common/Uniform.h"
#include "core/JSModuleObject.h"
#include "core/NativeObject.h"
#include "math/Color.h"
#include "math/Matrix2.h"
#include "math/Matrix3.h"
#include "math/Matrix4.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
JSValue defaultValue(const std::string& name){
    if(name=="NumberUniform")return 0;
    if(name=="Vector2Uniform")return nativeVector(std::make_shared<Vector2>());
    if(name=="Vector3Uniform")return nativeVector(std::make_shared<Vector3>());
    if(name=="Vector4Uniform")return nativeVector(std::make_shared<Vector4>());
    if(name=="ColorUniform")return nativeColor(std::make_shared<Color>());
    if(name=="Matrix2Uniform")return nativeMatrix2Constructor().construct();
    if(name=="Matrix3Uniform")return nativeMatrix(std::make_shared<Matrix3>());
    return nativeMatrix(std::make_shared<Matrix4>());
}
}
JSValue nativeRendererUniforms(){
    static thread_local JSWeakValue weak;static thread_local bool initializing=false;
    if(const auto module=weak.lock();module&&module->isObject())return *module;
    JSModuleInitializationScope scope(initializing,"renderer Uniform module");auto module=JSValue::object();
    auto base=JSValue::constructor([](const JSValue& receiver,const Args& args,const Args&)->JSValue{
        auto self=receiver;self.set("name",arg(args,0));self.set("value",arg(args,1));self.set("boundary",0);self.set("itemSize",0);self.set("offset",0);self.set("index",-1);return {};
    },{},"Uniform",2);
    auto basePrototype=base.get("prototype");
    for(const auto* name:{"setValue","getValue"}){JSPropertyDescriptor d;d.value=JSValue::function([](const JSValue& receiver,const Args& args,const Args& c)->JSValue{
        if(c[0].strictEquals("getValue"))return receiver.get("value");auto self=receiver;self.set("value",arg(args,0));return {};
    },{name},name,std::string(name)=="setValue"?1:0);d.writable=d.configurable=true;basePrototype.defineProperty(name,d);}
    struct Info{const char* name;int boundary,itemSize;};
    for(const auto& info:std::initializer_list<Info>{{"NumberUniform",4,1},{"Vector2Uniform",8,2},{"Vector3Uniform",16,3},{"Vector4Uniform",16,4},{"ColorUniform",16,3},{"Matrix2Uniform",8,4},{"Matrix3Uniform",48,12},{"Matrix4Uniform",64,16}}){
        auto state=JSValue::object(nullptr);state.set("name",info.name);state.set("boundary",info.boundary);state.set("itemSize",info.itemSize);
        auto ctor=JSValue::builtinConstructor([](const JSValue& target,const Args& args,const Args& c)->JSValue{
            const auto state=c[0];const auto name=state.get("name").string();const auto value=arg(args,1).isUndefined()?defaultValue(name):arg(args,1);
            auto self=state.get("constructor").getPrototype().construct({arg(args,0),value},target);
            self.set("is"+name,true);self.set("boundary",state.get("boundary"));self.set("itemSize",state.get("itemSize"));return self;
        },{state},info.name,1);
        state.set("constructor",ctor);ctor.setPrototype(base);ctor.get("prototype").setPrototype(basePrototype);module.set(info.name,ctor);
    }
    module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
