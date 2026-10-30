#include "catch.hpp"
#include "core/JSValue.h"
#include "core/EventDispatcher.h"
#include <unordered_set>

namespace {
struct NativeOwner {
    JSValue field;
    std::shared_ptr<NativeOwner> child, other;
    int* destroyed = nullptr;
    const JSValue* collectDuringDestruction = nullptr;
    std::size_t* nestedCollected = nullptr;
    int* readDuringDestruction = nullptr;
    JSValue* saveDuringDestruction = nullptr;
    void (*finalize)(NativeOwner&) = nullptr;
    void* finalizationState = nullptr;
    ~NativeOwner() {
        if (destroyed) ++*destroyed;
        if (finalize) finalize(*this);
        if (readDuringDestruction) {
            try { *readDuringDestruction = static_cast<int>(field.get("answer").number()); }
            catch (...) { *readDuringDestruction = -1; }
        }
        if (saveDuringDestruction) {
            try { *saveDuringDestruction = field; }
            catch (...) { *saveDuringDestruction = nullptr; }
        }
        if (collectDuringDestruction) *nestedCollected = collectDuringDestruction->collectGarbage();
    }
};
void describe(std::vector<JSNativeOwnerNode>& nodes,const std::shared_ptr<NativeOwner>& owner,std::unordered_set<NativeOwner*>& seen) {
    if(!owner||!seen.insert(owner.get()).second)return;
    JSNativeOwnerNode node{owner,{&owner->field,&owner->field},{}};
    if(owner->child)node.edges.push_back({&owner->child,owner->child});
    if(owner->other)node.edges.push_back({&owner->other,owner->other});
    nodes.push_back(node);nodes.push_back(node); // Repeated descriptions must not subtract extra roots/owners.
    describe(nodes,owner->child,seen);describe(nodes,owner->other,seen);
}
JSValue wrap(const std::shared_ptr<NativeOwner>& owner) {
    auto value=JSValue::hostObject(owner,owner.get(),"test.NativeOwner");
    static const auto hooks=[] {auto h=std::make_shared<JSHostObjectHooks>();h->nativeOwnership=[](const JSValue& self){
        auto owner=std::static_pointer_cast<NativeOwner>(self.hostPayload("test.NativeOwner"));std::vector<JSNativeOwnerNode> nodes;std::unordered_set<NativeOwner*> seen;describe(nodes,owner,seen);return nodes;
    };return h;}();value.setHostHooks(hooks);return value;
}
JSValue aliasWrap(const std::shared_ptr<NativeOwner>& owner) {
    std::shared_ptr<void> alias(owner,&owner->field);auto value=JSValue::hostObject(alias,&owner->field,"test.NativeAlias");
    static const auto hooks=[] {auto h=std::make_shared<JSHostObjectHooks>();h->nativeOwnership=[](const JSValue& self){
        auto p=self.hostPayload("test.NativeAlias");return std::vector<JSNativeOwnerNode>{{p,{static_cast<const JSValue*>(p.get())},{}}};
    };return h;}();value.setHostHooks(hooks);return value;
}
}
TEST_CASE("Native ownership retains external owners and collects JS-mediated owner cycles", "[runtime-native]") {
    auto witness=JSValue::object();witness.collectGarbage();const auto before=witness.heapObjectCount();int destroyed=0;
    auto owner=std::make_shared<NativeOwner>();owner->destroyed=&destroyed;std::weak_ptr<NativeOwner> weak=owner;
    {auto host=wrap(owner);owner->field=JSValue::object();owner->field.set("host",host);owner->field.set("answer",42);}
    witness.collectGarbage();REQUIRE(owner->field.get("answer").number()==42);REQUIRE(destroyed==0);
    owner.reset();witness.collectGarbage();REQUIRE(weak.expired());REQUIRE(destroyed==1);REQUIRE(witness.heapObjectCount()==before);
}
TEST_CASE("Native ownership deduplicates aliasing payloads and native roots", "[runtime-native]") {
    auto witness=JSValue::object();witness.collectGarbage();const auto before=witness.heapObjectCount();
    auto owner=std::make_shared<NativeOwner>();std::weak_ptr<NativeOwner> weak=owner;
    {auto original=wrap(owner),alias=aliasWrap(owner);owner->field=JSValue::object();owner->field.set("original",original);owner->field.set("alias",alias);}
    std::shared_ptr<JSValue> externalAlias(owner,&owner->field);owner.reset();witness.collectGarbage();
    REQUIRE_FALSE(weak.expired());REQUIRE(externalAlias->get("alias").isHostObject("test.NativeAlias"));
    externalAlias.reset();witness.collectGarbage();REQUIRE(weak.expired());REQUIRE(witness.heapObjectCount()==before);
}
TEST_CASE("External native owners retain existing wrapper properties through child and alias ownership", "[runtime-native]") {
    auto witness=JSValue::object();witness.collectGarbage();const auto before=witness.heapObjectCount();
    auto parent=std::make_shared<NativeOwner>(),child=std::make_shared<NativeOwner>();parent->child=child;
    std::weak_ptr<NativeOwner> weakParent=parent,weakChild=child;
    {auto host=wrap(parent),childHost=wrap(child),alias=aliasWrap(child);host.set("extra",7);childHost.set("extra",11);alias.set("extra",13);}
    child.reset();witness.collectGarbage();
    REQUIRE(wrap(parent).get("extra").number()==7);REQUIRE(wrap(parent->child).get("extra").number()==11);REQUIRE(aliasWrap(parent->child).get("extra").number()==13);
    parent.reset();witness.collectGarbage();REQUIRE(weakParent.expired());REQUIRE(weakChild.expired());REQUIRE(witness.heapObjectCount()==before);
}
TEST_CASE("Native ownership follows unwrapped child owners and distinct shared pointer slots", "[runtime-native]") {
    auto witness=JSValue::object();witness.collectGarbage();const auto before=witness.heapObjectCount();int destroyed=0;
    auto parent=std::make_shared<NativeOwner>(),child=std::make_shared<NativeOwner>();parent->destroyed=child->destroyed=&destroyed;
    std::weak_ptr<NativeOwner> weakParent=parent,weakChild=child;parent->child=child;parent->other=child;
    {auto host=wrap(parent);child->field=JSValue::object();child->field.set("parent",host);}
    parent.reset();witness.collectGarbage();REQUIRE_FALSE(weakParent.expired());REQUIRE(destroyed==0);
    child.reset();witness.collectGarbage();REQUIRE(weakParent.expired());REQUIRE(weakChild.expired());REQUIRE(destroyed==2);REQUIRE(witness.heapObjectCount()==before);
}
TEST_CASE("Native callback collection keeps receiver and captured owner graphs alive", "[runtime-native]") {
    auto witness=JSValue::object();witness.collectGarbage();const auto before=witness.heapObjectCount();std::weak_ptr<NativeOwner> weak;
    {auto owner=std::make_shared<NativeOwner>();weak=owner;auto host=wrap(owner);
        owner->field=JSValue::function([](const JSValue& self,const std::vector<JSValue>&,const std::vector<JSValue>& captures){self.collectGarbage();return captures[0].get("value");},{host});host.set("value",17);
        auto callback=owner->field;owner.reset();host={};REQUIRE(callback.call(witness).number()==17);
    }
    witness.collectGarbage();REQUIRE(weak.expired());REQUIRE(witness.heapObjectCount()==before);
}
TEST_CASE("Native graph edges retain wrappers even without a child ownership description", "[runtime-native]") {
    auto witness=JSValue::object();witness.collectGarbage();const auto before=witness.heapObjectCount();
    auto parent=std::make_shared<NativeOwner>(),child=std::make_shared<NativeOwner>();parent->child=child;
    std::weak_ptr<NativeOwner> weakParent=parent,weakChild=child;
    {auto host=JSValue::hostObject(parent,parent.get(),"test.PartialOwner");auto hooks=std::make_shared<JSHostObjectHooks>();
        hooks->nativeOwnership=[](const JSValue& self){auto p=std::static_pointer_cast<NativeOwner>(self.hostPayload("test.PartialOwner"));return std::vector<JSNativeOwnerNode>{{p,{&p->field},{{&p->child,p->child}}}};};host.setHostHooks(hooks);
        auto childHost=JSValue::hostObject(child,child.get(),"test.UnregisteredChild");childHost.set("extra",19);host.set("child",childHost);
    }
    child.reset();witness.collectGarbage();REQUIRE(JSValue::hostObject(parent->child,parent->child.get(),"test.UnregisteredChild").get("extra").number()==19);
    parent.reset();witness.collectGarbage();REQUIRE(weakParent.expired());REQUIRE(weakChild.expired());REQUIRE(witness.heapObjectCount()==before);
}
TEST_CASE("Native payload destructors can request collection without reentering the sweep", "[runtime-native]") {
    auto witness=JSValue::object();witness.collectGarbage();const auto before=witness.heapObjectCount();int destroyed=0;std::size_t nestedCollected=99;
    {auto owner=std::make_shared<NativeOwner>();owner->destroyed=&destroyed;owner->collectDuringDestruction=&witness;owner->nestedCollected=&nestedCollected;
        auto host=wrap(owner);owner->field=JSValue::object();owner->field.set("host",host);
    }
    REQUIRE(witness.collectGarbage()==2);REQUIRE(destroyed==1);REQUIRE(nestedCollected==0);REQUIRE(witness.heapObjectCount()==before);
}
TEST_CASE("Native payload destructors can read and externally retain their declared JS fields", "[runtime-native]") {
    auto witness=JSValue::object();witness.collectGarbage();const auto before=witness.heapObjectCount();
    JSValue saved;int read=-2,destroyed=0;std::weak_ptr<NativeOwner> weak;
    {auto owner=std::make_shared<NativeOwner>();weak=owner;owner->destroyed=&destroyed;owner->readDuringDestruction=&read;owner->saveDuringDestruction=&saved;
        auto host=wrap(owner);owner->field=JSValue::object();owner->field.set("answer",41);owner->field.set("host",host);
    }
    witness.collectGarbage();REQUIRE(destroyed==1);REQUIRE(weak.expired());REQUIRE(read==41);REQUIRE(saved.get("answer").number()==41);
    // The field may survive finalization; its already-finalized native payload
    // cannot be resurrected. The wrapper's ordinary properties remain readable.
    REQUIRE_FALSE(saved.get("host").isHostObject("test.NativeOwner"));
    saved={};witness.collectGarbage();REQUIRE(witness.heapObjectCount()==before);
}
TEST_CASE("Native finalizers can retain a different owner before it is finalized", "[runtime-native]") {
    auto witness=JSValue::object();witness.collectGarbage();const auto before=witness.heapObjectCount();JSValue saved;
    int parentDestroyed=0,childDestroyed=0;std::weak_ptr<NativeOwner> parentWeak,childWeak;
    {auto parent=std::make_shared<NativeOwner>(),child=std::make_shared<NativeOwner>();parentWeak=parent;childWeak=child;
        parent->child=child;parent->destroyed=&parentDestroyed;child->destroyed=&childDestroyed;parent->saveDuringDestruction=&saved;
        auto host=wrap(parent),childHost=wrap(child);childHost.set("answer",43);parent->field=JSValue::object();parent->field.set("child",childHost);
    }
    witness.collectGarbage();REQUIRE(parentWeak.expired());REQUIRE(parentDestroyed==1);REQUIRE_FALSE(childWeak.expired());REQUIRE(childDestroyed==0);
    REQUIRE(saved.get("child").isHostObject("test.NativeOwner"));REQUIRE(saved.get("child").get("answer").number()==43);
    saved={};witness.collectGarbage();REQUIRE(childWeak.expired());REQUIRE(childDestroyed==1);REQUIRE(witness.heapObjectCount()==before);
}
TEST_CASE("All alias identities retire before a native finalizer reuses their addresses", "[runtime-native]") {
    auto witness=JSValue::object();witness.collectGarbage();const auto before=witness.heapObjectCount();
    struct Outputs { JSValue original,alias; } outputs;
    {auto owner=std::make_shared<NativeOwner>();auto host=wrap(owner),alias=aliasWrap(owner);host.set("old",true);alias.set("old",true);
        owner->finalizationState=&outputs;owner->finalize=[](NativeOwner& dying){auto& out=*static_cast<Outputs*>(dying.finalizationState);auto payload=std::make_shared<int>(47);
            out.original=JSValue::hostObject(payload,&dying,"test.NativeOwner");out.alias=JSValue::hostObject(payload,&dying.field,"test.NativeAlias");
        };
    }
    witness.collectGarbage();REQUIRE(outputs.original.get("old").isUndefined());REQUIRE(outputs.alias.get("old").isUndefined());
    REQUIRE(*std::static_pointer_cast<int>(outputs.original.hostPayload("test.NativeOwner"))==47);
    REQUIRE(*std::static_pointer_cast<int>(outputs.alias.hostPayload("test.NativeAlias"))==47);
    outputs={};witness.collectGarbage();REQUIRE(witness.heapObjectCount()==before);
}
TEST_CASE("Runtime events preserve native callback owners during collection then release their cycle", "[runtime-native]") {
    auto witness=JSValue::object();
    EventDispatcher::runtimePrototype();
    witness.collectGarbage();const auto before=witness.heapObjectCount();std::weak_ptr<NativeOwner> weak;
    {auto owner=std::make_shared<NativeOwner>();weak=owner;auto host=wrap(owner);host.set("answer",37);
        auto dispatcher=EventDispatcher::runtimeObject();owner->field=dispatcher;
        auto callback=JSValue::function([](const JSValue&receiver,const std::vector<JSValue>&args,const std::vector<JSValue>&captures){
            receiver.collectGarbage();auto event=args[0];event.set("answer",captures[0].get("answer"));return JSValue();
        },{host});
        dispatcher.get("addEventListener").call(dispatcher,{"x",callback});owner.reset();host={};callback={};
        auto event=JSValue::object();event.set("type","x");dispatcher.get("dispatchEvent").call(dispatcher,{event});
        REQUIRE(event.get("answer").number()==37);REQUIRE(event.get("target").isNull());REQUIRE_FALSE(weak.expired());
    }
    witness.collectGarbage();REQUIRE(weak.expired());REQUIRE(witness.heapObjectCount()==before);
}
TEST_CASE("Native strong self cycle retains valid JS fields until explicitly broken", "[runtime-native]") {
    auto witness=JSValue::object();witness.collectGarbage();const auto before=witness.heapObjectCount();std::weak_ptr<NativeOwner> weak;
    {auto owner=std::make_shared<NativeOwner>();weak=owner;owner->child=owner;auto host=wrap(owner);owner->field=JSValue::object();owner->field.set("answer",29);}
    witness.collectGarbage();auto owner=weak.lock();REQUIRE(owner);
    REQUIRE(owner->field.get("answer").number()==29);
    owner->child.reset();owner.reset();witness.collectGarbage();REQUIRE(weak.expired());REQUIRE(witness.heapObjectCount()==before);
}
TEST_CASE("Native strong owner cycles retain aliases and graph exits without falsely collecting", "[runtime-native]") {
    auto witness=JSValue::object();witness.collectGarbage();const auto before=witness.heapObjectCount();std::weak_ptr<NativeOwner> firstWeak,secondWeak,exitWeak;
    {auto first=std::make_shared<NativeOwner>(),second=std::make_shared<NativeOwner>(),exit=std::make_shared<NativeOwner>();
        firstWeak=first;secondWeak=second;exitWeak=exit;first->child=second;second->child=first;second->other=exit;
        auto host=wrap(first),alias=aliasWrap(second);exit->field=JSValue::object();exit->field.set("value",31);second->field=JSValue::object();second->field.set("alias",alias);
    }
    witness.collectGarbage();auto first=firstWeak.lock();REQUIRE(first);REQUIRE(first->child->other->field.get("value").number()==31);
    first->child->child.reset();first.reset();witness.collectGarbage();REQUIRE(firstWeak.expired());REQUIRE(secondWeak.expired());REQUIRE(exitWeak.expired());REQUIRE(witness.heapObjectCount()==before);
}
// GC passive-batch regressions. The pre-existing test prefix above is unchanged.
#include "core/NativeArray.h"
#include <algorithm>
#include <array>
#include <optional>

