#pragma once
#include "renderers/webgl/WebGLUniforms.h"
#include "core/NativeArray.h"
#include <cmath>
#include <limits>
namespace UniformsFixture {
using Args=std::vector<JSValue>;using A=JsonValue::Array;using O=JsonValue::Object;
inline void prop(JSValue object,const std::string& name,JSValue value){JSPropertyDescriptor d;d.value=std::move(value);d.writable=d.enumerable=d.configurable=true;object.defineProperty(name,d);}
inline JSValue array(Args values={}){auto out=JSValue::array();for(std::size_t i=0;i<values.size();++i)prop(out,std::to_string(i),values[i]);return out;}
inline void push(JSValue value,JSValue item){value.get("push").call(value,{item});}
inline JSValue invoke(JSValue receiver,const char* name,Args args={}){return receiver.get(name).call(receiver,args);}
inline JsonValue encode(const JSValue& value){
 if(value.isUndefined())return "undefined";if(value.isNull())return nullptr;if(value.type()==JSValue::Type::Boolean)return value.toBoolean();if(value.type()==JSValue::Type::String)return value.string();
 if(value.type()==JSValue::Type::Number){const auto n=value.number();if(std::isnan(n))return "NaN";if(std::isinf(n))return n<0?"-Infinity":"Infinity";if(n==0&&std::signbit(n))return "-0";return n;}
 if(value.isCallable())return O{{"function",value.get("name").toString()}};
 if(value.hasOwn("tag"))return O{{"ref",value.get("tag").toString()}};
 if(value.isHostObject("three.AnimationArray")){A out;for(unsigned i=0;i<value.get("length").toNumber();++i)out.push_back(encode(value.get(std::to_string(i))));return O{{"typed",value.get("constructor").get("name").toString()},{"values",out}};}
 if(value.get("isTexture").strictEquals(true)){O out;for(const char* name:{"isTexture","isDepthTexture","isCubeTexture","isCubeDepthTexture","isDataArrayTexture","isData3DTexture","version","compareFunction"})out[name]=encode(value.get(name));return O{{"texture",out}};}
 if(value.type()==JSValue::Type::Array){A out;for(unsigned i=0;i<value.length();++i)out.push_back(encode(value.get(std::to_string(i))));return out;}
 O out;for(const auto& key:value.keys()){const auto d=value.getOwnPropertyDescriptor(key);out[key]=d&&d->value?encode(*d->value):JsonValue("accessor");}return out;
}
inline void log(JSValue state,Args row){push(state.get("log"),array(std::move(row)));}
inline JSValue snap(const JSValue& value){return JSValue::fromJSON(encode(value));}
template<class F>JsonValue outcome(F action){try{return encode(action());}catch(const JSTypeError&){return O{{"error","TypeError"}};}catch(const JSRangeError&){return O{{"error","RangeError"}};}catch(const std::exception& e){return O{{"error","Error"},{"message",e.what()}};}}
inline JSValue info(std::string name,int type=0x1406,JSValue size=1){auto x=JSValue::object();prop(x,"name",name);prop(x,"type",type);prop(x,"size",size);return x;}
inline const std::vector<int>& types(){static const std::vector<int> values{0x1406,0x8b50,0x8b51,0x8b52,0x8b5a,0x8b5b,0x8b5c,0x1404,0x8b56,0x8b53,0x8b57,0x8b54,0x8b58,0x8b55,0x8b59,0x1405,0x8dc6,0x8dc7,0x8dc8,0x8b5e,0x8d66,0x8dca,0x8dd2,0x8b62,0x8b5f,0x8dcb,0x8dd3,0x8b60,0x8dcc,0x8dd4,0x8dc5,0x8dc1,0x8dcf,0x8dd7,0x8dc4};return values;}
inline JSValue command(const JSValue& self,const Args& args,const Args& cap){
 auto state=cap[0];const auto name=cap[1].string();Args row{name,self.sameValue(state.get(cap[2].string()))};for(const auto& arg:args)row.push_back(snap(arg));log(state,std::move(row));
 if(state.get("throw").strictEquals(name))throw JSRangeError("command");
 if(name=="getProgramParameter")return state.get("count");
 if(name=="getActiveUniform")return state.get("infos").get(args[1].toString());
 if(name=="getUniformLocation")return "location:"+args[1].toString();
 if(name=="allocateTextureUnit"){const auto n=state.get("unit");state.set("unit",n.toNumber()+1);return n;}
 if(state.get("reenter").strictEquals(name)&&!state.get("acted").toBoolean()){state.set("acted",true);invoke(state.get("manager"),"setValue",{state.get("gl"),"u",state.get("replacement"),state.get("textures")});}
 return {};
}
struct Fixture{
 JSValue state=JSValue::object(),gl=JSValue::object(),textures=JSValue::object(),program=JSValue::object(),infos,manager;
 explicit Fixture(Args uniforms={info("u")}):infos(array(std::move(uniforms))){
  prop(state,"log",array());prop(state,"gl",gl);prop(state,"textures",textures);prop(state,"infos",infos);prop(state,"count",infos.length());prop(state,"unit",0);prop(state,"throw","");prop(state,"reenter","");prop(state,"acted",false);prop(program,"tag","program");
  prop(gl,"ACTIVE_UNIFORMS",35718);prop(gl,"SAMPLER_2D_SHADOW",0x8b62);prop(gl,"SAMPLER_CUBE_SHADOW",0x8dc5);prop(gl,"SAMPLER_2D_ARRAY_SHADOW",0x8dc4);
  for(const char* name:{"getProgramParameter","getActiveUniform","getUniformLocation","uniform1f","uniform2f","uniform3f","uniform4f","uniform1fv","uniform2fv","uniform3fv","uniform4fv","uniform1i","uniform2i","uniform3i","uniform4i","uniform1iv","uniform2iv","uniform3iv","uniform4iv","uniform1ui","uniform2ui","uniform3ui","uniform4ui","uniform1uiv","uniform2uiv","uniform3uiv","uniform4uiv","uniformMatrix2fv","uniformMatrix3fv","uniformMatrix4fv"})prop(gl,name,JSValue::function(command,{state,name,"gl"},name));
  for(const char* name:{"allocateTextureUnit","setTexture2D","setTexture3D","setTextureCube","setTexture2DArray"})prop(textures,name,JSValue::function(command,{state,name,"textures"},name));
  manager=WebGLUniforms(gl,program).object();prop(state,"manager",manager);
 }
 JSValue ctor(){return manager.getPrototype().get("constructor");}
 JSValue set(JSValue value,JSValue name="u"){return invoke(manager,"setValue",{gl,name,value,textures});}
 JSValue leaf(){return manager.get("map").get("u");}
 void resetUnits(){state.set("unit",0);}
 JsonValue trace(){return encode(state.get("log"));}
};
inline JSValue numbers(unsigned count,double offset=0){auto v=JSValue::array();for(unsigned i=0;i<count;++i)prop(v,std::to_string(i),i+0.1+offset);return v;}
inline JSValue texture(const char* name="texture"){auto t=JSValue::object();prop(t,"tag",name);return t;}
inline JSValue vector(unsigned n,bool color=false){auto v=JSValue::object();const char* names[]={"x","y","z","w"};const char* colors[]={"r","g","b","a"};for(unsigned i=0;i<n;++i)prop(v,color?colors[i]:names[i],i+0.1);return v;}
inline JSValue toArrayObject(JSValue values,JSValue state,unsigned action=0){auto v=JSValue::object();prop(v,"values",values);
 prop(v,"toArray",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{auto state=c[0];log(state,{"toArray",self.sameValue(c[1]),args[1]});if(c[2].strictEquals(1))throw JSRangeError("toArray");auto target=args[0];const auto offset=args[1].toNumber();const auto values=self.get("values");for(unsigned i=0;i<values.get("length").toNumber();++i)target.set(JSValue(offset+i).toString(),values.get(std::to_string(i)));return target;},{state,v,action},"toArray",2));return v;}
