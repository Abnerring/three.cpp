#include "catch.hpp"
#include "core/JSValue.h"
#include <exception>
#include <functional>
#include <thread>
#include <unordered_set>

namespace {
void inFreshThread(const std::function<void()>& task) {
 std::exception_ptr error;std::thread thread([&]{try{task();}catch(...){error=std::current_exception();}});
 thread.join();if(error)std::rethrow_exception(error);
}
void requireGraph(bool value){if(!value)throw std::runtime_error("native graph stability check failed");}
struct GraphNode {
 JSValue field;
 std::shared_ptr<GraphNode> child;
 std::weak_ptr<GraphNode> replacement,other;
 bool fired=false,throwAudit=false;
 unsigned traceCalls=0,settleAfter=0;
 unsigned* destroyed=nullptr;
 JSValue* escapedField=nullptr;
 const JSValue* formerField=nullptr;
 bool aliasNull=false;
 std::vector<JSValue> values;
 std::optional<JSValue> optionalField;
 std::unique_ptr<JSValue> uniqueField;
 ~GraphNode(){if(destroyed)++*destroyed;if(escapedField)*escapedField=field;}
};
thread_local std::shared_ptr<GraphNode>* escapedOwner=nullptr;
thread_local std::weak_ptr<GraphNode> previousDescriptor;
thread_local bool descriptorMutation=false;
std::vector<JSNativeOwnerNode> graphOwners(const JSValue& self) {
 auto root=std::static_pointer_cast<GraphNode>(self.hostPayload("test.StableNativeGraph"));
 if(root->throwAudit&&root->fired){root->throwAudit=false;throw std::runtime_error("after graph replacement");}
 std::vector<JSNativeOwnerNode> result;std::unordered_set<GraphNode*> seen;
 std::function<void(const std::shared_ptr<GraphNode>&)> visit=[&](const auto& node){
  if(!node||!seen.insert(node.get()).second)return;
  JSNativeOwnerNode description{node->aliasNull?std::shared_ptr<void>(node,nullptr):node,{&node->field},{}};
  for(const auto& value:node->values)description.roots.push_back(&value);
  if(node->optionalField)description.roots.push_back(&*node->optionalField);
  if(node->uniqueField)description.roots.push_back(node->uniqueField.get());
  if(node->child)description.edges.push_back({&node->child,node->child});
  result.push_back(std::move(description));visit(node->child);
 };
 visit(root);return result;
}
JSValue graphWrapper(const std::shared_ptr<GraphNode>& node) {
 auto value=JSValue::hostObject(node,node.get(),"test.StableNativeGraph");
 auto hooks=std::make_shared<JSHostObjectHooks>();hooks->nativeOwnership=graphOwners;
 value.setHostHooks(hooks);return value;
}
void installTrace(JSValue& value,std::vector<JSValue>(*callback)(const JSValue&)) {
 auto hooks=std::make_shared<JSHostObjectHooks>();hooks->nativeOwnership=graphOwners;hooks->trace=callback;
 value.setHostHooks(hooks);
}
}
TEST_CASE("Native trace replacement of an existing child preserves its live wrapper") {
 inFreshThread([] {
  auto parent=std::make_shared<GraphNode>(),a=std::make_shared<GraphNode>(),b=std::make_shared<GraphNode>();
  parent->child=a;parent->replacement=b;auto root=graphWrapper(parent);
  JSWeakValue old(graphWrapper(a)),next(graphWrapper(b));
  installTrace(root,+[](const JSValue& self){
   auto node=std::static_pointer_cast<GraphNode>(self.hostPayload("test.StableNativeGraph"));
   if(!node->fired){node->fired=true;node->child=node->replacement.lock();}
   return std::vector<JSValue>{};
  });
  parent.reset();a.reset();b.reset();root.collectGarbage();
  auto retained=next.lock();requireGraph(retained&&retained->isHostObject("test.StableNativeGraph"));
  requireGraph(old.expired());
 });REQUIRE(true);
}
TEST_CASE("Native trace swaps two existing edges despite unchanged native owner counts") {
 inFreshThread([] {
  auto p=std::make_shared<GraphNode>(),q=std::make_shared<GraphNode>();
  auto a=std::make_shared<GraphNode>(),b=std::make_shared<GraphNode>();
  p->child=a;q->child=b;p->other=q;auto root=graphWrapper(p);
  auto qWrapper=graphWrapper(q);JSWeakValue old(graphWrapper(a)),next(graphWrapper(b));
  installTrace(root,+[](const JSValue& self){
   auto node=std::static_pointer_cast<GraphNode>(self.hostPayload("test.StableNativeGraph"));
   if(!node->fired){node->fired=true;auto other=node->other.lock();std::swap(node->child,other->child);}
   return std::vector<JSValue>{};
  });
  p.reset();q.reset();a.reset();b.reset();qWrapper={};root.collectGarbage();
  auto retained=next.lock();requireGraph(retained&&retained->isHostObject("test.StableNativeGraph"));
  requireGraph(old.expired());
 });REQUIRE(true);
}
TEST_CASE("Native graph field identity swaps while per-record JS root counts remain unchanged") {
 inFreshThread([] {
  auto p=std::make_shared<GraphNode>(),q=std::make_shared<GraphNode>();
  auto a=JSValue::object(),b=JSValue::object();a.set("name","old");b.set("name","new");
  p->field=a;q->field=b;p->other=q;
  auto root=graphWrapper(p);auto qWrapper=graphWrapper(q);JSWeakValue old(a),next(b);
  installTrace(root,+[](const JSValue& self){
   auto node=std::static_pointer_cast<GraphNode>(self.hostPayload("test.StableNativeGraph"));
   if(!node->fired){node->fired=true;auto other=node->other.lock();std::swap(node->field,other->field);}
   return std::vector<JSValue>{};
  });
  p.reset();q.reset();a={};b={};qWrapper={};root.collectGarbage();
  auto retained=next.lock();requireGraph(retained&&retained->get("name").string()=="new");requireGraph(old.expired());
 });REQUIRE(true);
}
TEST_CASE("Native trace external-owner escape is detected without new JS allocations") {
 inFreshThread([] {
  std::shared_ptr<GraphNode> escaped;escapedOwner=&escaped;
  auto p=std::make_shared<GraphNode>(),b=std::make_shared<GraphNode>();p->replacement=b;
  auto root=graphWrapper(p);JSWeakValue next(graphWrapper(b));
  installTrace(root,+[](const JSValue& self){
   auto node=std::static_pointer_cast<GraphNode>(self.hostPayload("test.StableNativeGraph"));
   if(!node->fired){node->fired=true;*escapedOwner=node->replacement.lock();}
   return std::vector<JSValue>{};
  });
  p.reset();b.reset();root.collectGarbage();
  {auto retained=next.lock();requireGraph(retained&&retained->isHostObject("test.StableNativeGraph"));}
  escaped.reset();root.collectGarbage();requireGraph(next.expired());escapedOwner=nullptr;
 });REQUIRE(true);
}
TEST_CASE("Native graph hooks converge over multiple edge replacements before sweeping") {
 inFreshThread([] {
  auto p=std::make_shared<GraphNode>(),a=std::make_shared<GraphNode>(),b=std::make_shared<GraphNode>();
  p->child=a;p->replacement=b;p->other=a;p->settleAfter=3;auto root=graphWrapper(p);
  JSWeakValue next(graphWrapper(b));static_cast<void>(graphWrapper(a));
  installTrace(root,+[](const JSValue& self){
   auto node=std::static_pointer_cast<GraphNode>(self.hostPayload("test.StableNativeGraph"));
   ++node->traceCalls;requireGraph(node->traceCalls<16);
   if(node->traceCalls<=node->settleAfter)
    node->child=(node->traceCalls%2?node->replacement:node->other).lock();
   return std::vector<JSValue>{};
  });
  p.reset();a.reset();b.reset();root.collectGarbage();
  auto retained=next.lock();requireGraph(retained&&retained->isHostObject("test.StableNativeGraph"));
  auto owner=std::static_pointer_cast<GraphNode>(root.hostPayload("test.StableNativeGraph"));
  requireGraph(owner->traceCalls>=4);
 });REQUIRE(true);
}
TEST_CASE("Native post-trace audit errors propagate before any payload retirement and allow retry") {
 inFreshThread([] {
  auto p=std::make_shared<GraphNode>(),a=std::make_shared<GraphNode>(),b=std::make_shared<GraphNode>();
  p->child=a;p->replacement=b;p->throwAudit=true;auto root=graphWrapper(p);
  JSWeakValue old(graphWrapper(a)),next(graphWrapper(b));
  installTrace(root,+[](const JSValue& self){
   auto node=std::static_pointer_cast<GraphNode>(self.hostPayload("test.StableNativeGraph"));
   if(!node->fired){node->fired=true;node->child=node->replacement.lock();}
   return std::vector<JSValue>{};
  });
  p.reset();a.reset();b.reset();bool threw=false;
  try{root.collectGarbage();}catch(const std::runtime_error& error){threw=std::string(error.what())=="after graph replacement";}
  requireGraph(threw);
  {auto retained=next.lock();requireGraph(retained&&retained->isHostObject("test.StableNativeGraph"));}
  root.collectGarbage();requireGraph(old.expired());
  auto retained=next.lock();requireGraph(retained&&retained->isHostObject("test.StableNativeGraph"));
 });REQUIRE(true);
}