TEST_CASE("Passive numeric payload batches collect aliases and retain external typed handles", "[runtime-native][passive-gc]") {
    auto witness=JSValue::object();
    std::vector<JSWeakValue> bases,views,buffers;
    std::optional<Float32Array> external;
    for(unsigned i=0;i<512;++i){
        Float32Array values({1,2,3,4,5,6,7,8});auto view=values.subarray(2,6);
        auto base=NativeArray_detail::wrap(InterpolantArray(values));
        auto host=NativeArray_detail::wrap(InterpolantArray(view));auto buffer=host.get("buffer");
        REQUIRE(base.hasPassiveNativeFinalizer());REQUIRE(host.hasPassiveNativeFinalizer());REQUIRE(buffer.hasPassiveNativeFinalizer());
        host.set("extra",i);bases.emplace_back(base);views.emplace_back(host);buffers.emplace_back(buffer);
        if(i==0)external=view;
    }
    witness.collectGarbage();
    for(unsigned i=0;i<bases.size();++i){
        REQUIRE(bases[i].expired());
        REQUIRE(views[i].expired()==(i!=0));REQUIRE(buffers[i].expired()==(i!=0));
    }
    {
        auto live=views[0].lock();REQUIRE(live);REQUIRE(live->get("extra").number()==0);
        REQUIRE(live->get("0").number()==3);external->setIndex(0,99);REQUIRE(live->get("0").number()==99);
        REQUIRE(NativeArray_detail::wrap(InterpolantArray(*external)).sameValue(*live));
        REQUIRE(nativeArrayBuffer(external->buffer).sameValue(*buffers[0].lock()));
    }
    external.reset();witness.collectGarbage();REQUIRE(views[0].expired());REQUIRE(buffers[0].expired());
}