inline JsonValue tree(const JSValue& node){O out;for(const char* key:{"id","type","size","addr","cache"})if(node.hasOwn(key))out[key]=encode(node.get(key));
 const auto setter=node.get("setValue");out["setter"]=setter.isUndefined()?JsonValue("undefined"):JsonValue(setter.get("name").toString());
 if(node.hasOwn("seq")){A children;const auto seq=node.get("seq");for(unsigned i=0;i<seq.length();++i)children.push_back(tree(seq.get(std::to_string(i))));out["seq"]=children;A keys;for(const auto& key:node.get("map").ownPropertyNames())keys.emplace_back(key);out["mapKeys"]=keys;}return out;}
inline JSValue valueFor(Fixture& f,unsigned mode){
 switch(mode){case 0:return 2;case 1:return JSValue();case 2:return nullptr;case 3:return -0.;case 4:return std::nan("");case 5:return "2";case 6:return numbers(4);case 7:return vector(4);case 8:return vector(3,true);
 case 9:{auto v=JSValue::object();prop(v,"elements",numbers(16));return v;}
 case 10:return NativeArray_detail::wrap(Float32Array(std::vector<float>{.1f,1.1f,2.1f,3.1f}));
 case 11:return array();case 12:return array({toArrayObject(numbers(4),f.state),toArrayObject(numbers(4,4),f.state)});
 case 13:return array({texture(),nullptr});case 14:return texture();case 15:return false;case 16:return numbers(32);case 17:return array({toArrayObject(numbers(16),f.state),toArrayObject(numbers(16,16),f.state)});
 default:return {};
 }}
