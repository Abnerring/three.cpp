#include "extras/ControlsHost.h"
#include <array>
#include <thread>
#include <utility>

struct NativeControlsInput::State {
    double width=0,height=0,left=0,top=0,surfaceWidth=0,surfaceHeight=0;
    std::array<std::string,3> touch;
    std::array<JSWeakValue,3> targets;
};
struct NativeControlsBindingRegistry;
struct NativeControlsInput::BindingOwner {
    JSValue realm;
    std::weak_ptr<BindingOwner> self;
    std::shared_ptr<NativeControlsBindingRegistry> registry;
    std::thread::id thread;
    bool active=false;
    ~BindingOwner();
};
struct NativeControlsBindingRegistry {
    std::weak_ptr<NativeControlsInput::BindingOwner> active;
};
namespace {
using Args=std::vector<JSValue>;
constexpr const char* targetTag="native.ControlsInputTarget";
constexpr const char* styleTag="native.ControlsInputPolicy";
struct TargetState {std::shared_ptr<NativeControlsInput::State> input;unsigned kind;};
void own(JSValue object,const std::string& key,const JSValue& value){JSPropertyDescriptor d;d.value=value;d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
JSValue argument(const Args& args,unsigned i){return i<args.size()?args[i]:JSValue{};}
std::shared_ptr<TargetState> targetState(const JSValue& value,const char* tag=targetTag){auto payload=std::static_pointer_cast<TargetState>(value.hostPayload(tag));if(!payload)throw JSTypeError("Illegal native controls input invocation");return payload;}
JSValue listeners(JSValue target,const std::string& type,bool create){auto map=target.getHostSlot("listeners");if(map.isUndefined()&&create){map=JSValue::object(nullptr);target.setHostSlot("listeners",map);}if(map.isUndefined())return {};auto list=map.get(type);if(list.isUndefined()&&create){list=JSValue::array();map.set(type,list);}return list;}
JSValue add(const JSValue& self,const Args& args,const Args&){targetState(self);const auto type=argument(args,0).toString();const auto callback=argument(args,1);if(callback.isNull()||callback.isUndefined())return {};if(!callback.isCallable())throw JSTypeError("Native controls listener must be callable");auto list=listeners(self,type,true);for(std::uint32_t i=0;i<list.length();++i){const auto entry=list.get(std::to_string(i));if(!entry.get("removed").toBoolean()&&entry.get("callback").sameValue(callback))return {};}auto entry=JSValue::object();own(entry,"callback",callback);own(entry,"removed",false);list.set(std::to_string(list.length()),entry);return {};}
JSValue remove(const JSValue& self,const Args& args,const Args&){targetState(self);const auto type=argument(args,0).toString();const auto callback=argument(args,1);auto list=listeners(self,type,false);if(list.isUndefined())return {};for(std::uint32_t i=0;i<list.length();++i){auto entry=list.get(std::to_string(i));if(!entry.get("removed").toBoolean()&&entry.get("callback").sameValue(callback)){entry.set("removed",true);entry.set("callback",JSValue{});break;}}return {};}
JSValue metric(const JSValue& self,const Args&,const Args& captures){const auto state=targetState(self);const auto& data=*state->input;const auto field=captures[0].string();if(field=="innerWidth")return data.width;if(field=="innerHeight")return data.height;if(field=="offsetLeft")return state->kind==2?data.left:0;if(field=="offsetTop")return state->kind==2?data.top:0;if(field=="offsetWidth")return state->kind==2?data.surfaceWidth:data.width;return state->kind==2?data.surfaceHeight:data.height;}
JSValue touchGet(const JSValue& self,const Args&,const Args&){const auto state=targetState(self,styleTag);return state->input->touch[state->kind];}
JSValue touchSet(const JSValue& self,const Args& args,const Args&){const auto state=targetState(self,styleTag);state->input->touch[state->kind]=argument(args,0).toString();return {};}
JSValue hostRealm(){static thread_local JSWeakValue weak;if(const auto value=weak.lock();value&&value->isObject())return *value;auto realm=JSValue::object(nullptr);realm.retainForHeapLifetime();weak=JSWeakValue(realm);return realm;}
std::shared_ptr<NativeControlsBindingRegistry> bindingRegistry(){
 static thread_local std::weak_ptr<NativeControlsBindingRegistry> weak;
 if(auto current=weak.lock())return current;
 auto current=std::make_shared<NativeControlsBindingRegistry>();weak=current;return current;
}
bool sameBindingOwner(const std::weak_ptr<NativeControlsInput::BindingOwner>& a,const std::weak_ptr<NativeControlsInput::BindingOwner>& b){return !a.owner_before(b)&&!b.owner_before(a);}
void releaseBinding(NativeControlsInput::BindingOwner& owner){
 if(!owner.active)return;
 // The owner retains this registry independently of TLS destruction order.
 auto registry=owner.registry;
 if(registry&&sameBindingOwner(registry->active,owner.self)){
  auto realm=owner.realm;
  realm.setInternalSlot("window",JSValue{});realm.setInternalSlot("document",JSValue{});
  registry->active.reset();
 }
 owner.active=false;owner.realm={};owner.registry.reset();
}
}
NativeControlsInput::BindingOwner::~BindingOwner(){releaseBinding(*this);}
NativeControlsInput::NativeControlsInput():state_(std::make_shared<State>()),binding_(std::make_shared<BindingOwner>()){binding_->self=binding_;}
JSValue NativeControlsInput::target(Target which)const{
 const auto index=static_cast<unsigned>(which);if(index>2)throw std::invalid_argument("Unknown native controls target");if(const auto value=state_->targets[index].lock();value&&value->isObject())return *value;
 auto payload=std::make_shared<TargetState>(TargetState{state_,index});auto result=JSValue::hostObject(payload,payload.get(),targetTag);auto hooks=std::make_shared<JSHostObjectHooks>();hooks->passiveFinalizer=true;result.setHostHooks(hooks);
 own(result,"addEventListener",JSValue::function(add,{},"addEventListener",2));own(result,"removeEventListener",JSValue::function(remove,{},"removeEventListener",2));
 for(const auto* key:{"innerWidth","innerHeight","offsetWidth","offsetHeight","offsetLeft","offsetTop"}){JSPropertyDescriptor d;d.getter=JSValue::function(metric,{key},std::string("get ")+key,0);d.enumerable=d.configurable=true;result.defineProperty(key,d);}
 auto stylePayload=std::make_shared<TargetState>(TargetState{state_,index});auto style=JSValue::hostObject(stylePayload,stylePayload.get(),styleTag);style.setHostHooks(hooks);JSPropertyDescriptor d;d.getter=JSValue::function(touchGet,{},"get touchAction",0);d.setter=JSValue::function(touchSet,{},"set touchAction",1);d.enumerable=d.configurable=true;style.defineProperty("touchAction",d);own(result,"style",style);
 state_->targets[index]=JSWeakValue(result);return result;
}
void NativeControlsInput::setWindowSize(double width,double height){state_->width=width;state_->height=height;}
void NativeControlsInput::setSurfaceBounds(double left,double top,double width,double height){state_->left=left;state_->top=top;state_->surfaceWidth=width;state_->surfaceHeight=height;}
std::string NativeControlsInput::touchAction(Target which)const{return state_->touch.at(static_cast<unsigned>(which));}
bool NativeControlsInput::dispatch(Target which,const std::string& type,JSValue event)const{
 auto receiver=target(which);if(event.isUndefined())event=JSValue::object();if(!event.isObject())throw JSTypeError("Native input event must be an object");event.set("type",type);event.set("target",receiver);event.set("currentTarget",receiver);event.set("defaultPrevented",false);event.set("preventDefault",JSValue::function(+[](const JSValue& self,const Args&,const Args&)->JSValue{auto writable=self;writable.set("defaultPrevented",true);return {};},{},"preventDefault",0));
 auto list=listeners(receiver,type,false);std::vector<JSValue> snapshot;if(!list.isUndefined())for(std::uint32_t i=0;i<list.length();++i)snapshot.push_back(list.get(std::to_string(i)));
 try{for(const auto& entry:snapshot)if(!entry.get("removed").toBoolean())entry.get("callback").call(receiver,{event});}catch(...){event.set("currentTarget",JSValue(nullptr));throw;}event.set("currentTarget",JSValue(nullptr));return event.get("defaultPrevented").toBoolean();
}
void NativeControlsInput::keyDown(const std::string& code,bool altKey)const{auto e=JSValue::object();e.set("code",code);e.set("altKey",altKey);dispatch(Target::Window,"keydown",e);}
void NativeControlsInput::keyUp(const std::string& code)const{auto e=JSValue::object();e.set("code",code);dispatch(Target::Window,"keyup",e);}
void NativeControlsInput::pointerMove(double pageX,double pageY)const{auto e=JSValue::object();e.set("pageX",pageX);e.set("pageY",pageY);dispatch(Target::Surface,"pointermove",e);}
void NativeControlsInput::pointerDown(int button)const{auto e=JSValue::object();e.set("button",button);dispatch(Target::Surface,"pointerdown",e);}
void NativeControlsInput::pointerUp(int button)const{auto e=JSValue::object();e.set("button",button);dispatch(Target::Surface,"pointerup",e);}
void NativeControlsInput::pointerCancel()const{dispatch(Target::Surface,"pointercancel");}
bool NativeControlsInput::contextMenu()const{return dispatch(Target::Surface,"contextmenu");}
void bindNativeControlsInput(const NativeControlsInput& input){
 const auto owner=input.binding_;
 if(!owner)throw JSTypeError("Cannot bind a moved native controls input");
 if(owner->active&&owner->thread!=std::this_thread::get_id())throw JSTypeError("Native controls binding belongs to another thread");
 auto realm=hostRealm();auto window=input.window(),document=input.document();
 auto registry=bindingRegistry();auto previous=registry->active.lock();
 // Validate and publish real host values while the local realm is rooted.
 realm.setInternalSlot("window",window);realm.setInternalSlot("document",document);
 if(previous&&previous!=owner){previous->active=false;previous->realm={};previous->registry.reset();}
 owner->realm=realm;owner->registry=registry;owner->thread=std::this_thread::get_id();owner->active=true;registry->active=owner;
}
void clearNativeControlsInput(){
 auto registry=bindingRegistry();
 if(auto owner=registry->active.lock()){releaseBinding(*owner);return;}
 auto realm=hostRealm();realm.setInternalSlot("window",JSValue{});realm.setInternalSlot("document",JSValue{});
}
JSValue nativeControlsHostBinding(const std::string& name){auto value=hostRealm().getInternalSlot(name);if(value.isUndefined())throw JSReferenceError(name+" is not defined");return value;}