TEST_CASE("Passive capabilities follow genuine ArrayBuffer copies and keep an existing typed cache", "[runtime-native][passive-gc]") {
    ArrayBuffer bytes(32);auto host=nativeArrayBuffer(bytes);REQUIRE(host.hasPassiveNativeFinalizer());
    auto reconstructed=arrayBuffer(host);auto copy=reconstructed;Float32Array values(copy);auto assigned=Float32Array(0);assigned=values;
    auto typed=NativeArray_detail::wrap(InterpolantArray(assigned));REQUIRE(typed.hasPassiveNativeFinalizer());
    REQUIRE(typed.get("buffer").sameValue(host));
    const auto unwrapped=NativeArray_detail::unwrap(typed);
    REQUIRE(NativeArray_detail::wrap(unwrapped).sameValue(typed));REQUIRE(typed.hasPassiveNativeFinalizer());
    auto proxy=JSValue::proxy(host,JSValue::object());REQUIRE_FALSE(proxy.hasPassiveNativeFinalizer());
    REQUIRE_FALSE(JSValue(1).hasPassiveNativeFinalizer());
}

TEST_CASE("Custom ArrayBuffer control blocks stay untrusted through reconstruction and views", "[runtime-native][passive-gc]") {
    auto witness=JSValue::object();int deleted=0;std::size_t nested=99;JSWeakValue weak;
    {
        auto bytes=std::shared_ptr<std::vector<std::byte>>(new std::vector<std::byte>(32),[&](auto* value){++deleted;nested=witness.collectGarbage();delete value;});
        auto forged=JSValue::hostObject(bytes,bytes.get(),"three.ArrayBuffer");REQUIRE_FALSE(forged.hasPassiveNativeFinalizer());
        auto reconstructed=arrayBuffer(forged),copy=reconstructed;
        REQUIRE(nativeArrayBuffer(copy).sameValue(forged));REQUIRE_FALSE(forged.hasPassiveNativeFinalizer());
        Float32Array values(copy);auto host=NativeArray_detail::wrap(InterpolantArray(values));weak=JSWeakValue(host);
        REQUIRE_FALSE(host.hasPassiveNativeFinalizer());REQUIRE_FALSE(host.get("buffer").hasPassiveNativeFinalizer());
        host.set("0",17);REQUIRE(host.get("0").number()==17);
        const auto unwrapped=NativeArray_detail::unwrap(host);REQUIRE_FALSE(NativeArray_detail::wrap(unwrapped).hasPassiveNativeFinalizer());
    }
    witness.collectGarbage();REQUIRE(deleted==1);REQUIRE(nested==0);REQUIRE(weak.expired());
}