// Upstream allocTexUnits uses i !== n; undefined lengths do not terminate.
// Exercise only terminating sampler-array inputs in this synchronous oracle.
inline bool finiteCase(unsigned type,unsigned pure,unsigned mode){return !pure||type<19||mode==1||mode==2||mode==5||mode==6||mode==10||mode==11||mode==12||mode==13||mode==16||mode==17;}
inline JsonValue typeCase(unsigned type,unsigned pure,unsigned mode){Fixture f({info(pure?"u[0]":"u",types()[type],2)});auto v=valueFor(f,mode);const auto before=tree(f.manager);A result;
 for(unsigned i=0;i<3;++i){f.resetUnits();result.push_back(outcome([&]{return f.set(v);}));if(i==1){if(v.type()==JSValue::Type::Array&&!v.length())push(v,1);else if(v.isObject()&&v.hasOwn("x"))v.set("x",7);else if(v.type()==JSValue::Type::Array)v.set("0",7);else if(v.type()==JSValue::Type::Number)v=7;}}
 return O{{"kind","type"},{"type",type},{"pure",pure},{"mode",mode},{"before",before},{"after",tree(f.manager)},{"results",result},{"trace",f.trace()}};}
inline const std::vector<std::vector<std::string>>& paths(){static const std::vector<std::vector<std::string>> p{
 {"u"},{"u[0]"},{"u[1]"},{"u[12]"},{"light.color","light.intensity"},{"lights[0].color","lights[1].color"},
 {"nested.a[0].b[1].x","nested.a[0].b[0].x"},{"u","u"},{"u.x","u.y","u.x"},{"u.x","u"},{"u","u.x"},
 {"__proto__.x"},{"constructor.x"},{"toString.x"},{"u[-1]"},{"u[4294967295].x"},{"u[2147483648].x"},{"bad-name"},{""},{"u."},
 {"lights[0].weights[0]","lights[1].weights[0]"},{"_a2.$b"},{"123"},{"u[00]"}};return p;}
