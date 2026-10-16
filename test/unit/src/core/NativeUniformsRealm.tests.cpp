#include "catch.hpp"
#include "core/Uniform.h"
#include "core/UniformsGroup.h"
#include <thread>
#include <exception>

namespace {
template<class F> void isolated(F task) {
    std::exception_ptr error;
    std::thread worker([&] { try { task(); } catch (...) { error=std::current_exception(); } });
    worker.join(); if(error)std::rethrow_exception(error);
}
void check(bool value) { if(!value)throw std::runtime_error("Native uniform realm check failed"); }
}
TEST_CASE("Uniform and UniformsGroup constructors retire when the last realm handle drops") {
    isolated([] {
        check(!JSValue::hasActiveHeap());
        for(unsigned n=0;n<4;++n) {
            JSWeakValue uniform,group;
            {
                auto keeper=JSValue::object();
                Uniform u(1);UniformsGroup g;
                uniform=JSWeakValue(Uniform::prototype().get("constructor"));
                group=JSWeakValue(UniformsGroup::constructor());
                g.add(u);keeper.collectGarbage();
                check(!uniform.expired()&&!group.expired());
            }
            check(uniform.expired()&&group.expired());
            check(!JSValue::hasActiveHeap());
        }
    });REQUIRE(true);
}
TEST_CASE("UniformsGroup IDs and mutable prototypes belong to each native realm") {
    isolated([] {
        for(unsigned n=0;n<3;++n) {
            {
                auto keeper=JSValue::object();
                UniformsGroup a,b;
                check(a.id.toNumber()==0&&b.id.toNumber()==1);
                auto ctor=UniformsGroup::constructor();
                ctor.get("prototype").set("realmMarker",n);
                keeper.collectGarbage();
                check(UniformsGroup::constructor().sameValue(ctor));
                check(UniformsGroup::prototype().get("realmMarker").number()==n);
                UniformsGroup c;check(c.id.toNumber()==2);
            }
            check(!JSValue::hasActiveHeap());
        }
    });REQUIRE(true);
}
TEST_CASE("Independent uniform realms use independent ID counters concurrently") {
    auto task=[] {
        auto keeper=JSValue::object();
        for(unsigned n=0;n<32;++n){UniformsGroup group;check(group.id.toNumber()==n);}
    };
    std::exception_ptr first,second;
    std::thread a([&]{try{task();}catch(...){first=std::current_exception();}});
    std::thread b([&]{try{task();}catch(...){second=std::current_exception();}});
    a.join();b.join();if(first)std::rethrow_exception(first);if(second)std::rethrow_exception(second);
    REQUIRE(true);
}