TEST_CASE("A custom typed view control block cannot acquire passive provenance on unwrap and wrap", "[runtime-native][passive-gc]") {
    auto witness=JSValue::object();int deleted=0;std::size_t nested=99;JSWeakValue weak;
    {
        Float32Array values({1,2,3});auto original=nativeArrayOwnership(values).nodes.front().owner.lock();
        auto payload=std::shared_ptr<void>(original.get(),[original,&deleted,&nested,&witness](void*)mutable{++deleted;nested=witness.collectGarbage();original.reset();});
        auto forged=JSValue::hostObject(payload,values.identity(),"three.AnimationArray");forged.setHostSlot("kind",6);
        REQUIRE_FALSE(forged.hasPassiveNativeFinalizer());auto unwrapped=NativeArray_detail::unwrap(forged);
        auto host=NativeArray_detail::wrap(unwrapped);weak=JSWeakValue(host);REQUIRE(host.sameValue(forged));
        REQUIRE_FALSE(host.hasPassiveNativeFinalizer());REQUIRE(host.get("0").number()==1);
    }
    witness.collectGarbage();REQUIRE(deleted==1);REQUIRE(nested==0);REQUIRE(weak.expired());
}

namespace PassiveNativeRegression {
struct BoundaryState {
    JSValue* saved=nullptr;const JSValue* witness=nullptr;JSWeakValue victim;
    std::array<char,3> order{};unsigned count=0;std::size_t nested=99;
};
struct BoundaryNode {
    char role='?';BoundaryState* state=nullptr;std::shared_ptr<BoundaryNode> child;
    ~BoundaryNode(){
        state->order[state->count++]=role;
        if(role=='C'){
            if(const auto value=state->victim.lock())*state->saved=*value;
            state->nested=state->witness->collectGarbage();
        }
    }
};
JSValue boundaryWrap(const std::shared_ptr<BoundaryNode>& owner,bool passive){
    auto result=JSValue::hostObject(owner,owner.get(),"test.PassiveBoundary");
    static const auto variants=[] {
        std::array<std::shared_ptr<const JSHostObjectHooks>,2> result;
        for(unsigned i=0;i<2;++i){auto h=std::make_shared<JSHostObjectHooks>();h->passiveFinalizer=i!=0;
            h->nativeOwnership=[](const JSValue& self){auto p=std::static_pointer_cast<BoundaryNode>(self.hostPayload("test.PassiveBoundary"));std::vector<JSNativeOwnerNode> nodes;
                for(;p;p=p->child){JSNativeOwnerNode node{p,{},{}};if(p->child)node.edges.push_back({&p->child,p->child});nodes.push_back(node);}return nodes;
            };result[i]=std::move(h);
        }return result;
    }();result.setHostHooks(variants[passive?1:0]);return result;
}
struct AuditState {
    unsigned reads=0;bool changed=false;JSValue* saved=nullptr;const JSValue* witness=nullptr;
    JSWeakValue victim;unsigned activeHookDeaths=0;std::size_t nested=99;
};
struct AuditPayload { AuditState* state=nullptr; };
std::vector<JSNativeOwnerNode> auditOwnership(const JSValue& self){
    auto p=std::static_pointer_cast<AuditPayload>(self.hostPayload("test.PassiveAuditGate"));auto& state=*p->state;
    if(++state.reads==2&&!state.changed){
        state.changed=true;auto h=std::shared_ptr<JSHostObjectHooks>(new JSHostObjectHooks,[s=&state](auto* value){
            ++s->activeHookDeaths;if(const auto victim=s->victim.lock())*s->saved=*victim;
            s->nested=s->witness->collectGarbage();delete value;
        });h->nativeOwnership=auditOwnership;auto target=self;target.setHostHooks(std::move(h));
    }
    return{{p,{},{}}};
}
}

