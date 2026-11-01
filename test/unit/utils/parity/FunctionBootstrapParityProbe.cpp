#include "core/JSFunction.h"
#include "core/JSGenerator.h"
#include "core/JSGlobal.h"
#include <iostream>
#include <string>
namespace {
using Args=std::vector<JSValue>;
JSValue empty(const JSValue&,const Args&,const Args&){return {};}
JSGeneratorStep body(const JSValue&,const JSGeneratorCompletion& c,const JSValue&,const Args&,const Args&){if(c.kind==JSGeneratorCompletionKind::Throw)return JSGeneratorStep::abrupt(c.value);return JSGeneratorStep::complete();}
void property(JSValue object,const char* name,JSValue value,bool writable=true,bool configurable=true){JSPropertyDescriptor d;d.value=value;d.writable=writable;d.enumerable=false;d.configurable=configurable;object.defineProperty(name,d);}
void freeze(JSValue object){for(const auto& key:object.ownPropertyKeys()){auto d=*object.getOwnPropertyDescriptor(key);d.configurable=false;if(d.value)d.writable=false;object.defineProperty(key,d);}object.preventExtensions();}
JSValue run(const std::string& mode){
 // First managed allocation is an ordinary function, before explicit intrinsic/module access.
 const auto first=JSValue::function(empty,{},"sample",0);auto fp=first.getPrototype();
 const auto earlyDescriptor=fp.getOwnPropertyDescriptor("constructor");const auto earlyConstructor=first.get("constructor");
 const bool earlyOwn=earlyDescriptor.has_value();const bool earlyName=earlyConstructor.get("name").strictEquals("Function");
 const bool earlyPrototype=earlyConstructor.get("prototype").sameValue(fp);
 const auto privateFunction=nativeFunctionConstructor();
 auto out=JSValue::object();out.set("earlyOwnConstructor",earlyOwn);out.set("earlyConstructorName",earlyName);
 out.set("earlyPrototypeIdentity",earlyPrototype);out.set("earlyPrivateIdentity",earlyConstructor.sameValue(privateFunction));
 out.set("earlyDescriptor",earlyDescriptor&&earlyDescriptor->writable.value_or(false)&&!earlyDescriptor->enumerable.value_or(false)&&earlyDescriptor->configurable.value_or(false));
 auto global=nativeGlobalObject();out.set("initialGlobalIdentity",global.get("Function").sameValue(privateFunction));
 const auto marker=JSValue::object(nullptr);property(marker,"marker",17);
 if(mode=="preventExtensions")fp.preventExtensions();else if(mode=="freeze")freeze(fp);
 else if(mode=="nonconfigurable")property(fp,"constructor",marker,false,false);
 else if(mode=="replaceConstructor")property(fp,"constructor",marker);
 else if(mode=="deleteConstructor"){auto mutablePrototype=fp;mutablePrototype.erase("constructor");}
 else if(mode=="replaceGlobal")global.set("Function",marker);
 else if(mode=="deleteGlobal")global.erase("Function");
 const auto before=fp.getOwnPropertyDescriptor("constructor");const bool extensibleBefore=fp.isExtensible();
 try{
  const auto gf=nativeGeneratorFunctionConstructor(),gfp=nativeGeneratorFunctionPrototype();
  const auto function=createNativeGeneratorFunction(body,{},"fixture");const auto generator=function.call();
  const auto after=fp.getOwnPropertyDescriptor("constructor");
  bool same=before.has_value()==after.has_value();if(before&&after)same=same&&before->value->sameValue(*after->value)&&before->writable==after->writable&&before->enumerable==after->enumerable&&before->configurable==after->configurable;
  out.set("imported",true);out.set("prototypeDescriptorUnchanged",same);out.set("prototypeExtensibilityUnchanged",fp.isExtensible()==extensibleBefore);
  out.set("privateFunctionStable",nativeFunctionConstructor().sameValue(privateFunction));
  out.set("generatorConstructorParent",gf.getPrototype().sameValue(privateFunction));
  out.set("generatorFunctionPrototypeParent",gfp.getPrototype().sameValue(fp));
  out.set("newOrdinaryFunctionParent",JSValue::function(empty).getPrototype().sameValue(fp));
  out.set("generatorWorks",generator.get("next").call(generator).get("done").strictEquals(true));
  out.set("globalUnchanged",mode=="replaceGlobal"?global.get("Function").sameValue(marker):mode=="deleteGlobal"?!global.has("Function"):global.get("Function").sameValue(privateFunction));
 }catch(const JSTypeError&){out.set("error","#TypeError");}catch(const std::exception&){out.set("error","#Error");}
 return out;
}
void lifetime(){for(unsigned round=0;round<3;++round){JSWeakValue weakFunction,weakPrototype,weakGenerator,weakGeneratorPrototype;{
 const auto first=JSValue::function(empty);const auto fp=first.getPrototype(),ctor=first.get("constructor");
 if(!fp.hasOwn("constructor")||!ctor.sameValue(nativeFunctionConstructor()))throw std::logic_error("Function intrinsic was not bootstrapped");
 weakFunction=JSWeakValue(ctor);weakPrototype=JSWeakValue(fp);freeze(fp);
 const auto generatorConstructor=nativeGeneratorFunctionConstructor();weakGenerator=JSWeakValue(generatorConstructor);weakGeneratorPrototype=JSWeakValue(nativeGeneratorPrototype());
 const auto generator=createNativeGeneratorFunction(body).call();generator.get("next").call(generator);generator.collectGarbage();
 if(weakFunction.expired()||weakPrototype.expired()||weakGenerator.expired()||weakGeneratorPrototype.expired())throw std::logic_error("Live intrinsic collected");
 }JSValue::nativeSafepoint();if(!weakFunction.expired()||!weakPrototype.expired()||!weakGenerator.expired()||!weakGeneratorPrototype.expired()||JSValue::hasActiveHeap())throw std::logic_error("Bootstrap intrinsic retained retired heap");}
 std::cout<<"FunctionBootstrap lifetime: 3 passed\n";
}
}
int main(int argc,char**argv){try{const std::string mode=argc>1?argv[1]:"fresh";if(mode=="--lifetime")lifetime();else std::cout<<run(mode).toJSON()->dump();}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
