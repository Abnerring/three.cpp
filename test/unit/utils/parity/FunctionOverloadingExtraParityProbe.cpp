// The original 170/8/15 suite is included byte-for-byte and remains the default.
#include "FunctionOverloadingOriginalProbeSupport.inc"
#include "core/JSError.h"
#include "core/JSProxy.h"
#include "core/JSSourceTrace.h"
namespace {
constexpr const char* sourceFile="file:///D:/Temp/surui/threejs-pinned-11738a9-20261101/src/nodes/utils/FunctionOverloadingNode.js";
JSValue extraCapture(){
 auto constructor=nativeErrorConstructor();const auto saved=constructor.getOwnPropertyDescriptor("prepareStackTrace");
 JSPropertyDescriptor d;d.value=JSValue::function(+[](const JSValue&,const Args&a,const Args&)->JSValue{return arg(a,1);});d.writable=d.configurable=true;constructor.defineProperty("prepareStackTrace",d);
 JSValue sites;try{sites=constructor.construct().get("stack");}catch(...){if(saved)constructor.defineProperty("prepareStackTrace",*saved);else constructor.erase("prepareStackTrace");throw;}
 if(saved)constructor.defineProperty("prepareStackTrace",*saved);else constructor.erase("prepareStackTrace");
 auto rows=array();for(unsigned i=0;i<sites.length();++i){auto site=sites.get(JSValue(i));if(!call(site,"getFileName").strictEquals(sourceFile))continue;
  push(rows,rec({{"functionName",call(site,"getFunctionName")},{"line",call(site,"getLineNumber")},{"column",call(site,"getColumnNumber")},{"position",call(site,"getPosition")},{"enclosingLine",call(site,"getEnclosingLineNumber")},{"enclosingColumn",call(site,"getEnclosingColumnNumber")},{"isConstructor",call(site,"isConstructor")},{"isAsync",call(site,"isAsync")},{"isNative",call(site,"isNative")}}));
 }return rows;
}
void extraNote(const JSValue& context,const JSValue& label){
 push(context.get("events"),label);push(context.get("frames"),rec({{"label",label},{"frames",extraCapture()}}));
 const auto request=context.get("request");if(request.get("gc").toBoolean())context.collectGarbage();if(request.get("throw").toBoolean())throw JSThrownValue(context.get("reason"));
 if(request.get("reenter").toBoolean()&&!context.get("entered").toBoolean()){
  auto mutableContext=context;mutableContext.set("entered",true);push(context.get("events"),"reenter.before");call(context.get("node"),"getCandidateFn",{context.get("builder")});push(context.get("events"),"reenter.after");
 }
}
void extraGetter(JSValue target,const JSValue& key,const JSValue& label,const JSValue& value,const JSValue& context){
 JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{extraNote(c[0],c[1]);return c[2];},{context,label,value},"get",0);target.defineProperty(key,d);
}
JSValue extraTrace(const JSValue& request){
 auto context=rec({{"request",request},{"events",array()},{"frames",array()},{"reason",rec({{"marker","reason"}})},{"builder",rec({{"marker","builder"}})},{"entered",false}});
 auto parameter=JSValue::object();own(parameter,"getNodeType",JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{extraNote(c[0],"param.call");return "float";},{context},"getNodeType",0));
 auto candidate=JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{extraNote(c[0],"candidate.call");return "result";},{context},"candidate",0),input=rec({{"type","float"}}),inputs=array({input}),layout=rec({{"inputs",inputs},{"type","float"}}),shader=rec({{"layout",layout}});own(candidate,"shaderNode",shader);
 auto ctor=nativeFunctionOverloadingNodeConstructor(),n=ctor.construct({array({candidate}),parameter}),proto=ctor.get("prototype");context.set("node",n);auto target=request.get("target");JSValue changed,key;std::optional<JSPropertyDescriptor> saved;
 try{
  if(target.strictEquals("constructor")){
   key=request.get("key");changed=proto;saved=proto.getOwnPropertyDescriptor(key);JSPropertyDescriptor d;d.configurable=true;
   d.setter=JSValue::function(+[](const JSValue&self,const Args&a,const Args&c)->JSValue{extraNote(c[0],"constructor.set:"+c[1].toString());own(self,c[1],arg(a));return {};},{context,key},"set",1);proto.defineProperty(key,d);
   if(request.get("factory").toBoolean())nativeFunctionOverloadingNodeBinding("overloadingFn").call({}, {array()}).call();else ctor.construct();
  }else{
   if(target.strictEquals("parameters"))extraGetter(n,"parametersNodes","parameters.get",array({parameter}),context);
   else if(target.strictEquals("cacheGet"))extraGetter(n,"_candidateFn","cache.get",nullptr,context);
   else if(target.strictEquals("cacheSet")){JSPropertyDescriptor d;d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args&)->JSValue{return nullptr;});d.setter=JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{extraNote(c[0],"cache.set");return {};},{context});n.defineProperty("_candidateFn",d);}
   else if(target.strictEquals("functions"))extraGetter(n,"functionNodes","functions.get",array({candidate}),context);
   else if(target.strictEquals("shader"))extraGetter(candidate,"shaderNode","shader.get",shader,context);
   else if(target.strictEquals("layout"))extraGetter(shader,"layout","layout.get",layout,context);
   else if(target.strictEquals("inputs"))extraGetter(layout,"inputs","inputs.get",inputs,context);
   else if(target.strictEquals("paramsLength")){auto value=rec({{"0",parameter}});extraGetter(value,"length","params.length",1,context);n.set("parametersNodes",value);}
   else if(target.strictEquals("inputsLength")){auto value=rec({{"0",input}});extraGetter(value,"length","inputs.length",1,context);layout.set("inputs",value);}
   else if(target.strictEquals("paramIndex")){auto value=rec({{"length",1}});extraGetter(value,"0","params.0",parameter,context);n.set("parametersNodes",value);}
   else if(target.strictEquals("inputIndex")){auto value=rec({{"length",1}});extraGetter(value,"0","inputs.0",input,context);layout.set("inputs",value);}
   else if(target.strictEquals("paramMethod"))extraGetter(parameter,"getNodeType","param.method",parameter.get("getNodeType"),context);
   else if(target.strictEquals("inputType"))extraGetter(input,"type","input.type","float",context);
   else if(target.strictEquals("resultShader")){n.set("_candidateFn",candidate);extraGetter(candidate,"shaderNode","result.shader",shader,context);}
   else if(target.strictEquals("candidate"))n.set("_candidateFn",candidate);
   else if(target.strictEquals("method"))extraGetter(n,"getCandidateFn","method.get",n.get("getCandidateFn"),context);
   else if(target.strictEquals("iterator")){
    auto state=rec({{"index",0}}),iterator=JSValue::object();auto next=JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{extraNote(c[0],"iterator.next.call");auto state=c[1];const auto index=state.get("index").number();state.set("index",index+1);return rec({{"done",index>0},{"value",c[2]}});},{context,state,candidate},"",0);extraGetter(iterator,"next","iterator.next.get",next,context);
    auto iterable=JSValue::object();own(iterable,JSValue::wellKnownSymbol("iterator"),JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{extraNote(c[0],"iterator.call");return c[1];},{context,iterator},"[Symbol.iterator]",0));n.set("functionNodes",iterable);
   }else if(target.strictEquals("spread")){
    auto state=rec({{"index",0}}),iterable=JSValue::object();n.set("_candidateFn",candidate);
    own(iterable,JSValue::wellKnownSymbol("iterator"),JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{extraNote(c[0],"spread.iterator");auto iterator=JSValue::object();own(iterator,"next",JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{extraNote(c[0],"spread.next");auto state=c[1];const auto index=state.get("index").number();state.set("index",index+1);return rec({{"done",index>0},{"value",c[2]}});},c,"next",0));return iterator;},{context,state,parameter},"[Symbol.iterator]",0));n.set("parametersNodes",iterable);
   }else if(target.strictEquals("error")){
    shader.set("layout",nullptr);changed=nativeGlobalObject();key="Error";saved=changed.getOwnPropertyDescriptor(key);auto handler=JSValue::object();own(handler,"construct",JSValue::function(+[](const JSValue&,const Args&a,const Args&c)->JSValue{extraNote(c[0],"Error.construct");Args values;for(unsigned i=0;i<arg(a,1).length();++i)values.push_back(arg(a,1).get(JSValue(i)));return arg(a).construct(values,arg(a,2));},{context},"construct",3));changed.set("Error",createNativeProxy(nativeErrorConstructor(),handler));
   }else throw std::logic_error("Unknown trace target");
   auto method=request.get("method");call(n,method.isUndefined()?"getCandidateFn":method.toString(),{context.get("builder")});
  }
 }catch(...){auto e=nativeExceptionValue(std::current_exception());push(context.get("events"),e.strictEquals(context.get("reason"))?JSValue("reason"):e.get("name"));}
 if(changed.isObject()){if(saved)changed.defineProperty(key,*saved);else changed.erase(key);}
 return rec({{"name",request.get("name")},{"events",context.get("events")},{"frames",context.get("frames")}});
}
JSValue extraConstruct(const JSValue& request){
 auto events=array(),reason=rec({{"marker","reason"}}),functions=array(),parameter=rec({{"marker","parameter"}}),ctor=nativeFunctionOverloadingNodeConstructor(),kind=request.get("kind");JSValue newTarget;
 if(kind.strictEquals("implicit")||kind.strictEquals("explicit")){
  newTarget=JSValue::builtinConstructor(+[](const JSValue&target,const Args&a,const Args&c)->JSValue{const bool explicitCtor=c[2].strictEquals("explicit");if(explicitCtor)push(c[1],"child.before");auto result=c[0].construct(a,target);if(explicitCtor)push(c[1],"child.after");return result;},{ctor,events,kind},"Child",0);newTarget.setPrototype(ctor);auto prototype=newTarget.get("prototype");prototype.setPrototype(ctor.get("prototype"));
 }else{
  newTarget=JSValue::constructor(+[](const JSValue&self,const Args&,const Args&)->JSValue{return self;},{},"Alternate",0,JSValue::ConstructorKind::Callable);
  auto proto=request.get("prototype");newTarget.set("prototype",proto.strictEquals("null")?JSValue(nullptr):proto.strictEquals("primitive")?JSValue(17):rec({{"marker","alternate"}}));
 }
 const auto originalPrototype=newTarget.get("prototype");auto target=newTarget;
 if(request.get("proxy").toBoolean()){
  auto handler=JSValue::object();own(handler,"get",JSValue::function(+[](const JSValue&,const Args&a,const Args&c)->JSValue{if(arg(a,1).strictEquals("prototype")){push(c[0],"newTarget.prototype");if(c[2].get("gc").toBoolean())c[0].collectGarbage();if(c[2].get("throw").toBoolean())throw JSThrownValue(c[1]);}return arg(a).get(arg(a,1),arg(a,2));},{events,reason,request},"get",3));target=createNativeProxy(newTarget,handler);
 }
 JSValue value;try{value=kind.strictEquals("implicit")||kind.strictEquals("explicit")?target.construct({functions,parameter}):ctor.construct({functions,parameter},target);}catch(...){const auto e=nativeExceptionValue(std::current_exception());return rec({{"name",request.get("name")},{"events",events},{"error",e.strictEquals(reason)?JSValue("reason"):e.get("name")}});}
 auto keys=array();for(const auto&key:value.ownPropertyKeys())push(keys,key);const auto prototype=request.get("prototype");const bool fallback=prototype.strictEquals("null")||prototype.strictEquals("primitive");auto objectPrototype=nativeGlobalBinding("Object").get("prototype");bool isNode=false;auto parent=value.getPrototype();while(parent.isObject()){if(parent.sameValue(nativeNodeConstructor().get("prototype"))){isNode=true;break;}parent=parent.getPrototype();}
 return rec({{"name",request.get("name")},{"events",events},{"prototypeSame",value.getPrototype().sameValue(fallback?objectPrototype:originalPrototype)},{"functionNodesSame",value.get("functionNodes").sameValue(functions)},{"parameterSame",value.get("parametersNodes").get("0").sameValue(parameter)},{"restFresh",!value.get("parametersNodes").sameValue(functions)},{"candidate",value.get("_candidateFn")},{"global",value.get("global")},{"isNode",isNode},{"ownKeys",keys},{"id",value.get("id")},{"uuid",value.get("uuid")}});
}
JsonValue extraHeapLifetime(){
 unsigned modules=0,constructors=0,closures=0,nodes=0;
 for(unsigned round=0;round<3;++round){JSWeakValue weakModule,weakCtor,weakClosure,weakNode;{
  auto keeper=JSValue::object(),module=nativeFunctionOverloadingNodeModule(),ctor=nativeFunctionOverloadingNodeConstructor(),factory=nativeFunctionOverloadingNodeBinding("overloadingFn").call({}, {array()}),node=factory.call();
  weakModule=JSWeakValue(module);weakCtor=JSWeakValue(ctor);weakClosure=JSWeakValue(factory);weakNode=JSWeakValue(node);keeper.collectGarbage();
  if(weakModule.expired()||weakCtor.expired()||weakClosure.expired()||weakNode.expired())throw std::logic_error("Live real module graph was lost");
 }endJSJob();JSValue::nativeSafepoint();if(!weakModule.expired()||!weakCtor.expired()||!weakClosure.expired()||!weakNode.expired()||JSValue::hasActiveHeap())throw std::logic_error("FunctionOverloading heap or actual graph retained after last root");++modules;++constructors;++closures;++nodes;}
 return JsonValue::Object{{"heaps",3},{"retired",3},{"moduleExpired",modules},{"constructorExpired",constructors},{"closureExpired",closures},{"nodeExpired",nodes}};
}
}
int main(int argc,char**argv){try{
 if(argc==2&&std::string_view(argv[1])=="--heap-lifetime"){std::cout<<extraHeapLifetime().dump();return 0;}
 if(argc==2&&(std::string_view(argv[1])=="--extra-trace"||std::string_view(argv[1])=="--extra-construct")){
  initialize();auto keep=nativeFunctionOverloadingNodeModule();const auto input=Animation_detail::parseJSON(std::string(std::istreambuf_iterator<char>(std::cin),{}));const bool trace=std::string_view(argv[1])=="--extra-trace";
  auto entry=JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{auto result=array(),input=c[0];for(unsigned i=0;i<input.length();++i)push(result,c[1].toBoolean()?extraTrace(input.get(JSValue(i))):extraConstruct(input.get(JSValue(i))));return result;},{input,trace});std::cout<<entry.call().toJSON()->dump();randomState={};return 0;
 }
 initialize();auto keep=nativeFunctionOverloadingNodeModule();if(argc==2&&std::string_view(argv[1])=="--metadata"){std::cout<<metadata().toJSON()->dump();return 0;}if(argc==2&&std::string_view(argv[1])=="--lifetime"){std::cout<<lifetime().toJSON()->dump();return 0;}if(argc!=1)throw std::logic_error("Usage: FunctionOverloadingExtraParityProbe [--metadata|--lifetime|--extra-trace|--extra-construct|--heap-lifetime]");auto input=Animation_detail::parseJSON(std::string(std::istreambuf_iterator<char>(std::cin),{}));auto entry=JSValue::function(+[](const JSValue&,const Args&,const Args&c)->JSValue{auto rows=array(),input=c[0];for(unsigned i=0;i<input.length();++i)push(rows,run(input.get(JSValue(i))));return rows;},{input});std::cout<<entry.call().toJSON()->dump();return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
