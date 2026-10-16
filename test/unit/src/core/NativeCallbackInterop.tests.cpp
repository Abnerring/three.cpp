#include "catch.hpp"
#include "core/NativeObject.h"
#include "math/Quaternion.h"
#include "math/Euler.h"
#include <thread>
#include <vector>
using Args=std::vector<JSValue>;
TEST_CASE("native registration snapshots preserve mutable state and synchronize after wrapping", "[properties][native][callback]") {
    std::vector<bool> evidence;
    std::thread worker([&]{
        auto verify=[&](auto value,auto wrap){
            std::vector<unsigned> counts;auto self=wrap(value);
            value->_onChange([count=0u,&counts]()mutable{counts.push_back(++count);});
            const auto previous=self.get("_onChangeCallback");
            value->x=1;previous.call(nullptr,{});
            value->_onChange([count=0u,&counts]()mutable{counts.push_back(10+(++count));});
            previous.call(nullptr,{});value->x=2;
            evidence.push_back(counts==std::vector<unsigned>{1,2,3,11});
            self.erase("_onChangeCallback");value->x=3;evidence.push_back(counts.size()==4);
            auto copy=std::make_shared<std::remove_reference_t<decltype(*value)>>(*value);
            copy->x=4;evidence.push_back(counts.size()==4);
        };
        verify(std::make_shared<Quaternion>(),[](auto q){return nativeQuaternion(q);});
        verify(std::make_shared<Euler>(),[](auto q){return nativeEuler(q);});
    });worker.join();REQUIRE(evidence.size()==6);for(bool value:evidence)REQUIRE(value);
}
TEST_CASE("native callback adapter reads current managed replacement without owning a graph handle", "[properties][native][callback][gc]") {
    std::vector<bool> evidence;
    std::thread worker([&]{
        auto verify=[&](auto value,auto wrap){
            unsigned nativeCalls=0;value->_onChange([&]{++nativeCalls;});JSWeakValue weakObject,weakCallback;std::weak_ptr<void> owner=value;
            {
                auto self=wrap(value);weakObject=JSWeakValue(self);auto state=JSValue::object();state.set("calls",0);
                auto function=JSValue::function([](const JSValue& receiver,const Args&,const Args& captures){auto state=captures[0];state.set("calls",state.get("calls").toNumber()+1);state.set("same",receiver.sameValue(captures[1]));JSValue::object().collectGarbage();return JSValue{};},{state,self});
                weakCallback=JSWeakValue(function);self.set("_onChangeCallback",function);value->x=3;
                evidence.push_back(nativeCalls==0&&state.get("calls").strictEquals(1)&&state.get("same").toBoolean());
            }
            JSValue::nativeSafepoint();evidence.push_back(weakObject.expired()&&weakCallback.expired()&&!owner.expired());
            value->x=4;evidence.push_back(nativeCalls==1);
            value.reset();JSValue::nativeSafepoint();evidence.push_back(owner.expired()&&!JSValue::hasActiveHeap());
        };
        verify(std::make_shared<Quaternion>(),[](auto q){return nativeQuaternion(q);});
        verify(std::make_shared<Euler>(),[](auto q){return nativeEuler(q);});
    });worker.join();REQUIRE(evidence.size()==8);for(bool value:evidence)REQUIRE(value);
}
TEST_CASE("escaped callback snapshots survive later registration and retire their owner cycles", "[properties][native][callback][gc]") {
    std::vector<bool> evidence;
    std::thread worker([&]{
        auto verify=[&](auto value,auto wrap){
            unsigned old=0,next=0;value->_onChange([&]{++old;});JSValue escaped;JSWeakValue objectWeak;std::weak_ptr<void> owner=value;
            {
                auto self=wrap(value);objectWeak=JSWeakValue(self);escaped=self.get("_onChangeCallback");
                value->_onChange([&]{++next;});value.reset();
            }
            JSValue::nativeSafepoint();escaped.call({},{});evidence.push_back(old==1&&next==0&&!owner.expired()&&!objectWeak.expired());
            escaped={};JSValue::nativeSafepoint();evidence.push_back(owner.expired()&&objectWeak.expired()&&!JSValue::hasActiveHeap());
        };
        verify(std::make_shared<Quaternion>(),[](auto q){return nativeQuaternion(q);});
        verify(std::make_shared<Euler>(),[](auto q){return nativeEuler(q);});
    });worker.join();REQUIRE(evidence.size()==4);for(bool value:evidence)REQUIRE(value);
}
TEST_CASE("managed native dispatch rejects an operation from a foreign thread", "[properties][native][thread]") {
    bool rejected=false;std::thread worker([&]{auto value=std::make_shared<Quaternion>();auto self=nativeQuaternion(value);std::thread foreign([&]{try{value->x=1;}catch(const std::exception&){rejected=true;}});foreign.join();});worker.join();REQUIRE(rejected);
}

TEST_CASE("failed native registration restores its snapshot without reverting nested registrations", "[properties][native][callback]") {
    std::vector<bool> evidence;
    std::thread worker([&]{
        auto verify=[&](auto value,auto wrap){
            unsigned old=0,next=0;value->_onChange([&]{++old;});
            {
                auto self=wrap(value);JSPropertyDescriptor flags;flags.writable=false;self.defineProperty("_onChangeCallback",flags);
                bool threw=false;try{value->_onChange([&]{++next;});}catch(const JSTypeError&){threw=true;}
                evidence.push_back(threw);value->x=1;evidence.push_back(old==1&&next==0);
            }
            JSValue::nativeSafepoint();value->x=2;evidence.push_back(old==2&&next==0);
        };
        verify(std::make_shared<Quaternion>(),[](auto q){return nativeQuaternion(q);});
        verify(std::make_shared<Euler>(),[](auto q){return nativeEuler(q);});
        std::function<void()> current=[]{};const auto record=propertyChangeRecord(current);unsigned old=0,nested=0;
        registerPropertyChangeCallback(current,[&]{++old;});
        record->managedRegister=[&](const PropertyChangeSnapshot&){record->managedRegister={};registerPropertyChangeCallback(current,[&]{++nested;});throw JSRangeError("setter");};
        bool threw=false;try{registerPropertyChangeCallback(current,[]{});}catch(const JSRangeError&){threw=true;}
        notifyPropertyChange(current);evidence.push_back(threw&&old==0&&nested==1);
    });worker.join();REQUIRE(evidence.size()==7);for(bool value:evidence)REQUIRE(value);
}

namespace {
struct FailingObserverCopy {
    std::shared_ptr<bool> fail;
    explicit FailingObserverCopy(std::shared_ptr<bool> value):fail(std::move(value)){}
    FailingObserverCopy(const FailingObserverCopy& other):fail(other.fail){if(*fail)throw std::bad_alloc();}
    void operator()(const PropertyChangeSnapshot&) const{}
};
}
TEST_CASE("observer allocation failure precedes internal callback registration", "[properties][native][callback]") {
    std::function<void()> current=[]{};unsigned old=0,next=0;
    registerPropertyChangeCallback(current,[&]{++old;});
    const auto record=propertyChangeRecord(current);const auto before=record->callback;const auto version=record->registrationVersion;
    auto fail=std::make_shared<bool>(false);record->managedRegister=FailingObserverCopy(fail);*fail=true;
    REQUIRE_THROWS_AS(registerPropertyChangeCallback(current,[&]{++next;}),std::bad_alloc);
    REQUIRE(record->callback==before);REQUIRE(record->registrationVersion==version);
    notifyPropertyChange(current);REQUIRE(old==1);REQUIRE(next==0);
}