TEST_CASE("An ineligible active child remains a barrier before passive victims are retired", "[runtime-native][passive-gc]") {
    using namespace PassiveNativeRegression;
    auto witness=JSValue::object();JSValue saved;BoundaryState state;state.saved=&saved;state.witness=&witness;
    std::weak_ptr<BoundaryNode> parentWeak,childWeak,victimWeak;
    {
        std::array<std::shared_ptr<BoundaryNode>,3> owners{std::make_shared<BoundaryNode>(),std::make_shared<BoundaryNode>(),std::make_shared<BoundaryNode>()};
        std::sort(owners.begin(),owners.end(),std::owner_less<std::shared_ptr<BoundaryNode>>{});
        auto parent=owners[0],child=owners[1],victim=owners[2];parentWeak=parent;childWeak=child;victimWeak=victim;
        parent->role='P';child->role='C';victim->role='V';for(auto& owner:owners)owner->state=&state;
        parent->child=child;auto a=boundaryWrap(parent,true),b=boundaryWrap(child,false),c=boundaryWrap(victim,true);
        state.victim=JSWeakValue(c);
    }
    witness.collectGarbage();REQUIRE(parentWeak.expired());REQUIRE(childWeak.expired());REQUIRE_FALSE(victimWeak.expired());
    REQUIRE(state.count==2);REQUIRE(state.order[0]=='P');REQUIRE(state.order[1]=='C');REQUIRE(state.nested==0);
    REQUIRE(saved.isHostObject("test.PassiveBoundary"));saved={};witness.collectGarbage();
    REQUIRE(victimWeak.expired());REQUIRE(state.count==3);REQUIRE(state.order[2]=='V');
}

