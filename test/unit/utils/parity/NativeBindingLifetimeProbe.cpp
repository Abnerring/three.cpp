#include "core/JSValue.h"
#include "nodes/tsl/TSLCore.h"
#include "nodes/math/MathNode.h"
#include "nodes/math/OperatorNode.h"
#include "nodes/core/PropertyNode.h"
#include "nodes/core/NodeUtils.h"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
using Getter=JSValue(*)(const std::string&);
using Module=JSValue(*)();
void require(bool value,const char* message){if(!value)throw std::logic_error(message);}
void check(Getter getter,Module module,const std::string& name,const std::vector<JSValue>& args,bool nodeResult=true){
    // No managed object exists when the first getter is entered. Keeping only a
    // weak witness afterwards also detects an owning thread-local module cache.
    JSWeakValue witness;
    {
        const auto binding=getter(name);
        require(binding.isCallable(),"Fresh binding getter lost its initialization heap");
        auto facade=module();
        require(binding.sameValue(facade.get(name)),"Initial facade differs from lexical binding");
        facade.set(name,nullptr);
        require(facade.get(name).isNull(),"Compatibility facade did not accept mutation");
        require(getter(name).sameValue(binding),"Facade mutation replaced an internal import");
        const auto value=binding.call({},args);
        if(nodeResult)require(value.get("isNode").strictEquals(true),"Original binding stopped creating real nodes");
        else require(value.strictEquals(432860093794037.0),"Original hashString binding changed its pinned-upstream value");
        binding.collectGarbage();
        require(getter(name).sameValue(binding),"GC collected the private lexical table");
        witness=JSWeakValue(binding);
    }
    JSValue::nativeSafepoint();
    require(witness.expired(),"Module cache retained ownership of a retired heap");
}
}
int main(){try{
    // Primitive arguments have no heap, so each check begins in an empty realm.
    for(unsigned round=0;round<2;++round){
        check(nativeTSLCoreBinding,nativeTSLCore,"float",{0.5});
        check(nativeMathNodeBinding,nativeMathNodeModule,"sin",{0.5});
        check(nativeOperatorNodeBinding,nativeOperatorNodeModule,"sub",{0.3,0.7});
        check(nativePropertyNodeBinding,nativePropertyNodeModule,"property",{"float","fresh"});
        check(nativeNodeUtilsBinding,nativeNodeUtils,"hashString",{"x"},false);
    }
    std::cout<<"10 fresh module binding lifetimes passed\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
