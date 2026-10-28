#include "renderers/common/StorageBufferAttribute.h"
#include "core/NativeObject.h"
#include "core/JSGlobal.h"
#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
#include "StorageBufferAttributeSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
}
JSValue nativeStorageBufferAttributeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"StorageBufferAttribute module");
    const auto base=nativeBufferAttributeConstructor();auto holder=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& c)->JSValue{
        const auto count=arg(args,0),size=arg(args,1),type=args.size()<3||args[2].isUndefined()?nativeGlobalBinding("Float32Array"):args[2];
        const auto isView=invoke(nativeGlobalBinding("ArrayBuffer"),"isView");JSValue array;
        if(isView({count}).toBoolean())array=count;else{const auto n=count.toNumber();const auto m=size.toNumber();array=type.construct({n*m});}
        auto self=superConstruct(target,c,{array,size});self.set("isStorageBufferAttribute",true);return self;
    },{holder},"StorageBufferAttribute",2);holder.set("constructor",ctor);ctor.setPrototype(base);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(StorageBufferAttributeSource::StorageBufferAttribute);
    own(bindings,"default",ctor);auto module=JSValue::object();own(module,"default",ctor);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeStorageBufferAttributeBinding(const std::string& name){const auto keeper=nativeStorageBufferAttributeModule();return lexical().lock()->get(name);}
JSValue nativeStorageBufferAttributeConstructor(){return nativeStorageBufferAttributeBinding("default");}