TEST_CASE("Final ownership audit hook changes revoke batch permission before an active hook rescues its victim", "[runtime-native][passive-gc]") {
    using namespace PassiveNativeRegression;
    auto witness=JSValue::object();JSValue saved;AuditState state;state.saved=&saved;state.witness=&witness;JSWeakValue weakVictim;
    {
        auto first=std::make_shared<AuditPayload>(),second=std::make_shared<AuditPayload>();
        if(std::owner_less<std::shared_ptr<AuditPayload>>{}(second,first))std::swap(first,second);
        first->state=&state;auto gate=JSValue::hostObject(first,first.get(),"test.PassiveAuditGate");
        auto gateHooks=std::make_shared<JSHostObjectHooks>();gateHooks->nativeOwnership=auditOwnership;gateHooks->passiveFinalizer=true;gate.setHostHooks(gateHooks);
        auto victim=JSValue::hostObject(second,second.get(),"test.PassiveAuditVictim");auto victimHooks=std::make_shared<JSHostObjectHooks>();victimHooks->passiveFinalizer=true;
        victimHooks->nativeOwnership=[](const JSValue& self){return std::vector<JSNativeOwnerNode>{{self.hostPayload("test.PassiveAuditVictim"),{},{}}};};victim.setHostHooks(victimHooks);
        state.victim=weakVictim=JSWeakValue(victim);state.reads=0;
    }
    witness.collectGarbage();REQUIRE(state.changed);REQUIRE(state.activeHookDeaths==1);REQUIRE(state.nested==0);
    REQUIRE_FALSE(weakVictim.expired());REQUIRE(saved.isHostObject("test.PassiveAuditVictim"));
    saved={};witness.collectGarbage();REQUIRE(weakVictim.expired());
}

