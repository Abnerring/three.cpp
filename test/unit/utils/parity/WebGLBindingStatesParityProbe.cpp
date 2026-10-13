#include "WebGLBindingStatesFixture.h"
#include <iostream>
using namespace BindingFixture;
namespace {
void emit(JsonValue value){std::cout<<value.dump()<<'\n';JSValue::object().collectGarbage();}
JsonValue metadataCase(unsigned method,unsigned mode) {
    Fixture f;const char* names[]={"setup","reset","resetDefaultState","dispose","releaseStatesOfGeometry","releaseStatesOfObject","releaseStatesOfProgram","initAttributes","enableAttribute","disableUnusedAttributes"};
    const auto fn=f.manager.get(names[method]),proto=fn.get("prototype");
    auto target=fn,expected=proto;auto calls=JSValue::array();
    if(mode) {
        target=JSValue::constructor(+[](const JSValue&,const Args&,const Args& c)->JSValue{push(c[0],"target-body");return {};},{calls},"Other",0,JSValue::ConstructorKind::Callable);
        if(mode==1){expected=JSValue::object();target.set("prototype",expected);}
        else {target.set("prototype",nullptr);expected=JSValue::object().getPrototype();}
    }
    const Args args=method==0?Args{f.object,f.material,f.program,f.geometry,nullptr}:method==4?Args{f.geometry}:method==5?Args{f.object}:method==6?Args{f.program}:method==8?Args{1}:Args{};
    const auto result=fn.construct(args,target);A own,resultOwn;
    for(const auto& name:fn.ownPropertyNames())own.emplace_back(name);
    for(const auto& name:result.ownPropertyNames())resultOwn.emplace_back(name);
    const auto d=fn.getOwnPropertyDescriptor("prototype");
    return O{{"kind","metadata"},{"method",method},{"mode",mode},{"name",encode(fn.get("name"))},{"length",encode(fn.get("length"))},
             {"own",own},{"prototypeFlags",A{*d->writable,*d->enumerable,*d->configurable}},
             {"constructorIdentity",proto.get("constructor").sameValue(fn)},{"resultPrototype",result.getPrototype().sameValue(expected)},
             {"resultOwn",resultOwn},{"targetCalls",encode(calls)},{"trace",f.trace()}};
}
}
int main() {
    try {
        for(unsigned mode=0;mode<4;++mode)for(unsigned size:{4u,7u,8u,10u,11u,14u,0u})for(unsigned field=0;field<8;++field)
            for(unsigned value=0;value<scalars().size();++value)emit(attributeCase(mode,size,field,value));
        emit(attributeCase(4,7,0,7));
        for(unsigned variant=0;variant<16;++variant)emit(sequenceCase(variant));
        for(unsigned length=0;length<6;++length)for(unsigned mode=0;mode<4;++mode)emit(defaultCase(length,mode));
        for(unsigned variant=0;variant<8;++variant)emit(releaseCase(variant));
        for(unsigned behavior=0;behavior<7;++behavior)for(unsigned fail=0;fail<11;++fail)emit(behaviorCase(behavior,fail));
        for(unsigned max:{4u,7u,8u,10u,14u,0u})for(unsigned index=0;index<scalars().size();++index)emit(manualCase(max,index));
        for(unsigned method=0;method<10;++method)for(unsigned mode=0;mode<3;++mode)emit(metadataCase(method,mode));
        for(unsigned variant=0;variant<14;++variant)emit(getterCase(variant));
        for(unsigned variant=0;variant<11;++variant)emit(enumerationCase(variant));
        for(unsigned variant=0;variant<16;++variant)for(unsigned phase=1;phase<5;++phase)emit(dynamicEnumerationCase(variant,phase));
        for(unsigned variant=0;variant<10;++variant)emit(invalidCase(variant));
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
