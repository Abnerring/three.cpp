#include "renderers/common/StorageInstancedBufferAttribute.h"
#include "core/InstancedBufferAttribute.h"
#include "core/JSGlobal.h"
#include "nodes/utils/UtilityNodeRuntime.h"
namespace {
using namespace UtilityNodeRuntime;
#include "StorageInstancedBufferAttributeSource.inc"
JSWeakValue& lexical(){static thread_local JSWeakValue weak;return weak;}
}
JSValue nativeStorageInstancedBufferAttributeModule(){
    using namespace UtilityNodeRuntime;static thread_local JSWeakValue weak;if(const auto v=weak.lock();v&&v->isObject())return *v;
    static thread_local bool initializing=false;JSModuleInitializationScope scope(initializing,"StorageInstancedBufferAttribute module");
    const auto base=nativeInstancedBufferAttributeConstructor();auto holder=JSValue::object(nullptr),bindings=JSValue::object(nullptr);
    auto ctor=JSValue::builtinConstructor(+[](const JSValue& target,const Args& args,const Args& c)->JSValue{
        const auto count=arg(args,0),size=arg(args,1),type=args.size()<3||args[2].isUndefined()?nativeGlobalBinding("Float32Array"):args[2];
        const auto isView=invoke(nativeGlobalBinding("ArrayBuffer"),"isView");JSValue array;
        if(isView({count}).toBoolean())array=count;else{const auto n=count.toNumber();const auto m=size.toNumber();array=type.construct({n*m});}
        auto self=superConstruct(target,c,{array,size});self.set("isStorageInstancedBufferAttribute",true);return self;
    },{holder},"StorageInstancedBufferAttribute",2);holder.set("constructor",ctor);ctor.setPrototype(base);auto p=ctor.get("prototype");p.setPrototype(base.get("prototype"));ctor.setFunctionSourceText(StorageInstancedBufferAttributeSource::StorageInstancedBufferAttribute);
    own(bindings,"default",ctor);auto module=JSValue::object();own(module,"default",ctor);bindings.retainForHeapLifetime();lexical()=JSWeakValue(bindings);module.retainForHeapLifetime();weak=JSWeakValue(module);return module;
}
JSValue nativeStorageInstancedBufferAttributeBinding(const std::string& name){const auto keeper=nativeStorageInstancedBufferAttributeModule();return lexical().lock()->get(name);}
JSValue nativeStorageInstancedBufferAttributeConstructor(){return nativeStorageInstancedBufferAttributeBinding("default");}