inline JsonValue pathCase(unsigned variant){Fixture f({});auto list=array();for(const auto& path:paths()[variant])push(list,info(path));f.state.set("infos",list);f.infos=list;f.state.set("count",list.length());JSValue built;
 const auto result=outcome([&]{built=WebGLUniforms(f.gl,f.program).object();return JSValue();});return O{{"kind","path"},{"variant",variant},{"result",result},{"tree",built.isUndefined()?JsonValue(nullptr):tree(built)},{"trace",f.trace()}};}
inline JsonValue structureCase(unsigned mode){Fixture f({info("lights[0].color",0x8b51),info("lights[0].gain"),info("lights[1].color",0x8b51),info("lights[1].gain"),info("other")});
 auto one=JSValue::object(),two=JSValue::object();prop(one,"color",vector(3));prop(two,"color",vector(3,true));prop(one,"gain",1);prop(two,"gain",2);auto values=array({one,two});
 if(mode==1)values.set("1",nullptr);if(mode==2)one.erase("color");if(mode==3)values.set("length",1);if(mode==4){auto prototype=JSValue::object();prop(prototype,"0",one);prop(prototype,"1",two);values=JSValue::object(prototype);}
 const auto first=outcome([&]{return f.set(values,"lights");});const auto second=outcome([&]{return f.set(values,"lights");});return O{{"kind","structure"},{"mode",mode},{"first",first},{"second",second},{"tree",tree(f.manager)},{"trace",f.trace()}};}
inline JSValue record(JSValue value,JSValue needs={}){auto r=JSValue::object();prop(r,"value",value);prop(r,"needsUpdate",needs);return r;}
inline JsonValue uploadCase(unsigned mode){Fixture f({info("a"),info("b",0x8b51),info("c")});auto values=JSValue::object();prop(values,"a",record(1));prop(values,"b",record(vector(3)));prop(values,"c",record(3));
 if(mode<8){const Args needs{JSValue(),false,true,nullptr,0,"false",1,std::nan("")};values.get("b").set("needsUpdate",needs[mode]);}
 if(mode==8)values.erase("b");if(mode==9)values.set("b",nullptr);if(mode==10)values.set("b",JSValue());
 if(mode==11){auto prototype=JSValue::object();prop(prototype,"b",values.get("b"));values.erase("b");values.setPrototype(prototype);}
 if(mode==12){JSPropertyDescriptor d;d.value=values.get("b");d.enumerable=false;d.configurable=true;values.defineProperty("b",d);}
 if(mode==13)values=3;if(mode==14)values=nullptr;
 JSValue selected;const auto filter=outcome([&]{selected=invoke(f.ctor(),"seqWithValue",{f.manager.get("seq"),values});return JSValue();});A ids,identities;
 if(!selected.isUndefined())for(unsigned i=0;i<selected.length();++i){const auto item=selected.get(std::to_string(i));ids.push_back(encode(item.get("id")));identities.push_back(item.sameValue(f.manager.get("map").get(item.get("id").toString())));}
 const auto all=outcome([&]{return invoke(f.ctor(),"upload",{f.gl,f.manager.get("seq"),values,f.textures});});
 const auto filtered=outcome([&]{return invoke(f.ctor(),"upload",{f.gl,selected,values,f.textures});});
 return O{{"kind","upload"},{"mode",mode},{"filter",filter},{"ids",ids},{"identities",identities},{"all",all},{"filtered",filtered},{"tree",tree(f.manager)},{"trace",f.trace()}};}
inline void watch(Fixture& f,JSValue target,const char* name,const char* label,unsigned action=0){const auto value=target.get(name);JSPropertyDescriptor d;d.enumerable=d.configurable=true;
 d.getter=JSValue::function(+[](const JSValue& self,const Args&,const Args& c)->JSValue{auto state=c[0];log(state,{"getter",c[1],c[2],self.sameValue(c[3])});if(c[5].strictEquals(1))throw JSRangeError("getter");if(c[5].strictEquals(2))state.get("manager").get("map").get("u").set("addr","changed-location");return c[4];},{f.state,label,name,target,value,action});target.defineProperty(name,d);}