TEST_CASE("An active native owner can rescue its child while passive numeric garbage is present", "[runtime-native][passive-gc]") {
    auto witness=JSValue::object();JSValue saved;int parentDestroyed=0,childDestroyed=0;std::size_t nested=99;
    std::weak_ptr<NativeOwner> childWeak;std::vector<JSWeakValue> numeric;
    {
        auto parent=std::make_shared<NativeOwner>(),child=std::make_shared<NativeOwner>();parent->child=child;childWeak=child;
        parent->destroyed=&parentDestroyed;child->destroyed=&childDestroyed;parent->saveDuringDestruction=&saved;
        parent->collectDuringDestruction=&witness;parent->nestedCollected=&nested;
        auto a=wrap(parent),b=wrap(child);b.set("answer",71);parent->field=JSValue::object();parent->field.set("child",b);
        for(unsigned i=0;i<64;++i)numeric.emplace_back(NativeArray_detail::wrap(InterpolantArray(Float32Array({1,2,3,4}))));
    }
    witness.collectGarbage();REQUIRE(parentDestroyed==1);REQUIRE(childDestroyed==0);REQUIRE(nested==0);
    REQUIRE(saved.get("child").isHostObject("test.NativeOwner"));REQUIRE(saved.get("child").get("answer").number()==71);
    for(const auto& weak:numeric)REQUIRE(weak.expired());saved={};witness.collectGarbage();REQUIRE(childWeak.expired());REQUIRE(childDestroyed==1);
}
TEST_CASE("Every same-owner alias must be passive before its group can share a batch with victims", "[runtime-native][passive-gc]") {
    for(const bool activeOnFirst:{false,true}){
        auto witness=JSValue::object();JSValue saved;JSWeakValue victimWeak;unsigned callbacks=0;std::size_t nested=99;
        {
            auto group=std::make_shared<std::array<int,2>>(),victimOwner=std::make_shared<std::array<int,2>>();
            if(std::owner_less<std::shared_ptr<std::array<int,2>>>{}(victimOwner,group))std::swap(group,victimOwner);
            auto first=JSValue::hostObject(group,group.get(),"test.PassiveAliasA");
            std::shared_ptr<void> alias(group,&(*group)[1]);auto second=JSValue::hostObject(alias,alias.get(),"test.PassiveAliasB");
            auto victim=JSValue::hostObject(victimOwner,victimOwner.get(),"test.PassiveAliasVictim");victimWeak=JSWeakValue(victim);
            auto passive=std::make_shared<JSHostObjectHooks>();passive->passiveFinalizer=true;
            passive->nativeOwnership=activeOnFirst?+[](const JSValue& self){return std::vector<JSNativeOwnerNode>{{self.hostPayload("test.PassiveAliasB"),{},{}}};}:+[](const JSValue& self){return std::vector<JSNativeOwnerNode>{{self.hostPayload("test.PassiveAliasA"),{},{}}};};
            auto active=std::shared_ptr<JSHostObjectHooks>(new JSHostObjectHooks,[&](auto* h){++callbacks;if(const auto target=victimWeak.lock())saved=*target;nested=witness.collectGarbage();delete h;});
            active->nativeOwnership=activeOnFirst?+[](const JSValue& self){return std::vector<JSNativeOwnerNode>{{self.hostPayload("test.PassiveAliasA"),{},{}}};}:+[](const JSValue& self){return std::vector<JSNativeOwnerNode>{{self.hostPayload("test.PassiveAliasB"),{},{}}};};
            first.setHostHooks(activeOnFirst?active:passive);second.setHostHooks(activeOnFirst?passive:active);
            auto vh=std::make_shared<JSHostObjectHooks>();vh->passiveFinalizer=true;vh->nativeOwnership=[](const JSValue& self){return std::vector<JSNativeOwnerNode>{{self.hostPayload("test.PassiveAliasVictim"),{},{}}};};victim.setHostHooks(vh);
        }
        witness.collectGarbage();REQUIRE(callbacks==1);REQUIRE(nested==0);REQUIRE_FALSE(victimWeak.expired());
        REQUIRE(saved.isHostObject("test.PassiveAliasVictim"));saved={};witness.collectGarbage();REQUIRE(victimWeak.expired());
    }
}
