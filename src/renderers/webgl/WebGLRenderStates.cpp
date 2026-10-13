// Direct port of three.js at 11738a9d55e502fb484cefffcea3a66dc44e9fe9.
#include "renderers/webgl/WebGLRenderStates.h"
#include "renderers/webgl/WebGLLights.h"
#include "core/JSWeakMap.h"
#include <algorithm>

namespace {
using Args=std::vector<JSValue>;
JSValue arg(const Args& args,std::size_t i){return i<args.size()?args[i]:JSValue{};}
void literal(JSValue object,const char* key,JSValue value){JSPropertyDescriptor d;d.value=std::move(value);d.writable=d.enumerable=d.configurable=true;object.defineProperty(key,d);}
JSValue ordinary(JSValue::NativeFunction fn,const Args& captures,const char* name,unsigned arity){return JSValue::constructor(fn,captures,name,arity,JSValue::ConstructorKind::Callable);}
JSValue init(const JSValue&,const Args& args,const Args& captures){auto state=captures[0];state.set("camera",arg(args,0));for(std::size_t i=1;i<4;++i){auto array=captures[i];array.set("length",0);}return {};}
JSValue pushLight(const JSValue&,const Args& args,const Args& captures){const auto light=arg(args,0);const auto array=light.get("isLightProbeGrid").toBoolean()?captures[1]:captures[0];array.get("push").call(array,{light});return {};}
JSValue pushShadow(const JSValue&,const Args& args,const Args& captures){const auto array=captures[0];array.get("push").call(array,{arg(args,0)});return {};}
JSValue setupLights(const JSValue&,const Args&,const Args& captures){const auto lights=captures[0];lights.get("setup").call(lights,{captures[1]});return {};}
JSValue setupLightsView(const JSValue&,const Args& args,const Args& captures){const auto lights=captures[0];lights.get("setupView").call(lights,{captures[1],arg(args,0)});return {};}
std::vector<std::uint16_t> units(const std::string& text){std::vector<std::uint16_t> out;for(std::size_t i=0;i<text.size();){auto c=static_cast<unsigned char>(text[i++]);unsigned count=c<0x80?0:c<0xe0?1:c<0xf0?2:3;std::uint32_t value=count?c&(0x7f>>count):c;while(count--)value=(value<<6)|(static_cast<unsigned char>(text[i++])&63);if(value<=0xffff)out.push_back(static_cast<std::uint16_t>(value));else{value-=0x10000;out.push_back(static_cast<std::uint16_t>(0xd800+(value>>10)));out.push_back(static_cast<std::uint16_t>(0xdc00+(value&1023)));}}return out;}
bool greaterEqual(const JSValue& left,const JSValue& right){const auto a=left.toPrimitive(JSValue::PreferredType::Number),b=right.toPrimitive(JSValue::PreferredType::Number);if(a.type()==JSValue::Type::String&&b.type()==JSValue::Type::String){const auto x=units(a.string()),y=units(b.string());return !std::lexicographical_compare(x.begin(),x.end(),y.begin(),y.end());}const auto x=a.toNumber();const auto y=b.toNumber();return x>=y;}
JSValue get(const JSValue&,const Args& args,const Args& captures){const auto closure=captures[0],scene=arg(args,0);auto depth=arg(args,1);if(depth.isUndefined())depth=0;const auto map=closure.get("states"),array=map.get("get").call(map,{scene});JSValue result;if(array.isUndefined()){result=WebGLRenderState(captures[1]).object();auto entries=JSValue::array();literal(entries,"0",result);const auto live=closure.get("states");live.get("set").call(live,{scene,entries});}else{const auto length=array.get("length");if(greaterEqual(depth,length)){result=WebGLRenderState(captures[1]).object();array.get("push").call(array,{result});}else result=array.get(depth.toPrimitive(JSValue::PreferredType::String).toString());}return result;}
JSValue dispose(const JSValue&,const Args&,const Args& captures){auto closure=captures[0];closure.set("states",createNativeWeakMap());return {};}
}
WebGLRenderState::WebGLRenderState(JSValue extensions):object_(JSValue::object()) {
    const auto lights=WebGLLights(extensions).object(),lightsArray=JSValue::array(),shadowsArray=JSValue::array(),probeGrids=JSValue::array();auto state=JSValue::object();
    literal(state,"lightsArray",lightsArray);literal(state,"shadowsArray",shadowsArray);literal(state,"lightProbeGridArray",probeGrids);literal(state,"camera",nullptr);literal(state,"lights",lights);literal(state,"transmissionRenderTarget",JSValue::object());literal(state,"textureUnits",0);
    literal(object_,"init",ordinary(::init,{state,lightsArray,shadowsArray,probeGrids},"init",1));literal(object_,"state",state);
    literal(object_,"setupLights",ordinary(::setupLights,{lights,lightsArray},"setupLights",0));literal(object_,"setupLightsView",ordinary(::setupLightsView,{lights,lightsArray},"setupLightsView",1));
    literal(object_,"pushLight",ordinary(::pushLight,{lightsArray,probeGrids},"pushLight",1));literal(object_,"pushShadow",ordinary(::pushShadow,{shadowsArray},"pushShadow",1));
}
JSValue WebGLRenderState::init(JSValue camera){return object_.get("init").call(object_,{camera});}
JSValue WebGLRenderState::pushLight(JSValue light){return object_.get("pushLight").call(object_,{light});}
JSValue WebGLRenderState::pushShadow(JSValue light){return object_.get("pushShadow").call(object_,{light});}
JSValue WebGLRenderState::setupLights(){return object_.get("setupLights").call(object_);}
JSValue WebGLRenderState::setupLightsView(JSValue camera){return object_.get("setupLightsView").call(object_,{camera});}
WebGLRenderStates::WebGLRenderStates(JSValue extensions):object_(JSValue::object()){auto closure=JSValue::object(nullptr);literal(closure,"states",createNativeWeakMap());literal(object_,"get",ordinary(::get,{closure,extensions},"get",1));literal(object_,"dispose",ordinary(::dispose,{closure},"dispose",0));}
JSValue WebGLRenderStates::get(JSValue scene,JSValue depth){return object_.get("get").call(object_,{scene,depth});}
JSValue WebGLRenderStates::dispose(){return object_.get("dispose").call(object_);}