inline JsonValue observedCase(unsigned mode){const int type=mode<5?0x8b51:mode<9?0x8b5b:mode<12?0x8b5e:0x1406;Fixture f({info("u",type)});auto v=mode<5?vector(3):mode<9?valueFor(f,9):mode<12?texture():JSValue(1);
 if(mode==0)for(const char* name:{"x","y","z"})watch(f,v,name,"value");if(mode==1)watch(f,v,"x","value",1);
 if(mode==2)watch(f,f.gl,"uniform3f","gl",2);if(mode==3){f.state.set("reenter","uniform3f");prop(f.state,"replacement",vector(3,true));}
 if(mode==4)f.state.set("throw","uniform3f");
 if(mode==5)watch(f,v,"elements","value");if(mode==6){v.get("elements").set("length",9);watch(f,f.gl,"uniformMatrix3fv","gl",2);}
 if(mode==7){v.get("elements").set("length",9);f.state.set("throw","uniformMatrix3fv");}if(mode==8){v.get("elements").set("length",9);f.state.set("reenter","uniformMatrix3fv");auto r=JSValue::object();prop(r,"elements",numbers(9,20));prop(f.state,"replacement",r);}
 if(mode==9)watch(f,f.textures,"allocateTextureUnit","textures");if(mode==10)f.state.set("throw","setTexture2D");if(mode==11)f.state.set("throw","uniform1i");
 if(mode==12){f.state.set("reenter","uniform1f");prop(f.state,"replacement",9);}if(mode==13)f.state.set("throw","uniform1f");
 A results;f.resetUnits();results.push_back(outcome([&]{return f.set(v);}));const auto cache=encode(f.leaf().get("cache"));f.state.set("throw","");f.resetUnits();results.push_back(outcome([&]{return f.set(v);}));
 return O{{"kind","observed"},{"mode",mode},{"results",results},{"firstCache",cache},{"tree",tree(f.manager)},{"trace",f.trace()}};}
inline JsonValue optionalCase(unsigned mode){Fixture f;auto value=JSValue::object();prop(value,"u",mode==0?JSValue():JSValue(3));if(mode==2)watch(f,value,"u","optional");if(mode==3)watch(f,value,"u","optional",1);if(mode==4){auto p=value;value=JSValue::object(p);}if(mode==5)value=nullptr;
 const auto result=outcome([&]{return invoke(f.manager,"setOptional",{f.gl,value,"u"});});const auto absent=outcome([&]{return f.set(2,"missing");});return O{{"kind","optional"},{"mode",mode},{"result",result},{"absent",absent},{"tree",tree(f.manager)},{"trace",f.trace()}};}
inline JsonValue metadata(JSValue fn){A names;for(const auto& name:fn.ownPropertyNames())names.emplace_back(name);O out{{"name",encode(fn.get("name"))},{"length",encode(fn.get("length"))},{"own",names},{"constructible",fn.isConstructible()}};
 if(const auto d=fn.getOwnPropertyDescriptor("prototype"))out["prototypeFlags"]=A{d->writable.value_or(false),d->enumerable.value_or(false),d->configurable.value_or(false)};return out;}
inline JsonValue metadataCase(unsigned type,unsigned pure){Fixture f({info(pure?"u[0]":"u",types()[type],2)});auto leaf=f.leaf(),ctor=f.ctor();return O{{"kind","metadata"},{"type",type},{"pure",pure},{"root",metadata(ctor)},{"rootSet",metadata(f.manager.get("setValue"))},{"optional",metadata(f.manager.get("setOptional"))},{"upload",metadata(ctor.get("upload"))},{"seqWithValue",metadata(ctor.get("seqWithValue"))},{"leafCtor",metadata(leaf.getPrototype().get("constructor"))},{"leafSetter",metadata(leaf.get("setValue"))}};}
inline JsonValue textureSequenceCase(unsigned type,unsigned mode){Fixture f({info("u[0]",types()[type],3)});auto values=array({nullptr,texture("A"),false});A results,caches;
 for(unsigned step=0;step<5;++step){if(step==1)values.set("length",1);if(step==3){values.set("length",3);values.set("1",texture("B"));}
  f.state.set("unit",mode==0?JSValue(0):mode==1?JSValue(step):JSValue(4294967295.));results.push_back(outcome([&]{return f.set(values);}));caches.push_back(encode(f.leaf().get("cache")));}
 return O{{"kind","texture-sequence"},{"type",type},{"mode",mode},{"results",results},{"caches",caches},{"trace",f.trace()}};}
