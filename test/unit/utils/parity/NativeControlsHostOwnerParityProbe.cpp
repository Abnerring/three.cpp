#include "extras/ControlsHost.h"
#include "controls/FlyControls.h"
#include "core/NativeObject.h"
#include "core/Object3D.h"
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
using Args=std::vector<JSValue>;
JSValue call(const JSValue& self,const std::string& name,const Args& args={}){return self.get(name).call(self,args);}
void require(bool condition,const char* message){if(!condition)throw std::logic_error(message);}
bool missingWindow(){try{static_cast<void>(nativeControlsHostBinding("window"));return false;}catch(const JSReferenceError&){return true;}}
void retired(const JSWeakValue& ctor,const JSWeakValue& window){JSValue::nativeSafepoint();require(ctor.expired()&&window.expired(),"Native input owner retained retired realm/window");}

void ownerProtocols(){
 // Default copies share native state and the binding lease; the last copy owns cleanup.
 {JSWeakValue weakCtor,weakWindow;std::optional<NativeControlsInput> copy;
  {NativeControlsInput input;bindNativeControlsInput(input);copy=input;
   {const auto C=nativeFlyControlsConstructor();weakCtor=JSWeakValue(C);weakWindow=JSWeakValue(nativeControlsHostBinding("window"));}}
  JSValue::nativeSafepoint();require(!weakCtor.expired()&&!weakWindow.expired(),"Last native input copy lost live binding");
  {const auto window=nativeControlsHostBinding("window");require(window.sameValue(copy->window()),"Native input copy changed window identity");}
  copy.reset();retired(weakCtor,weakWindow);
 }
 // Destroying an input replaced by another input must leave the newer binding intact.
 {JSWeakValue weakCtor,weakWindow;std::optional<NativeControlsInput> oldInput,current;
  oldInput.emplace();bindNativeControlsInput(*oldInput);current.emplace();bindNativeControlsInput(*current);
  {const auto C=nativeFlyControlsConstructor();weakCtor=JSWeakValue(C);weakWindow=JSWeakValue(nativeControlsHostBinding("window"));}
  oldInput.reset();JSValue::nativeSafepoint();
  {const auto window=nativeControlsHostBinding("window");require(window.sameValue(current->window()),"Old input destructor cleared replacement binding");}
  current.reset();retired(weakCtor,weakWindow);
 }
 // Explicit clear releases the lease even while the unbound C++ input is still alive.
 {NativeControlsInput input;JSWeakValue weakCtor,weakWindow;
  bindNativeControlsInput(input);{const auto C=nativeFlyControlsConstructor();weakCtor=JSWeakValue(C);weakWindow=JSWeakValue(nativeControlsHostBinding("window"));}
  clearNativeControlsInput();retired(weakCtor,weakWindow);require(missingWindow(),"Explicit clear left window bound");
  bindNativeControlsInput(input);
  {const auto C=nativeFlyControlsConstructor();const auto window=nativeControlsHostBinding("window");weakCtor=JSWeakValue(C);weakWindow=JSWeakValue(window);
   bindNativeControlsInput(input);require(window.sameValue(nativeControlsHostBinding("window")),"Rebinding live input changed window identity");}
  clearNativeControlsInput();retired(weakCtor,weakWindow);
 }
 // A retained target keeps its own heap alive, but is not an active global binding.
 {JSWeakValue weakCtor,weakWindow;JSValue retained;
  {NativeControlsInput input;bindNativeControlsInput(input);const auto C=nativeFlyControlsConstructor();weakCtor=JSWeakValue(C);retained=nativeControlsHostBinding("window");weakWindow=JSWeakValue(retained);}
  require(missingWindow(),"Input destructor did not clear its binding");require(!weakWindow.expired(),"Externally retained real target disappeared");
  retained={};retired(weakCtor,weakWindow);
 }
 // No JSValue or host payload may be borrowed from a different owning thread.
 {NativeControlsInput input;bindNativeControlsInput(input);bool rejected=false,foreignMissing=false;
  std::thread foreign([&]{try{bindNativeControlsInput(input);}catch(const JSTypeError&){rejected=true;}foreignMissing=missingWindow();});foreign.join();
  require(rejected&&foreignMissing,"Foreign thread borrowed active input/realm");
  {const auto window=nativeControlsHostBinding("window");require(window.sameValue(input.window()),"Foreign bind changed owning-thread window");}
  clearNativeControlsInput();
 }
 JSValue::nativeSafepoint();
}

void naturalLifecycle(){
 std::vector<std::string> observations;
 for(unsigned round=0;round<3;++round){JSWeakValue weakCtor,weakWindow,weakDocument,weakControl,weakCallback;{
  // Deliberately no JSValue root before bind: the public native input owns the lease.
  NativeControlsInput input;input.setWindowSize(800,600);input.setSurfaceBounds(0,0,800,600);bindNativeControlsInput(input);
  const auto C=nativeFlyControlsConstructor();weakCtor=JSWeakValue(C);
  weakWindow=JSWeakValue(nativeControlsHostBinding("window"));weakDocument=JSWeakValue(nativeControlsHostBinding("document"));JSValue retained;
  double zAfterDown=0,zAfterUp=0;
  {auto control=C.construct({nativeObject3DConstructor().construct(),input.surface()});
   input.keyDown("KeyW");call(control,"update",{.25});zAfterDown=control.get("object").get("position").get("z").toNumber();
   input.keyUp("KeyW");call(control,"update",{.5});zAfterUp=control.get("object").get("position").get("z").toNumber();
   control.set("cycle",control);weakControl=JSWeakValue(control);retained=control.get("_onKeyDown");weakCallback=JSWeakValue(retained);}
  C.collectGarbage();const bool aliveWhileConnected=!weakControl.expired();require(aliveWhileConnected,"Bound real host did not retain connected control");
  {auto control=*weakControl.lock();call(control,"dispose");}
  C.collectGarbage();const bool callbackKeepsReceiver=!weakControl.expired();retained={};C.collectGarbage();const bool released=weakControl.expired();
  require(callbackKeepsReceiver&&released&&weakCallback.expired(),"Native bound callback ownership mismatch");
  auto row=JSValue::object();row.set("round",round);row.set("zAfterDown",zAfterDown);row.set("zAfterUp",zAfterUp);row.set("aliveWhileConnected",aliveWhileConnected);row.set("callbackKeepsReceiver",callbackKeepsReceiver);row.set("released",released);observations.push_back(row.toJSON()->dump());
  // No explicit clear here: native input RAII must remove the real binding itself.
  }JSValue::nativeSafepoint();require(weakCtor.expired()&&weakWindow.expired()&&weakDocument.expired(),"Natural native input scope failed to retire its realm");
 }
 std::cout<<"{\"rounds\":3,\"observations\":[";for(unsigned i=0;i<observations.size();++i){if(i)std::cout<<',';std::cout<<observations[i];}std::cout<<"]}";
}
}

int main(int argc,char** argv){try{if(argc!=1&&!(argc==2&&std::string(argv[1])=="--lifetime"))throw std::invalid_argument("Usage: probe [--lifetime]");ownerProtocols();naturalLifecycle();return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
