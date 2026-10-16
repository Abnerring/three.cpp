#include "WebGLUniformsGroupsFixture.h"
using namespace GroupsFixture;
int main() {
    for(unsigned i=0;i<21;++i)std::cout<<scalarCase(i).dump()<<'\n';
    for(unsigned i=0;i<3;++i)std::cout<<packedCase(i).dump()<<'\n';
    for(unsigned i=0;i<7;++i)std::cout<<arrayCase(i).dump()<<'\n';
    for(unsigned i=0;i<9;++i)for(unsigned j=0;j<4;++j)std::cout<<typedCase(i,j).dump()<<'\n';
    for(unsigned i=0;i<12;++i)std::cout<<failureCase(i).dump()<<'\n';
    for(unsigned i=0;i<5;++i)std::cout<<lifecycleCase(i).dump()<<'\n';
    for(unsigned i=0;i<12;++i)std::cout<<observedCase(i).dump()<<'\n';
    for(unsigned i=0;i<7;++i)std::cout<<unsupportedCase(i).dump()<<'\n';
    for(unsigned i=0;i<28;++i)std::cout<<edgeCase(i).dump()<<'\n';
    std::cout<<metadataCase().dump()<<'\n';
    for(unsigned i=0;i<4;++i)std::cout<<identityCase(i).dump()<<'\n';
    auto anchor=JSValue::object();JSWeakValue state,group,manager,ctor;
    {Fixture f;state=JSWeakValue(f.state);group=JSWeakValue(f.group);manager=JSWeakValue(f.manager);ctor=JSWeakValue(nativeWebGLUniformsGroupsConstructor());f.group.set("uniforms",arr({uniform(1)}));f.update();anchor.collectGarbage();if(state.expired()||group.expired()||manager.expired())throw std::runtime_error("Live managed closure graph lost");}
    anchor.collectGarbage();if(!state.expired()||!group.expired()||!manager.expired()||ctor.expired())throw std::runtime_error("Managed cycle or module root lifetime mismatch");
}