inline JsonValue shadowOrderCase(){Fixture f({info("plain",0x8b5e),info("shadowCube",0x8dc5),info("value"),info("shadow2d",0x8b62),info("nested.shadow",0x8dc5),info("array[0]",0x8dc4,2),info("cube",0x8b60)});auto values=JSValue::object();
 for(const char* name:{"plain","shadowCube","shadow2d","cube"})prop(values,name,record(nullptr));prop(values,"value",record(3));auto nested=JSValue::object();prop(nested,"shadow",nullptr);prop(values,"nested",record(nested));prop(values,"array",record(array({nullptr,nullptr})));
 const auto result=outcome([&]{return invoke(f.ctor(),"upload",{f.gl,f.manager.get("seq"),values,f.textures});});return O{{"kind","shadow-order"},{"result",result},{"tree",tree(f.manager)},{"trace",f.trace()}};}
inline JsonValue flattenCase(unsigned type,unsigned mode){const int widths[]={2,3,4,4,9,16},codes[]={0x8b50,0x8b51,0x8b52,0x8b5a,0x8b5b,0x8b5c};const auto width=widths[type];Fixture f({info("u[0]",codes[type],mode==0?0:2)});
 auto first=toArrayObject(numbers(width),f.state,mode==3?1:0),second=toArrayObject(numbers(width,10),f.state);auto values=array({first,second});
 if(mode==1||mode==2){first.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{auto state=c[0];const auto n=state.get("conversions").isUndefined()?1:state.get("conversions").toNumber()+1;state.set("conversions",n);log(state,{"toPrimitive",args[0],n});return c[1].strictEquals(1)?JSValue(n==1?std::nan(""):1):JSValue(std::nan(""));},{f.state,mode}));}
 if(mode==4)values.set("1",nullptr);if(mode==5){watch(f,values,"0","input");watch(f,first,"toArray","first");watch(f,values,"1","input");watch(f,second,"toArray","second");}
 if(mode==6)values=array({"2",3});if(mode==7)values=array({std::nan(""),3});if(mode==8)values=array({nullptr,3});if(mode==9)values=array({JSValue(),3});
 const auto result=outcome([&]{return f.set(values);});const auto again=outcome([&]{return f.set(values);});return O{{"kind","flatten"},{"type",type},{"mode",mode},{"result",result},{"again",again},{"tree",tree(f.manager)},{"trace",f.trace()}};}
inline JsonValue matrixScratchCase(unsigned dimension){const int type=0x8b58+dimension;Fixture first({info("u",type)}),second({info("u",type)});auto retained=array();
 const auto name="uniformMatrix"+std::to_string(dimension)+"fv";
 auto callback=JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{push(c[0],args[2]);return {};},{retained},name,3);first.gl.set(name,callback);second.gl.set(name,callback);
 auto value=JSValue::object();prop(value,"elements",numbers(dimension*dimension));first.set(value);const auto before=encode(retained.get("0"));
 auto changed=JSValue::object();prop(changed,"elements",numbers(dimension*dimension,10));second.set(changed);JSValue::object().collectGarbage();const auto after=encode(retained.get("0"));
 auto shortValue=JSValue::object();prop(shortValue,"elements",numbers(1,90));first.set(shortValue);return O{{"kind","matrix-scratch"},{"dimension",dimension},{"before",before},{"after",after},{"sharedIdentity",retained.get("0").sameValue(retained.get("1"))},{"shortUpload",encode(retained.get("2"))},{"firstCache",encode(first.leaf().get("cache"))},{"secondCache",encode(second.leaf().get("cache"))}};}
