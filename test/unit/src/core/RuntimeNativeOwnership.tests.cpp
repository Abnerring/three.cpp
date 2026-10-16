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