TEST_CASE("Native trace pins a described child without a cached wrapper until field reads finish") {
 for(bool aliasNull:{false,true})inFreshThread([=] {
  unsigned destroyed=0;
  auto p=std::make_shared<GraphNode>(),a=std::shared_ptr<GraphNode>(new GraphNode),b=std::make_shared<GraphNode>();
  a->field=JSValue::object();a->destroyed=&destroyed;a->aliasNull=aliasNull;
  p->child=a;p->other=a;p->replacement=b;auto root=graphWrapper(p);
  JSWeakValue oldField(a->field),next(graphWrapper(b));
  installTrace(root,+[](const JSValue& self){
   auto node=std::static_pointer_cast<GraphNode>(self.hostPayload("test.StableNativeGraph"));
   if(!node->fired){node->fired=true;node->child=node->replacement.lock();requireGraph(!node->other.expired());}
   return std::vector<JSValue>{};
  });
  p.reset();a.reset();b.reset();root.collectGarbage();
  requireGraph(destroyed==1&&oldField.expired());
  auto retained=next.lock();requireGraph(retained&&retained->isHostObject("test.StableNativeGraph"));
 });REQUIRE(true);
}
TEST_CASE("Releasing the native graph snapshot detects a finalizer publishing an existing JS field") {
 inFreshThread([] {
  unsigned destroyed=0;JSValue escaped;
  auto p=std::make_shared<GraphNode>(),a=std::shared_ptr<GraphNode>(new GraphNode),b=std::make_shared<GraphNode>();
  a->field=JSValue::object();a->field.set("saved",42);a->destroyed=&destroyed;a->escapedField=&escaped;
  p->child=a;p->other=a;p->replacement=b;auto root=graphWrapper(p);JSWeakValue oldField(a->field);
  static_cast<void>(graphWrapper(b));
  installTrace(root,+[](const JSValue& self){
   auto node=std::static_pointer_cast<GraphNode>(self.hostPayload("test.StableNativeGraph"));
   if(!node->fired){node->fired=true;node->child=node->replacement.lock();requireGraph(!node->other.expired());}
   return std::vector<JSValue>{};
  });
  p.reset();a.reset();b.reset();root.collectGarbage();
  requireGraph(destroyed==1&&escaped.get("saved").number()==42&&!oldField.expired());
  escaped={};root.collectGarbage();requireGraph(oldField.expired());
 });REQUIRE(true);
}
TEST_CASE("Expired native owner descriptions are discarded before their former field address is read") {
 inFreshThread([] {
  std::weak_ptr<GraphNode> expired;const JSValue* formerField=nullptr;
  {auto old=std::shared_ptr<GraphNode>(new GraphNode);expired=old;formerField=&old->field;}
  auto owner=std::make_shared<GraphNode>();owner->replacement=expired;owner->formerField=formerField;
  auto root=graphWrapper(owner);auto hooks=std::make_shared<JSHostObjectHooks>();
  hooks->nativeOwnership=+[](const JSValue& self){
   auto descriptions=graphOwners(self);
   auto node=std::static_pointer_cast<GraphNode>(self.hostPayload("test.StableNativeGraph"));
   descriptions.push_back({node->replacement,{node->formerField},{}});return descriptions;
  };
  root.setHostHooks(hooks);owner.reset();root.collectGarbage();requireGraph(root.isHostObject("test.StableNativeGraph"));
  JSWeakValue weak(root);root={};requireGraph(weak.expired());
 });REQUIRE(true);
}
TEST_CASE("Native trace can destroy vector optional and unique field storage before the next native mark") {
 for(unsigned mode=0;mode<3;++mode)inFreshThread([=] {
  auto owner=std::make_shared<GraphNode>();owner->settleAfter=mode;
  auto old=JSValue::object();old.set("name","old");owner->field=JSValue::object();owner->field.set("name","new");
  if(mode==0)owner->values.push_back(old);
  else if(mode==1)owner->optionalField=old;
  else owner->uniqueField=std::make_unique<JSValue>(old);
  auto root=graphWrapper(owner);JSWeakValue oldField(old),next(owner->field);
  installTrace(root,+[](const JSValue& self){
   auto node=std::static_pointer_cast<GraphNode>(self.hostPayload("test.StableNativeGraph"));
   if(!node->fired){
    node->fired=true;
    if(node->settleAfter==0){std::vector<JSValue>().swap(node->values);node->values.reserve(64);node->values.push_back(node->field);}
    else if(node->settleAfter==1)node->optionalField.reset();
    else node->uniqueField.reset();
   }
   return std::vector<JSValue>{};
  });
  owner.reset();old={};root.collectGarbage();requireGraph(oldField.expired());
  {auto retained=next.lock();requireGraph(retained&&retained->get("name").string()=="new");}
  root={};requireGraph(next.expired());
 });REQUIRE(true);
}
TEST_CASE("Native roots are snapshotted before the next ownership hook changes earlier field storage") {
 inFreshThread([] {
  auto a=std::make_shared<GraphNode>(),b=std::make_shared<GraphNode>();
  a->uniqueField=std::make_unique<JSValue>(JSValue::object());b->uniqueField=std::make_unique<JSValue>(JSValue::object());
  JSWeakValue oldA(*a->uniqueField),oldB(*b->uniqueField);auto rootA=graphWrapper(a),rootB=graphWrapper(b);
  auto hooks=std::make_shared<JSHostObjectHooks>();
  hooks->nativeOwnership=+[](const JSValue& self){
   auto node=std::static_pointer_cast<GraphNode>(self.hostPayload("test.StableNativeGraph"));
   auto description=graphOwners(self);
   if(!descriptorMutation){
    if(auto previous=previousDescriptor.lock()){
     if(previous!=node){previous->uniqueField.reset();descriptorMutation=true;}
    }else previousDescriptor=node;
   }
   return description;
  };
  rootA.setHostHooks(hooks);rootB.setHostHooks(hooks);a.reset();b.reset();rootA.collectGarbage();
  requireGraph(descriptorMutation&&oldA.expired()!=oldB.expired());
  rootA={};rootB={};requireGraph(oldA.expired()&&oldB.expired());previousDescriptor.reset();
 });REQUIRE(true);
}