inline JsonValue constructCase(unsigned mode){Fixture f;auto ctor=f.ctor();auto target=JSValue::constructor(+[](const JSValue&,const Args&,const Args&)->JSValue{throw JSRangeError("newTarget body must not run");}, {},"Other",0,JSValue::ConstructorKind::Callable);auto prototype=JSValue::object();target.set("prototype",mode==1?JSValue(nullptr):prototype);A intercepted;
 if(mode==2){JSPropertyDescriptor d;d.value=0;d.writable=false;d.configurable=true;prototype.defineProperty("seq",d);}if(mode==3){auto retained=array();prop(f.state,"intercepted",retained);JSPropertyDescriptor d;d.setter=JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{push(c[0],args[0]);prop(self,"seq",args[0]);return {};},{retained},"setter",1);d.configurable=true;prototype.defineProperty("seq",d);}
 JSValue result;const auto action=outcome([&]{result=mode==4?ctor.call(JSValue(),{f.gl,f.program}):ctor.construct({f.gl,f.program},target);return JSValue();});
 return O{{"kind","construct"},{"mode",mode},{"action",action},{"customPrototype",result.isObject()?result.getPrototype().sameValue(prototype):false},{"fallbackPrototype",result.isObject()?result.getPrototype().sameValue(JSValue::object().getPrototype()):false},{"tree",result.isObject()?tree(result):JsonValue(nullptr)},{"intercepted",encode(f.state.get("intercepted"))},{"trace",f.trace()}};}
inline JsonValue regexReentryCase(unsigned mode){Fixture f({}),inner({info("xxxx")});auto path=JSValue::object();prop(path,"length",11);prop(f.state,"conversions",0);
 path.setPrimitiveConverter(JSValue::function(+[](const JSValue&,const Args& args,const Args& c)->JSValue{auto state=c[0];const auto n=state.get("conversions").toNumber()+1;state.set("conversions",n);log(state,{"path-toPrimitive",args[0],n});
  if(c[3].strictEquals(5))throw JSRangeError("path");
  if((c[3].strictEquals(1)&&n==1)||(c[3].strictEquals(2)&&n==2)){const auto root=WebGLUniforms(c[1],c[2]).object();log(state,{"inner-tree",JSValue::fromJSON(tree(root))});}
  return "branch.tail";
 },{f.state,inner.gl,inner.program,mode}));
 if(mode==3||mode==4){JSPropertyDescriptor d;d.enumerable=d.configurable=true;d.getter=JSValue::function(+[](const JSValue&,const Args&,const Args& c)->JSValue{log(c[0],{"path-length"});if(c[3].strictEquals(3)){const auto ignored=WebGLUniforms(c[1],c[2]).object();}return c[3].strictEquals(4)?JSValue("11"):JSValue(11);},{f.state,inner.gl,inner.program,mode});path.defineProperty("length",d);}
 auto entry=info("unused");entry.set("name",path);f.state.set("infos",array({entry}));f.state.set("count",1);
 f.gl.set("getUniformLocation",JSValue::function(+[](const JSValue& self,const Args& args,const Args& c)->JSValue{log(c[0],{"opaque-location",self.sameValue(c[1]),snap(args[0]),snap(args[1])});return "opaque-location";},{f.state,f.gl},"getUniformLocation",2));
 JSValue built;const auto result=outcome([&]{built=WebGLUniforms(f.gl,f.program).object();return JSValue();});return O{{"kind","regex-reentry"},{"mode",mode},{"result",result},{"tree",built.isObject()?tree(built):JsonValue(nullptr)},{"trace",f.trace()},{"innerTrace",inner.trace()}};}
}
